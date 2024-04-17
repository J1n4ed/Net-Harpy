module;

// Harpy includes

#include "..\headers\shared_types.h"

// Core includes
#include <iostream>
#include <string>
#include <algorithm>
#include <vector>
#include <regex>
#include <sstream>
#include <memory>
#include <chrono>
#include <ctime> 
#include <map>
#include <unordered_set>

export module indexer;

namespace harpy
{
	bool BothAreSpaces(char lhs, char rhs) { return (lhs == rhs) && (lhs == ' '); }
	
	export class Indexer final
	{
	public:

		harpy::WebPage getResult();

		Indexer() = delete;		
		Indexer(std::string, std::string, bool);

	protected:

	private:

		// PRIVATE VARIABLES

		std::string page;
		harpy::WebPage webpage;	
		bool isEnd = false;
		std::vector<std::string> links;		
		std::map<std::string, int> wordsLib;
		bool debug = false;

		// PRIVATE METHODS DECL
			
		void SearchForNodesUsingLoop();
		void process_string();
		void findErase(std::string *, std::string, std::string);		
		void buildWordLib();
		std::vector<std::string> sort_links(const std::vector<std::string>& _links);
		
	}; // !Indexer

} // !HARPY

// METHODS -------------------------------------

harpy::Indexer::Indexer(std::string _page, std::string _url, bool _isEnd)
{
	page = _page;
	isEnd = _isEnd;

	if (!page.empty())
	{
		// DEBUG
		// std::cout << "\n> PAGE RECIEVED FOR PROCESSING, PRINTING: \n\n" << page << "\n\n";
		// std::system("pause");		

		webpage.set_address(_url);	

		// USE THIS TO SWITCH ON DEBUG OUTPUT FOR SPECIFIC URL
		// DEBUG
		/*
		if (_url == "https://wiki.openssl.org/index.php/Main_Page")
			debug = true;
		*/
		// !DEBUG

		// converting the page into vector of strings	

		process_string();	
	}

} // !Indexer

// remove dupes
std::vector<std::string> harpy::Indexer::sort_links(const std::vector<std::string>& _links)
{
	std::unordered_set<std::string> mySet;
	std::vector<std::string> newVector;

	for (const auto& el : _links)
	{
		mySet.insert(el);
	}

	for (auto& el : mySet)
	{
		newVector.push_back(el);
	}

	return newVector;	
}

void harpy::Indexer::process_string()
{
	bool canContinue = true;

	// DEBUG
	if (debug)
	{
		std::cout << "\n> DEBUG: Debug is on for URL " << webpage.get_address();
	}
	// !DEBUG

	try
	{
		SearchForNodesUsingLoop();
	}
	catch (std::exception & e)
	{
		std::cout << "\nEXCEPTION in Indexer: " << e.what();
		canContinue = false;
	}	

	if (canContinue)
	{

		try
		{
			buildWordLib();
		}
		catch (std::exception& e)
		{
			std::cout << "\nEXCEPTION in Indexer: " << e.what();
			canContinue = false;
		}

	}	

	if (canContinue)
	{

		if (!links.empty())
		{
			webpage.set_isLinksEmpty(false);
			webpage.set_links(links);
		}
		else
		{			
			webpage.set_isLinksEmpty(true);
		}
	
		if (wordsLib.empty())
		{
			// skip
		}
		else
		{			
			webpage.set_wordslib(wordsLib);			
		}	
	}
}

