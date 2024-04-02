module;

///----------------------------------------------------------------------------|
/// Внимание!
/// Работоспособность проверена в среде VS2019(Toolset 140)
///                                   и VS2019(LLVM clang-cl)
///----------------------------------------------------------------------------:

#include <iostream>
#include <cstdio>
#include <windows.h>
#include <curl/curl.h>

export module curl;

namespace harpy
{
    struct string 
    {
        char * ptr;
        size_t len;
    };

    // CPP WAY
    size_t CurlWrite_CallbackFunc_StdString(void* contents, size_t size, size_t nmemb, std::string* s);

	export class Webget
	{
	public:

        void setHost(std::string, std::string);

        std::string getData();

        bool checkData();

        bool checkReady();

        void curlGet();		

        void cleanGetter();

        Webget();

		Webget(std::string, std::string);		

        ~Webget();

	private:

        CURL* curl;
        CURLcode res;
        struct string s;

        std::string host;
        std::string port;

        std::string result;

        bool ready = false;

	}; // !Webget

} // !harpy

// METHODS

harpy::Webget::Webget()
{
    ready = false;
}

harpy::Webget::Webget(std::string _host, std::string _port)
{
    host = _host;
    port = _port;    
    ready = true;
}


void harpy::Webget::curlGet()
{
    curl_global_init(CURL_GLOBAL_DEFAULT);

    if (ready)
    {
        curl = curl_easy_init();

        if (curl) 
        {       

            curl_easy_setopt(curl, CURLOPT_URL, host.c_str());
            curl_easy_setopt(curl, CURLOPT_PORT, port.c_str());
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 1L);            
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &CurlWrite_CallbackFunc_StdString);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);

            res = curl_easy_perform(curl);                   

            curl_easy_cleanup(curl);

            host.clear();
            port.clear();

            ready = false;   
        }
    }
} 

void harpy::Webget::setHost(std::string _host, std::string _port)
{
    host = _host;
    port = _port;
    ready = true;
}

std::string harpy::Webget::getData()
{
    if (checkData())
    {        
        return result;
    }
    else
    {
        return "<error>";
    }
}

bool harpy::Webget::checkData()
{
    if (result.empty())
    {
        return false;
    }
    else
        return true;
}

bool harpy::Webget::checkReady()
{
    return ready;
}

harpy::Webget::~Webget()
{    
}

void harpy::Webget::cleanGetter()
{
    result.clear();
    ready = false;
}

size_t harpy::CurlWrite_CallbackFunc_StdString(void* contents, size_t size, size_t nmemb, std::string* s)
{
    size_t newLength = size * nmemb;
    try
    {
        s->append((char*)contents, newLength);
    }
    catch (std::bad_alloc& e)
    {
        //handle memory problem
        return 0;
    }
    return newLength;
}