#include <boost/asio.hpp>
#include <iostream>
#include <memory.h>
#include <queue>

using namespace boost::asio::ip;
using namespace std::placeholders;
const int RECVSIZE = 1024;

class Session;

class MsgNode
{
    friend class Session;

public:
    MsgNode(const char *msg, int totalLen) : m_totalLen(totalLen), m_curLen(0)
    {
        m_msg = new char[totalLen];
        std::memcpy(m_msg, msg, totalLen);
    }
    MsgNode(int totalLen) : m_totalLen(totalLen), m_curLen(0)
    {
        m_msg = new char[totalLen];
    }
    ~MsgNode()
    {
        delete[] m_msg;
    }

private:
    int m_totalLen;
    int m_curLen;
    char *m_msg;
};

class Session
{
public:
    Session(std::shared_ptr<tcp::socket> socket) : m_socket(socket)
    {
    }
    void connect(const tcp::endpoint &ep)
    {
        m_socket->connect(ep);
    }
    void writeCallBackErr(const boost::system::error_code &ec, std::size_t bytesTransferred, std::shared_ptr<MsgNode> sendNode)
    {
        if (bytesTransferred + sendNode->m_curLen < sendNode->m_totalLen)
        {
            sendNode->m_curLen += bytesTransferred;
            this->m_socket->async_write_some(boost::asio::buffer(sendNode->m_msg + sendNode->m_curLen, sendNode->m_totalLen - sendNode->m_curLen),
                                             std::bind(&Session::writeCallBackErr, this, _1, _2, sendNode));
        }
        return;
    }
    void writeToSocketErr(const std::string buf)
    {
        m_sendNode = std::make_shared<MsgNode>(buf.c_str(), buf.length());
        this->m_socket->async_write_some(boost::asio::buffer(m_sendNode->m_msg, m_sendNode->m_totalLen),
                                         std::bind(&Session::writeCallBackErr, this, _1, _2, m_sendNode));
    }

    void WriteToSocket(const std::string &buf)
    {
        // 插入发送队列
        auto node = std::make_shared<MsgNode>(buf.c_str(), buf.length());
        m_sendQueue.emplace(node);
        // pending状态说明上一次有未发送完的数据
        if (m_sendPending == true)
        {
            return;
        }
        // 异步发送数据，因为异步所以不会一下发送完
        this->m_socket->async_write_some(boost::asio::buffer(buf), std::bind(&Session::WriteCallBack, this, _1, _2));
        m_sendPending = true;
    }
    void Session::WriteCallBack(const boost::system::error_code &ec, std::size_t bytes_transferred)
    {
        if (ec.value() != 0)
        {
            std::cout << "Error , code is " << ec.value() << " . Message is " << ec.message();
            return;
        }
        // 取出队首元素即当前未发送完数据
        auto &send_data = m_sendQueue.front();
        send_data->m_curLen += bytes_transferred;
        // 数据未发送完， 则继续发送
        if (send_data->m_curLen < send_data->m_totalLen)
        {
            this->m_socket->async_write_some(boost::asio::buffer(send_data->m_msg + send_data->m_curLen, send_data->m_totalLen - send_data->m_curLen),
                                             std::bind(&Session::WriteCallBack,
                                                       this, _1, _2));
            return;
        }
        // 如果发送完，则pop出队首元素
        m_sendQueue.pop();
        // 如果队列为空，则说明所有数据都发送完,将pending设置为false
        if (m_sendQueue.empty())
        {
            m_sendPending = false;
        }
        // 如果队列不是空，则继续将队首元素发送
        if (!m_sendQueue.empty())
        {
            auto &send_data = m_sendQueue.front();
            this->m_socket->async_write_some(boost::asio::buffer(send_data->m_msg + send_data->m_curLen, send_data->m_totalLen - send_data->m_curLen),
                                             std::bind(&Session::WriteCallBack,
                                                       this, _1, _2));
        }
    }
        // 不能与async_write_some混合使用
        void WriteAllToSocket(const std::string &buf)
        {
            // 插入发送队列
            m_sendQueue.emplace(new MsgNode(buf.c_str(), buf.length()));
            // pending状态说明上一次有未发送完的数据
            if (m_sendPending)
            {
                return;
            }
            // 异步发送数据，因为异步所以不会一下发送完
            this->m_socket->async_send(boost::asio::buffer(buf),
                                       std::bind(&Session::WriteAllCallBack, this,
                                                 _1, _2));
            m_sendPending = true;
        }
        void WriteAllCallBack(const boost::system::error_code &ec, std::size_t bytes_transferred)
        {
            if (ec.value() != 0)
            {
                std::cout << "Error occured! Error code = "
                          << ec.value()
                          << ". Message: " << ec.message();
                return;
            }
            // 如果发送完，则pop出队首元素
            m_sendQueue.pop();
            // 如果队列为空，则说明所有数据都发送完,将pending设置为false
            if (m_sendQueue.empty())
            {
                m_sendPending = false;
            }
            // 如果队列不是空，则继续将队首元素发送
            if (!m_sendQueue.empty())
            {
                auto &send_data = m_sendQueue.front();
                this->m_socket->async_send(boost::asio::buffer(send_data->m_msg + send_data->m_curLen, send_data->m_totalLen - send_data->m_curLen),
                                           std::bind(&Session::WriteAllCallBack,
                                                     this, _1, _2));
            }
        }
private:
    std::shared_ptr<tcp::socket> m_socket;
    std::shared_ptr<MsgNode> m_sendNode;
    std::queue<std::shared_ptr<MsgNode>> m_sendQueue;
    bool m_sendPending;
};