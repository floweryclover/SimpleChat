#include <iostream>
#include <memory>

#include "boost/asio.hpp"
#include "boost/bind/bind.hpp"

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
		std::shared_ptr<boost::asio::ip::tcp::socket> remote_socket = std::make_shared<boost::asio::ip::tcp::socket>(boost::asio::ip::tcp::socket(io_context_));
		std::shared_ptr<boost::asio::ip::tcp::endpoint> remote_endpoint = std::make_shared<boost::asio::ip::tcp::endpoint>(boost::asio::ip::tcp::endpoint());
		acceptor_.async_accept(*remote_socket, *remote_endpoint, boost::bind(&ChatServer::AcceptHandler, this, boost::asio::placeholders::error, remote_socket, remote_endpoint));
	}

	void AcceptHandler(const boost::system::error_code& error, std::shared_ptr<boost::asio::ip::tcp::socket> remote_socket, std::shared_ptr<boost::asio::ip::tcp::endpoint> remote_endpoint)
	{
		if (!error)
		{
			std::cout << *remote_endpoint << std::endl;
			message_ = "테스트 메세지";
			remote_socket->async_send(boost::asio::buffer(message_), boost::bind(&ChatServer::SendHandler, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
			WaitForAccept();
		}
		else
		{
			std::cerr << "An error occured" << std::endl;
			std::cerr << error << std::endl;
			return;
		}
	}

	void WaitForReceive(std::shared_ptr<boost::asio::ip::tcp::socket> remote_socket)
	{
		//remote_socket->async_receive(boost::bind(&ChatServer::ReceiveHandler, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
	}

	void ReceiveHandler(const boost::system::error_code& error, size_t bytes_transferred)
	{

	}

	void SendHandler(const boost::system::error_code& /*error*/, size_t /*bytes_transferred*/)
	{

	}

	std::string message_;
	boost::asio::io_context& io_context_;
	boost::asio::ip::tcp::acceptor acceptor_;
	
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