#pragma once
#include"Singleton.h"
#include<boost/asio.hpp>
#include<vector>

class AsioThreadPool:public Singleton<AsioThreadPool>
{
    friend class Singleton<AsioThreadPool>;
public:
    ~AsioThreadPool();
    AsioThreadPool(const AsioThreadPool&)=delete;
    AsioThreadPool& operator=(const AsioThreadPool&)=delete;
    boost::asio::io_context&getIOService();
    void stop();
private:
   AsioThreadPool(size_t threadNum=std::thread::hardware_concurrency());
   boost::asio::io_context _service;
   std::unique_ptr<boost::asio::io_context::work>_work;
   std::vector<std::thread>_threads;
};

