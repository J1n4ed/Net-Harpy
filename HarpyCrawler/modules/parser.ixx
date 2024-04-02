module;

// INCLUDES

#include <iostream>
#include <fstream>
#include <map>
#include <exception>
#include <string>
#include <memory>
#include <optional>
#include <any>
#include <vector>

// DEFINES

export module parser;

namespace harpy
{
	// EXCEPTIONS SECTION ----------------------------------------------------------------------------------------

	export class type_mismatch : public std::exception
	{
	public:

		const char* what() const override
		{
			return errorline.c_str();
		}

		type_mismatch(std::string type)
		{
			_requestedType = type;
			errorline = "\n> ERROR: requested type <" + _requestedType + "> mismatch!";
		}

	private:
		std::string _requestedType;
		std::string errorline;
	}; // END type_mismatch

	export class file_not_found : public std::exception
	{
	public:
		const char* what() const override
		{		
			return "\n> ERROR: file not found!";		
		}
	}; // END file_not_found

	export class unknown_type : public std::exception
	{
	public:
		const char* what() const override
		{
			return "\n> ERROR: uknown data type!";
		}
	}; // END unknown_type


	export class data_not_found : public std::exception
	{
	public:
		const char* what() const override
		{		

			return errorline.c_str();
		}

		data_not_found(std::string request, const std::map< std::string, std::any> & container)
		{			
			_request = request;

			std::string::iterator it = std::find(request.begin(), request.end(), '.');
			_section = std::string(request.begin(), it);

			for (const auto& element : container)
			{	
				if ( (element.first.find(_section) != std::string::npos) && element.first != request)
				{					
					_possibleLines.push_back(element.first);
				}
			}

			// for some reason the none existent request is pushed into vector as last actual element wth
			/*
			std::cout << "\nDEBUG: possbile lines: \n";
			for (const auto& elem : _possibleLines)
			{
				std::cout << elem << '\n';
			}
			*/

			errorline = "\n> ERROR: data requested with <" + _request + "> not found in container!\n" +
				"> Possible missmatch?";

			if (!_possibleLines.empty())
			{	
				errorline += "\nPossible options:\n";

				for (const auto& line : _possibleLines)
				{
					errorline += '\n' + line;
				}				
			}
		}

	private:
		std::string _request;
		std::string _section;
		std::map< std::string, std::any> * _container;
		std::vector<std::string> _possibleLines;
		std::string errorline;
	}; // END


	export class location_is_empty : public std::exception
	{
	public:
		const char* what() const override
		{
			return _errorline.c_str();
		}

		location_is_empty(std::string request)
		{
			std::string::iterator it = std::find(request.begin(), request.end(), '.');
			_sectionName = std::string(request.begin() + 1, it - 1);
			_varName = std::string(it + 1, request.end() - 1);

			_errorline = "\n> ERROR: requested section <" + _sectionName + "> and variable <" + _varName + "> has no assigned value!";
		}

	private:
		std::string _sectionName;
		std::string _varName;
		std::string _errorline;
	}; // END invalid_line


	export class invalid_line : public std::exception
	{
	public:
		const char* what() const override
		{
			return "\n> ERROR: invalid line found at file at line #" + errorLine + '!';
		}

		invalid_line(int line)
		{
			errorLine = line;
		}

	private:
		int errorLine;
	}; // END invalid_line

	export class invalid_variable_name : public std::exception
	{
	public:
		const char* what() const override
		{
			return errorline.c_str();
		}

		invalid_variable_name(int line, std::string invalidVar) : _errorLine(line), _errorVar(invalidVar) 
		{
			errorline = "\n> ERROR: invalid variable name <" + _errorVar + "> found at file at line #" + std::to_string(_errorLine) + "!";
		}

	private:
		int _errorLine;
		std::string _errorVar;
		std::string errorline;
	}; // END

	export class invalid_section_name : public std::exception
	{
	public:
		const char* what() const override
		{
			return errorline.c_str();
		}

		invalid_section_name(int line, std::string invalidSec) : _errorLine(line), _errorSec(invalidSec) 
		{
			errorline = "\n> ERROR: invalid section name <" + _errorSec + "> found at file at line #" + std::to_string(_errorLine) + "!";
		}

	private:
		int _errorLine;
		std::string _errorSec;
		std::string errorline;
	}; // END

