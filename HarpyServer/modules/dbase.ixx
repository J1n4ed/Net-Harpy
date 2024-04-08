// module file with dbase control class

module;

// Core includes
#include <iostream>
#include <memory>
#include <map>
#include <string>
#include <queue>
#include <vector>
#include <set>

// WT Lib
#include <Wt/Dbo/Dbo.h>
#include <Wt/Dbo/backend/Postgres.h>

// Custom includes
#include "..\headers\shared_types.h"

#define MAX_SEARCH_OUTPUT 20

export module database;

namespace harpy
{
// ============================================================================================================================

	// fw delc

	class pages;
	class words;	
	class crossings;

	// typedef std::pair<Wt::Dbo::ptr<harpy::pages>, int> pairs;

	struct Pairs
	{
		Wt::Dbo::ptr<harpy::pages> page_id;
		int page_relevance;
	};
	
	struct RelPage
	{
		std::string url;
		std::string title;		

		constexpr auto operator==(const RelPage& rhs)
		{
			return this->url == rhs.url;
		}		

		constexpr auto operator!=(const RelPage& rhs)
		{ 
			return !(*this == rhs); 
		}

		constexpr auto operator<=>(const RelPage&) const = default;
	};

	export class DBASE final // --------------------------------------------------------------------- DBASE
	{
	public:

		void write_data(std::vector<harpy::WebPage>& _webPages);

		std::string search_results(std::vector<std::string> _wordsLib);
		
		DBASE() = delete;
		
		DBASE(std::string);		
		
	protected:
		
	private:		

		std::string _connectionString;
		Wt::Dbo::Session _session;				

	}; // !DBASE ------------------------------------------------------------------------------ !DBASE

	// Классы таблиц
	
	/*
	- id
	- url
	- title
	*/
	class pages final
	{
	public:
		
		Wt::Dbo::collection < Wt::Dbo::ptr< crossings > > id;
		std::string title;
		std::string url;		

		template<class Action>
		void persist(Action& a)
		{			
			Wt::Dbo::hasMany(a, id, Wt::Dbo::ManyToOne, "page");
			Wt::Dbo::field(a, title, "title");
			Wt::Dbo::field(a, url, "url");
		}
	}; // !pages ++++++++++++

	/*
	- id
	- word
	*/
	class words final
	{
	public:

		Wt::Dbo::collection < Wt::Dbo::ptr< crossings > > id;
		std::string word;

		template<class Action>
		void persist(Action& a)
		{					
			Wt::Dbo::hasMany(a, id, Wt::Dbo::ManyToOne, "word");
			Wt::Dbo::field(a, word, "word");
		}
	}; // !word +++++++++++++++++	

	/*
	- id_pages
	_ id_words
	- repeats
	*/
	class crossings final
	{
	public:

		Wt::Dbo::ptr<pages> page;
		Wt::Dbo::ptr<words> word;
		int repeats;

		template<class Action>
		void persist(Action& a)
		{
			Wt::Dbo::belongsTo(a, page, "page");
			Wt::Dbo::belongsTo(a, word, "word");
			Wt::Dbo::field(a, repeats, "repeats");
		}
	}; // !word +++++++++++++++++	

	// -------------------- METHODS ------------------------

	/*
	Constructor for DBASE class
	*/
	DBASE::DBASE(std::string connectionString) : _connectionString(connectionString)
	{
		std::cout << "\n> JOB: Creating dbase object...";
		
		auto postgres = std::make_unique<Wt::Dbo::backend::Postgres>(_connectionString);		

		try
		{
			_session.setConnection(std::move(postgres));	
		}
		catch (Wt::Dbo::Exception & ex)
		{
			std::cout << "\n> DBO: " << ex.what();
		}

		// MAP PAGES
		try
		{
			_session.mapClass<pages>("pages");
		}
		catch (Wt::Dbo::Exception& ex)
		{
			std::cout << "\n> DBO: " << ex.what();
		}

		// MAP WORDS
		try
		{
			_session.mapClass<words>("words");
		}
		catch (Wt::Dbo::Exception& ex)
		{
			std::cout << "\n> DBO: " << ex.what();
		}		

		// MAP CROSSINGS
		try
		{
			_session.mapClass<crossings>("crossings");
		}
		catch (Wt::Dbo::Exception& ex)
		{
			std::cout << "\n> DBO: " << ex.what();
		}

		// MAKE TRANSACTION
		Wt::Dbo::Transaction transaction(_session);

		try
		{
			std::cout << "\n> JOB: Creating tables...";

			// make tables
			_session.createTables();					

			std::cout << "\n> Creating tables complete!";

		}
		catch (const Wt::Dbo::Exception& e)
		{
			std::cout << std::endl << "> DBO: Exception while creating tables!\nOutput: " << e.what() << std::endl;
		}	

		transaction.commit();		

	} // !DBASE	

// ============================================================================================================================

} // !HARPY

