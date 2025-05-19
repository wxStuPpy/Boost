#include<boost/asio.hpp>
#include<iostream>
#include<memory.h>
#include<set>

using namespace boost::asio::ip;
using boost::asio::ip::tcp;
using socket_ptr=std::shared_ptr<tcp::socket>;

std::set<std::shared_ptr<std::thread>>thread_set;

const int MAX_LEN=1024;

void session(socket_ptr sock){
    try
    {
        while(1){
            char data[MAX_LEN];
            memset(data,'\0',MAX_LEN);
            boost::system::error_code error;
            //size_t len=boost::asio::read(sock,boost::asio::buffer(data,MAX_LEN));
            size_t len=sock->read_some(boost::asio::buffer(data,MAX_LEN));
            if(error==boost::asio::error::eof){
                std::cout<<"connection close by peer"<<"\n";
                break;
            }
            else if(error){
                throw boost::system::system_error(error);
            }
            std::cout<<"receive from"<<sock->remote_endpoint().address().to_string()<<"\n";
            std::cout<<"data = "<<data<<"\n";
            //回传个给对方
            boost::asio::write(*sock,boost::asio::buffer(data,len));
        }
    }
    catch(const std::exception& e)
    {
        std::cerr <<"Exception in thread"<< e.what() << '\n';
    }
    
}

void server(boost::asio::io_context&ioc,unsigned short port){
    tcp::acceptor a(ioc,tcp::endpoint(tcp::v4(),port));
    while(1){
        socket_ptr socket=std::make_shared<tcp::socket>(ioc);
        a.accept(*socket);
        auto t=std::make_shared<std::thread>(session,socket);
        thread_set.insert(t);
    }
}

int main(){
    try
    {
       boost::asio::io_context ioc;
       server(ioc,8888);
       for(auto&t:thread_set){
        t->join();
       }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    return 0;
}
