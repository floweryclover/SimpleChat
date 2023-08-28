#include <iostream>

#include "boost/asio.hpp"
#include "boost/bind/bind.hpp"
#include "boost/array.hpp"

int main(int argc, char* argv[])
{
	if (argc != 4)
	{
		std::cout << "usage: client <server-ip> <port> <nickname>" << std::endl;
		return 1;
	}
	
	try
	{
		boost::asio::io_context io_context;
		boost::asio::ip::tcp::socket socket(io_context);
		boost::asio::ip::tcp::endpoint localhost_endpoint(boost::asio::ip::address::from_string(argv[1]), std::atoi(argv[2]));
		socket.connect(localhost_endpoint);
		for (;;)
		{
			boost::array<char, 128> buf;
			boost::system::error_code error;
			size_t len = socket.read_some(boost::asio::buffer(buf), error);

			if (error == boost::asio::error::eof)
				break;
			else if (error)
				throw boost::system::system_error(error);

			std::cout.write(buf.data(), len);
		}
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
	system("pause");
	
	return 0;
}