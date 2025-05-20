#include <iostream>
#include <boost/asio.hpp>
#include <memory>

using namespace boost::asio;
using namespace boost::asio::ip;

const int maxLen = 1024;

class Session : public std::enable_shared_from_this<Session>
{
public:
    Session(io_context &ioc) : m_socket(ioc) {}
    static std::shared_ptr<Session> create(io_context &ioc)
    {
        return std::make_shared<Session>(ioc);
    }

    tcp::socket &socket() { return m_socket; }

    void start()
    {
        doRead();
    }

private:
    void doRead()
    {
        auto self = shared_from_this();
        m_socket.async_read_some(
            buffer(m_data, maxLen),
            [self](const boost::system::error_code &ec, std::size_t bytes)
            {
                self->handleRead(ec, bytes);
            });
    }

    void handleRead(const boost::system::error_code &ec, std::size_t bytesTransferred)
    {
        if (ec)
        {
            if (ec != error::eof)
            {
                std::cout << "Read error: " << ec.message() << "\n";
            }
            return;
        }
        std::cout << "recv data = " << std::string(m_data, bytesTransferred) << "\n";
        doWrite(bytesTransferred);
    }

    void doWrite(std::size_t bytesToWrite)
    {
        auto self = shared_from_this();
        async_write(
            m_socket,
            buffer(m_data, bytesToWrite),
            [self](const boost::system::error_code &ec, std::size_t)
            {
                if (!ec)
                {
                    std::memset(self->m_data, 0, maxLen);
                    self->doRead();
                }
            });
    }

    tcp::socket m_socket;
    char m_data[maxLen] = {};
};

class Server
{
public:
    Server(io_context &ioc, unsigned short port)
        : m_ioc(ioc), m_acceptor(ioc, tcp::endpoint(tcp::v4(), port))
    {
        std::cout << "Server started on port " << port << "\n";
        startAccept();
    }

private:
    void startAccept()
    {
        auto newSession = Session::create(m_ioc);
        m_acceptor.async_accept(
            newSession->socket(),
            [this, newSession](const boost::system::error_code &ec)
            {
                handleAccept(newSession, ec);
            });
    }

    void handleAccept(std::shared_ptr<Session> newSession, const boost::system::error_code &ec)
    {
        if (!ec)
        {
            newSession->start();
        }
        startAccept();
    }

    io_context &m_ioc;
    tcp::acceptor m_acceptor;
};

int main()
{
    try
    {
        io_context ioc;
        Server server(ioc, 8888);
        ioc.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    return 0;
}