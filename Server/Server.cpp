#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <algorithm>

#include "boost/asio.hpp"
#include "boost/bind/bind.hpp"


class TcpConnection
{
public:
	TcpConnection(boost::asio::io_context& io_context) : socket_(io_context)
	{

	}

	boost::asio::ip::tcp::socket& Socket() { return this->socket_; }
	boost::asio::ip::tcp::endpoint& Endpoint() { return this->endpoint_; }

	void StartReceive()
	{
		header_to_receive_ = header_.size();
		socket_.async_receive(boost::asio::buffer(receive_buffer_, header_to_receive_), boost::bind(&TcpConnection::ReceiveHandler, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
	}

private:

	void ReceiveHandler(const boost::system::error_code& error, size_t bytes_transferred)
	{
		if (error)
		{
			std::cerr << "An error occured: " << error << std::endl;
			return;
		}

		// 헤더가 미완성이면 헤더 먼저 채우기
		// 헤더의 크기에 딱 맞게 받거나 모자라게 받는 경우밖에 없음 (async_receive 호출 시 남은 헤더 크기만큼 요청하기 때문에)
		if (header_to_receive_ > 0)
		{
			std::copy(receive_buffer_.begin(), receive_buffer_.begin() + bytes_transferred, header_.begin() + (header_.size() - header_to_receive_));
			header_to_receive_ -= bytes_transferred;

			if (header_to_receive_ > 0)
			{
				socket_.async_receive(boost::asio::buffer(receive_buffer_, header_to_receive_), boost::bind(&TcpConnection::ReceiveHandler, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
			}
			else
			{
				// 바디 길이 얻기
				std::memcpy(&body_to_receive_, header_.data(), sizeof(body_to_receive_));
				body_.resize(body_to_receive_);
				size_t read_size = body_to_receive_ > receive_buffer_.size() ? receive_buffer_.size() : body_to_receive_;
				socket_.async_receive(boost::asio::buffer(receive_buffer_, read_size), boost::bind(&TcpConnection::ReceiveHandler, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
			}
		}
		else // 바디 채우기
		{
			std::copy(receive_buffer_.begin(), receive_buffer_.begin() + bytes_transferred, body_.begin() + (body_.size() - body_to_receive_));
			body_to_receive_ -= bytes_transferred;

			// 바디를 다 채웠으면 다음 헤더 받을 준비
			if (body_to_receive_ == 0)
			{
				std::string message(body_.begin(), body_.end());
				size_t header;
				std::memcpy(&header, header_.data(), sizeof(header_.data()));
				std::cout << "[HEADER: BODY_LENGTH=" << header << "BYTES, (" << sizeof(header_.data()) << "bytes), BODY: MESSAGE=" << message << "(" << body_.size() << "bytes)]" << std::endl;
				header_to_receive_ = header_.size();
				socket_.async_receive(boost::asio::buffer(receive_buffer_, header_to_receive_), boost::bind(&TcpConnection::ReceiveHandler, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
			}
			// 남은 바디 채우기
			else
			{
				size_t read_size = body_to_receive_ > receive_buffer_.size() ? receive_buffer_.size() : body_to_receive_;
				socket_.async_receive(boost::asio::buffer(receive_buffer_, read_size), boost::bind(&TcpConnection::ReceiveHandler, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));

			}
		}
	}

	void SendHandler(const boost::system::error_code& /*error*/, size_t /*bytes_transferred*/)
	{
		
	}

	boost::asio::ip::tcp::socket socket_;
	boost::asio::ip::tcp::endpoint endpoint_;
	std::string send_buffer_;

	std::array<char, 64> receive_buffer_;
	std::array<char, 8> header_;
	size_t header_to_receive_ = 0;
	std::vector<char> body_;
	size_t body_to_receive_ = 0;
};

class ChatServer
{
public:
	ChatServer(boost::asio::io_context& io_context) :
		io_context_(io_context),
		acceptor_(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 31414))
	{
		WaitForAccept();
	}
private:
	void WaitForAccept()
	{
		new_connection_ = std::make_unique<TcpConnection>(io_context_);
		acceptor_.async_accept(new_connection_->Socket(), new_connection_->Endpoint(), boost::bind(&ChatServer::AcceptHandler, this, boost::asio::placeholders::error));
	}

	void AcceptHandler(const boost::system::error_code& error)
	{
		if (!error)
		{
			new_connection_->StartReceive();
			connections_.emplace_back(std::move(this->new_connection_));
			WaitForAccept();
		}
		else
		{
			std::cerr << "An error occured: " << error << std::endl;
			return;
		}
	}

	std::string message_;
	std::vector<std::unique_ptr<TcpConnection>> connections_;
	boost::asio::io_context& io_context_;
	boost::asio::ip::tcp::acceptor acceptor_;
	std::unique_ptr<TcpConnection> new_connection_;
};

int main()
{
	try
	{
		boost::asio::io_context io_context;
		ChatServer chat_server(io_context);
		io_context.run();
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
	return 0;
}