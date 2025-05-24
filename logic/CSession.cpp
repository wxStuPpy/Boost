#include "CSession.h"
#include <nlohmann/json.hpp>
#include <iostream>


using nlohmann::json;

CSession::CSession(boost::asio::io_context &ioc, Server *server) : _socket(ioc), _server(server), _isHeadParse(false), _isClose(false)
{
    boost::uuids::uuid a_uuid = boost::uuids::random_generator()();
    _uuid = boost::uuids::to_string(a_uuid);
    _recvMsgHead = std::make_shared<RecvNode>(HEAD_TOTAL_LEN);
}

CSession::~CSession()
{
    std::cout << "~CSession " << _uuid << " destruct" << std::endl;
}

tcp::socket &CSession::getSocket()
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
    memset(_data, 0, MAX_LENGTH);
    _socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
                            std::bind(&CSession::handleRead, this, _1, _2, shared_from_this()));
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
        close();
        _server->clearCSession(_uuid);
    }
}

void CSession::send(std::string msg,short msgId)
{
    bool pending = false;
    std::lock_guard<std::mutex> lock(_sendMutex);
    if(_sendQueue.size()>MAX_QUEUE_SIZE){
        std::cout<<"sendQueue is fulled, size is"<<MAX_QUEUE_SIZE<<std::endl;
        return;
    }
    if (_sendQueue.size() > 0)
    {
        pending = true;
    }
    _sendQueue.push(std::make_shared<SendNode>(msg.c_str(), msg.length(),msgId));
    if (pending)
    {
        return;
    }
    auto &msgnode = _sendQueue.front();
    boost::asio::async_write(_socket, boost::asio::buffer(msgnode->_data, msgnode->_totalLen),
                             std::bind(&CSession::handleWrite, this, _1, shared_from_this()));
}

LogicNode::LogicNode(std::shared_ptr<CSession>session,std::shared_ptr<RecvNode>recvnode):_session(session),_recvNode(recvnode){
    
}