// Me gusta
void harpy::Indexer::SearchForNodesUsingLoop()
{
	int counter = 1;
	
	std::string tmp;
	std::string url = webpage.get_address();

	// finding pure address for related links

	{
		size_t start = url.find("://");

		if (start != std::string::npos)
		{
			size_t pos = url.find("/", start + 3);
			url.erase(pos, url.size());
		}
	}	

	bool titleFound = false;
	bool modDateFound = false;

	size_t linePosBegin = 0;
	size_t linePosEnd = 0;

	{ // TITLE FINDER

		std::string title = page;

		// check for title meta info
		linePosBegin = title.find("<title>");
	
		if (linePosBegin != std::string::npos)
		{	
			linePosEnd = title.find(">", linePosBegin);

			if (linePosEnd != std::string::npos)
			{
				title.erase(0, linePosEnd + 1);
			}
		

			linePosBegin = title.find("</title>");

			if (linePosBegin != std::string::npos)
			{
				if (linePosEnd != std::string::npos)
				{
					title.erase(linePosBegin, title.size());
				}
			}

			webpage.set_title(title);

			titleFound = true;
		}

	} // !TITLE FINDER
		
	// DEBUG
	if (debug)
	{
		std::cout << "\n> Line by line processing...\n\n";
	}

	while (page.find("\n", linePosBegin) != std::string::npos)
	{
		linePosEnd = page.find("\n", linePosBegin);

		if (linePosBegin >= page.size() || linePosEnd >= page.size())
		{
			break;
		}

		std::string line = page.substr(linePosBegin, linePosEnd);

		//DEBUG
		/*if (debug)
		{
			std::cout << '\n' << line;
		}*/

		linePosBegin = ++linePosEnd;		

		// check for last modified meta info
		if (line.find("last-modified") != std::string::npos && !modDateFound)
		{
			std::string tmp = line;
			auto startpos = tmp.find("last-modified");
			tmp.erase(0, startpos + 13);

			if (line.find("</last-modified>") != std::string::npos)
			{
				auto endpos = tmp.find("</title>");
				tmp.erase(endpos, tmp.size());				
			}			

			webpage.set_lastmod(tmp);

			modDateFound = true;
		}				

		// Recieved line, going thru looking for signs of links
		if (!isEnd) // CHECK FOR LAST ITERATION IN DEPTH
		{
			while (line.find("<a ") != std::string::npos)
			{
				auto startpos = line.find("<a ");
			
					if ((startpos + 2) <= line.size())
						line.erase(0, startpos + 2);
					else if ((startpos + 1) <= line.size())
						line.erase(0, startpos + 1);
					else if ((startpos) <= line.size())
						line.erase(0, startpos);
			

				if (line.find("/a>") != std::string::npos)
				{
					auto endpos = line.find("/a>");
					line.erase(endpos, line.size());
				}				

				size_t pos = line.size() - 1;

				bool relatedLink = false;

				if (line.find("#") != std::string::npos)
				{
					line.erase(line.find("#"), line.size());
				}

				if (line.find("&") != std::string::npos)
				{
					line.erase(line.find("&"), line.size());
				}
				
				// remove last symbol if it's /
				if (line.back() == '/')
				{
					line.erase(line.size());					
				}

				// related link
				if (line.find("href=\"/") != std::string::npos)
				{					
					relatedLink = true;

					size_t pos = line.find('/');
					line.erase(0, pos);					
				}
				
				if (line.find("http:") != std::string::npos || line.find("https:") != std::string::npos || relatedLink)
				{
					if (relatedLink)
					{
						line = url + line;
					}
					
					auto startpos = line.find("http");
					line.erase(0, startpos);

					auto endpos = line.find("\"");
					if (endpos != std::string::npos)
					{
						line.erase(endpos, line.size());							

						if (line.find(".pdf") != std::string::npos || line.find(".doc") != std::string::npos || line.find(".docx") != std::string::npos || line.find(".xls") != std::string::npos
							|| line.find(".xlsx") != std::string::npos || line.find(".zip") != std::string::npos || line.find(".7z") != std::string::npos || line.find(".exe") != std::string::npos
							|| line.find(".png") != std::string::npos || line.find(".jpg") != std::string::npos || line.find(".jpeg") != std::string::npos || line.find(".gif") != std::string::npos
							|| line.find(".scr") != std::string::npos || line.find(".bat") != std::string::npos || line.find(".ps2") != std::string::npos || line.find(".iso") != std::string::npos
							|| line.find(".xps") != std::string::npos || line.find(".part") != std::string::npos || line.find(".backup") != std::string::npos)
						{
							// SKIP
						}
						else
						{							
							links.push_back(line);
						}
					}
				}
				else
				{
					break;
				}
			}

			// DEBUG
			if (debug)
			{
				std::cout << "\n> DEBUG: Links found for URL " << webpage.get_address();
				
				for (const auto& el : links)
				{
					std::cout << "\n- " << el;
				}
			}
			// !DEBUG

		} // !CHECK FOR LAST ITERATION IN DEPTH
			
	} // !WHILE

	

	// DEBUG
	/*if (debug)
	{
		std::system("pause");

		std::cout << "\n> HALFWAY PAGE PROCESS RESULT:\n\n" << page << "\n\n";
	}*/

	if (!modDateFound)
	{
		std::time_t time = std::time(0);
		std::tm* now;
		now = new std::tm();
		localtime_s(now, &time);
		webpage.set_lastmod((std::to_string(now->tm_year + 1900) + '.' + std::to_string(now->tm_mon + 1) + '.' + std::to_string(now->tm_mday) +
			' ' + std::to_string(now->tm_hour) + ':' + std::to_string(now->tm_min) + ':' + std::to_string(now->tm_sec)));

		modDateFound = true;
		delete now;
	}
	
	{ // ERASE ENDLINES
		// page.erase(std::remove(page.begin(), page.end(), '\n'), page.cend());

		// replace instead of erase
		std::regex pattern;
		pattern = "\n";
		page = regex_replace(page, pattern, " ");
	}

	// remove scripts
	findErase(&page, "<script>", "</script>");
	findErase(&page, "/script>", ">");

	// DEBUG
	/*if (debug)
	{
		std::system("pause");

		std::cout << "\n> SCRIPT REMOVAL DONE:\n\n" << page << "\n\n";
	}*/
	
	// REGEXP
	std::regex pattern("\\<.*?\\>");
	page = regex_replace(page, pattern, " ");
	
	pattern = "\\[.*?\\]";
	page = regex_replace(page, pattern, " ");

	pattern = "\\{.*?\\}";
	page = regex_replace(page, pattern, " ");

	pattern = "\t";
	page = regex_replace(page, pattern, " ");	

	// DEBUG
	/*if (debug)
	{
		std::system("pause");

		std::cout << "\n> REGEXPS DONE:\n\n" << page << "\n\n";
	}*/

	std::remove_if(page.begin(), page.end(), ispunct);

	page.erase(std::remove_if(page.begin(), page.end(), ::isdigit), page.end());

	// DEBUG
	/*if (debug)
	{
		std::system("pause");

		std::cout << "\n> PUNCT & DIGITS REMOVAL DONE:\n\n" << page << "\n\n";
	}*/

	// Spaces

	std::string::iterator new_end = std::unique(page.begin(), page.end(), &harpy::BothAreSpaces);
	page.erase(new_end, page.end());

	// DEBUG
	if (debug)
	{
		std::system("pause");

		std::cout << "\n> EXTRA SPACES REMOVAL DONE:\n\n" << page << "\n\n";
	}

	// DEBUG
	// std::cout << "\n> DEBUG: PRINTING RESULT PAGE...\n\n" << page << "\n\n ------------------------------------------\n";
}