	/*
	TODO: Exceptions for parser class
	*/


	// CLASS (ini parser) ------------------------------------------------------------------------------------------

		/*
		ini parser class
		create by passing ini filename
		*/

	export class parser final
	{
	public:
		// PIUBLIC AREA

		// - public methods

		/*
		Print while _content container
		*/
		void print()
		{
			for (const auto& element : _content)
			{
				std::cout << "KEY: " << element.first << "\nVALUE: ";

				if (element.second.type().name() == typeid(std::string).name()) // is string
				{
					std::cout << std::any_cast<std::string>(element.second);
				}
				else if (element.second.type().name() == typeid(int).name()) // is int
				{
					std::cout << std::any_cast<int>(element.second);
				}
				else if (element.second.type().name() == typeid(double).name()) // is double
				{
					std::cout << std::any_cast<double>(element.second);
				}
				else if (element.second.type().name() == typeid(unsigned short).name()) // is no value dummy
				{
					std::cout << "<VARIABLE HAVE NO VALUE>";
				}
				else
				{
					// Throw unknown type exception
					throw harpy::unknown_type();
				}

				std::cout << std::endl;
			}
		}

		/*
		Template function to fetch values
		*/
		template<typename T>
		T get_value(std::string location)
		{
			T value;

			// tmp

			if (_content[location].has_value())
			{

				// handle the empty slots with unigned int dummy zeroes
				if (_content[location].type().name() == typeid(unsigned int).name())
				{
					throw harpy::location_is_empty(location);
				}

				if (_content[location].type().name() == typeid(T).name())
				{
					value = std::any_cast<T>(_content[location]);
				}
				else
				{
					std::string type;

					if (typeid(T).name() == typeid(int).name())
					{
						type = "int";
					}
					else if (typeid(T).name() == typeid(double).name())
					{
						type = "double";
					}
					else if (typeid(T).name() == typeid(std::string).name())
					{
						type = "std::string";
					}
					else
					{
						type = "unhandled_type";
					}

					// throw wrong type exception
					throw type_mismatch(type);
				}
			}
			else
			{
				// throw no such data exception
				throw harpy::data_not_found(location, _content);

			}

			// get value from ini info here

			return value;
		}

		// - overloads

		// constructors

		parser() = delete;

		parser(const parser&) = delete;

		parser(parser&&) = delete;

