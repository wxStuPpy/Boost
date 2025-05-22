#pragma once

#include <boost/asio.hpp>
#include <iostream>
#include <map>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <queue>
#include <mutex>
#include "Server.h"

using namespace boost::asio::ip;
using namespace std::placeholders;

#define MAX_LENGTH 1024 * 2
#define HEAD_LENGTH 2
#define MAX_QUEUE_SIZE 1000

class Server;

class MsgNode
{
    friend class CSession;

public:
    MsgNode(char *msg, int maxLen) : _totalLen(maxLen + HEAD_LENGTH), _curLen(0)
    {
        _data = new char[_totalLen + 1];
        // 网络字节序
        int maxLenHost = boost::asio::detail::socket_ops::host_to_network_short(maxLen);
        memcpy(_data, &maxLenHost, HEAD_LENGTH);
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
        _curLen = 0;
    }

private:
    int _totalLen;
    int _curLen;
    char *_data;
};

class CSession : public std::enable_shared_from_this<CSession>
{
public:
    CSession(boost::asio::io_context &ioc, Server *server);
    ~CSession();
    tcp::socket &Socket();
    std::string getUuid()const;
    void Start();
    void close();

private:
    void handleRead(const boost::system::error_code &error, size_t bytes_transfered, std::shared_ptr<CSession> selfShared);
    void handleWrite(const boost::system::error_code &error, std::shared_ptr<CSession> selfShared);
    void send(char *msg, int max_length);
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