void harpy::Indexer::findErase(std::string * _text, std::string _start, std::string _end)
{
	while (_text->find(_start) != std::string::npos)
	{
		size_t start;
		start = _text->find(_start);

		if (start != std::string::npos) // if not trash value
		{			
			auto end = _text->find(_end, start);

			_text->insert(start, " ");
			_text->insert(end, " ");

			_text->erase(start, end + sizeof(_end));
		}
	}
}

harpy::WebPage harpy::Indexer::getResult()
{
	return webpage;
}

void harpy::Indexer::buildWordLib()
{
	// build unique_word array
	std::string _page = page;	

	// DEBUG
	if (debug)
	{
		system("pause");
		std::cout << "\n> DEBUG:page size = " << _page.size();		
	}
	// !DEBUG

	if (_page.size() != 0)
	{
		while (_page.find(' ') != std::string::npos || _page.find(' ') != _page.back())
		{
			if (_page.find(' ') == _page.front())
			{
				_page.erase(0, 1);
			}
			else
			{
				size_t wordPosEnd = _page.find(' ');
				std::string word;

				if (_page.size() == 0 || wordPosEnd >= _page.size())
				{
					break;
				}

				word = _page.substr(0, wordPosEnd);
				_page.erase(0, wordPosEnd + 1);			

				std::transform(word.begin(), word.end(), word.begin(), [](unsigned char c) { return std::tolower(c); });

				std::transform(word.begin(), word.end(), word.begin(), towlower);

				// Skip small words
				if (word.size() >= 3)
				{
					if (!wordsLib.count(word))
					{
						wordsLib.insert(make_pair(word, 1));
						
						if (debug)
						{
							std::cout << "\n> new word added to lib: " << word;
						}

					}
					else
					{
						wordsLib[word]++;

						if (debug)
						{
							std::cout << "\n> repeat for word added to lib: " << word;
						}
					}
				}
			}

			if (_page.empty())
				break;
		}

		std::vector<std::string> for_removal;

		if (debug)
		{
			std::system("pause");
			std::cout << "\n\n> CLEANUP OF WORDSLIB\n\n";
		}
	
		// remove short and long words
		for (const auto element : wordsLib)
		{
			// DEBUG
			if (debug)
			{
				std::cout << "\n> word: " << element.first << ", result: ";
			}

			if (element.first.length() < 3)
			{
				for_removal.push_back(element.first);
			}
		
			if (element.first.length() > 14)
			{
				for_removal.push_back(element.first);
			}

			// DEBUG
			if (element.first == "openssl")
			{
				std::cout << "\n> word: openssl found in URL: " << webpage.get_address();
			}
		
			/*
			// some cleanups
			if (	element.first._Starts_with("?") || element.first._Starts_with("¹") || element.first._Starts_with(" ") 
				||  element.first._Starts_with("(") || element.first._Starts_with(".") || element.first._Starts_with("0")
				||	element.first._Starts_with("1") || element.first._Starts_with("2") || element.first._Starts_with("3") 
				||  element.first._Starts_with("4")	|| element.first._Starts_with("5") || element.first._Starts_with("6") 
				||  element.first._Starts_with("7") || element.first._Starts_with("8") || element.first._Starts_with("9") 
				||  element.first._Starts_with("&")
				)
			{
				for_removal.push_back(element.first);
			}
			*/			

			//// extremely lazy filtering
			//if (	element.first.rfind("A", 0) == std::string::npos || element.first.rfind("B", 0) == std::string::npos || element.first.rfind("C", 0) == std::string::npos || element.first.rfind("D", 0) == std::string::npos || element.first.rfind("E", 0) == std::string::npos
			//	||	element.first.rfind("F", 0) == std::string::npos || element.first.rfind("G", 0) == std::string::npos || element.first.rfind("H", 0) == std::string::npos || element.first.rfind("I", 0) == std::string::npos || element.first.rfind("J", 0) == std::string::npos
			//	||	element.first.rfind("K", 0) == std::string::npos || element.first.rfind("L", 0) == std::string::npos || element.first.rfind("M", 0) == std::string::npos || element.first.rfind("N", 0) == std::string::npos || element.first.rfind("O", 0) == std::string::npos
			//	||  element.first.rfind("P", 0) == std::string::npos || element.first.rfind("Q", 0) == std::string::npos || element.first.rfind("R", 0) == std::string::npos || element.first.rfind("S", 0) == std::string::npos || element.first.rfind("T", 0) == std::string::npos
			//	||  element.first.rfind("U", 0) == std::string::npos || element.first.rfind("V", 0) == std::string::npos || element.first.rfind("W", 0) == std::string::npos || element.first.rfind("X", 0) == std::string::npos || element.first.rfind("Y", 0) == std::string::npos
			//	||  element.first.rfind("Z", 0)
			//	||  element.first.rfind("a", 0) == std::string::npos || element.first.rfind("b", 0) == std::string::npos || element.first.rfind("c", 0) == std::string::npos || element.first.rfind("d", 0) == std::string::npos || element.first.rfind("y", 0) == std::string::npos
			//	||  element.first.rfind("f", 0) == std::string::npos || element.first.rfind("g", 0) == std::string::npos || element.first.rfind("h", 0) == std::string::npos || element.first.rfind("i", 0) == std::string::npos || element.first.rfind("j", 0) == std::string::npos
			//	||  element.first.rfind("k", 0) == std::string::npos || element.first.rfind("l", 0) == std::string::npos || element.first.rfind("m", 0) == std::string::npos || element.first.rfind("n", 0) == std::string::npos || element.first.rfind("o", 0) == std::string::npos
			//	||  element.first.rfind("p", 0) == std::string::npos || element.first.rfind("q", 0) == std::string::npos || element.first.rfind("r", 0) == std::string::npos || element.first.rfind("s", 0) == std::string::npos || element.first.rfind("t", 0) == std::string::npos
			//	||  element.first.rfind("u", 0) == std::string::npos || element.first.rfind("v", 0) == std::string::npos || element.first.rfind("w", 0) == std::string::npos || element.first.rfind("x", 0) == std::string::npos || element.first.rfind("y", 0) == std::string::npos
			//	||  element.first.rfind("z", 0)
			//	||  element.first.rfind("À", 0) == std::string::npos || element.first.rfind("Á", 0) == std::string::npos || element.first.rfind("Â", 0) == std::string::npos || element.first.rfind("Ã", 0) == std::string::npos || element.first.rfind("Ä", 0) == std::string::npos
			//	||  element.first.rfind("Å", 0) == std::string::npos || element.first.rfind("¨", 0) == std::string::npos || element.first.rfind("Æ", 0) == std::string::npos || element.first.rfind("Ç", 0) == std::string::npos || element.first.rfind("È", 0) == std::string::npos
			//	||  element.first.rfind("É", 0) == std::string::npos || element.first.rfind("Ê", 0) == std::string::npos || element.first.rfind("Ë", 0) == std::string::npos || element.first.rfind("Ì", 0) == std::string::npos || element.first.rfind("Í", 0) == std::string::npos
			//	||  element.first.rfind("Î", 0) == std::string::npos || element.first.rfind("Ï", 0) == std::string::npos || element.first.rfind("Ð", 0) == std::string::npos || element.first.rfind("Ñ", 0) == std::string::npos || element.first.rfind("Ò", 0) == std::string::npos
			//	||  element.first.rfind("Ó", 0) == std::string::npos || element.first.rfind("Ô", 0) == std::string::npos || element.first.rfind("Õ", 0) == std::string::npos || element.first.rfind("Ö", 0) == std::string::npos || element.first.rfind("×", 0) == std::string::npos
			//	||  element.first.rfind("Ø", 0) == std::string::npos || element.first.rfind("Ù", 0) == std::string::npos || element.first.rfind("Û", 0) == std::string::npos || element.first.rfind("Ý", 0) == std::string::npos || element.first.rfind("Þ", 0) == std::string::npos
			//	||  element.first.rfind("ß", 0)
			//	||  element.first.rfind("à", 0) == std::string::npos || element.first.rfind("á", 0) == std::string::npos || element.first.rfind("â", 0) == std::string::npos || element.first.rfind("ã", 0) == std::string::npos || element.first.rfind("ä", 0) == std::string::npos
			//	||  element.first.rfind("å", 0) == std::string::npos || element.first.rfind("¸", 0) == std::string::npos || element.first.rfind("æ", 0) == std::string::npos || element.first.rfind("ç", 0) == std::string::npos || element.first.rfind("è", 0) == std::string::npos
			//	||  element.first.rfind("é", 0) == std::string::npos || element.first.rfind("ê", 0) == std::string::npos || element.first.rfind("ë", 0) == std::string::npos || element.first.rfind("ì", 0) == std::string::npos || element.first.rfind("í", 0) == std::string::npos
			//	||  element.first.rfind("î", 0) == std::string::npos || element.first.rfind("ï", 0) == std::string::npos || element.first.rfind("ð", 0) == std::string::npos || element.first.rfind("ñ", 0) == std::string::npos || element.first.rfind("ò", 0) == std::string::npos
			//	||  element.first.rfind("ó", 0) == std::string::npos || element.first.rfind("ô", 0) == std::string::npos || element.first.rfind("õ", 0) == std::string::npos || element.first.rfind("ö", 0) == std::string::npos || element.first.rfind("÷", 0) == std::string::npos
			//	||  element.first.rfind("ø", 0) == std::string::npos || element.first.rfind("ù", 0) == std::string::npos || element.first.rfind("û", 0) == std::string::npos || element.first.rfind("ý", 0) == std::string::npos || element.first.rfind("þ", 0) == std::string::npos
			//	||  element.first.rfind("ÿ", 0) == std::string::npos
			//	)
			//{
			//	for_removal.push_back(element.first);

			//	// DEBUG
			//	if (debug)
			//	{
			//		std::cout << "FOR REMOVAL";
			//	}
			//}
			//else
			//{
			//	// DEBUG
			//	if (debug)
			//	{
			//		std::cout << "ALLOWED TO DATABASE";
			//	}
			//}

			// DEBUG, ONLY 1 DEBUG WORD REMAINS <--------------------------------------------------------------- REMOVE!
			/*if (element.first != "openssl" && element.first != "get" && element.first != "open")
			{
				for_removal.push_back(element.first);
			}*/
		}

		for (const auto element : for_removal)
		{
			wordsLib.erase(element);
		}

		for_removal.clear();

		// DEBUG
		if (debug)
		{
			std::system("pause");
			std::cout << "\n> DEBUG: Printing words library for URL " << webpage.get_address();

			for (const auto& el : wordsLib)
			{
				std::cout << "\n- " << el.first << ", repeats = " << el.second;
			}
		}
		// !DEBUG
	}
}