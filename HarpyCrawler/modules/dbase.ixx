// module file with dbase control class

module;

// Core includes
#include <iostream>
#include <memory>
#include <map>
#include <string>
#include <queue>
#include <vector>

// WT Lib
#include <Wt/Dbo/Dbo.h>
#include <Wt/Dbo/backend/Postgres.h>

// Custom includes
#include "..\headers\shared_types.h"

export module database;

namespace harpy
{
// ============================================================================================================================

	// fw delc

	class pages;
	class words;	
	class crossings;

	export class DBASE final // --------------------------------------------------------------------- DBASE
	{
	public:

		// process vector of pages
		void process_pages(std::vector<harpy::WebPage>& _webPages);

		// process one page
		void process_page(harpy::WebPage & webpage);
		
		DBASE() = delete;
		
		DBASE(std::string);		
		
	protected:
		
	private:		

		std::string _connectionString;
		Wt::Dbo::Session _session;			

		void write_data(harpy::WebPage & _webPages);
		void build_crossings(harpy::WebPage & _webPages);

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
		
		Wt::Dbo::collection < Wt::Dbo::ptr< crossings > > cross;
		std::string title;
		std::string url;		

		template<class Action>
		void persist(Action& a)
		{			
			Wt::Dbo::hasMany(a, cross, Wt::Dbo::ManyToOne, "page");
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

		Wt::Dbo::collection < Wt::Dbo::ptr< crossings > > cross;
		std::string word;

		template<class Action>
		void persist(Action& a)
		{					
			Wt::Dbo::hasMany(a, cross, Wt::Dbo::ManyToOne, "word");
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

void harpy::DBASE::process_page(harpy::WebPage & webpage)
{
	write_data(webpage);
	build_crossings(webpage);

	std::cout << "\n> Operations complete!";
}

void harpy::DBASE::process_pages(std::vector<harpy::WebPage>& _webPages)
{
	// std::cout << "\n> Processing the pages and words...";

	if (!_webPages.empty())
	{
		for (auto webpage : _webPages)
		{
			write_data(webpage);
		}

		// std::cout << "\n> COMPLETE!";
		
	}
	else
	{
		// std::cout << "\n> Datgabase job: vector of pages is empty... nothing is written";
	}

	// std::cout << "\n> Building crossings...";

	if (!_webPages.empty())
	{
		for (auto webpage : _webPages)
		{
			build_crossings(webpage);
		}

		// std::cout << "\n> COMPLETE!";
	}
	else
	{
		// std::cout << "\n> Datgabase job: vector of pages is empty... nothing is written";
	}

	std::cout << "\n> Operations complete!";
}

void harpy::DBASE::write_data(harpy::WebPage & webpage)
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
	}
 }

void harpy::DBASE::build_crossings(harpy::WebPage & webpage)
{
	std::string url = webpage.get_address();
	std::map<std::string, int> extracted_words = webpage.get_wordslib();

	for (auto webword : extracted_words)
	{
		std::string keyword = webword.first;						

		try
		{
			Wt::Dbo::Transaction transaction(_session);

			Wt::Dbo::collection < Wt::Dbo::ptr<crossings> > crossing_w = _session.find<crossings>().where("word_id = ?").bind(_session.find<words>().where("word = ?").bind(keyword.c_str()).resultValue().id());
			Wt::Dbo::collection < Wt::Dbo::ptr<crossings> > crossing_p = _session.find<crossings>().where("page_id = ?").bind(_session.find<pages>().where("url = ?").bind(url.c_str()).resultValue().id());

			if (crossing_p.empty()) // page do not exist
			{
				std::unique_ptr<crossings> cross(new crossings);
				cross->repeats = webword.second;
				Wt::Dbo::ptr<crossings> new_cross = _session.add(std::move(cross));
				_session.find<pages>().where("url = ?").bind(url).resultValue().modify()->cross.insert(new_cross);
				_session.find<words>().where("word = ?").bind(keyword).resultValue().modify()->cross.insert(new_cross);						
			}
			else if (!crossing_p.empty() && crossing_w.empty()) // page exists, word is not
			{
				std::unique_ptr<crossings> cross(new crossings);
				cross->repeats = webword.second;
				Wt::Dbo::ptr<crossings> new_cross = _session.add(std::move(cross));
				_session.find<pages>().where("url = ?").bind(url).resultValue().modify()->cross.insert(new_cross);
				_session.find<words>().where("word = ?").bind(keyword).resultValue().modify()->cross.insert(new_cross);
			}
			else // page-word pair both exists
			{
				// UPDATE CROSSING WITH NEW VALUE
								 
				Wt::Dbo::collection < Wt::Dbo::ptr<crossings> > this_cross = _session.find<crossings>().where("page_id = ?").bind(_session.find<pages>().where("url = ?").bind(url.c_str()).resultValue().id());

				for (Wt::Dbo::ptr<crossings> cr : this_cross)
				{
					if (cr->word->word == keyword)
					{
						cr.modify()->repeats = webword.second;
					}
				}

				/*if (this_cross.size() != 0 && this_cross.size() > 1)
				{
					std::cout << "\n> multiple crossings for some reason, can't update value...";
				}
				else if (this_cross.size() == 1)
				{
					this_cross.front().modify()->repeats = webword.second;
				}*/				

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