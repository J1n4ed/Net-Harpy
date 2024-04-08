// main function

#include "..\headers\shared_types.h"

// Core includes
#include <iostream>
#include <memory>
#include <string>
#include <cstdlib>
#include <memory>
#include <queue>
#include <map>
#include <functional>
#include <future>
#include <chrono>
#include <thread>

// WT Lib
#include <Wt/Dbo/Dbo.h>
#include <Wt/Dbo/backend/Postgres.h>

// import of modules
import database;
import indexer;
import utility;
import parser;
import curl;
import threadq;

// DEFINES

#define HTTP_PORT 80
#define HTTPS_PORT 443

// Forward declarations

struct page_processor : public harpy::Task
{ 
	page_processor(

		const std::string& id,
		const std::string& _host,
		std::vector<std::string>& _storage,
		const int& _depth,
		std::map<int, std::queue<std::string>*>& _linksQ,
		const int& _stage,
		std::vector<harpy::WebPage>& _indexed_pages,
		std::mutex& _mute,
		std::shared_ptr<harpy::DBASE> _dbase
	
	) : Task(id)
	
	{ // ------------------------------------------
		current_stage = _stage;		

		if (current_stage >= _depth)
		{
			isLast = true;
		}

		host = _host;
		storage = &_storage;
		depth = _depth;
		linksQ = &_linksQ;
		mute = &_mute;
		indexed_pages = &_indexed_pages;
		dbase = std::move(_dbase);
	};

	

	void one_thread_method() override
	{
		bool canContinue = true;
		std::string page;

		// CURL GET		
		mute->lock();
		std::cout << "\n> Page processor started for URL: " << host <<
			"\n> Thread: " << std::this_thread::get_id() << '\n' <<
			"> Stage: " << current_stage <<
		"\n> Queue status: ";

		for (int i = 1; i <= depth; ++i)
		{
			std::cout << "\n> Quene #" << i << ", size: " << linksQ->find(i)->second->size();
		}

		mute->unlock();

		std::unique_ptr<harpy::Webget> Getter(new harpy::Webget);

		try 
		{		
			if (host.find("https:/") != std::string::npos)
				Getter->setHost(host, std::to_string(HTTPS_PORT));
			else if (host.find("http:/") != std::string::npos)
				Getter->setHost(host, std::to_string(HTTP_PORT));
			else
				Getter->setHost(host, std::to_string(HTTPS_PORT));

			Getter->curlGet();

			page = Getter->getData();

			Getter->cleanGetter();
		}
		catch (std::exception & e)
		{
			std::cout << "\nCURL ERR: " << e.what();
			canContinue = false;
		}

		if (canContinue)
		{
			// Send page to indexer
			std::unique_ptr<harpy::Indexer> Index(new harpy::Indexer(page, host, isLast));

			// Get back indexed structure
			harpy::WebPage indexedPage = Index->getResult();
			page.clear();
			std::cout << std::endl;

			mute->lock();

			if (!indexedPage.get_isLinksEmpty())
			{
				std::vector<std::string> tmp = indexedPage.get_links();

				for (const auto& url : *storage)
				{
					auto pos = std::find(tmp.begin(), tmp.end(), url);

					if (pos != tmp.end())
					{
						tmp.erase(pos);
					}
				}

				for (const auto it : tmp)
				{
					linksQ->find(current_stage + 1)->second->push(it);
					storage->push_back(it);
				}
			}

			if (!indexedPage.get_wordslib().empty())
				indexed_pages->push_back(indexedPage);

			mute->unlock();

			dbase->process_page(indexedPage);

		}
		else
		{
			return;
		}
	}

private:

	std::string id;
	std::string host;
	std::vector<std::string> * storage;
	std::mutex * mute;
	int current_stage;
	int depth;
	std::map<int, std::queue<std::string> *> * linksQ;
	bool isLast = false;
	std::vector<harpy::WebPage> * indexed_pages;
	std::shared_ptr<harpy::DBASE> dbase;
};

// 

