#include "../headers/netbeast.h"

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
	else
	{
		std::cerr << "\nINVALID HOST OR PORT";
	}
	
} // !sendPage

// Get page string if class is ready
std::string harpy::Beast::getPage()
{
	try 
	{
	if (ready)
	{
		std::string pureHost = host;

		// The io_context is required for all I/O
		net::io_context ioc;

		// The SSL context is required, and holds certificates
		// ssl::context ctx(ssl::context::sslv23_client);
		// ssl::context ctx(ssl::context::sslv3_client);
		ssl::context ctx(ssl::context::sslv23);

		// ctx.set_verify_mode(ssl::verify_peer);

		ssl::stream<tcp::socket> ssocket = { ioc, ctx };			
		
		// These objects perform out I/O
		tcp::resolver resolver{ ioc };			

		std::cout << "\n> DEBUG: full host address is " << host << std::endl;

		if (pureHost.find("http://") != std::string::npos)
		{
			pureHost.erase(0, 7);			
		} 
		else if (pureHost.find("https://") != std::string::npos)
		{
			pureHost.erase(0, 8);
		}
		else
		{

		}

		if (pureHost.find("/") != std::string::npos)
		{ 
			pureHost.erase(pureHost.find("/"), pureHost.size() - 1);
		}

		std::cout << "\n> DEBUG: pure host address is " << pureHost << std::endl;		
		
		if (!SSL_set_tlsext_host_name(ssocket.native_handle(), &host))
		{
			boost::system::error_code ec{ static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category() };
			throw boost::system::system_error{ ec };
		}		

		// Lookup the domain name
		// auto const results = resolver.resolve(host, port);
		auto const results = tcp::resolver::query(host, port);

		tcp::resolver::iterator iter = resolver.resolve(results);

		// Make the connection on the IP address we get from a lookup
		net::connect(ssocket.lowest_layer(), iter);

		ssocket.handshake(ssl::stream_base::handshake_type::client);

		// Set up an HTTP GET request message
		http::request<http::string_body> req{ http::verb::get, target, version };
		req.set(http::field::host, host);
		req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);		

		//Send the HTTP request to remote host
		http::write(ssocket, req);

		// Declare a container to hold the response		
		http::response<http::string_body> res;

		// boost::beast::http::response<boost::beast::http::dynamic_body> res;

		// Recieve the HTTP response
		http::read(ssocket, buffer, res);

		// std::string page = boost::asio::buffer_cast<const char*>(res.body().data());		
		// std::cout << res;
		// std::cout << "\n ---------- HEAD ---------- \n" << res.base();
		// std::cout << "\n\n ---------- BODY ---------- \n" << res.body();
		// std::cout << "\n-----------------------------\n";
		// std::string page = " <DEBUG> ";

		std::string page = "\n ---------- HEAD ---------- \n";
		page += boost::lexical_cast<std::string>(res.base());
		page += "\n\n ---------- BODY ---------- \n";
		page += boost::lexical_cast<std::string>(res.body());
		page += "\n-----------------------------\n";

		// deactivate class before new read
		host.clear();
		port.clear();
		buffer.clear();
		ready = false;

		// gracyfull shutdown
		boost::system::error_code ec;
		ssocket.shutdown(ec);

		if (ec == boost::asio::error::eof)
		{
			ec.assign(0, ec.category());
		}

		if (ec)
		{
			throw boost::system::system_error{ ec };
		}

		return page;
	}

	} 
	catch (std::exception const & e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return "ERR";
	}
} // !getPage