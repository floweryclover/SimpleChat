#include <iostream>
#include <string>
#include <vector>
#include <thread>

#include "boost/asio.hpp"
#include "boost/bind/bind.hpp"
#include "boost/array.hpp"

int main(int argc, char* argv[])
{
	if (argc != 3)
	{
		std::cout << "usage: client <server-ip> <port>" << std::endl;
		return 1;
	}
	
	try
	{
		boost::asio::io_context io_context;
		boost::asio::ip::tcp::socket socket(io_context);
		boost::asio::ip::tcp::endpoint localhost_endpoint(boost::asio::ip::address::from_string(argv[1]), std::atoi(argv[2]));
		socket.connect(localhost_endpoint);


		std::string body;
		std::cout << "서버 운영자가 설정한 인증키를 입력하세요: ";
		std::cin >> body;
		size_t header = body.size();
		std::vector<char> send_buffer(sizeof(header) + body.size());
		std::memcpy(send_buffer.data(), &header, sizeof(header));
		std::memcpy(send_buffer.data()+sizeof(header), body.data(), body.size());
		boost::system::error_code error;
		socket.send(boost::asio::buffer(send_buffer));

		std::array<char, 1> auth_response_buf = {};
		size_t len = socket.read_some(boost::asio::buffer(auth_response_buf), error);

		if (error == boost::asio::error::eof)
		{
			std::cerr << "서버와의 연결이 끊겼습니다." << std::endl;
			return 1;
		}
		else if (error)
			throw boost::system::system_error(error);

		if (*auth_response_buf.begin() == '0')
		{
			std::cerr << "인증키가 올바르지 않습니다." << std::endl;
			system("pause");
			return 1;
		}

		std::cout << "닉네임을 설정하세요: ";
		std::cin >> body;
		header = body.size();
		send_buffer.resize(sizeof(header) + body.size());
		std::memcpy(send_buffer.data(), &header, sizeof(header));
		std::memcpy(send_buffer.data() + sizeof(header), body.data(), body.size());
		socket.send(boost::asio::buffer(send_buffer));

		std::cout << "이제 채팅을 시작하세요." << std::endl;

		std::thread input_thread = std::thread([&socket]()
			{
				std::string message;
				size_t header;
				std::vector<char> send_buffer;
				while (true)
				{
					std::cin >> message;
					header = message.size();
					send_buffer.resize(sizeof(header) + message.size());
					std::memcpy(send_buffer.data(), &header, sizeof(header));
					std::memcpy(send_buffer.data() + sizeof(header), message.data(), message.size());
					socket.send(boost::asio::buffer(send_buffer));
				}
			});
		input_thread.join();

	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
	system("pause");
	
	return 0;
}