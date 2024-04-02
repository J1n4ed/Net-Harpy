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

		// PRIVATE METHODS DECL
			
		void SearchForNodesUsingLoop();
		void process_string();
		void findErase(std::string *, std::string, std::string);		
		void buildWordLib();
		
	}; // !Indexer

} // !HARPY

// METHODS -------------------------------------

harpy::Indexer::Indexer(std::string _page, std::string _url, bool _isEnd)
{
	page = _page;
	isEnd = _isEnd;

	webpage.set_address(_url);	

	// converting the page into vector of strings	

	process_string();	

	

} // !Indexer

void harpy::Indexer::process_string()
{
	SearchForNodesUsingLoop();
	buildWordLib();

	if (!links.empty())
	{		
		webpage.set_isLinksEmpty(false);
		webpage.set_links(links);
	}
	else
	{
		webpage.set_isLinksEmpty(true);
	}
	
	webpage.set_wordslib(wordsLib);
}

// Me gusta
void harpy::Indexer::SearchForNodesUsingLoop()
{
	int counter = 1;
	
	std::string tmp;
	std::string url;

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

				if (line.back() == '/')
				{
					line.erase(line.size());
				}
				
				if (line.find("http:") != std::string::npos || line.find("https:") != std::string::npos)
				{
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
	
	{
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
		size_t start; // absurd large number for html to check for successful init
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

	std::string word;
	std::istringstream iss(page);

	do 
	{
		iss >> word;

		std::transform(word.begin(), word.end(), word.begin(), [](unsigned char c) { return std::tolower(c); });

		std::transform(word.begin(), word.end(), word.begin(), towlower);		

		if (!wordsLib.count(word))
			wordsLib.insert(make_pair(word, 1));
		else
			wordsLib[word]++;

	} while (iss);

	std::vector<std::string> for_removal;
	
	// remove short and long words
	for (const auto element : wordsLib)
	{
		if (element.first.length() < 4)
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
}