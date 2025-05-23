#include <iostream>
#include <boost/asio.hpp>
#include <thread>
#include <nlohmann/json.hpp>

using nlohmann::json;
using namespace std;
using namespace boost::asio::ip;
const int MAX_LENGTH = 1024 * 2;
const int HEAD_LENGTH = 2;
int main()
{
	try {
		//创建上下文服务
		boost::asio::io_context   ioc;
		//构造endpoint
		tcp::endpoint  remote_ep(address::from_string("127.0.0.1"), 8888);
		tcp::socket  sock(ioc);
		boost::system::error_code   error = boost::asio::error::host_not_found; ;
		sock.connect(remote_ep, error);
		if (error) {
			cout << "connect failed, code is " << error.value() << " error msg is " << error.message();
			return 0;
		}

		thread send_thread([&sock] {
			for (;;) {
				this_thread::sleep_for(std::chrono::milliseconds(2000));
				json js;
				js["id"]=1;
				js["data"]="hello world";
				std::string request=js.dump();
				short request_length = strlen(request.c_str());
				char send_data[MAX_LENGTH] = { 0 };
				//转为网络字节序
				short request_host_length = boost::asio::detail::socket_ops::host_to_network_short(request_length);
				memcpy(send_data, &request_host_length, 2);
				memcpy(send_data + 2, request.c_str(), request_length);
				boost::asio::write(sock, boost::asio::buffer(send_data, request_length + 2));
			}
			});

		thread recv_thread([&sock] {
			for (;;) {
				this_thread::sleep_for(std::chrono::milliseconds(2));
				cout << "begin to receive..." << endl;
				char reply_head[HEAD_LENGTH];
				boost::asio::read(sock, boost::asio::buffer(reply_head, HEAD_LENGTH));
				short msglen = 0;
				memcpy(&msglen, reply_head, HEAD_LENGTH);
				//转为本地字节序
				msglen = boost::asio::detail::socket_ops::network_to_host_short(msglen);
				char msg[MAX_LENGTH] = { 0 };
				size_t msg_lenght=boost::asio::read(sock, boost::asio::buffer(msg, msglen));
				json js=json::parse(std::string(msg,msg_lenght));
				std::cout<<"id is "<<js["id"]<<" msg is "<<js["data"]<<std::endl;	
			}
			});

		send_thread.join();
		recv_thread.join();
	}
	catch (std::exception& e) {
		std::cerr << "Exception: " << e.what() << endl;
	}
	return 0;
}