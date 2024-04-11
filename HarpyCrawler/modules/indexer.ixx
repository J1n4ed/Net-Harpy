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

	webpage.set_address(_url);	

	// USE THIS TO SWITCH ON DEBUG OUTPUT FOR SPECIFIC URL
	// DEBUG
	/*if (_url == "https://wiki.openssl.org/index.php/Use_of_Git")
		debug = true;*/
	// !DEBUG

	// converting the page into vector of strings	

	process_string();		

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
		
	while (page.find("\n", linePosBegin) != std::string::npos)
	{
		linePosEnd = page.find("\n", linePosBegin);

		if (linePosBegin >= page.size() || linePosEnd >= page.size())
		{
			break;
		}

		std::string line = page.substr(linePosBegin, linePosEnd);

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
	
}

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
		page.erase(std::remove(page.begin(), page.end(), '\n'), page.cend());
	}

	// remove scripts
	findErase(&page, "<script>", "</script>");
	findErase(&page, "/script>", ">");
	
	// REGEXP	

	std::regex pattern("\\<.*?\\>");
	page = regex_replace(page, pattern, " ");
	
	pattern = "\\[.*?\\]";
	page = regex_replace(page, pattern, " ");

	pattern = "\\{.*?\\}";
	page = regex_replace(page, pattern, " ");

	pattern = "\t";
	page = regex_replace(page, pattern, " ");	

	std::remove_if(page.begin(), page.end(), ispunct);

	page.erase(std::remove_if(page.begin(), page.end(), ::isdigit), page.end());

	// Spaces

	std::string::iterator new_end = std::unique(page.begin(), page.end(), &harpy::BothAreSpaces);
	page.erase(new_end, page.end());	
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
						wordsLib.insert(make_pair(word, 1));
					else
						wordsLib[word]++;
				}
			}

		}

		std::vector<std::string> for_removal;
	
		// remove short and long words
		for (const auto element : wordsLib)
		{
			if (element.first.length() < 3)
			{
				for_removal.push_back(element.first);
			}
		
			if (element.first.length() > 14)
			{
				for_removal.push_back(element.first);
			}
		
			if (	element.first._Starts_with("?") || element.first._Starts_with("¹") || element.first._Starts_with(" ") 
				||  element.first._Starts_with("(") || element.first._Starts_with(".") || element.first._Starts_with("0")
				||	element.first._Starts_with("1") || element.first._Starts_with("2") || element.first._Starts_with("3") 
				||  element.first._Starts_with("4")	|| element.first._Starts_with("5") || element.first._Starts_with("6") 
				||  element.first._Starts_with("7") || element.first._Starts_with("8") || element.first._Starts_with("9") 
				||  element.first._Starts_with("&"))
			{
				for_removal.push_back(element.first);
			}
		}

		for (const auto element : for_removal)
		{
			wordsLib.erase(element);
		}

		for_removal.clear();

		// DEBUG
		if (debug)
		{
			std::cout << "\n> DEBUG: Printing words library for URL " << webpage.get_address();

			for (const auto& el : wordsLib)
			{
				std::cout << "\n- " << el.first << ", repeats = " << el.second;
			}
		}
		// !DEBUG

	}
}