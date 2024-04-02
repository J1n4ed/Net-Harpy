module;

// Core includes
#include <iostream>
#include <string>
#include <cstdlib>

// Boost / Beast includes
#include <boost/beast.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>

export module netbeast;

// using tcp = boost::asio::ip::tcp;
// namespace http = boost::beast::http;

namespace harpy
{
	// ============================================================================================================================

	export class Beast final
	{
	public: // ----PUBLIC METHODS----

		// Public methods

		bool checkReady();

		void sendPage(std::string _host, std::string _port);

		std::string getPage();

		// Constructor

		// Beast();

	protected: // ----PROTECTED METHODS----

	private: // ----PRIVATE METHODS----

		std::string host;
		std::string port;
		std::string target = "/";
		int version = 11; // DEFAULT 1.1 HTTP ver (no options)
		bool ready = false;	

		//This buffer is used for reading and must be persisted
		boost::beast::flat_buffer buffer;

	}; // !BEAST

} // !HARPY

// Methods

// Constructor
//harpy::Beast::Beast()
//{
//	
//} // !Constructor

// Check if class is ready, host and port are loaded
bool harpy::Beast::checkReady()
{
	return ready;
} // !checkReady

// Load host and port into class
void harpy::Beast::sendPage(std::string _host, std::string _port)
{
	if (_port.length() > 0 && _host.length() > 0)
	{
		host = _host;
		port = _port;

		ready = true;
	}

	std::cerr << "\nINVALID HOST OR PORT";
} // !sendPage

// Get page string if class is ready
std::string harpy::Beast::getPage()
{
	if (ready)
	{
		// The io_context is required for all I/O
		boost::asio::io_context ioc;

		// These objects perform out I/O
		boost::asio::ip::tcp::resolver resolver{ ioc };
		boost::asio::ip::tcp::socket socket{ ioc };

		// Lookup the domain name
		auto const results = resolver.resolve(host, port);

		// Make the connection on the IP address we get from a lookup
		boost::asio::connect(socket, results.begin(), results.end());

		// Set up an HTTP GET request message
		boost::beast::http::request<boost::beast::http::string_body> req{ boost::beast::http::verb::get, target, version };
		req.set(boost::beast::http::field::host, host);
		req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);

		//Send the HTTP request to remote host
		boost::beast::http::write(socket, req);

		// Declare a container to hold the response
		// http::response<http::dynamic_body> res;
		boost::beast::http::response<boost::beast::http::string_body> res;

		// Recieve the HTTP response
		boost::beast::http::read(socket, buffer, res);

		// std::string page = boost::asio::buffer_cast<const char*>(res.body().data());
		std::string page = res.body().data();

		// deactivate class before new read
		host.clear();
		port.clear();
		buffer.clear();
		ready = false;	

		return page;
	}
} // !getPage