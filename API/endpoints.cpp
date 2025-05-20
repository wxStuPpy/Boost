#include <boost.h>
#include <iostream>
#include "endpoint.h"
using namespace boost;
using namespace boost::asio::ip;

int client_end_point()
{
    // 1. 原始 IP 地址和端口号
    std::string raw_ip_addr = "0.0.0.0"; // 监听所有接口的 IP 地址
    unsigned short port = 8888;          // 端口号
    // 2. 错误码对象（用于捕获地址解析错误）
    boost::system::error_code ec;
    // 3. 从字符串解析 IP 地址
    address ip_addr = asio::ip::address::from_string(raw_ip_addr, ec);
    // 4. 检查地址解析是否成功
    if (ec.value() != 0)
    {
        std::cerr << "Error: Invalid IP address - " << ec.message() << std::endl;
        return 1; // 解析失败，退出程序
    }
    // 5. 创建 TCP 端点（IP 地址 + 端口号）
    tcp::endpoint ep(ip_addr, port);
    // 6. 输出端点信息（可选）
    std::cout << "Created endpoint: "
              << ep.address().to_string() << ":" << ep.port() << std::endl;
    return 0;
}

int server_end_point()
{
    unsigned short port = 8888;
    address ip_addr = address_v4::any();
    tcp::endpoint ep(ip_addr, port);
    return 0;
}

int create_tcp_socket()
{
    asio::io_context ioc;
    tcp protocol = tcp::v4();
    tcp::socket sock(ioc);
    boost::system::error_code ec;
    sock.open(protocol, ec);
    if (ec.value() != 0)
    {
        std::cerr << "Error:" << ec.message() << std::endl;
        return 1; // 解析失败，退出程序
    }
    return 0;
}

int create_acceptor_socket()
{
    asio::io_context ioc;
    tcp::acceptor acceptor(ioc, tcp::endpoint(tcp::v4(), 8888));
    return 0;
}

int connect_to_end()
{
    std::string raw_ip_addr = "0.0.0.0";
    unsigned short port = 8888;
    try
    {
        tcp::endpoint ep(address::from_string(raw_ip_addr), port);
        asio::io_context ios;
        tcp::socket sock(ios, ep.protocol());
        sock.connect(ep);
    }
    catch (system::system_error &e)
    {
    }
}

int dns_connect_to_end()
{
    std::string host = "llfc.club"; // 目标域名
    unsigned short port = 8888;     // 目标端口
    asio::io_context ioc;           // IO 上下文

    try
    {
        // 1. 创建 DNS 解析器
        tcp::resolver resolver(ioc);

        // 2. 构造 DNS 查询（禁止自动解析端口为服务名，强制使用数值端口）
        tcp::resolver::query resolver_query(
            host,
            std::to_string(port),                 // 需将端口转为字符串（numeric_service 要求）
            tcp::resolver::query::numeric_service // 标记端口为数值型（非服务名）
        );

        // 3. 执行 DNS 解析（同步方法，返回 endpoints 列表）
        tcp::resolver::results_type endpoints = resolver.resolve(resolver_query);

        // 4. 创建 TCP 套接字并连接到目标端点
        tcp::socket socket(ioc);

        // 尝试连接所有解析出的端点（自动处理失败重试）
        boost::asio::connect(socket, endpoints);

        std::cout << "成功连接到 "
                  << socket.remote_endpoint().address().to_string() << ":"
                  << socket.remote_endpoint().port() << std::endl;

        // 5. 可选：在此处添加数据收发逻辑
        // 例如：socket.write_some(buffer("Hello, Server!\n"));

        return 0; // 连接成功返回 0
    }
    catch (const boost::system::system_error &se)
    {
        // 捕获系统错误（如 DNS 解析失败、连接被拒绝等）
        std::cerr << "错误代码: " << se.code().value()
                  << ", 错误信息: " << se.what() << std::endl;
        return se.code().value(); // 返回错误码
    }
    catch (const std::exception &e)
    {
        // 捕获其他异常（如参数错误）
        std::cerr << "异常: " << e.what() << std::endl;
        return -1;
    }
}

int accept_new_connection(){
    // The size of the queue containing the pending connection
            // requests.
    const int BACKLOG_SIZE = 30;
    // Step 1. Here we assume that the server application has
    // already obtained the protocol port number.
    unsigned short port_num = 3333;
    // Step 2. Creating a server endpoint.
    asio::ip::tcp::endpoint ep(asio::ip::address_v4::any(),
        port_num);
    asio::io_context  ios;
    try {
        // Step 3. Instantiating and opening an acceptor socket.
        asio::ip::tcp::acceptor acceptor(ios, ep.protocol());
        // Step 4. Binding the acceptor socket to the 
        // server endpint.
        acceptor.bind(ep);
        // Step 5. Starting to listen for incoming connection
        // requests.
        acceptor.listen(BACKLOG_SIZE);
        // Step 6. Creating an active socket.
        asio::ip::tcp::socket sock(ios);
        // Step 7. Processing the next connection request and 
        // connecting the active socket to the client.
        acceptor.accept(sock);
        // At this point 'sock' socket is connected to 
        //the client application and can be used to send data to
        // or receive data from it.
    }
    catch (system::system_error& e) {
        std::cout << "Error occured! Error code = " << e.code()
            << ". Message: " << e.what();
        return e.code().value();
    }
}