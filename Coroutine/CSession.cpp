#include "CSession.h"
#include "LogicSystem.h"
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>

using nlohmann::json;

CSession::CSession(boost::asio::io_context &ioc, Server *server)
    : _ioc(ioc), _socket(ioc), _server(server), _isHeadParse(false), _isClose(false)
{
  boost::uuids::uuid a_uuid = boost::uuids::random_generator()();
  _uuid = boost::uuids::to_string(a_uuid);
  _recvMsgHead = std::make_shared<RecvNode>(HEAD_TOTAL_LEN);
}

CSession::~CSession()
{
  std::cout << "~CSession " << _uuid << " destruct" << std::endl;
  close();
}

tcp::socket &CSession::getSocket() { return _socket; }

std::string CSession::getUuid() const { return _uuid; }

void CSession::close()
{
  _socket.close();
}

void CSession::Start()
{
  auto shared_this = shared_from_this();
  // 开始协程接收
  boost::asio::co_spawn(_ioc, [=,this]() -> boost::asio::awaitable<void>
  {
    try
    {
      while (!_isClose)
        {
          _recvMsgHead->clear();
          // cpp 20co_wait co_return
          size_t len = co_await boost::asio::async_read(_socket,
                                                        boost::asio::buffer(_recvMsgHead->_data, HEAD_TOTAL_LEN),
                                                        boost::asio::use_awaitable);
          if (len == 0)
          {
            close();
            _server->clearCSession(_uuid);
            co_return;
          }

          // 获取头部ID信息
          short msgID = 0;
          std::memcpy(&msgID, _recvMsgHead->_data, HEAD_ID_LEN);
          // 转为本地字节序
          msgID = boost::asio::detail::socket_ops::network_to_host_short(msgID);
          std::cout << "msgID is" << msgID << std::endl;
          if (msgID > MAX_LENGTH)
          {
            std::cout << "invalid msg_id " << msgID << std::endl;
            _server->clearCSession(_uuid);
            co_return;
          }
          // 从头部解析出消息体长度
          short data_len = 0;
          std::memcpy(&data_len, _recvMsgHead->_data + HEAD_ID_LEN,
                      HEAD_DATA_LEN); // 读取头部中的数据长度
          // 转为本地字节序
          data_len =
              boost::asio::detail::socket_ops::network_to_host_short(data_len);
          std::cout << "解析到消息体长度: " << data_len << std::endl;

          // 校验消息体长度是否合法（防止非法数据导致缓冲区溢出）
          if (data_len > MAX_LENGTH)
          {
            std::cout << "非法消息长度: " << data_len
                      << ", 最大允许长度: " << MAX_LENGTH << std::endl;
            _server->clearCSession(_uuid); // 清除会话
            co_return;
          }

          _recvMsgNode = std::make_shared<RecvNode>(data_len, msgID);
          // 读出包体
          len = co_await boost::asio::async_read(_socket,
           boost::asio::buffer(_recvMsgNode->_data, _recvMsgNode->_totalLen),
           boost::asio::use_awaitable);
          if(len==0){
            std::cout<<"recvive peer closed"<<std::endl;
            _server->clearCSession(_uuid);
            co_return;
          }
          std::cout<<"receive data is"<<_recvMsgNode->_data<<std::endl;
        }
    }
    catch (const std::exception &e)
    {
      std::cerr << e.what() << '\n';
      close();
      _isClose = true;
    } 
  }
  ,boost::asio::detached);
}

void CSession::handleWrite(const boost::system::error_code &error,
                           std::shared_ptr<CSession> selfShared)
{
  if (!error)
  {
    std::lock_guard<std::mutex> lock(_sendMutex);
    _sendQueue.pop();
    if (!_sendQueue.empty())
    {
      auto &msgNode = _sendQueue.front();
      boost::asio::async_write(
          _socket, boost::asio::buffer(msgNode->_data, msgNode->_totalLen),
          std::bind(&CSession::handleWrite, this, _1, selfShared));
    }
  }
  else
  {
    std::cerr << "write error: " << error.message() << std::endl;
    close();
    _server->clearCSession(_uuid);
  }
}

void CSession::send(std::string msg, short msgID)
{
  bool pending = false;
  std::lock_guard<std::mutex> lock(_sendMutex);
  if (_sendQueue.size() > MAX_QUEUE_SIZE)
  {
    std::cout << "sendQueue is fulled, size is" << MAX_QUEUE_SIZE << std::endl;
    return;
  }
  if (_sendQueue.size() > 0)
  {
    pending = true;
  }
  _sendQueue.push(std::make_shared<SendNode>(msg.c_str(), msg.length(), msgID));
  if (pending)
  {
    return;
  }
  auto &msgnode = _sendQueue.front();
  boost::asio::async_write(
      _socket, boost::asio::buffer(msgnode->_data, msgnode->_totalLen),
      std::bind(&CSession::handleWrite, this, _1, shared_from_this()));
}

LogicNode::LogicNode(std::shared_ptr<CSession> session,
                     std::shared_ptr<RecvNode> recvnode)
    : _session(session), _recvNode(recvnode) {}
