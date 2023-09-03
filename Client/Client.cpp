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
		size_t len = socket.receive(boost::asio::buffer(auth_response_buf));

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

		
		while (true)
		{
			std::array<char, 8> header = {};
			size_t header_received = 0;
			boost::system::error_code error;
			while (header_received < 8)
			{
				std::array<char, 8> buf = {};
				size_t recv = socket.receive(boost::asio::buffer(buf, 8 - header_received));

				std::copy(buf.begin(), buf.begin() + recv, header.begin() + header_received);
				header_received += recv;
			}

			size_t body_length;
			
			std::memcpy(&body_length, header.data(), 8);
			size_t body_received = 0;
			std::vector<char> body(body_length);
			while (body_received < body_length)
			{
				std::array<char, 4> buf = {};
				size_t to_recv = 4 < (body_length - body_received) ? 4 : body_length - body_received;
				size_t recv = socket.receive(boost::asio::buffer(buf, to_recv));
				
				std::copy(buf.begin(), buf.begin() + recv, body.begin() + body_received);
				body_received += recv;
			}

			std::cout.write(body.data(), body_length);
			std::cout << std::endl;
		}



	}
	catch (boost::system::system_error& e)
	{
		if (e.code() == boost::asio::error::eof)
		{
			std::cerr << "서버와의 연결이 끊어졌습니다." << std::endl;
		}
		else
		{
			std::cerr << "에러가 발생했습니다: " << e.what() << std::endl;
		}
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
	system("pause");
	
	return 0;
}