#include "MsgNode.h"
#include "const.h"
#include <boost/asio.hpp>
#include <iostream>

MsgNode::MsgNode(short len) : _curLen(0), _totalLen(len) {
  _data = new char[_totalLen + 1];
  _data[_totalLen] = '\0';
}

MsgNode::~MsgNode() {
  std::cout << "destruct MsgNode" << std::endl;
  if (_data) {
    delete[] _data;
  }
  _data = nullptr;
}

void MsgNode::clear() {
  std::memset(_data, 0, _totalLen);
  _curLen = 0;
}

RecvNode::RecvNode(short len, short msgID) : MsgNode(len), _msgID(msgID) {}

short RecvNode::getMsgID() const { return _msgID; }

SendNode::SendNode(const char *msg, short len, short msgID)
    : MsgNode(len + HEAD_TOTAL_LEN), _msgID(msgID) {
  // 先发id
  short msgIDHost =
      boost::asio::detail::socket_ops::host_to_network_short(_msgID);
  memcpy(_data, &msgIDHost, HEAD_ID_LEN);
  // 再发长度
  short lenHost = boost::asio::detail::socket_ops::host_to_network_short(len);
  memcpy(_data + HEAD_ID_LEN, &lenHost, HEAD_DATA_LEN);
  // 发送消息体
  memcpy(_data + HEAD_TOTAL_LEN, msg, len);
}

short SendNode::getMsgID() const { return _msgID; }