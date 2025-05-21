#include <boost/asio.hpp>
#include <iostream>
#include <map>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <queue>
#include <mutex>

using namespace boost::asio::ip;
using namespace std::placeholders;

#define MAX_LENGTH 1024 * 2
#define HEAD_LENGTH 2

class MsgNode
{
    friend class CSession;

public:
    MsgNode(char *msg, int maxLen) : _totalLen(maxLen + HEAD_LENGTH), _curLen(0)
    {
        _data = new char[_totalLen + 1];
        memcpy(_data, &maxLen, HEAD_LENGTH);
        memcpy(_data + HEAD_LENGTH, msg, maxLen);
        _data[_totalLen] = '\0';
    }
    MsgNode(int maxLen) : _totalLen(maxLen), _curLen(0)
    {
        _data = new char[_totalLen + 1];
    }
    ~MsgNode()
    {
        delete[] _data;
    }
    void clear()
    {
        ::memset(_data, 0, _totalLen);
    }

private:
    int _curLen;
    int _totalLen;
    char *_data;
};

class CSession;

class Server
{
public:
    Server(boost::asio::io_context &ioc, short port);
    void clearCSession(std::string uuid); // 声明成员函数

private:
    void startAccept();
    void handleAccept(std::shared_ptr<CSession> newCSession, const boost::system::error_code &error);

    boost::asio::io_context &_ioc;
    tcp::acceptor _acceptor;
    std::map<std::string, std::shared_ptr<CSession>> _CSessions;
};

class CSession : public std::enable_shared_from_this<CSession>
{
public:
    CSession(boost::asio::io_context &ioc, Server *server) : _socket(ioc), _server(server)
    {
        boost::uuids::uuid a_uuid = boost::uuids::random_generator()();
        _uuid = boost::uuids::to_string(a_uuid);
    }

    tcp::socket &Socket() { return _socket; }
    std::string getUuid() const { return _uuid; }
    void Start();

private:
    void handleRead(const boost::system::error_code &error, size_t bytes_transfered, std::shared_ptr<CSession> selfShared);
    void handleWrite(const boost::system::error_code &error, std::shared_ptr<CSession> selfShared);
    void send(char *msg, int max_length);
    tcp::socket _socket;
    enum
    {
        max_length = 1024
    };
    char _data[max_length];
    Server *_server;
    std::string _uuid;
    std::queue<std::shared_ptr<MsgNode>> _sendQueue;
    std::mutex _sendMutex;
    // 收到的消息结构
    std::shared_ptr<MsgNode> _recvMsgNode;
    bool _isHeadParse;
    // 收到的头部结构
    std::shared_ptr<MsgNode> _recvMsgHead;
};

Server::Server(boost::asio::io_context &ioc, short port)
    : _ioc(ioc), _acceptor(ioc, tcp::endpoint(tcp::v4(), port))
{
    startAccept();
}

void Server::clearCSession(std::string uuid)
{
    _CSessions.erase(uuid);
}

void Server::startAccept()
{
    auto newCSession = std::make_shared<CSession>(_ioc, this);
    _acceptor.async_accept(newCSession->Socket(),
                           std::bind(&Server::handleAccept, this, newCSession, _1));
}

void Server::handleAccept(std::shared_ptr<CSession> newCSession, const boost::system::error_code &error)
{
    if (!error)
    {
        newCSession->Start();
        _CSessions[newCSession->getUuid()] = newCSession;
    }
    else
    {
        std::cerr << "accept error: " << error.message() << std::endl;
    }
    startAccept();
}

void CSession::Start()
{
    memset(_data, 0, max_length);
    _socket.async_read_some(boost::asio::buffer(_data, max_length),
                            std::bind(&CSession::handleRead, this, _1, _2, shared_from_this()));
}

