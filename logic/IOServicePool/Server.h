#pragma once

#include <boost/asio.hpp>
#include "CSession.h"
#include <memory.h>
#include <map>


using boost::asio::ip::tcp;

class CSession;

class Server
{
public:
    Server(boost::asio::io_context &ioc, short port);
    void clearCSession(std::string uuid); 

private:
    void startAccept();
    void handleAccept(std::shared_ptr<CSession> newCSession, const boost::system::error_code &error);

    boost::asio::io_context &_ioc;
    tcp::acceptor _acceptor;
    short _port;
    std::map<std::string, std::shared_ptr<CSession>> _sessions;
 
};