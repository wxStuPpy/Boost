#pragma once

#include <boost/asio.hpp>
#include <iostream>
#include <map>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <queue>
#include <mutex>
#include "MsgNode.h"
#include "const.h"

using namespace boost::asio::ip;
using namespace std::placeholders;

class Server;

class CSession : public std::enable_shared_from_this<CSession>
{
public:
    CSession(boost::asio::io_context &ioc, Server *server);
    ~CSession();
    tcp::socket &Socket();
    std::string getUuid() const;
    void Start();
    void close();
    void send(std::string msg);

private:
    void handleReadHead(const boost::system::error_code &error, size_t bytes_transfered, std::shared_ptr<CSession> selfShared);
    void handleReadBody(const boost::system::error_code &error, size_t bytes_transfered, std::shared_ptr<CSession> selfShared);
    void handleRead(const boost::system::error_code &error, size_t bytes_transfered, std::shared_ptr<CSession> selfShared);
    void handleWrite(const boost::system::error_code &error, std::shared_ptr<CSession> selfShared);

    tcp::socket _socket;
    char _data[MAX_LENGTH];
    Server *_server;
    std::string _uuid;
    std::queue<std::shared_ptr<MsgNode>> _sendQueue;
    std::mutex _sendMutex;
    // 收到的消息结构
    std::shared_ptr<MsgNode> _recvMsgNode;
    // 收到的头部结构
    std::shared_ptr<MsgNode> _recvMsgHead;
    bool _isHeadParse;
    bool _isClose;
};