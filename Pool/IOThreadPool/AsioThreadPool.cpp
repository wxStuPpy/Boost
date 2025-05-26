#include"AsioThreadPool.h"

AsioThreadPool::AsioThreadPool(size_t threadNum)
:_work(new boost::asio::io_context::work(_service))
{   
    for(size_t i=0;i<threadNum;++i){
        _threads.emplace_back([this](){
            _service.run();
        });
    }
}

AsioThreadPool::~AsioThreadPool(){
    
}

boost::asio::io_context&AsioThreadPool::getIOService(){
    return _service;
}

void AsioThreadPool::stop()
{
    _work.reset();
    for (auto &thread : _threads)
    {
        thread.join();
    }
}
