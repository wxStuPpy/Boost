#pragma once

#include <iostream>
#include <memory>
#include <memory>
#include <mutex>

using namespace std;

template <class T> 
class Singleton {
protected:
  Singleton() = default;
  Singleton<T> &operator=(const Singleton<T> &) = delete;
  Singleton(const Singleton<T> &) = delete;

  static std::shared_ptr<T> _instance;

public:
  ~Singleton() {std::cout<<"destruct Singleton"<<std::endl;}

  static shared_ptr<T>getInstance(){
    static std::once_flag s_flag;
    std::call_once(s_flag,[&](){
        _instance=std::shared_ptr<T>(new T);
    });
    return _instance;
  }
};

template<class T>
std::shared_ptr<T> Singleton<T>::_instance=nullptr;