void harpy::DBASE::write_data(std::vector<harpy::WebPage> & _webPages)
{
	std::cout << "\n> Database job: writing webpages data to database...";	

	// bool result = true;

	if (!_webPages.empty())
	{
		std::vector<harpy::WebPage> extracted_page = _webPages;

		for (auto webpage : extracted_page)
		{
			std::string title = webpage.get_title();
			std::string url = webpage.get_address();
			std::map<std::string, int> extracted_words = webpage.get_wordslib();			

			try
			{
				Wt::Dbo::Transaction transaction(_session);				

				Wt::Dbo::collection<Wt::Dbo::ptr<pages>> p1 = _session.find<pages>().where("url = ?").bind(url.c_str());							

				if (p1.empty())
				{
					std::unique_ptr<pages> page(new pages);
					page->title = title;
					page->url = url;
					Wt::Dbo::ptr<pages> new_page = _session.add(std::move(page));					
				}

				for (auto webword : extracted_words)
				{			
					std::string keyword = webword.first;
					int numRepeats = webword.second;

					Wt::Dbo::collection < Wt::Dbo::ptr<words> > w1  = _session.find<words>().where("word = ?").bind(keyword.c_str());

					if (w1.empty())
					{						
						std::unique_ptr<words> word(new words);
						word->word = keyword;
						Wt::Dbo::ptr<words> new_word = _session.add(std::move(word));						
					}
				}

				transaction.commit();
			}
			catch (Wt::Dbo::Exception& ex)
			{
				std::cout << "\n> DBO: " << ex.what();
				// result = false;
			}			
		}

		std::cout << "\n> Building crossings...";

		_session.rereadAll();

		if (1)
		{
			for (auto webpage : extracted_page)
			{
				std::string url = webpage.get_address();
				std::map<std::string, int> extracted_words = webpage.get_wordslib();

				Wt::Dbo::collection < Wt::Dbo::ptr<pages> > page = _session.find<pages>().where("url = ?").bind(url.c_str());

				auto id_page = page.front().id();

				for (auto webword : extracted_words)
				{
					std::string keyword = webword.first;

					Wt::Dbo::collection < Wt::Dbo::ptr<words> > word = _session.find<words>().where("word = ?").bind(keyword.c_str());

					auto id_word = word.front().id();

					try
					{
						Wt::Dbo::Transaction transaction(_session);

						Wt::Dbo::collection < Wt::Dbo::ptr<crossings> > crossing_w = _session.find<crossings>().where("id_word = ?").bind(id_word);
						Wt::Dbo::collection < Wt::Dbo::ptr<crossings> > crossing_p = _session.find<crossings>().where("id_page = ?").bind(id_page);

						if (crossing_p.empty()) // page do not exist
						{
							std::unique_ptr<crossings> cross(new crossings);
							cross->page = page.front();
							cross->word = word.front();
							cross->repeats = webword.second;
							Wt::Dbo::ptr<crossings> new_cross = _session.add(std::move(cross));
						}
						else if (!crossing_p.empty() && crossing_w.empty()) // page exists, word is not
						{
							std::unique_ptr<crossings> cross(new crossings);
							cross->page = page.front();
							cross->word = word.front();
							cross->repeats = webword.second;
							Wt::Dbo::ptr<crossings> new_cross = _session.add(std::move(cross));
						}
						else // page-word pair both exists
						{
							// UPDATE CROSSING WITH NEW VALUE

							// TO-DO
						}

						transaction.commit();
					}
					catch (Wt::Dbo::Exception& ex)
					{
						std::cout << "\n> DBO: " << ex.what();		
					}
				}
			}
		}

		std::cout << "\n> COMPLETE!";
	}
	else
	{
		std::cout << "\n> Datgabase job: vector of pages is empty... nothing is written";
	}	
 }

 bool cmp(std::pair<harpy::RelPage, int> & a, std::pair<harpy::RelPage, int> & b)
 {
	 return a.second > b.second;
 }

 std::vector<std::pair<harpy::RelPage,int>> sort(std::map<harpy::RelPage, int> & M)
 {

	 // Declare vector of pairs 
	 std::vector<std::pair<harpy::RelPage, int> > A;

	 // Copy key-value pair from Map 
	 // to vector of pairs 
	 for (auto& it : M) 
	 {
		 A.push_back(it);
	 }

	 // Sort using comparator function 
	 sort(A.begin(), A.end(), cmp);	 

	 return A;
 }

 // SEARCH FUNCTION
 std::string harpy::DBASE::search_results(std::vector<std::string> _searchLib)
 {
	std::string result;
	 
	 std::vector<Pairs> search_set;

	 _session.rereadAll();

	 Wt::Dbo::Transaction transaction(_session);

	 for (const auto & element : _searchLib)
	 {
		 std::string current_word = element;
		 current_word.shrink_to_fit();

		 try
		 {
			 Wt::Dbo::collection < Wt::Dbo::ptr<words> > word_check = _session.find<words>().where("word = ?").bind(current_word.c_str());

			 if (!word_check.empty())
			 {
				 // std::cout << "\n> Word: " << current_word << " is found in database!";

				 Pairs par;

				for (const auto & iter : word_check)
				{					
					Wt::Dbo::collection <Wt::Dbo::ptr<crossings> > word_cross = iter->id;		

					// std::cout << "\n> Number of entreese: " << word_cross.size();

					for (const auto& iter_cross : word_cross)
					{
						par.page_id = iter_cross->page;
						par.page_relevance = iter_cross->repeats;

						/*std::cout << "\n> PAGE: " << par.page_id->url << '\n'
							<< "> RELEVANCE: " << par.page_relevance;*/

						search_set.push_back(par);
					}
				}				
			
			 }
			 else
			 {
				 // std::cout << "\n> Word: " << current_word << " not found!";
			 }
		 }
		 catch (Wt::Dbo::Exception& ex)
		 {
			 std::cout << "\n> DBO: " << ex.what();
		 }
	 }	

	 if (search_set.size() != 0)
	 {
		 std::cout << "\n> Thread: " << std::thread::id() << ", search vector recieved " << search_set.size() << " elements.";		 

		 // map to hold the relevance
		 std::map<RelPage, int> relevance;		

		 for (const auto& elem : search_set)
		 {
			 RelPage tmp;
			 tmp.url = elem.page_id->url;
			 tmp.title = elem.page_id->title;
			
			 if (relevance.find(tmp) == relevance.end())
			 {
				 relevance.insert(std::make_pair(tmp, elem.page_relevance));
			 }
			 else
			 {
				 relevance[tmp] += elem.page_relevance;				 
			 }
		 }		

		 // std::cout << "\n> Relevance map finished, printing:\n\n";

		 /*for (const auto& element : relevance)
		 {
			 std::cout << "\n> Relevance: " << element.second << "\nTitle: " << element.first.title << "\nURL: " << element.first.url << "\n---------------";
		 }*/
		 
		 // FORM RETURN STRING, LIST OF WEBSITES

		 std::vector<std::pair<harpy::RelPage, int>> relevance_sorted = sort(relevance);
		 int count = 1;
		 result = "<ul>";

		 for (const auto& page : relevance_sorted)
		 {
			 result += ("<li><div><p>Result #" + std::to_string(count) + ", relevance: " + std::to_string(page.second)) + "</p>" +
				 "<p><a href=\"" + page.first.url + "\">" + page.first.title + "</a></p>" + "</div></li>";
			 ++count;

			 // LIMITER (maximum number of search results displayed (default = 20)
			 if (count == MAX_SEARCH_OUTPUT)
			 {
				 break;
			 }
		 }		 

		 result += "</ul>";
	 }
	 else
	 {
		 std::cout << "\n> Thread: " << std::thread::id() << ", search vector is empty!";

		 result = "Nothing found!";
	 }

	 transaction.commit();	 

	 return result;
 }