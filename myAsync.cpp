#include <iostream>
#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <functional>

using namespace boost::asio::ip;
using namespace std::placeholders;
const int maxLen = 1024;

class Session : public boost::enable_shared_from_this<Session> {
public:
    // 使用 shared_ptr 管理 Session 生命周期
    static boost::shared_ptr<Session> create(boost::asio::io_context& ioc) {
        return boost::shared_ptr<Session>(new Session(ioc));
    }

    tcp::socket& socket() { return m_socket; }

    void start() {
        // 启动异步读取，使用 shared_from_this() 保证生命周期
        doRead();
    }

private:
    Session(boost::asio::io_context& ioc) : m_socket(ioc) {}

    void doRead() {
        auto self(shared_from_this()); // 保持对象存活
        m_socket.async_read_some(
            boost::asio::buffer(m_data, maxLen),
            [this, self](const boost::system::error_code& ec, std::size_t bytesTransferred) {
                if (ec) {
                    if (ec != boost::asio::error::eof) {
                        std::cerr << "Read error: " << ec.message() << std::endl;
                    }
                    return;
                }
                std::cout << "Received: " << std::string(m_data, bytesTransferred) << std::endl;
                doWrite(bytesTransferred); // 发送接收到的数据（回显）
            }
        );
    }

    void doWrite(std::size_t length) {
        auto self(shared_from_this());
        boost::asio::async_write(
            m_socket,
            boost::asio::buffer(m_data, length), // 只发送实际接收到的数据
            [this, self](const boost::system::error_code& ec, std::size_t) {
                if (ec) {
                    std::cerr << "Write error: " << ec.message() << std::endl;
                    return;
                }
                doRead(); // 继续读取下一条消息
            }
        );
    }

    tcp::socket m_socket;
    char m_data[maxLen];
};

// 示例：使用 Acceptor 接受连接并创建 Session
class Server {
public:
    Server(boost::asio::io_context& ioc, short port)
        : m_acceptor(ioc, tcp::endpoint(tcp::v4(), port)) {
        doAccept();
    }

private:
    void doAccept() {
        m_acceptor.async_accept(
            [this](const boost::system::error_code& ec, tcp::socket socket) {
                if (!ec) {
                    auto session = Session::create(socket.get_executor().context());
                    session->socket() = std::move(socket);
                    session->start();
                }
                doAccept(); // 继续接受新连接
            }
        );
    }

    tcp::acceptor m_acceptor;
};

int main() {
    boost::asio::io_context io;
    Server server(io, 8080); // 监听 8080 端口
    io.run(); // 启动事件循环
    return 0;
}