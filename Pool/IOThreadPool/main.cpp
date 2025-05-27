#include "CSession.h"
#include "Server.h"
#include <boost/asio/signal_set.hpp>
#include <condition_variable>
#include <csignal>
#include <iostream>
#include <mutex>
#include <signal.h>
#include "AsioThreadPool.h"

int main()
{
  try
  {
    // auto pool = AsioServicePool::getInstance();
    auto pool = AsioThreadPool::getInstance();
    boost::asio::io_context io_context;
    boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
    signals.async_wait([pool, &io_context](auto, auto)
                       {
			pool->stop();
    io_context.stop(); });

    Server server(pool->getIOService(), 10086);

    io_context.run();
    std::cout << "server exited ...." << std::endl;
  }
  catch (const std::exception &e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
    return 1;
  }
  return 0;
}