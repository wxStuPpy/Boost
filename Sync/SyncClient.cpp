#include <boost/asio.hpp>
#include <iostream>
#include <cstring> // for memcpy

using namespace boost::asio::ip;
using namespace std;

const int MAX_LENGTH = 1024 * 2; // 最大消息长度（含头部）
const int HEAD_LENGTH = 2;       // 头部长度（short 类型，2字节）

int main() {
    try {
        boost::asio::io_context ioc;
        tcp::endpoint remote_ep(address::from_string("127.0.0.1"), 8888);
        tcp::socket sock(ioc);
        boost::system::error_code error;

        // 连接服务器
        sock.connect(remote_ep, error);
        if (error) {
            cerr << "连接失败: " << error.message() << endl;
            return 1;
        }
        cout << "已连接到服务器 " << remote_ep.address() << ":" << remote_ep.port() << endl;

        // 读取用户输入
        char request[MAX_LENGTH] = {0};
        cout << "请输入消息（最大 " << MAX_LENGTH - HEAD_LENGTH << " 字节）: ";
        cin.read(request, MAX_LENGTH - HEAD_LENGTH); // 最多读取 MAX_LENGTH-HEAD_LENGTH 字节
        size_t request_length = cin.gcount(); // 实际读取长度
        request[request_length] = '\0'; // 添加终止符（可选，仅用于文本显示）

        // 构造协议包（头部+消息体）
        char send_data[MAX_LENGTH] = {0};
        short net_len = boost::asio::detail::socket_ops::host_to_network_short(static_cast<short>(request_length));
        memcpy(send_data, &net_len, HEAD_LENGTH);         // 头部（网络字节序）
        memcpy(send_data + HEAD_LENGTH, request, request_length); // 消息体

        // 发送数据
        boost::asio::write(sock, boost::asio::buffer(send_data, request_length + HEAD_LENGTH), error);
        if (error) {
            cerr << "发送失败: " << error.message() << endl;
            return 1;
        }
        cout << "已发送 " << request_length << " 字节数据" << endl;

        // 接收回复头部
        char reply_head[HEAD_LENGTH] = {0};
        size_t reply_head_len = boost::asio::read(sock, boost::asio::buffer(reply_head, HEAD_LENGTH), error);
        if (error) {
            if (error == boost::asio::error::eof) {
                cerr << "服务器断开连接" << endl;
            } else {
                cerr << "接收头部失败: " << error.message() << endl;
            }
            return 1;
        }

        // 解析回复长度（网络字节序转主机字节序）
        short msglen = 0;
        memcpy(&msglen, reply_head, HEAD_LENGTH);
        msglen = boost::asio::detail::socket_ops::network_to_host_short(msglen);

        // 接收回复消息体
        if (msglen <= 0 || msglen > MAX_LENGTH - HEAD_LENGTH) {
            cerr << "非法回复长度: " << msglen << endl;
            return 1;
        }
        char msg[MAX_LENGTH] = {0};
        size_t msg_len = boost::asio::read(sock, boost::asio::buffer(msg, msglen), error);
        if (error) {
            cerr << "接收消息体失败: " << error.message() << endl;
            return 1;
        }

        // 输出结果
        cout << "\n服务器回复(长度 " << msglen << "):" << endl;
        cout.write(msg, msglen) << endl;

    } catch (const exception& e) {
        cerr << "异常: " << e.what() << endl;
        return 1;
    }

    return 0;
}