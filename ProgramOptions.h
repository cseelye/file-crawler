#ifndef PROGRAMOPTIONS_H
#define PROGRAMOPTIONS_H

#include <boost/program_options.hpp>
#include <iostream>
#include <string>
#include <sstream>

/**
 * Exception thrown when there is a parsing or validation error in the ProgramOptions class
 */
class ProgramOptionsException : public std::runtime_error
{
public:
	ProgramOptionsException(const std::string &message)
		: runtime_error(message)
	{ }
};

/**
 * Class to handle displaying, parsing and basic validation of command line options to the program
 */
class ProgramOptions
{
public:

	/**
	 * ProgramOptions constructor
	 */
	ProgramOptions()
	{
		// Setup args to be parsed
		boost::program_options::options_description generalOptions("Options");
	    generalOptions.add_options()
	        ("help,h", 
	                "show this help message")
	        ("threads,t", 
	                boost::program_options::value<int>()->default_value(3)->required(), 
	                "the number of file processor threads to use")
	        ;

		boost::program_options::options_description hiddenOptions;
		hiddenOptions.add_options()
			("path,p",
					boost::program_options::value<std::string>(),
					"the path to search")
		;

		mPositionalOptions.add("path", 1);

		mAllOptions.add(generalOptions).add(hiddenOptions);

		// Create the help message
		std::stringstream buffer;
		buffer << "Usage: ssfi PATH [options]" << std::endl;
		buffer << "Index all text files in PATH" << std::endl;
		buffer << std::endl;
		buffer << generalOptions << std::endl;
		mHelpMessage = buffer.str();
	}

	/**
	 * Print the command line help to stdout
	 */
	void DisplayHelp() const
	{
		std::cout << mHelpMessage;
	}

	/**
	 * Parse the command line and check for errors
	 * @param argc
	 * @param argv
	 */
	void Parse(int argc, char** argv)
	{
		try
		{
			boost::program_options::store(
						boost::program_options::command_line_parser(argc, argv)
							.positional(mPositionalOptions)
							.options(mAllOptions)
							.run(), 
						mVarMap);
			boost::program_options::notify(mVarMap);
		}
		catch (boost::program_options::error &e)
		{
			// Ignore errors if help option is set
			if (mVarMap.count("help"))
				return;
			throw ProgramOptionsException(e.what());
		}

		// No further validation if help option is set
		if (mVarMap.count("help"))
			return;

		if (mVarMap.count("path") <= 0)
		{
			throw ProgramOptionsException("You must specify a PATH to index");
		}
		
		if (mVarMap["threads"].as<int>() <= 0)
			throw ProgramOptionsException("option 'threads' must be a positive integer");
	}

	/**
	 * Check if the help option was specified
	 * @return	True if the help option was requested, false otherwise
	 */
	bool HelpRequested() const
	{
		return (mVarMap.count("help") > 0);
	}

	/**
	 * Return the value of an option, or throw an exception if the option is not present
	 * @param optionName	The name of the option to retrieve
	 * @return				The value of the option
	 */
	template<typename T>
	T GetOptionValue(const std::string &optionName) const
	{
		if (mVarMap.count(optionName))
			return mVarMap[optionName].as<T>();
		else
			throw ProgramOptionsException("Option '" + optionName + "' not found");
	}


private:
	std::string mHelpMessage;
	boost::program_options::positional_options_description mPositionalOptions;
	boost::program_options::options_description mAllOptions;
	boost::program_options::variables_map mVarMap;

	// No copying
	ProgramOptions(const ProgramOptions&);
	ProgramOptions& operator=(const ProgramOptions& other);
	
};


#endif // PROGRAMOPTIONS_H