void CSession::handleRead(const boost::system::error_code &error, size_t bytes_transferred, std::shared_ptr<CSession> selfShared)
{
    if (!error)
    {                              // 检查异步读取是否成功（无错误码）
        int copy_len = 0;          // 已处理（复制）的数据长度
        bool need_continue = true; // 是否需要继续循环处理剩余数据

        while (need_continue && bytes_transferred > 0)
        { // 循环处理本次读取到的所有数据
            if (!_isHeadParse)
            { // 未完成头部解析（_b_head_parse 表示是否已解析消息头部）
                // ----------------------
                // 处理消息头部（HEAD_LENGTH 为固定长度，如 2 字节表示数据长度）
                // ----------------------
                // 情况 1：当前收到的数据不足以填充头部（头部不完整）
                if (bytes_transferred + _recvMsgHead->_curLen < HEAD_LENGTH)
                {
                    // 将剩余数据复制到头部接收缓冲区
                    memcpy(_recvMsgHead->_data + _recvMsgHead->_curLen, _data + copy_len, bytes_transferred);
                    _recvMsgHead->_curLen += bytes_transferred; // 更新头部已接收长度
                    memset(_data, 0, MAX_LENGTH);               // 清空当前数据缓冲区

                    // 继续异步读取数据，补充不完整的头部
                    _socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
                                            std::bind(&CSession::handleRead, this, _1, _2, selfShared));
                    need_continue = false; // 退出循环，等待下次读取
                    return;
                }

                // 情况 2：当前数据足够解析头部，且包含部分消息体
                int head_remain = HEAD_LENGTH - _recvMsgHead->_curLen;                              // 头部剩余未读取的字节数
                memcpy(_recvMsgHead->_data + _recvMsgHead->_curLen, _data + copy_len, head_remain); // 填充完整头部
                copy_len += head_remain;                                                            // 已处理的数据长度增加
                bytes_transferred -= head_remain;                                                   // 剩余未处理的数据长度减少

                // 从头部解析出消息体长度（假设头部是 short 类型，2 字节）
                short data_len = 0;
                memcpy(&data_len, _recvMsgHead->_data, HEAD_LENGTH); // 读取头部中的数据长度
                std::cout << "解析到消息体长度: " << data_len << std::endl;

                // 校验消息体长度是否合法（防止非法数据导致缓冲区溢出）
                if (data_len > MAX_LENGTH)
                {
                    std::cout << "非法消息长度: " << data_len << ", 最大允许长度: " << MAX_LENGTH << std::endl;
                    _server->clearCSession(_uuid); // 清除会话
                    return;
                }

                // 创建消息体接收节点，初始化总长度为头部指定长度
                _recvMsgNode = std::make_shared<MsgNode>(data_len);

                // 情况 3：消息体数据未接收完整（当前剩余数据 < 消息体长度）
                if (bytes_transferred < data_len)
                {
                    memcpy(_recvMsgNode->_data + _recvMsgNode->_curLen, _data + copy_len, bytes_transferred); // 复制部分消息体
                    _recvMsgNode->_curLen += bytes_transferred;                                               // 更新已接收的消息体长度
                    memset(_data, 0, MAX_LENGTH);                                                             // 清空当前数据缓冲区

                    // 继续异步读取数据，补充不完整的消息体
                    _socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
                                            std::bind(&CSession::handleRead, this, _1, _2, selfShared));
                    _isHeadParse = true;   // 标记头部已解析，下次进入消息体处理分支
                    need_continue = false; // 退出循环，等待下次读取
                    return;
                }

                // 情况 4：消息体数据完整（当前剩余数据 >= 消息体长度）
                memcpy(_recvMsgNode->_data + _recvMsgNode->_curLen, _data + copy_len, data_len); // 复制完整消息体
                _recvMsgNode->_curLen += data_len;                                               // 更新已接收的消息体长度
                copy_len += data_len;                                                            // 已处理的数据长度增加
                bytes_transferred -= data_len;                                                   // 剩余未处理的数据长度减少

                // 终止符处理（假设消息以 '\0' 结尾，根据协议实际情况调整）
                _recvMsgNode->_data[_recvMsgNode->_totalLen] = '\0';
                std::cout << "接收到完整消息: " << _recvMsgNode->_data << std::endl;

                // 业务逻辑：回显消息（示例）
                send(_recvMsgNode->_data, _recvMsgNode->_totalLen);

                // 重置状态，准备处理下一条消息
                _isHeadParse = false;
                _recvMsgHead->clear(); // 清空头部接收节点

                // 若还有剩余数据未处理，继续循环（可能包含下一条消息的头部或体）
                if (bytes_transferred <= 0)
                {
                    memset(_data, 0, MAX_LENGTH); // 清空当前数据缓冲区
                    _socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
                                            std::bind(&CSession::handleRead, this, _1, _2, selfShared));
                    need_continue = false; // 退出循环，等待下次读取
                }
            }
            else
            {
                // ----------------------
                // 处理已解析头部的消息体（续传数据）
                // ----------------------
                int remain_msg = _recvMsgNode->_totalLen - _recvMsgNode->_curLen; // 消息体剩余未接收的字节数

                // 情况 1：当前剩余数据不足以填充消息体
                if (bytes_transferred < remain_msg)
                {
                    memcpy(_recvMsgNode->_data + _recvMsgNode->_curLen, _data + copy_len, bytes_transferred); // 复制部分数据
                    _recvMsgNode->_curLen += bytes_transferred;                                               // 更新已接收长度
                    memset(_data, 0, MAX_LENGTH);                                                             // 清空当前数据缓冲区

                    // 继续异步读取数据，补充剩余消息体
                    _socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
                                            std::bind(&CSession::handleRead, this, _1, _2, selfShared));
                    need_continue = false; // 退出循环，等待下次读取
                    return;
                }

                // 情况 2：当前剩余数据足够填充消息体
                memcpy(_recvMsgNode->_data + _recvMsgNode->_curLen, _data + copy_len, remain_msg); // 复制完整剩余数据
                _recvMsgNode->_curLen += remain_msg;                                               // 更新已接收长度
                bytes_transferred -= remain_msg;                                                   // 剩余未处理的数据长度减少
                copy_len += remain_msg;                                                            // 已处理的数据长度增加

                // 终止符处理（同上）
                _recvMsgNode->_data[_recvMsgNode->_totalLen] = '\0';
                std::cout << "接收到完整消息: " << _recvMsgNode->_data << std::endl;

                // 业务逻辑：回显消息（示例）
                send(_recvMsgNode->_data, _recvMsgNode->_totalLen);

                // 重置状态，准备处理下一条消息
                _isHeadParse = false;
                _recvMsgHead->clear();

                // 若还有剩余数据未处理，继续循环（可能包含下一条消息）
                if (bytes_transferred <= 0)
                {
                    memset(_data, 0, MAX_LENGTH); // 清空当前数据缓冲区
                    _socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
                                            std::bind(&CSession::handleRead, this, _1, _2, selfShared));
                    need_continue = false; // 退出循环，等待下次读取
                }
            }
        }
    }
    else
    {
        // ----------------------
        // 处理读取错误（如连接断开、超时等）
        // ----------------------
        std::cout << "读取失败，错误码: " << error.value() << ", 错误信息: " << error.message() << std::endl;
        // Close(); // 关闭 socket 连接（需实现 Close 函数）
        _server->clearCSession(_uuid); // 从服务器中移除当前会话
    }
}
void CSession::handleWrite(const boost::system::error_code &error, std::shared_ptr<CSession> selfShared)
{
    if (!error)
    {
        std::lock_guard<std::mutex> lock(_sendMutex);
        _sendQueue.pop();
        if (!_sendQueue.empty())
        {
            auto &msgNode = _sendQueue.front();
            boost::asio::async_write(_socket, boost::asio::buffer(msgNode->_data, msgNode->_totalLen), std::bind(&CSession::handleWrite, this, _1, selfShared));
        }
    }
    else
    {
        std::cerr << "write error: " << error.message() << std::endl;
        _server->clearCSession(_uuid);
    }
}

void CSession::send(char *msg, int max_length)
{
    bool pending = false;
    std::lock_guard<std::mutex> lock(_sendMutex);
    if (_sendQueue.size() > 0)
    {
        pending = true;
    }
    _sendQueue.push(std::make_shared<MsgNode>(msg, max_length));
    if (pending == true)
    {
        return;
    }
    boost::asio::async_write(_socket, boost::asio::buffer(msg, max_length),
                             std::bind(&CSession::handleWrite, this, _1, shared_from_this()));
}

int main()
{
    try
    {
        boost::asio::io_context io_context;
        Server server(io_context, 8888);
        io_context.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
        return 1;
    }
    return 0;
}