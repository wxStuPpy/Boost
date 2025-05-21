#include <boost/asio.hpp>
#include <iostream>
#include <map>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <queue>
#include <mutex>

using namespace boost::asio::ip;
using namespace std::placeholders;

class MsgNode
{
    friend class CSession;

public:
    MsgNode(char *msg, int max_len)
    {
        _data = new char[max_len];
        memcpy(_data, msg, max_len);
    }
    ~MsgNode()
    {
        delete[] _data;
    }

private:
    int _cur_len;
    int _max_len;
    char *_data;
};

class Session;

class Server
{
public:
    Server(boost::asio::io_context &ioc, short port);
    void clearSession(std::string uuid); // 声明成员函数

private:
    void startAccept();
    void handleAccept(std::shared_ptr<Session> newSession, const boost::system::error_code &error);

    boost::asio::io_context &_ioc;
    tcp::acceptor _acceptor;
    std::map<std::string, std::shared_ptr<Session>> _sessions;
};

class Session : public std::enable_shared_from_this<Session>
{
public:
    Session(boost::asio::io_context &ioc, Server *server) : _socket(ioc), _server(server)
    {
        boost::uuids::uuid a_uuid = boost::uuids::random_generator()();
        _uuid = boost::uuids::to_string(a_uuid);
    }

    tcp::socket &Socket() { return _socket; }
    std::string getUuid() const { return _uuid; }
    void Start();

private:
    void handleRead(const boost::system::error_code &error, size_t bytes_transfered, std::shared_ptr<Session> selfShared);
    void handleWrite(const boost::system::error_code &error, std::shared_ptr<Session> selfShared);
    void send(char *msg, int max_length);
    tcp::socket _socket;
    enum
    {
        max_length = 1024
    };
    char _data[max_length];
    Server *_server;
    std::string _uuid;
    std::queue<std::shared_ptr<MsgNode>> _sendQueue;
    std::mutex _sendMutex;
};

Server::Server(boost::asio::io_context &ioc, short port)
    : _ioc(ioc), _acceptor(ioc, tcp::endpoint(tcp::v4(), port))
{
    startAccept();
}

void Server::clearSession(std::string uuid)
{
    _sessions.erase(uuid);
}

void Server::startAccept()
{
    auto newSession = std::make_shared<Session>(_ioc, this);
    _acceptor.async_accept(newSession->Socket(),
                           std::bind(&Server::handleAccept, this, newSession, _1));
}

void Server::handleAccept(std::shared_ptr<Session> newSession, const boost::system::error_code &error)
{
    if (!error)
    {
        newSession->Start();
        _sessions[newSession->getUuid()] = newSession;
    }
    else
    {
        std::cerr << "accept error: " << error.message() << std::endl;
    }
    startAccept();
}

void Session::Start()
{
    memset(_data, 0, max_length);
    _socket.async_read_some(boost::asio::buffer(_data, max_length),
                            std::bind(&Session::handleRead, this, _1, _2, shared_from_this()));
}

void Session::handleRead(const boost::system::error_code &error, size_t bytes_transfered, std::shared_ptr<Session> selfShared)
{
    if (!error)
    {
        std::cout << "server receive data is " << _data << std::endl;
        boost::asio::async_write(_socket, boost::asio::buffer(_data, bytes_transfered),
                                 std::bind(&Session::handleWrite, this, _1, selfShared));
    }
    else
    {
        std::cerr << "read error: " << error.message() << std::endl;
        _server->clearSession(_uuid); // 合法：Server的clearSession已声明
    }
}

void Session::handleWrite(const boost::system::error_code &error, std::shared_ptr<Session> selfShared)
{
    if (!error)
    {
        memset(_data, 0, max_length);
        _socket.async_read_some(boost::asio::buffer(_data, max_length),
                                std::bind(&Session::handleRead, this, _1, _2, selfShared));
    }
    else
    {
        std::cerr << "write error: " << error.message() << std::endl;
        _server->clearSession(_uuid); // 合法
    }
}

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