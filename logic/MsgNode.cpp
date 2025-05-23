#include "MsgNode.h"
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

MsgNode:: MsgNode(short len) : _curLen(0), _totalLen(len)
{
    _data = new char[_totalLen + 1];
    _data[_totalLen] = '\0';
}

MsgNode:: ~MsgNode()
{
    std::cout << "destruct MsgNode" << std::endl;
    if (_data)
    {
        delete[] _data;
    }
    _data = nullptr;
}

void MsgNode::clear(){
    std::memset(_data, 0, _totalLen);
        _curLen = 0;
}

RecvNode:: RecvNode(short len, short msgId):MsgNode(len),_msgId(msgId){

}

SendNode:: SendNode(const char *msg, short len, short msgId):MsgNode(len+HEAD_TOTAL_LEN),_msgId(msgId){
    //先发id
    short msgIdHost=boost::asio::detail::socket_ops::host_to_network_short(_msgId);
    memcpy(_data,&msgIdHost,HEAD_ID_LEN);
    //再发长度
    short lenHost=boost::asio::detail::socket_ops::host_to_network_short(len);
    memcpy(_data+HEAD_ID_LEN,&lenHost,HEAD_DATA_LEN);
    //发送消息体
    memcpy(_data+HEAD_TOTAL_LEN,msg,len);
}