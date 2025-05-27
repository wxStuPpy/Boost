#include "CSession.h"
#include "Server.h"
#include <boost/asio/signal_set.hpp>
#include <csignal>
#include <iostream>
#include "AsioServicePool.h"

int main(){
  try
  {
    auto pool=AsioServicePool::getInstance();
    boost::asio::io_context io_context;
    boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
     signals.async_wait([&io_context,&pool](auto, auto)
    { 
      std::cout << "Stopping server..." << std::endl;
      pool->stop();
      io_context.stop();
    });
    Server server(io_context, 8888);
    io_context.run();
  }
  catch(const std::exception& e)
  {
    std::cerr << e.what() << '\n';
  }
  
}