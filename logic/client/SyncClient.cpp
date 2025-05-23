#include <iostream>
#include <boost/asio.hpp>
#include <thread>
#include <nlohmann/json.hpp>
#include "../const.h"

using nlohmann::json;
using namespace std;
using namespace boost::asio::ip;

int main()
{
	try
	{
		// 创建上下文服务
		boost::asio::io_context ioc;
		// 构造endpoint
		tcp::endpoint remote_ep(address::from_string("127.0.0.1"), 8888);
		tcp::socket sock(ioc);
		boost::system::error_code error = boost::asio::error::host_not_found;
		;
		sock.connect(remote_ep, error);
		if (error)
		{
			cout << "connect failed, code is " << error.value() << " error msg is " << error.message();
			return 0;
		}

		thread send_thread([&sock]
						   {
			for (;;) {
				this_thread::sleep_for(std::chrono::milliseconds(2000));
				json js;
				js["data"]="hello world";
				//发送id
				int msgid=1001;
				int msgid_host=boost::asio::detail::socket_ops::host_to_network_short(msgid);
				js["id"]=msgid;
				std::string request=js.dump();
				short request_length = static_cast<short>(request.size());
				char send_data[MAX_LENGTH] = { 0 };
				memcpy(send_data, &msgid_host, 2);
				//转为网络字节序
				short request_host_length = boost::asio::detail::socket_ops::host_to_network_short(request_length);
				memcpy(send_data+2, &request_host_length, 2);
				memcpy(send_data + 4, request.c_str(), request_length);
				boost::asio::write(sock, boost::asio::buffer(send_data, request_length + 4));
			} });

		thread recv_thread([&sock]
						   {
			for (;;) {
				this_thread::sleep_for(std::chrono::milliseconds(2));
				cout << "begin to receive..." << endl;
				char reply_head[HEAD_TOTAL_LEN];
				boost::asio::read(sock, boost::asio::buffer(reply_head, HEAD_TOTAL_LEN));
				short msgid=0;
				memcpy(&msgid, reply_head+2, 2);
				short msglen = 0;
				memcpy(&msglen, reply_head+2, 2);
				//转为本地字节序
				msgid = boost::asio::detail::socket_ops::network_to_host_short(msgid);
				msglen = boost::asio::detail::socket_ops::network_to_host_short(msglen);
				char msg[MAX_LENGTH] = { 0 };
				size_t msg_lenght=boost::asio::read(sock, boost::asio::buffer(msg, msglen));
				json js=json::parse(std::string(msg,msg_lenght));
				std::cout<<"id is "<<js["id"]<<" msg is "<<js["data"]<<std::endl;	
			} });

		send_thread.join();
		recv_thread.join();
	}
	catch (std::exception &e)
	{
		std::cerr << "Exception: " << e.what() << endl;
	}
	return 0;
}