#include <boost/asio.hpp>
#include <iostream>

using namespace boost::asio::ip;
using namespace std::placeholders;

class Session
{
public:

    Session(boost::asio::io_context &ioc) : _socket(ioc)
    {
    }
    tcp::socket &Socket()
    {
        return _socket;
    }

    void Start()
    {
        memset(_data, 0, max_length);
        _socket.async_read_some(boost::asio::buffer(_data, max_length),
                                std::bind(&Session::handleRead, this, _1,
                                          _2));
    }

private:

    void handleRead(const boost::system::error_code &error, size_t bytes_transfered)
    {
        if (!error)
        {
            std::cout << "server receive data is " << _data << std::endl;
            boost::asio::async_write(_socket, boost::asio::buffer(_data, bytes_transfered),
                                     std::bind(&Session::handleWrite, this, _1));
        }
        else
        {
            delete this;
        }
    }

    void handleWrite(const boost::system::error_code &error)
    {
        if (!error)
        {
            memset(_data, 0, max_length);
            _socket.async_read_some(boost::asio::buffer(_data, max_length), std::bind(&Session::handleRead,
                                                                                      this, _1, _2));
        }
        else
        {
            delete this;
        }
    }
    tcp::socket _socket;
    enum
    {
        max_length = 1024
    };
    char _data[max_length];
};

class Server
{
public:
    Server(boost::asio::io_context &ioc, short port) : _ioc(ioc),
                                                               _acceptor(ioc, tcp::endpoint(tcp::v4(), port))
    {
        startAccept();
    }

private:
    void startAccept()
    {
        Session *new_session = new Session(_ioc);
        _acceptor.async_accept(new_session->Socket(),
                               std::bind(&Server::handleAccept, this, new_session, _1));
    }
    void handleAccept(Session *new_session, const boost::system::error_code &error)
    {
        if (!error)
        {
            new_session->Start();
        }
        else
        {
            delete new_session;
        }
        startAccept();
    }
    boost::asio::io_context &_ioc;
    tcp::acceptor _acceptor;
};

int main()
{
    try
    {
        boost::asio::io_context io_context;
        Server server(io_context, 8888);
        io_context.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
        return 1;
    }
    return 0;
}