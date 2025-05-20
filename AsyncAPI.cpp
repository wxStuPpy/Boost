#include <boost/asio.hpp>
#include <iostream>
#include <memory.h>
#include <queue>

using namespace boost::asio::ip;
using namespace std::placeholders;
const int RECVSIZE = 1024;

// 消息节点类，封装待发送或接收的数据
class MsgNode
{
    friend class Session;

public:
    // 构造函数：从外部复制数据到缓冲区（用于发送）
    MsgNode(const char *msg, int totalLen) : m_totalLen(totalLen), m_curLen(0)
    {
        m_msg = new char[totalLen];
        std::memcpy(m_msg, msg, totalLen);
    }
    
    // 构造函数：仅分配缓冲区，不复制数据（用于接收）
    MsgNode(int totalLen) : m_totalLen(totalLen), m_curLen(0)
    {
        m_msg = new char[totalLen];
    }
    
    // 析构函数：释放动态分配的缓冲区
    ~MsgNode()
    {
        delete[] m_msg;
    }

private:
    int m_totalLen;    // 消息总长度
    int m_curLen;      // 已发送/接收的长度（用于分段操作）
    char *m_msg;       // 数据缓冲区
};

// 会话类，管理TCP连接和数据收发
class Session
{
public:
    // 构造函数：接收共享的socket指针并初始化状态
    Session(std::shared_ptr<tcp::socket> socket) 
        : m_socket(socket), m_sendPending(false), m_recvPending(false)
    {
    }
    
    // 同步连接到指定端点
    void connect(const tcp::endpoint &ep)
    {
        m_socket->connect(ep);
    }
    
    // 旧版发送回调函数（存在问题，不建议使用）
    void writeCallBackErr(const boost::system::error_code &ec, std::size_t bytesTransferred, std::shared_ptr<MsgNode> sendNode)
    {
        // 若未发送完所有数据，继续发送剩余部分
        if (bytesTransferred + sendNode->m_curLen < sendNode->m_totalLen)
        {
            sendNode->m_curLen += bytesTransferred;
            this->m_socket->async_write_some(boost::asio::buffer(sendNode->m_msg + sendNode->m_curLen, sendNode->m_totalLen - sendNode->m_curLen),
                                             std::bind(&Session::writeCallBackErr, this, _1, _2, sendNode));
        }
    }
    
    // 旧版发送函数（存在问题，不建议使用）
    // 问题：每次调用覆盖当前发送节点，可能导致数据覆盖
    void writeToSocketErr(const std::string buf)
    {
        m_sendNode = std::make_shared<MsgNode>(buf.c_str(), buf.length());
        this->m_socket->async_write_some(boost::asio::buffer(m_sendNode->m_msg, m_sendNode->m_totalLen),
                                         std::bind(&Session::writeCallBackErr, this, _1, _2, m_sendNode));
    }

    // 改进的异步发送函数（推荐使用）
    void WriteToSocket(const std::string &buf)
    {
        // 将消息封装为节点并加入发送队列
        auto node = std::make_shared<MsgNode>(buf.c_str(), buf.length());
        m_sendQueue.emplace(node);
        
        // 若已有未完成的发送操作，则不启动新的发送
        if (m_sendPending == true)
        {
            return;
        }
        
        // 启动异步发送（async_write_some可能只发送部分数据）
        this->m_socket->async_write_some(boost::asio::buffer(buf), std::bind(&Session::WriteCallBack, this, _1, _2));
        m_sendPending = true;  // 标记有活跃的发送操作
    }
    
    // 改进的发送回调函数
    void WriteCallBack(const boost::system::error_code &ec, std::size_t bytesTransfered)
    {
        // 检查错误
        if (ec.value() != 0)
        {
            std::cout << "发送错误, 错误码: " << ec.value() << " 错误信息: " << ec.message();
            return;
        }
        
        // 获取队首元素（当前正在发送的消息）
        auto &send_data = m_sendQueue.front();
        send_data->m_curLen += bytesTransfered;
        
        // 若数据未发送完，则继续发送剩余部分
        if (send_data->m_curLen < send_data->m_totalLen)
        {
            this->m_socket->async_write_some(boost::asio::buffer(send_data->m_msg + send_data->m_curLen, send_data->m_totalLen - send_data->m_curLen),
                                             std::bind(&Session::WriteCallBack, this, _1, _2));
            return;
        }
        
        // 若发送完成，则从队列中移除当前消息
        m_sendQueue.pop();
        
        // 更新发送状态
        if (m_sendQueue.empty())
        {
            m_sendPending = false;  // 队列为空，标记无活跃发送
        }
        else
        {
            // 队列非空，继续发送下一条消息
            auto &next_data = m_sendQueue.front();
            this->m_socket->async_write_some(boost::asio::buffer(next_data->m_msg + next_data->m_curLen, next_data->m_totalLen - next_data->m_curLen),
                                             std::bind(&Session::WriteCallBack, this, _1, _2));
        }
    }

