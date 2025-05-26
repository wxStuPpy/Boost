#include "CSession.h"
#include "Server.h"
#include <boost/asio/signal_set.hpp>
#include <condition_variable>
#include <csignal>
#include <iostream>
#include <mutex>
#include <signal.h>
#include "AsioServicePool.h"

bool isStop = false;
std::condition_variable cv_quit;
std::mutex mutex_quit;

#if 0
void sigHandler(int signum) {
  if (signum == SIGINT || signum == SIGTERM) {
    std::unique_lock<std::mutex>lock_quit(mutex_quit);
    isStop=true;
    cv_quit.notify_one();
  }
  std::cout<<"quit"<<std::endl;
}
#endif

int main()
{
  try
  {
#if 1
    auto pool = AsioServicePool::getInstance();
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
#else
    boost::asio::io_context io_context;
    std::thread net_work_thread([&io_context]()
                                {
    Server server(io_context, 8888);
    io_context.run(); });
    signal(SIGINT, sigHandler);
    signal(SIGTERM, sigHandler);
    while (!isStop)
    {
      std::unique_lock<std::mutex> lock_quit(mutex_quit);
      cv_quit.wait(lock_quit);
    }
    io_context.stop();
    net_work_thread.join();
  }
#endif
  catch (const std::exception &e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
    return 1;
  }
  return 0;
}