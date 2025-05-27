#pragma once
#include"Singleton.h"
#include<boost/asio.hpp>
#include<vector>

class AsioServicePool:public Singleton<AsioServicePool>{
    friend Singleton<AsioServicePool>;
public:
    using IOService=boost::asio::io_context;
    /*防止ioc因为没有注册事件而退出*/
    using Work=boost::asio::io_context::work;
    /*利用智能指针独占work*/
    using WorkPtr=std::unique_ptr<Work>;
    ~AsioServicePool();
    AsioServicePool(const AsioServicePool&)=delete;
    AsioServicePool& operator=(const AsioServicePool&)=delete;

    /*使用round-robin方式返回一个io_context*/
    boost::asio::io_context&getIOService();
    void stop();

private:
    AsioServicePool(size_t size=std::thread::hardware_concurrency());
    std::vector<IOService>_ioServices;
    std::vector<WorkPtr>_works;
    std::vector<std::thread>_threads;
    size_t _nextIOIndex;
};
