// Utility functions for main header

module;

// INCLUDES

#include "../headers/cfg.h"
#include <iostream>

#ifdef _WIN32
#include <string>
#else // POSIX
#include <string.h>
#endif

#include <iomanip>

export module utility;

namespace harpy
{
	/*
	Очистка экрана консоли
	*/
	export void clear_screen()
	{
#ifdef _WIN32
		std::system("cls");
#else
		// Assume POSIX
		std::system("clear");
#endif
	} // END OF clear_screen()

	/*
	Печать версий ПО
	*/
	export void get_version()
	{
		std::cout << std::fixed << std::showpoint;
		std::cout << std::setprecision(2);

		std::cout << " ----------------------------- HARPY CRAWLER ------------------------------\n";
		std::cout << "|   CRAWLER VER:\t" << cfg::_VERSION << "                                               |\n";
		std::cout << "|   PARSER VER:\t\t" << cfg::_PARSERVERSION << "                                               |\n";
		std::cout << "|   INDEXER VER:\t" << cfg::_INDEXERVERSION << "                                               |\n";
		std::cout << "|   DB CONNECTOR VER:\t" << cfg::_DBCLASSVERSION << "                                               |\n";
		std::cout << " --------------------------------------------------------------------------\n";
	}	

} // END OF NAMESPACE