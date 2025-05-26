#include <iostream>
#include <boost/asio.hpp>
#include <thread>
#include <nlohmann/json.hpp>
#include "../const.h"
#include <chrono>
#include <vector>

using nlohmann::json;
using namespace std;
using namespace boost::asio::ip;

std::vector<std::thread> vecThreads;

int main()
{
	auto start = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < 10; ++i)
	{
		vecThreads.emplace_back([]()
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
			int j=0;
			while(j<500){
				json js;
				js["id"]=1001;
				js["data"]="hello world";
				std::string request=js.dump();
				size_t request_len=request.length();
				char send_data[MAX_LENGTH] = { 0 };
					int msgid = 1001;
					int msgid_host = boost::asio::detail::socket_ops::host_to_network_short(msgid);
					memcpy(send_data, &msgid_host, 2);
					//转为网络字节序
					int request_host_length = boost::asio::detail::socket_ops::host_to_network_short(request_len);
					memcpy(send_data + 2, &request_host_length, 2);
					memcpy(send_data + 4, request.c_str(), request_len);
					boost::asio::write(sock, boost::asio::buffer(send_data, request_len + 4));
					cout << "begin to receive..." << endl;

					char reply_head[HEAD_TOTAL_LEN];
					size_t reply_length = boost::asio::read(sock, boost::asio::buffer(reply_head, HEAD_TOTAL_LEN));

					msgid = 0;
					memcpy(&msgid, reply_head, HEAD_ID_LEN);
					short msglen = 0;
					memcpy(&msglen, reply_head + 2, HEAD_DATA_LEN);
					//转为本地字节序
					msglen = boost::asio::detail::socket_ops::network_to_host_short(msglen);
					msgid = boost::asio::detail::socket_ops::network_to_host_short(msgid);
					char msg[MAX_LENGTH] = { 0 };
					size_t  msg_length = boost::asio::read(sock, boost::asio::buffer(msg, msglen));
					json reader=json::parse(std::string(msg,msg_length));
					std::cout << "msg id is " << reader["id"] << " msg is " << reader["data"] << endl;
					j++;
			}
			}
			catch (const std::exception &e)
			{
			std::cerr << e.what() << '\n';
			} });
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	for (auto &thread : vecThreads)
	{
		thread.join();
	}
	// 执行一些需要计时的操作
	auto end = std::chrono::high_resolution_clock::now(); // 获取结束时间

	// 2. 以毫秒为单位
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Time spent: " << duration_ms.count() << " milliseconds" << std::endl;
#if 0
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
#endif
	return 0;
}