		parser(std::string filename)
		{

			std::cout << "\n> Parser recieved filename: " << filename;

			_datastream.open(filename, std::ios::in);

			if (!_datastream.is_open())
			{
				// exception, file not opened
				throw harpy::file_not_found();

			}
			else
				// IF reading is successful we populate the _content with ini data
			{
				// reading line by line and populating _content

				// VARIABLES

				std::string raw_string;					// current string from stream
				int lineNum = 0;						// counter for lines
				bool isEmpty = true;					// true if empty, flag for if string is empty string
				bool goBreak = false;					//
				bool inSection = false;					// flag indicates we're in current section, change to when next section is read
				bool isAllSpaces = true;				// flag indicates if string contains only spaces
				std::string currentSectioName = "";		// held value of current section
				std::string currentVarName;				// current value name 
				std::string currentVarValue = "";		// current var value before processing

				while (!_datastream.eof())
				{
					std::getline(_datastream, raw_string, '\n');

					// INITIAL VAR PARAMS FOR CURRENT STRING

					isEmpty = raw_string.empty();
					goBreak = false;
					isAllSpaces = true;
					++lineNum;

					// skip the rest is the line is empty
					if (!isEmpty)
					{
						// Check if line is spaces only
						for (const auto& character : raw_string)
						{
							if (character != ' ')
							{
								isAllSpaces = false;
								continue;
							}
						} // !Check for all spaces line -------------

						if (!isAllSpaces) // if isAllSpaces is true - skip all the further code
						{

							// Find and remove comments

							std::string::iterator it_begin = std::find(raw_string.begin(), raw_string.end(), ';');

							if (it_begin == raw_string.begin() + 1) // comment start from first character
							{
								isEmpty = true;
								it_begin = raw_string.begin(); // return it_begin to begin
								// further ignored, whole line is comment
							}
							else if (it_begin != raw_string.end()) // comment somewhere in line
							{								
								std::string temp_str = std::string(raw_string.begin(), it_begin);
								raw_string.clear();
								raw_string = temp_str;
								it_begin = raw_string.begin(); // return it_begin to begin
							}
							else
							{
								// none
							}

							it_begin = raw_string.begin(); // return it_begin to begin before next section
							// !Find and remove comments

							if (!isEmpty)
							{
								// if someth left after comments 'r removed, continue to process

								// check if string is [section]
								it_begin = std::find(raw_string.begin(), raw_string.end(), '[');

								if (it_begin != raw_string.end()) // [ found before end of line
								{
									std::string::iterator it_end = std::find(raw_string.begin(), raw_string.end(), ']');

									// if only [ found till the end of the line throw invalid line exception

									if (it_end != raw_string.end())
									{
										std::string section_string = std::string(it_begin + 1, it_end);

										// section_string contains [Section] info at this point

										// TO-DO

										// Section must not have spaces in it, check for spaces, throw exception if spaces in section name

										for (const auto& character : section_string)
										{
											if (character == ' ')
											{
												throw harpy::invalid_section_name(lineNum, section_string);
											}
										} // !check section name for invalid characters

										// SET CURRENT SECTION NAME TO FOUND SECTION STRING
										currentSectioName = section_string;

										/*
										// _content map iterator
										std::map< std::string, std::map<std::string, harpy::DataBase& > >::iterator _mapIter;
										// look for found section
										_mapIter = _content.find(section_string);

										if (_mapIter != _content.end())
										{
											// Section already exist in _content
											std::cout << "\nDEBUG: in _mapIter != end";
										}
										else
										{
											// Add new section to _content map
											std::cout << "\nDEBUG: in _mapIter == end";

										}
										*/

										// ------------------------------------------------------------------------------------------------------------------

										goBreak = true; // further processing skip

									} // end if
									else
									{	
										// if [ is found, but ] never found in line, invalid sytax in file
										// throw invalid line exception
										throw harpy::invalid_line(lineNum);
									}
								} // end if
								// !check if string is [section]

								// check if string is VAR

								if (!goBreak) // skip if Section was found and processed
								{	
									bool isVar = false;

									for (const auto& character : raw_string)
									{
										if (character == '=')
										{
											isVar = true;
										}
									} // check if string contains some variable with equation

									// if variable is found, check if syntax is corret for variable name (no spaces in variable name)
									if (isVar)
									{
										std::string::iterator eqFinder = std::find(raw_string.begin(), raw_string.end(), '=');										
										currentVarName = std::string(raw_string.begin(), eqFinder);
										if (raw_string.end() != eqFinder + 1)
										{
											currentVarValue = std::string(eqFinder + 1, raw_string.end());
										} 
										else
										{
											currentVarValue.clear();
										}

										// check if sytax for currentVarName is correct (no spaces)

										for (const auto& character : currentVarName)
										{
											if (character == ' ')
											{
												throw harpy::invalid_variable_name(lineNum, currentVarName);
											}
										} // Exception is thrown when variable name have invalid syntax (spaces in name)

										// continue if name is valid

										// At this point gotta find type of variable value (int, double, string or nothing) and insert into map

										/* SETUP AT THIS POINT

											- currentSectionName holds current working Section name or ""
											- currentVarName holds current found variable name
											- currentVarValue holds string with value after = (plus empty spaces that needs to be cleaned)

											Task

											- check if section exist
											- insert values to the corresponding maps

										*/

										// process the value string, first remove the possible spaces in front

										if (!currentVarValue.empty())
										{ 
											std::string::iterator elemVal = currentVarValue.begin();
											bool changedVal = false;
										

											while (*elemVal == ' ')
											{
												elemVal++;
												changedVal = true;
											}

											if (changedVal)
											{

												std::string tmp = std::string(elemVal, currentVarValue.end());
												currentVarValue.clear();
												currentVarValue = tmp;
											} // empty spaces (if found) are cut

										} 
										else
										{
											// std::cout << "\nDEBUG: currentValValue is empty";
										}

										// Determine the data type held in value

										short dataType = find_data_type(currentVarValue);

										/*
											-1	= std::optional (empty line, no value)
											0	= string
											1	= int
											2	= double
										*/

										switch (dataType)
										{
										case -1:
										{
											// Data type is std::optional (empty) - idea dropped

											std::cout << "\nDEBUG: Type: " << dataType << ", Section: " << currentSectioName << ", Var: " << currentVarName << ", Value: " << currentVarValue;
	
											// harpy::DataBase* data_ptr = std::unique_ptr<DataBase>(new harpy::Data(std::optional<bool> no_val));
											std::string key = currentSectioName + '.' + currentVarName;

											auto anyVal = std::make_any<unsigned short>(0);

											_content[key] = anyVal;

											break;
										}
										case 0:
										{
											// Data type is std::string

											std::cout << "\nDEBUG: Type: " << dataType << ", Section: " << currentSectioName << ", Var: " << currentVarName << ", Value: " << currentVarValue;
																
											// harpy::DataBase * data_ptr = std::unique_ptr<DataBase>(new harpy::Data(currentVarValue));
											std::string key = currentSectioName + '.' + currentVarName;

											auto anyVal = std::make_any<std::string>(currentVarValue);

											_content[key] = anyVal;

											break;
										}
										case 1:
										{
											// Data type is int

											std::cout << "\nDEBUG: Type: " << dataType << ", Section: " << currentSectioName << ", Var: " << currentVarName << ", Value: " << currentVarValue;

											int valueNum = std::atof(currentVarValue.c_str());
											auto anyVal = std::make_any<int>(valueNum);

											// harpy::DataBase* data_ptr = std::unique_ptr<DataBase>(new harpy::Data(valueNum));
											std::string key = currentSectioName + '.' + currentVarName;

											_content[key] = anyVal;

											break;
										}
										case 2:
										{
											// Data type is double											

											std::cout << "\nDEBUG: Type: " << dataType << ", Section: " << currentSectioName << ", Var: " << currentVarName << ", Value: " << currentVarValue;

											double valueNum = std::atof(currentVarValue.c_str());
											auto anyVal = std::make_any<double>(valueNum);

											// harpy::DataBase* data_ptr = std::unique_ptr<DataBase>(new harpy::Data(valueNum));
											std::string key = currentSectioName + '.' + currentVarName;

											_content[key] = anyVal;

											break;
										}

										} // !switch

									} // !isVar			

								} // if (!goBreak)

							} // if(!isEmpty) after comments removal

						} // if (!isAllSpaces) check if only spaces are in line and skip if true						

					} // if (!isEmpty)	
					
					raw_string.clear();
				}
			}
		}

