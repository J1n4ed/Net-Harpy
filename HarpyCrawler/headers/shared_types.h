#pragma once

#include <string>
#include <iostream>
#include <map>
#include <vector>

namespace harpy
{
	struct WebPage
	{
	public:

		// Getters

		std::string get_address();
		std::string get_title();
		std::string get_lastmod();
		std::map<std::string, int> get_wordslib();
		std::vector<std::string> get_links();
		bool get_isLinksEmpty();
		

		// Setters

		void set_address(std::string _address);
		void set_title(std::string _title);
		void set_lastmod(std::string _lastMod);
		void set_wordslib(std::map<std::string, int> _wordsLib);
		void set_links(std::vector<std::string>);
		void set_isLinksEmpty(bool);

		// Methods

		void print();

		// Constructors

		WebPage();

		WebPage(const harpy::WebPage & _webpage);
		WebPage(const harpy::WebPage * _webpage);

	private:

		std::string address;
		std::string title;
		std::string lastMod;
		std::map<std::string, int> wordsLib;
		std::vector<std::string> links;
		bool isLinksEmpty = true;
	};

} // !harpy