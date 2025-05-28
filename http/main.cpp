#include <boost.h>
#include <chrono>
#include <ctime>
#include <cstdlib>
#include <iostream>
#include <string>

using namespace boost::asio::ip;
using namespace boost::beast;

namespace myProgramState
{
    size_t requestCount()
    {
        static size_t count = 0;
        return ++count;
    }

    std::time_t now()
    {
        return std::time(0);
    }
};

class HttpConn:public std::enable_shared_from_this<HttpConn>{

public:
    HttpConn(tcp::socket socket):_socket(std::move(socket)){

    }
private:
    tcp::socket _socket;
    boost::beast::flat_buffer _buffer{8192};
    http::request<http::dynamic_body>_request;
    http::response<http::dynamic_body>_response;
    net::steady_timer _deadline{
        _socket.get_executor(),std::chrono::seconds(30)
    };

    void processRequest(){

    }

    void readRequest(){
        auto self=shared_from_this();
        http::async_read(_socket,_buffer,_request,
        [self](boost::beast::error_code ec,size_t bytesTransferred){
            boost::ignore_unused(bytesTransferred);
            if(!ec){
            self->processRequest();
            }
        });
    }

    void checkDeadline(){
        auto self=shared_from_this();
        _deadline.async_wait([self](const boost::system::error_code &ec){
            if(!ec){
            self->_socket.close(); 
            }
        });
    }
};

int main()
{
}