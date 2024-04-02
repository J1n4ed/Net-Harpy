#include <string>
#include <map>

#include "../headers/shared_types.h"

harpy::WebPage::WebPage()
{

} // !WebPage

std::string harpy::WebPage::get_address()
{
	return address;
} // !set_address

std::string harpy::WebPage::get_title()
{
	return title;
} // !set_title

std::string harpy::WebPage::get_lastmod()
{
	return lastMod;
} // !set_lastmod

std::map<std::string, int> harpy::WebPage::get_wordslib()
{
	return wordsLib;
} // !get_wordslib

std::vector<std::string> harpy::WebPage::get_links()
{
	return links;
} // !get_links

bool harpy::WebPage::get_isLinksEmpty()
{
	return isLinksEmpty;
} // !get_isLinksEmpty

void harpy::WebPage::set_address(std::string _address)
{
	address = _address;
} // !set_address

void harpy::WebPage::set_title(std::string _title)
{
	title = _title;
} // !set_title

void harpy::WebPage::set_lastmod(std::string _lastMod)
{
	lastMod = _lastMod;
} // !set_lastmod

void harpy::WebPage::set_wordslib(std::map<std::string, int> _wordsLib)
{
	wordsLib = _wordsLib;
} // !set_wordslib

void harpy::WebPage::set_links(std::vector<std::string> _links)
{
	links = _links;
} // !set_links

void harpy::WebPage::set_isLinksEmpty(bool _isLinksEmpty)
{
	isLinksEmpty = _isLinksEmpty;
} // !set_isLinksEmpty

void harpy::WebPage::print()
{
	std::cout << " --- Indexed page info --- " << std::endl;
	std::cout << " > URL:\t\t\t" << address << std::endl;
	std::cout << " > TITLE:\t\t" << title << std::endl;
	std::cout << " > LAST MODIFIED:\t" << lastMod << std::endl;	
	std::cout << "\n > Unique words library extracted:\n\n";
	for (const auto element : wordsLib)
	{
		std::cout << " > " << element.first << " - " << element.second << '\n';
	}
	std::cout << "\n\n ------------------------------------- \n";
} // !print

harpy::WebPage::WebPage(const harpy::WebPage& _webpage)
{
	address = _webpage.address;
	title = _webpage.title;
	lastMod = _webpage.lastMod;
	wordsLib = _webpage.wordsLib;
	links = _webpage.links;
	isLinksEmpty = _webpage.isLinksEmpty;	
}

harpy::WebPage::WebPage(const harpy::WebPage * _webpage)
{
	address = _webpage->address;
	title = _webpage->title;
	lastMod = _webpage->lastMod;
	wordsLib = _webpage->wordsLib;
	links = _webpage->links;
	isLinksEmpty = _webpage->isLinksEmpty;
}