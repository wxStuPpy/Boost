#include"CSession.h"
#include"Server.h"
#include<iostream>

int main()
{
    try
    {
        boost::asio::io_context io_context;
        Server server(io_context, 8888);
        io_context.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
        return 1;
    }
    return 0;
}