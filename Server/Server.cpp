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
			WaitForAccept();
		}
		else
		{
			std::cerr << error << std::endl;
			std::cerr << "An error occured" << std::endl;
			return;
		}
	}

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