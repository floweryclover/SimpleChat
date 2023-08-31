#include <iostream>
#include <string>
#include <vector>

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
			//
			std::string body("메세지1");
			size_t header = body.size();
			std::vector<char> send_buffer(sizeof(header) + body.size());
			std::memcpy(send_buffer.data(), &header, sizeof(header));
			std::memcpy(send_buffer.data()+sizeof(header), body.data(), body.size());
			boost::system::error_code error;
			socket.send(boost::asio::buffer(send_buffer));

			body = "두번째 메세지";
			header = body.size();
			send_buffer.resize(sizeof(header) + body.size());
			std::memcpy(send_buffer.data(), &header, sizeof(header));
			std::memcpy(send_buffer.data() + sizeof(header), body.data(), body.size());
			socket.send(boost::asio::buffer(send_buffer));

			body = "Third message";
			header = body.size();
			send_buffer.resize(sizeof(header) + body.size());
			std::memcpy(send_buffer.data(), &header, sizeof(header));
			std::memcpy(send_buffer.data() + sizeof(header), body.data(), body.size());
			socket.send(boost::asio::buffer(send_buffer));

			body = "A long long long long long long long long long long long long long long long long long long long long long long long long message";
			header = body.size();
			send_buffer.resize(sizeof(header) + body.size());
			std::memcpy(send_buffer.data(), &header, sizeof(header));
			std::memcpy(send_buffer.data() + sizeof(header), body.data(), body.size());
			socket.send(boost::asio::buffer(send_buffer));


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