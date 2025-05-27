#include "Server.h"
#include <iostream>
#include "AsioServicePool.h"

Server::Server(boost::asio::io_context &ioc, short port)
    : _ioc(ioc), _acceptor(ioc, tcp::endpoint(tcp::v4(), port)), _port(port)
{
    std::cout << "Server start success, listen on port : " << _port << std::endl;
    startAccept();
}

void Server::clearCSession(std::string uuid)
{
    _sessions.erase(uuid);
}

void Server::startAccept()
{   auto &_ioc=AsioServicePool::getInstance()->getIOService();
    auto newCSession = std::make_shared<CSession>(_ioc, this);
    _acceptor.async_accept(newCSession->getSocket(),
                           std::bind(&Server::handleAccept, this, newCSession, _1));
}

void Server::handleAccept(std::shared_ptr<CSession> newCSession, const boost::system::error_code &error)
{
    if (!error)
    {
        newCSession->Start();
        _sessions[newCSession->getUuid()] = newCSession;
    }
    else
    {
        std::cerr << "accept error: " << error.message() << std::endl;
    }
    startAccept();
}
