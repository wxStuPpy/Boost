#include "CSession.h"
#include <nlohmann/json.hpp>

using nlohmann::json;

CSession::CSession(boost::asio::io_context &ioc, Server *server) : _socket(ioc), _server(server), _isHeadParse(false), _isClose(false)
{
    boost::uuids::uuid a_uuid = boost::uuids::random_generator()();
    _uuid = boost::uuids::to_string(a_uuid);
    _recvMsgHead = std::make_shared<MsgNode>(HEAD_TOTAL_LEN);
}

CSession::~CSession()
{
    std::cout << "~CSession " << _uuid << " destruct" << std::endl;
}

tcp::socket &CSession::Socket()
{
    return _socket;
}

std::string CSession::getUuid() const
{
    return _uuid;
}

void CSession::close()
{
    _socket.close();
    _isClose = true;
}

void CSession::Start()
{
    _recvMsgHead->clear();
    async_read(_socket, boost::asio::buffer(_data, HEAD_LENGTH),
               std::bind(&CSession::handleReadHead, this, _1, _2, shared_from_this()));
}

void CSession::handleWrite(const boost::system::error_code &error, std::shared_ptr<CSession> selfShared)
{
    if (!error)
    {
        std::lock_guard<std::mutex> lock(_sendMutex);
        std::cout << "send data " << _sendQueue.front()->_data + HEAD_LENGTH << std::endl;
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
        close();
        _server->clearCSession(_uuid);
    }
}

void CSession::send(std::string msg)
{
    bool pending = false;
    std::lock_guard<std::mutex> lock(_sendMutex);
    if (_sendQueue.size() > 0)
    {
        pending = true;
    }
    _sendQueue.push(std::make_shared<MsgNode>(const_cast<char *>(msg.c_str()), msg.length()));
    if (pending)
    {
        return;
    }
    auto &msgnode = _sendQueue.front();
    boost::asio::async_write(_socket, boost::asio::buffer(msgnode->_data, msgnode->_totalLen),
                             std::bind(&CSession::handleWrite, this, _1, shared_from_this()));
}

void CSession::handleReadHead(const boost::system::error_code &error, size_t bytes_transfered, std::shared_ptr<CSession> selfShared)
{
    if (!error)
    {
        /*不可能发生的事 此处只作为展示*/
        if (bytes_transfered < HEAD_LENGTH)
        {
            close();
            _server->clearCSession(_uuid);
            return;
        }
        // 头部收全 解析头部
        short data_len = 0;
        memcpy(&data_len, _recvMsgHead->_data, HEAD_LENGTH); // 读取头部中的数据长度
        // 转为本地字节序
        data_len = boost::asio::detail::socket_ops::network_to_host_short(data_len);
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

        boost::asio::async_read(_socket, boost::asio::buffer(_recvMsgNode->_data, _recvMsgNode->_totalLen), std::bind(&CSession::handleReadBody, this, _1, _2, shared_from_this()));
    }
    else
    {
        std::cout << "读取失败，错误码: " << error.value() << ", 错误信息: " << error.message() << std::endl;
        close();                       
        _server->clearCSession(_uuid); 
    }
}

void CSession::handleReadBody(const boost::system::error_code &error, size_t bytes_transfered, std::shared_ptr<CSession> selfShared)
{
    if (!error)
    {
        _recvMsgNode->_data[_recvMsgNode->_totalLen] = '\0';
        json js = json::parse(std::string(_recvMsgNode->_data, _recvMsgNode->_totalLen));
        std::cout << "接收到完整消息: " << js["id"] << " " << js["data"] << std::endl;
        send(js.dump());
        _recvMsgNode->clear();
        Start();
    }
    else
    {
        std::cout << "读取失败，错误码: " << error.value() << ", 错误信息: " << error.message() << std::endl;
        close();                    
        _server->clearCSession(_uuid); 
    }
}
