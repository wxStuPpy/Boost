#include"AsioServicePool.h"

AsioServicePool::AsioServicePool(size_t size)
:_ioServices(size),_works(size),_nextIOIndex(0)
{
    for(size_t i=0;i<size;++i){
        #if 0
        /*两种方式都行*/
        auto unptr=std::unique_ptr<Work>(new Work(_ioServices[i]));
        //_works[i]=unptr;  _works.psuh_back(unptr); /*error unique_ptr不能被赋值*/
        _works.emplace_back(unptr);
        #endif 
        _works[i]=std::unique_ptr<Work>(new Work(_ioServices[i]));
    }

    //遍历多个ioservice 创建多个线程 每个内部启动ioservice
    for(size_t i=0;i<_ioServices.size();++i){
        _threads.emplace_back([this,i](){
            /*work被释放后 线程退出*/
            _ioServices[i].run();
        });
    }
}

AsioServicePool::~AsioServicePool(){
    std::cout<<"AsioServicePool destruct"<<std::endl;
}

boost::asio::io_context&AsioServicePool::getIOService(){
    auto &service=_ioServices[_nextIOIndex++];
    if(_nextIOIndex==_ioServices.size()){
        _nextIOIndex=0;
    }
    return service;
}

void AsioServicePool::stop(){
    for(auto&work:_works){
        //调用work的析构
       work.reset(); 
    }
    for(auto &thread:_threads){
        thread.join();
    }
}