		// destructor

		~parser() {}

	protected:
		// PROTECTED AREA

		/*
		Determine data type of value string
		*/
		short find_data_type(std::string line)
		{
			// VARIABLES

			short dataType = -1;

			// BODY

			if (!line.empty())
			{
				if (isInt(line))
				{
					dataType = 1;
					return dataType;
				}

				if (isDouble(line))
				{
					dataType = 2;
					return dataType;
				}

				dataType = 0;
				return dataType;
			}

			// RETURN

			return dataType;

			/*
			-1	= std::optional (empty line, no value)
			0	= string
			1	= int
			2	= double
			*/
		}

		/*
		check if value string holds int
		*/
		bool isInt(std::string line)
		{

			for (int i = 0; i < line.length(); ++i)
			{
				if (line[i] != ' ')
				{
					if (isdigit((unsigned char)line[i]) == false)
						return false;
				}
				else
				{
					// skip spaces in checks
				}
			}

			return true;
		} // !isInt

		/*
		check if value string holds double
		*/
		bool isDouble(std::string line)
		{

			if (isdigit((unsigned char)line[0]) == false)
			{
				return false; // starting element isn't digit
			}
			else
			{
				for (int i = 0; i < line.length(); ++i)
				{

					if (line[i] != ' ')
					{

						if (isdigit((unsigned char)line[i]) == false)
						{
							if (line[i] != '.')
							{
								return false;
							}
						}
					}
					else
					{
						// skip spaces in checks
					}
				}
			}

			return true;
		} // !isDouble

	private:
		// PRIVATE AREA

		std::ifstream _datastream;
		// std::map< std::string, harpy::Data<std::any>> _content;
		std::map< std::string, std::any> _content;

		/*
		key is section.varname
		value is abstract ptr to data
		*/
	}; // END OF parser
	
} // end of namespace harpy