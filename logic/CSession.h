#pragma once

#include "MsgNode.h"
#include "Server.h"
#include "const.h"
#include <boost/asio.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <memory>
#include <mutex>
#include <queue>

using namespace boost::asio::ip;
using namespace std::placeholders;

class Server;

class CSession : public std::enable_shared_from_this<CSession> {
public:
  CSession(boost::asio::io_context &ioc, Server *server);
  ~CSession();
  tcp::socket &getSocket();
  std::string getUuid() const;
  void Start();
  void close();
  void send(std::string msg, short msgID);

private:
  void handleRead(const boost::system::error_code &error,
                  size_t bytes_transfered,
                  std::shared_ptr<CSession> selfShared);
  void handleWrite(const boost::system::error_code &error,
                   std::shared_ptr<CSession> selfShared);

  tcp::socket _socket;
  char _data[MAX_LENGTH];
  Server *_server;
  std::string _uuid;
  std::queue<std::shared_ptr<SendNode>> _sendQueue;
  std::mutex _sendMutex;
  // 收到的消息结构
  std::shared_ptr<RecvNode> _recvMsgNode;
  // 收到的头部结构
  std::shared_ptr<RecvNode> _recvMsgHead;
  bool _isHeadParse;
  bool _isClose;
};

class LogicNode {
  friend class LogicSystem;

public:
  LogicNode(std::shared_ptr<CSession>, std::shared_ptr<RecvNode>);

private:
  std::shared_ptr<CSession> _session;
  std::shared_ptr<RecvNode> _recvNode;
};