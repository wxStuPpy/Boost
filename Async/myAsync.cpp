#include <iostream>
#include <boost/asio.hpp>
#include <memory>

using namespace boost::asio::ip;
constexpr size_t maxLen = 1024;

class Session : public std::enable_shared_from_this<Session>
{
public:
    Session(tcp::socket socket) : m_socket(std::move(socket)) {}
    
    void start() {
        doRead();
    }
    
private:
    void doRead() {
        auto self(shared_from_this());
        m_socket.async_read_some(
            boost::asio::buffer(m_data),
            [this, self](boost::system::error_code ec, std::size_t bytes) {
                if (ec) {
                    handleError(ec);
                    return;
                }
                std::cout << "Received: " << std::string(m_data, bytes) << "\n";
                doWrite(bytes);
            });
    }
    
    void doWrite(std::size_t length) {
        auto self(shared_from_this());
        m_socket.async_send(
            boost::asio::buffer(m_data, length),
            [this, self](boost::system::error_code ec, std::size_t) {
                if (ec) {
                    handleError(ec);
                    return;
                }
                doRead();
            });
    }
    
    void handleError(const boost::system::error_code& ec) {
        if (ec == boost::asio::error::eof) {
            std::cout << "Client disconnected\n";
        } else if (ec != boost::asio::error::operation_aborted) {
            std::cerr << "Error: " << ec.message() << "\n";
        }
    }

    tcp::socket m_socket;
    char m_data[maxLen]{};
};

class Server
{
public:
    Server(boost::asio::io_context& io_context, short port)
        : acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
    {
        doAccept();
    }

private:
    void doAccept() {
        acceptor_.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket) {
                if (!ec) {
                    std::make_shared<Session>(std::move(socket))->start();
                } else {
                    std::cerr << "Accept error: " << ec.message() << "\n";
                }
                doAccept();
            });
    }

    tcp::acceptor acceptor_;
};

int main()
{
    try {
        boost::asio::io_context io_context;
        Server server(io_context, 8888);
        io_context.run();
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        return 1;
    }
    return 0;
}