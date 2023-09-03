#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <algorithm>
#include <functional>

#include "boost/asio.hpp"
#include "boost/bind/bind.hpp"

enum ClientOperation
{
	Unauthorized,
	EnteringNickname,
	Online,
	Disconnected,
};

class TcpConnection
{
public:
	TcpConnection(
		boost::asio::io_context& io_context,
		const std::string& passkey,
		std::function<void(const std::string&, const std::string&)> fn_on_chat,
		std::function<void(const std::string&)> fn_on_disconnected) noexcept :

		socket_(io_context),
		header_({}),
		receive_buffer_({}),
		OnClientChat_(fn_on_chat),
		OnClientDisconnected_(fn_on_disconnected),
		nickname_("Not logged in"),
		current_operation_(ClientOperation::Unauthorized),
		passkey_(passkey)
	{

	}

	boost::asio::ip::tcp::socket& Socket() noexcept { return this->socket_; }
	boost::asio::ip::tcp::endpoint& Endpoint() noexcept { return this->endpoint_; }

	void StartReceive()
	{
		header_to_receive_ = header_.size();
		socket_.async_receive(boost::asio::buffer(receive_buffer_, header_to_receive_), boost::bind(&TcpConnection::ReceiveHandler, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
	}

	void AsyncSendMessage(const std::string& msg)
	{
		size_t header = msg.size();
		std::vector<char> send_buffer(sizeof(header) + msg.size());
		std::memcpy(send_buffer.data(), &header, sizeof(header));
		std::memcpy(send_buffer.data() + sizeof(header), msg.data(), msg.size());
		socket_.send(boost::asio::buffer(send_buffer));
	}

	bool IsDisconnected() const noexcept { return this->current_operation_ == Disconnected; }

private:
	void ReceiveHandler(const boost::system::error_code& error, size_t bytes_transferred)
	{
		if (error)
		{
			std::cerr << "[종료 코드] " << nickname_ << ": " << error << std::endl;
			this->current_operation_ = Disconnected;
			OnClientDisconnected_(nickname_);
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

			// 바디를 다 채웠으면 핸들러를 호출하고 다음 헤더 받을 준비
			if (body_to_receive_ == 0)
			{
				OnPacketReceived();
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

	void OnPacketReceived()
	{
		switch (current_operation_)
		{
		case Unauthorized:
		{
			std::string entered_key(body_.begin(), body_.end());
			if (entered_key == passkey_)
			{
				std::array<char, 1> buf = { '1' };
				socket_.async_send(boost::asio::buffer(buf),
					[this](const boost::system::error_code& /*e*/, size_t /*bytes_transferred*/)
					{
						current_operation_ = EnteringNickname;
					});
			}
			else
			{
				std::array<char, 1> buf = { '0' };
				socket_.async_send(boost::asio::buffer(buf),
					[this](const boost::system::error_code& /*e*/, size_t /*bytes_transferred*/)
					{
						std::cout << "인증 실패: " << endpoint_ << std::endl;
					});

			}
			break;
		}

		case EnteringNickname:
		{
			std::string nickname(body_.begin(), body_.end());
			nickname_ = nickname;
			std::cout << "[유저 입장]: " << nickname << std::endl;
			current_operation_ = Online;
			break;
		}

		case Online:
			std::string message(body_.begin(), body_.end());
			OnClientChat_(nickname_, message);
			break;
		}
	}

	const std::string passkey_;
	ClientOperation current_operation_;

	boost::asio::ip::tcp::socket socket_;
	boost::asio::ip::tcp::endpoint endpoint_;

	std::array<char, 64> receive_buffer_;
	std::array<char, 8> header_;
	size_t header_to_receive_ = 0;
	std::vector<char> body_;
	size_t body_to_receive_ = 0;

	std::string nickname_;

	std::function<void(const std::string&, const std::string&)> OnClientChat_;
	std::function<void(const std::string&)> OnClientDisconnected_;
};

class ChatServer
{
public:
	ChatServer(boost::asio::io_context& io_context, const std::string& ip, unsigned short port, const std::string& passkey) :
		io_context_(io_context),
		acceptor_(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(ip), port)),
		passkey_(passkey)
	{
		WaitForAccept();
	}
private:
	void WaitForAccept()
	{
		new_connection_ = std::make_shared<TcpConnection>(
			io_context_,
			passkey_,
			[this](const std::string& nickname, const std::string& message) { OnClientChatHandler(nickname, message); },
			[this](const std::string& nickname) { OnClientDisconnected(nickname); });
		acceptor_.async_accept(new_connection_->Socket(), new_connection_->Endpoint(), boost::bind(&ChatServer::AcceptHandler, this, boost::asio::placeholders::error));
	}

	void AcceptHandler(const boost::system::error_code& error)
	{
		if (!error)
		{
			new_connection_->StartReceive();
			connections_.push_back(new_connection_);
			new_connection_.reset();
			WaitForAccept();
		}
		else
		{
			std::cerr << "An error occured: " << error << std::endl;
			return;
		}
	}

	void OnClientChatHandler(const std::string& nickname, const std::string& chat)
	{
		std::string full_msg = nickname + ": " + chat;
		std::cout << full_msg << std::endl;

		// 브로드캐스트
		for (auto connection : connections_)
		{
			connection->AsyncSendMessage(full_msg);
		}
	}

	void OnClientDisconnected(const std::string& nickname)
	{
		std::cout << "[연결 종료]: " << nickname << std::endl;
		std::erase_if(connections_, [](std::shared_ptr<TcpConnection> con) { return con->IsDisconnected(); });
	}

	std::string passkey_;
	std::shared_ptr<TcpConnection> new_connection_;
	std::vector<std::shared_ptr<TcpConnection>> connections_;
	boost::asio::io_context& io_context_;
	boost::asio::ip::tcp::acceptor acceptor_;
};

int main(int argc, char* argv[])
{
	if (argc != 4)
	{
		std::cout << "usage: server <ip> <port> <channel-passkey>" << std::endl;
		return 1;
	}
	try
	{
		boost::asio::io_context io_context;
		ChatServer chat_server(io_context, std::string(argv[1]), std::atoi(argv[2]), std::string(argv[3]));
		io_context.run();
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
	return 0;
}