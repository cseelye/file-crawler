#include <assert.h>
#include <boost/asio/io_service.hpp>
#include <boost/make_shared.hpp>
#include <boost/thread/thread.hpp>
#include <dirent.h>
#include <fstream>
#include <iomanip>
#include <string>
#include <sys/stat.h>
#include <utility>
#include <vector>
#include "ProgramOptions.h"
#include "WordAccumulator.h"
using namespace std;


/**
 * Recursively find and index text files under a given path
 */
class FileIndexer
{
public:

	/**
	 * FileIndexer Constructor
	 * 
	 * @param basePath					The starting path to search
	 * @param fileProcessingThreads		The number of threads to use for processing files
	 * @param timingInfo				Collect info about lock timing
	 */
	FileIndexer(const string& basePath, const int& fileProcessingThreads = 3)
		: mBasePath(basePath),
		  mFileProcessingThreads(fileProcessingThreads),
		  mWordsFound()
	{ }

	/**
	 * Run the search/index
	 */
	void Run()
	{
		boost::thread_group workerThreads;
		{
			
			// Setup thread pool
			boost::asio::io_service::work work(mIOService);
			for (int i = 0; i < mFileProcessingThreads; ++i)
			{
				workerThreads.create_thread(boost::bind(&boost::asio::io_service::run, &mIOService));
			}

			// Use the main thread to run the search, which will post work items to the io_service
			mWordsFound.ClearResults();
			SearchForFiles(mBasePath);
		}
		// Wait for all of the work items to complete
		workerThreads.join_all();

		cout << mWordsFound.GetUniqueWordCount() << " words found" << endl;
	}

	/**
	 * Get a list of the top occurring words
	 * 
	 * @param count	The number of words to return
	 * @return		A vector of WordCountType that is count elements long, sorted from highest occurance to lowest
	 */
	vector<WordCountType> ListTopWords(const int& count) const
	{
		return mWordsFound.ListTopWords(count);
	}


private:
	string mBasePath;
	int mFileProcessingThreads;
	WordAccumulator mWordsFound;
	boost::asio::io_service mIOService;

	// No copying
	FileIndexer(const FileIndexer&);
	FileIndexer& operator=(const FileIndexer& other);

	/**
	 * Parse and count the words in a file
	 * 
	 * @param filename	The full path/name of the file to process
	 */
	void ProcessFile(const string& filename)
	{
		ifstream textFile(filename);
		if (!textFile.good())
		{
			int err = errno;
			string message(strerror(err));
			cout << "Failed to open '" << filename << "': [" << err << "] " << message << endl;
			return;
		}

		size_t buffSize = 2048; // 2k seems like more than enough characters for a single word; assert below catches possible overflow
		char* wordBuffer = new char[buffSize];
		size_t bufIndex = 0;
		string word;
		while(true)
		{
			char ch = textFile.get();
			if (!textFile.good())
				break;

			// Is this an aphanumeric character
			int ascii = (int)ch;
			bool alphanum = false;
			if ( (ascii >= 48 && ascii <= 57) ||	// numeric
				 (ascii >= 97 && ascii <= 122) )	// lowercase
			{
				alphanum = true;
			}
			else if (ascii >= 65 && ascii <= 90)	// uppercase
			{
				alphanum = true;
				ch = (char)(ascii + 32); // lowercase the character
			}

			// If the character is alphanumeric, add it to the buffer
			if (alphanum)
			{
				wordBuffer[bufIndex] = ch;
				bufIndex++;
				assert(bufIndex < buffSize); // Catch possible buffer overflow
			}
			// If it isn't alphanumeric and there is something in the buffer, this is the end of the previous word
			else if (bufIndex > 0)
			{
				wordBuffer[bufIndex] = '\0';
				// Convert to string
				word.assign(wordBuffer);
				mWordsFound.AddWord(word);
				bufIndex = 0;
			}
		}
		if (!textFile.eof())
		{
			int err = errno;
			cout << "Failed reading file '" << filename << "': [" << err << "] " << strerror(err) << endl;
		}
		delete wordBuffer;
		textFile.close();
	}

	/**
	 * Recursively search for text files under a given path, post found files to threadpool for processing
	 * 
	 * @param basePath	The path to search
	 */
	void SearchForFiles(const string& basePath)
	{
		DIR* dir = opendir(basePath.c_str());
		if (dir == NULL)
		{
			cout << strerror(errno) << endl;
			return;
		}

		struct dirent *entry;
		string entryPath;
		while ((entry = readdir(dir)) != NULL)
		{
			if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
				continue;

			// Make an absolute path
			entryPath.assign(basePath + "/" + entry->d_name);

			// Make sure the file exists and is readable
			struct stat entryStat;
			if(lstat(entryPath.c_str(), &entryStat) != 0)
			{
				continue;
			}

			// This implementation ignores symlinks.  It's not clear from the requirements if following symlinks is required,
			// so I am leaving that functionality out so as to not have to deal with all of the ways symlinks can be broken,
			// links to links, circular links, and any other link madness that is possible in Linux.
			// This means that the results of this searcher are the same as the command 'find <basePath> -type f -name "*.txt"'
			if (S_ISLNK(entryStat.st_mode))
			{
				continue;
			}

			if (S_ISREG(entryStat.st_mode))
			{
				// Test if the filename ends in ".txt"
				int len = strlen(entry->d_name);
				if (len >= 4 && strcmp(&entry->d_name[len-4], ".txt") == 0)
				{
					mIOService.post(boost::bind(&FileIndexer::ProcessFile, this, entryPath));
				}
			}
			else if (S_ISDIR(entryStat.st_mode))
			{
				SearchForFiles(entryPath);
			}
		}
		closedir(dir);
	}

};


int main(int argc, char** argv)
{
	// Command line options
	ProgramOptions options;
	try
	{
		options.Parse(argc, argv);
	}
	catch (ProgramOptionsException &e)
	{
		cout << e.what() << endl;
		cout << endl;
		options.DisplayHelp();
		return 1;
	}

	if (options.HelpRequested())
	{
		options.DisplayHelp();
		return 0;
	}

	string searchPath = options.GetOptionValue<string>("path");
	int threadCount = options.GetOptionValue<int>("threads");

	// Check that the specified path exists
	DIR *dir = opendir(searchPath.c_str());
	if (dir == NULL)
	{
		cout << "The specified path does not exist: " << searchPath << endl;
        return 1;
	}
	closedir(dir);

	// Create the indexer and run it
	FileIndexer ssfi(searchPath, threadCount);
	ssfi.Run();

	// Show the top 10 words	
	vector<WordCountType> topWords = ssfi.ListTopWords(10);
	for (const auto& word : topWords)
	{
		cout << word.first << "\t" << word.second << endl;
	}

	return 0;
}