    // 使用async_send的发送函数（注意：async_send可能不是标准函数，存在问题）
    void WriteAllToSocket(const std::string &buf)
    {
        // 将消息加入发送队列
        m_sendQueue.emplace(new MsgNode(buf.c_str(), buf.length()));
        
        // 若已有未完成的发送操作，则不启动新的发送
        if (m_sendPending)
        {
            return;
        }
        
        // 启动异步发送（假设async_send等同于async_write，会发送全部数据）
        this->m_socket->async_send(boost::asio::buffer(buf),
                                   std::bind(&Session::WriteAllCallBack, this, _1, _2));
        m_sendPending = true;
    }
    
    // async_send的回调函数
    void WriteAllCallBack(const boost::system::error_code &ec, std::size_t bytesTransfered)
    {
        // 检查错误
        if (ec.value() != 0)
        {
            std::cout << "发送错误! 错误码 = " << ec.value() << ". 错误信息: " << ec.message();
            return;
        }
        
        // 假设所有数据已发送完成，从队列中移除当前消息
        m_sendQueue.pop();
        
        // 更新发送状态
        if (m_sendQueue.empty())
        {
            m_sendPending = false;
        }
        else
        {
            // 继续发送下一条消息
            auto &next_data = m_sendQueue.front();
            this->m_socket->async_send(boost::asio::buffer(next_data->m_msg + next_data->m_curLen, next_data->m_totalLen - next_data->m_curLen),
                                       std::bind(&Session::WriteAllCallBack, this, _1, _2));
        }
    }

    // 异步读取数据（分段接收）
    void readFromSocket()
    {
        // 若已有未完成的接收操作，则不启动新的接收
        if (m_recvPending == true)
        {
            return;
        }
        
        // 创建接收缓冲区
        m_recvNode = std::make_shared<MsgNode>(RECVSIZE);
        m_socket->async_read_some(boost::asio::buffer(m_recvNode->m_msg, m_recvNode->m_totalLen),
                                  std::bind(&Session::readCallBack, this, _1, _2));
        m_recvPending = true;
    }
    
    // 异步读取回调（分段接收）
    void readCallBack(const boost::system::error_code &ec, std::size_t bytesTransfered)
    {
        // 检查错误（此处省略，实际应添加错误处理）
        
        // 更新已接收长度
        m_recvNode->m_curLen += bytesTransfered;
        
        // 若未接收完指定长度的数据，继续接收
        if (m_recvNode->m_curLen < m_recvNode->m_totalLen)
        {
            m_socket->async_read_some(boost::asio::buffer(m_recvNode->m_msg + m_recvNode->m_curLen, m_recvNode->m_totalLen - m_recvNode->m_curLen),
                                      std::bind(&Session::readCallBack, this, _1, _2));
            return;
        }
        
        // 接收完成，处理数据（此处仅重置状态，实际应添加数据处理逻辑）
        m_recvPending = false;
        m_recvNode = nullptr;
    }

    // 异步读取全部数据（假设一次接收完）
    void readAllFromSocket()
    {
        // 若已有未完成的接收操作，则不启动新的接收
        if (m_recvPending == true)
        {
            return;
        }
        
        // 创建接收缓冲区
        m_recvNode = std::make_shared<MsgNode>(RECVSIZE);
        this->m_socket->async_receive(boost::asio::buffer(m_recvNode->m_msg, m_recvNode->m_totalLen),
                                      std::bind(&Session::readAllCallBack, this, _1, _2));
        m_recvPending = true;
    }
    
    // 异步读取全部数据回调
    void readAllCallBack(const boost::system::error_code &ec, std::size_t bytesTransfered)
    {
        // 检查错误（此处省略，实际应添加错误处理）
        
        // 更新已接收长度
        m_recvNode->m_curLen += bytesTransfered;
        
        // 接收完成，处理数据（此处仅重置状态，实际应添加数据处理逻辑）
        m_recvPending = false;
        m_recvNode = nullptr;
    }

private:
    std::shared_ptr<tcp::socket> m_socket;            // 共享的TCP套接字
    std::shared_ptr<MsgNode> m_sendNode;              // 当前发送节点（用于旧版接口）
    std::queue<std::shared_ptr<MsgNode>> m_sendQueue; // 发送消息队列
    std::shared_ptr<MsgNode> m_recvNode;              // 当前接收节点
    bool m_sendPending;                               // 标记是否有未完成的发送操作
    bool m_recvPending;                               // 标记是否有未完成的接收操作
};