// MAIN FUNCTION ---------------------------------------------------------------------------------------------------------------------------------------
int main(int argc, char* argv[])
{

	// SETUP
	setlocale(LC_ALL, "ru_RU.UTF-8");

	// VARS

	harpy::clear_screen();
	harpy::get_version();

	std::string connectionString;
	// std::unique_ptr<harpy::DBASE> base;
	std::shared_ptr<harpy::DBASE> base;
	std::string confFile = "./cfg/cfg.ini";

	std::string startPage;
	int Depth = 0;

	std::string dbhost;
	int dbport;
	std::string dbname;
	std::string dbuser;
	std::string dbpasswd;

	std::map<int, std::queue<std::string> * > linksQ;
	std::vector<std::string> links_storage;
	std::vector<harpy::WebPage> indexed_pages;

	bool cmdArgs = false;
	bool isInitSuccess = false;
	bool pageRead = false;	
	bool allFinished = false;

	// Multithread stuff

	harpy::ThreadPool thread_pool(4);
	thread_pool.set_logger_flag(false);
	std::mutex * mute = new std::mutex;

	// SYSTEM INIT ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

	std::cout << " ///////////////// DATABASE CONNECTION ///////////////// ";

	std::cout << "\n> Lauched as: ";

	// Вызов должен быть HarpyCrawler.exe -host <DB HOST> -port <DB_PORT> -db <DB NAME> -u <USER NAME> -p <PWD>, порядок аргументов важен
	// HarpyCrawler.exe -host 192.168.1.2 -port 5432 -db harpydb -u harpy_adm -p Vfubcnhfkm882! // DEFAULT CALL

	for (int i = 0; i < argc; ++i)
	{
		std::cout << argv[i] << ' ';
	}	

	std::cout << "\n\n";

	// CFG OVERRIDE IF ARGS GIVEN	

	if (argc == 11)
	{
		// checks ------------

		bool check = false;

		if (strcmp(argv[1], "-host") == 0)
		{
			check = true;
		}

		cmdArgs = check;
		check = false;

		if (strcmp(argv[3], "-db") == 0)
		{
			check = true;
		}

		cmdArgs = check;
		check = false;

		if (strcmp(argv[5], "-db") == 0)
		{
			check = true;
		}

		cmdArgs = check;
		check = false;

		if (strcmp(argv[7], "-u") == 0)
		{
			check = true;
		}

		cmdArgs = check;
		check = false;

		if (strcmp(argv[9], "-p") == 0)
		{
			check = true;
		}

		cmdArgs = check;
		check = false;

		// !checks ---------

		if (cmdArgs)
		{
			dbhost = argv[2];
			dbport = std::stoi(argv[4]);
			dbname = argv[6];
			dbuser = argv[8];
			dbpasswd = argv[10];
		}
	}
	else
	{
		cmdArgs = false;
	}	

	if (!cmdArgs)
	{

		while (!isInitSuccess)
		{
			isInitSuccess = true;

			try
			{
				std::unique_ptr< harpy::parser> parser(new harpy::parser(confFile));

				std::cout << std::endl;

				std::cout << "> Content created in parser map, printing...\n\n";

				try
				{
					parser->print();
				}
				catch (const harpy::unknown_type& ex)
				{
					std::cout << ex.what();
				}
				catch (...)
				{
					std::cout << "> Unhandled exception!";
				}			

				std::cout << "\n ----- assining cfg -----\n";

				try
				{				
					auto value = parser->get_value<std::string>("Settings.StartPage");
					startPage = value;				
				}
				catch (const harpy::data_not_found& ex)
				{
					std::cout << ex.what();
				}
				catch (const harpy::type_mismatch& ex)
				{
					std::cout << ex.what();
				}			

				try
				{				
					auto value = parser->get_value<int>("Settings.Depth");
					Depth = value;				
				}
				catch (const harpy::data_not_found& ex)
				{
					std::cout << ex.what();
				}
				catch (const harpy::type_mismatch& ex)
				{
					std::cout << ex.what();
				}			

				try
				{				
					auto value = parser->get_value<std::string>("Connection.Db_Host");
					dbhost = value;				
				}
				catch (const harpy::data_not_found& ex)
				{
					std::cout << ex.what();
				}
				catch (const harpy::type_mismatch& ex)
				{
					std::cout << ex.what();
				}			

				try
				{
					auto value = parser->get_value<int>("Connection.Db_Port");
					dbport = value;				
				}
				catch (const harpy::data_not_found& ex)
				{
					std::cout << ex.what();
				}
				catch (const harpy::type_mismatch& ex)
				{
					std::cout << ex.what();
				}			

				try
				{
					auto value = parser->get_value<std::string>("Connection.Db_Name");
					dbname = value;				
				}
				catch (const harpy::data_not_found& ex)
				{
					std::cout << ex.what();
				}
				catch (const harpy::type_mismatch& ex)
				{
					std::cout << ex.what();
				}			

				try
				{
					auto value = parser->get_value<std::string>("Connection.Db_User");
					dbuser = value;				
				}
				catch (const harpy::data_not_found& ex)
				{
					std::cout << ex.what();
				}
				catch (const harpy::type_mismatch& ex)
				{
					std::cout << ex.what();
				}			

				try
				{
					auto value = parser->get_value<std::string>("Connection.Db_Pwd");
					dbpasswd = value;
				}
				catch (const harpy::data_not_found& ex)
				{
					std::cout << ex.what();
				}
				catch (const harpy::type_mismatch& ex)
				{
					std::cout << ex.what();
				}
			}
			catch (const harpy::file_not_found& ex)
			{
				std::cout << ex.what();
				isInitSuccess = false;
			}
			catch (const harpy::invalid_line& ex)
			{
				std::cout << ex.what();
				isInitSuccess = false;
			}
			catch (const harpy::invalid_section_name& ex)
			{
				std::cout << ex.what();
				isInitSuccess = false;
			}
			catch (const harpy::invalid_variable_name& ex)
			{
				std::cout << ex.what();
				isInitSuccess = false;
			}
			catch (...)
			{
				std::cout << "\n> Unknown Error!\n";
				isInitSuccess = false;
			}

			if (!isInitSuccess)
			{
				confFile.clear();
				std::cout << "\n> CFG not found, enter CFG location: ";
				std::cin >> confFile;
			}

			std::cout << std::endl;

		} // while (!isInitSuccess)	
	}

	// DATABASE INIT ---------------------------------------------------------------------------------------------------------------------------------------	

	std::cout << "\n> Connecting to host " << dbhost << ':' << std::to_string(dbport) << ", database: " << dbname << " as user: " << dbuser << '\n';	

	connectionString =
		"host=" + dbhost + " "
		"port=" + std::to_string(dbport) + " "
		"dbname=" + dbname + " "
		"user=" + dbuser + " "
		"password=" + dbpasswd;

	try
	{
		// base = std::make_unique<harpy::DBASE>(connectionString);		
		base = std::make_shared<harpy::DBASE>(connectionString);

		std::cout << std::endl;

		// EXIT

		isInitSuccess = true;
	}
	catch (const Wt::Dbo::Exception& e)
	{
		std::cout << "> DBO: Error connecting to database!\nError: " << e.what() << std::endl;
		isInitSuccess = false;
		cmdArgs = false;
	}	
	
	// SYSTEM BODY ------------------------------------------------------------------------------------------------------------------------------------------------

	// remove the pool from a pause, allowing streams to take on the tasks on the fly
	thread_pool.start();

	int stage = 1;

	for (int i = 1; i <= Depth; ++i)
	{
		linksQ.insert(std::make_pair(i, new std::queue<std::string>));
	}

	linksQ.find(stage)->second->push(startPage);

	bool linksQIsEmpty = false;
	bool firstIteration = true;

	mute->lock();

	std::string currentURL = " ";
	std::vector<std::string> trashLinks;

	mute->unlock();

	do
	{
		bool dupe = false;
		currentURL = " ";
		int currentStage = 1;
		bool pauseOnce = true;

		// Pop URL from Q
		do
		{
			dupe = false;
			currentStage = 0;				

			if (!firstIteration)
			{
				bool findOccupied = false;

				for (int i = 1; i <= Depth; ++i)
				{
					mute->lock();

					if (!findOccupied)
					{

						if (!linksQ.find(i)->second->empty())
						{						
							currentStage = i;
							findOccupied = true;
						}

					}

					mute->unlock();

					if (currentStage > Depth && !findOccupied)
					{
						linksQIsEmpty = true;
					}					
				}

				if (currentStage == 0)
				{
					linksQIsEmpty = true;
				}

				allFinished = true;

				for (int i = 1; i <= Depth; ++i)
				{
					if (!linksQ.find(i)->second->empty())
					{
						allFinished = false;
					}
				}
			}
			else
			{				
				firstIteration = false;
				currentStage = 1;
			}

			if (!linksQIsEmpty)
			{			
				mute->lock();

				if (linksQ.find(currentStage)->second->empty())
				{
					// skip
				}
				else
				{
					currentURL = linksQ.find(currentStage)->second->front();
					linksQ.find(currentStage)->second->pop();
				}

				mute->unlock();

				for (const auto& element : trashLinks)
				{
					if (element == currentURL)
					{
						dupe = true;
					}
				}

				if (!dupe)
					trashLinks.push_back(currentURL);
			}

		} while (dupe && !linksQIsEmpty);

		if (currentStage != 0)
		{

			if (currentURL != " ")

				// function to run in multithread
				thread_pool.add_task(

					page_processor(

						"PageProcessor_",
						currentURL,
						std::ref(links_storage),
						Depth,
						std::ref(linksQ),
						currentStage,
						std::ref(indexed_pages),
						*mute,
						base
					)		
			);	

			if (pauseOnce)
			{
				thread_pool.wait();
				pauseOnce = false;
			}
		}
	} while (!allFinished);	

	// TOTALS

	thread_pool.wait();

	std::cout << "\n ----------------------\n" <<
		"\n> Total pages acquired: " << indexed_pages.size();

	// DATABASE PROCESSINGS

	std::cout << "\nAcquired pages list: \n";
	for (auto & pages : indexed_pages)
	{
		std::cout << "\n > " << pages.get_title();
	}

	// base->process_pages(indexed_pages);

	// CLEANUP
	
	delete mute;

	for (auto & element : linksQ)
	{
		delete element.second;
	}
	
	// EXIT	

	std::cout << "\n> EXECUTION COMPLETED, EXITING\n";	
	return EXIT_SUCCESS;
}