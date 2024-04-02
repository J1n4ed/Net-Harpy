// Core includes
#include <iostream>
#include <string>
#include <cstdlib>

// Boost / Beast includes
// #include <boost/beast.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/ssl/stream.hpp>

#include <Windows.h>

using tcp = boost::asio::ip::tcp;
namespace http = boost::beast::http;
namespace net = boost::asio;
namespace ssl = net::ssl;

namespace harpy
{
	// ============================================================================================================================

	class Beast final
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
		// std::string target = "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.81\r\n";
		std::string target = "/";
		int version = 11; // DEFAULT 1.1 HTTP ver (no options)
		bool ready = false;

		//This buffer is used for reading and must be persisted
		boost::beast::flat_buffer buffer;		

	}; // !BEAST

} // !HARPY