void CSession::handleRead(const boost::system::error_code &error, size_t bytes_transferred, std::shared_ptr<CSession> selfShared)
{
    if (!error)
    {                     // 检查异步读取是否成功（无错误码）
        int copy_len = 0; // 已处理（复制）的数据长度
        while (bytes_transferred > 0)
        { // 循环处理本次读取到的所有数据
            if (!_isHeadParse)
            { // 未完成头部解析（_b_head_parse 表示是否已解析消息头部）
                // ----------------------
                // 处理消息头部（HEAD_LENGTH 为固定长度，如 2 字节表示数据长度）
                // ----------------------
                // 情况 1：当前收到的数据不足以填充头部（头部不完整）
                if (bytes_transferred + _recvMsgHead->_curLen < HEAD_TOTAL_LEN)
                {
                    // 将剩余数据复制到头部接收缓冲区
                    memcpy(_recvMsgHead->_data + _recvMsgHead->_curLen, _data + copy_len, bytes_transferred);
                    _recvMsgHead->_curLen += bytes_transferred; // 更新头部已接收长度
                    memset(_data, 0, MAX_LENGTH);               // 清空当前数据缓冲区

                    // 继续异步读取数据，补充不完整的头部
                    _socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
                                            std::bind(&CSession::handleRead, this, _1, _2, selfShared));
                    return;
                }

                // 情况 2：当前数据足够解析头部，且包含部分消息体
                int head_remain = HEAD_TOTAL_LEN - _recvMsgHead->_curLen;                              // 头部剩余未读取的字节数
                memcpy(_recvMsgHead->_data + _recvMsgHead->_curLen, _data + copy_len, head_remain); // 填充完整头部
                copy_len += head_remain;                                                            // 已处理的数据长度增加
                bytes_transferred -= head_remain;                                                   // 剩余未处理的数据长度减少
                //获取消息id
                short msg_id=0;
                memcpy(&msg_id, _recvMsgHead->_data, HEAD_ID_LEN);
                // 转为本地字节序
                msg_id = boost::asio::detail::socket_ops::network_to_host_short(msg_id);
                //判断id是否合法
                if(msg_id>MAX_LENGTH){
                    std::cout<<"invalid msg_id "<<msg_id<<std::endl;
                    _server->clearCSession(_uuid); 
                    return;
                }
                // 从头部解析出消息体长度
                short data_len = 0;
                memcpy(&data_len, _recvMsgHead->_data+HEAD_ID_LEN, HEAD_DATA_LEN); // 读取头部中的数据长度
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
                _recvMsgNode = std::make_shared<RecvNode>(data_len);

                // 情况 3：消息体数据未接收完整（当前剩余数据 < 消息体长度）
                if (bytes_transferred < static_cast<size_t>(data_len))
                {
                    memcpy(_recvMsgNode->_data + _recvMsgNode->_curLen, _data + copy_len, bytes_transferred); // 复制部分消息体
                    _recvMsgNode->_curLen += bytes_transferred;                                               // 更新已接收的消息体长度
                    memset(_data, 0, MAX_LENGTH);                                                             // 清空当前数据缓冲区

                    // 继续异步读取数据，补充不完整的消息体
                    _socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
                                            std::bind(&CSession::handleRead, this, _1, _2, selfShared));
                    _isHeadParse = true; // 标记头部已解析，下次进入消息体处理分支

                    return;
                }

                // 情况 4：消息体数据完整（当前剩余数据 >= 消息体长度）
                memcpy(_recvMsgNode->_data + _recvMsgNode->_curLen, _data + copy_len, data_len); // 复制完整消息体
                _recvMsgNode->_curLen += data_len;                                               // 更新已接收的消息体长度
                copy_len += data_len;                                                            // 已处理的数据长度增加
                bytes_transferred -= data_len;                                                   // 剩余未处理的数据长度减少

                // 终止符处理（假设消息以 '\0' 结尾，根据协议实际情况调整）
                _recvMsgNode->_data[_recvMsgNode->_totalLen] = '\0';
                json js = json::parse(std::string(_recvMsgNode->_data, _recvMsgNode->_totalLen));
                std::cout << "接收到完整消息: " << js["id"] << " " << js["data"] << std::endl;
                //js["data"] = json::array({"server has receieved msg , msg data is", js["data"]});
                // 业务逻辑：回显消息（示例）
                send(js.dump(),js["id"]);

                // 重置状态，准备处理下一条消息
                _isHeadParse = false;
                _recvMsgHead->clear(); // 清空头部接收节点

                // 若还有剩余数据未处理，继续循环（可能包含下一条消息的头部或体）
                if (bytes_transferred <= 0)
                {
                    memset(_data, 0, MAX_LENGTH); // 清空当前数据缓冲区
                    _socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
                                            std::bind(&CSession::handleRead, this, _1, _2, selfShared));
                    return;
                }
            }
            else
            {
                // ----------------------
                // 处理已解析头部的消息体（续传数据）
                // ----------------------
                int remain_msg = _recvMsgNode->_totalLen - _recvMsgNode->_curLen; // 消息体剩余未接收的字节数

                // 情况 1：当前剩余数据不足以填充消息体
                if (bytes_transferred < static_cast<size_t>(remain_msg))
                {
                    memcpy(_recvMsgNode->_data + _recvMsgNode->_curLen, _data + copy_len, bytes_transferred); // 复制部分数据
                    _recvMsgNode->_curLen += bytes_transferred;                                               // 更新已接收长度
                    memset(_data, 0, MAX_LENGTH);                                                             // 清空当前数据缓冲区

                    // 继续异步读取数据，补充剩余消息体
                    _socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
                                            std::bind(&CSession::handleRead, this, _1, _2, selfShared));

                    return;
                }

                // 情况 2：当前剩余数据足够填充消息体
                memcpy(_recvMsgNode->_data + _recvMsgNode->_curLen, _data + copy_len, remain_msg); // 复制完整剩余数据
                _recvMsgNode->_curLen += remain_msg;                                               // 更新已接收长度
                bytes_transferred -= remain_msg;                                                   // 剩余未处理的数据长度减少
                copy_len += remain_msg;                                                            // 已处理的数据长度增加

                // 终止符处理（同上）
                _recvMsgNode->_data[_recvMsgNode->_totalLen] = '\0';
                json js = json::parse(std::string(_recvMsgNode->_data, _recvMsgNode->_totalLen));
                std::cout << "接收到完整消息: " << js["id"] << " " << js["data"] << std::endl;
                //js["data"] = json::array({"server has receieved msg , msg data is", js["data"]});
                // 业务逻辑：回显消息（示例）
                send(js.dump(),js["id"]);
                // 重置状态，准备处理下一条消息
                _isHeadParse = false;
                _recvMsgHead->clear();

                // 若还有剩余数据未处理，继续循环（可能包含下一条消息）
                if (bytes_transferred <= 0)
                {
                    memset(_data, 0, MAX_LENGTH); // 清空当前数据缓冲区
                    _socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
                                            std::bind(&CSession::handleRead, this, _1, _2, selfShared));
                    return;
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
        close();                       // 关闭 socket 连接
        _server->clearCSession(_uuid); // 从服务器中移除当前会话
    }
}
