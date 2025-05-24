#include "LogicSystem.h"
#include "const.h"
#include <mutex>
#include <nlohmann/json_fwd.hpp>

using namespace std::placeholders;
using nlohmann::json;

LogicSystem::LogicSystem() : _isStop(false) {
  regCallBack();
  _workerThread = std::thread(&LogicSystem::dealMsg, this);
}

void LogicSystem::regCallBack() {
  _funCallBacks[MSG_HELLO_WORLD] =std::bind(&LogicSystem::helloWorldCallBack, this, _1, _2, _3);
}

void LogicSystem::helloWorldCallBack(std::shared_ptr<CSession> session,
                                     const short &msg_id,
                                     const std::string &msg_data) {
  json js=json::parse(msg_data);
  std::cout<<"server recv id is"<<js["id"]<<" data is"<<js["data"]<<std::endl;
  session->send(js.dump(),msg_id);
}

void LogicSystem::dealMsg(){
    while(1){
        std::unique_lock<std::mutex>unique_lk(_mutex);
        //判断队列为空 则用条件变量等待
        while(_msgQueue.empty()&&!_isStop){
            _cv.wait(unique_lk);
        }
        // 如果为关闭状态 取出逻辑队列所有数据 并退出循环
        if(_isStop){
            return;
        }
    }
}