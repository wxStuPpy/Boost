#include <iostream>
#include <boost/asio.hpp>
#include <functional>

using namespace boost::asio::ip;
using namespace std::placeholders;
const int maxLen = 1024;

class Session
{
public:
    // 修改构造函数，直接接受socket对象
    Session(tcp::socket socket) : m_socket(std::move(socket))
    {
    }
    
    void start()
    {
        doRead();
    }
    
    void doRead()
    {
        std::memset(m_data, 0, maxLen);
        m_socket.async_read_some(boost::asio::buffer(m_data, maxLen),
                                 std::bind(&Session::readHandle, this, _1, _2));
    }
    
    void readHandle(const boost::system::error_code &ec, std::size_t bytesTransferred)
    {
        if (ec)
        {
            if (ec != boost::asio::error::eof)
            {
                std::cout << "Read error: " << ec.message() << "\n";
            }
            return;
        }
        std::cout << "recv data = " << std::string(m_data, bytesTransferred) << "\n";
        doWrite(bytesTransferred);  // 修改：传入实际接收的字节数
    }
    
    void doWrite(std::size_t length)  // 修改：增加长度参数
    {
        // 修改：只发送实际接收的数据，不清空缓冲区
        m_socket.async_send(boost::asio::buffer(m_data, length),
                            std::bind(&Session::writeHandle, this, _1));
    }
    
    void writeHandle(const boost::system::error_code &ec)
    {
        if (ec)
        {
            return;
        }
        doRead();
    }

private:
    tcp::socket m_socket;
    char m_data[maxLen];
};

// 添加Server类来管理连接
class Server
{
public:
    Server(boost::asio::io_context& io_context, short port)
        : acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
    {
        do_accept();
    }

private:
    void do_accept()
    {
        acceptor_.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket)
            {
                if (!ec)
                {
                    // 修改：直接创建Session并转移socket所有权
                    std::make_shared<Session>(std::move(socket))->start();
                }
                do_accept();
            });
    }

    tcp::acceptor acceptor_;
};

int main()
{
    try
    {
        boost::asio::io_context io_context;
        Server server(io_context, 8080);  // 监听8080端口
        io_context.run();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}