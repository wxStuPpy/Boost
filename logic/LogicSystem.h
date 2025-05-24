#pragma once
#include "CSession.h"
#include "Singleton.h"
#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <queue>
#include <string>
#include <thread>

using funCallBack =
    function<void(std::shared_ptr<CSession>, const short &msg_id,
                  const std::string &msg_data)>;

class LogicSystem : public Singleton<LogicSystem> {
  friend class Singleton<LogicSystem>;

public:
  ~LogicSystem() { std::cout << "destruct LogicSystem" << std::endl; }
  void postMsgToQueue(std::shared_ptr<LogicNode> msg);

private:
  LogicSystem();
  void regCallBack();
  void helloWorldCallBack(std::shared_ptr<CSession>, const short &msg_id,
                          const std::string &msg_data);
  void dealMsg();
  std::queue<std::shared_ptr<LogicNode>> _msgQueue;
  std::mutex _mutex;
  std::condition_variable _cv;
  std::thread _workerThread;
  bool _isStop;
  std::map<short, funCallBack> _funCallBacks;
};