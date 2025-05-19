#include<boost/asio.hpp>
#include<iostream>

using namespace boost::asio::ip;

const int MAX_LEN=1024;

int main(){
    try
    {
       /*创建上下文服务*/
       boost::asio::io_context ioc;
       /*构造endpoint*/
       tcp::endpoint remote_ep(address::from_string("127.0.0.1"),8888);
       tcp::socket sock(ioc);
       boost::system::error_code error=boost::asio::error::host_not_found;
       sock.connect(remote_ep,error);
       if(error){
        std::cout<<"connect failed, code is"<<error.value()<<"error msg is"
        <<error.message()<<"\n";
        return 0;
       }
       std::cout<<"Enter message:";
       char request[MAX_LEN];
       std::cin.getline(request,MAX_LEN);
       size_t req_len=std::strlen(request);
       boost::asio::write(sock,boost::asio::buffer(request,req_len));

       char reply[MAX_LEN]={0};
       size_t reply_len=boost::asio::read(sock,boost::asio::buffer(reply,req_len));
       std::cout<<"Reply is:";
       std::cout.write(reply,reply_len);
       std::cout<<"\n";

    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
}