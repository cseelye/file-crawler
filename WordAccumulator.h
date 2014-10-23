#ifndef WORDACCUMULATOR_H
#define WORDACCUMULATOR_H

#include <boost/thread/mutex.hpp>
#include <string>
#include <utility>
#include <vector>

using WordCountType = std::pair<std::string, int>;

/**
 * The WordAccumulator class is a thread-safe, specialized container for counting the occurance of unique words
 */
class WordAccumulator
{
public:

	/**
	 * WordAccumulator constructor
	 */
	WordAccumulator()
		: mBins(BIN_COUNT),
		  mBinMutexes(BIN_COUNT)
	{ }

	/**
	 * Add a word.  If the word already exists, its count will be incremented
	 * 
	 * @param word	The word to add
	 */
	void AddWord(const std::string &word)
	{
		// Figure out which bin the word goes in and lock it
		size_t binIndex = mHasher(word) % mBins.size();
		boost::mutex::scoped_lock lock(mBinMutexes[binIndex]);

		// See if the word is already present in the bin and increment it if so
		// I'm not terribly fond of this linear search, but I tested other approaches (like using a map for each bin instead of a vector for each bin)
		// and this was the fastest, probably because the large number of bins causes a low number of unique words per bin
		for (auto &wordPair : mBins[binIndex])
		{
			if (wordPair.first == word)
			{
				wordPair.second++;
				return;
			}
		}

		// Add the word if it was not found
		mBins[binIndex].emplace_back(word, 1);
	}

	/**
	 * Remove all words
	 */
	void ClearResults()
	{
		// To atomically clear we need to lock all of the bins, clear each bin, then unlock them all

		// Lock every bin
		for (auto &mutex : mBinMutexes)
			mutex.lock();

		// Clear each bin
		for(auto &bin : mBins)
		{
			bin.clear();
		}

		// Unlock every bin
		for (auto &mutex : mBinMutexes)
			mutex.unlock();
	}

	/**
	 * Get a list of the top occurring words
	 * 
	 * @param count	The number of words to return
	 * @return		A vector of WordCountType that is count elements long, sorted from highest occurance to lowest
	 */
	std::vector<WordCountType> ListTopWords(const int &count) const
	{
		// Lock every bin
		for (auto &mutex : mBinMutexes)
			mutex.lock();

		// Count total the number of words and preallocate
		size_t totalWords = 0;
		for(const auto &bin : mBins)
		{
			totalWords += bin.size();
		}
		std::vector<WordCountType> allWords;
		allWords.reserve(totalWords);

		// Make a list of all the words from all the bins
		for(const auto &bin : mBins)
		{
			if (bin.size() <= 0)
				continue;
			allWords.insert(allWords.end(), bin.begin(), bin.end());
		}

		// Unlock every bin
		for (auto &mutex : mBinMutexes)
			mutex.unlock();

		if (allWords.size() <= 0)
			return allWords;

		// Sort by occurance and return the requested slice
		sort(allWords.begin(),
			 allWords.end(),
			 [](const std::pair<std::string, int>& a, const std::pair<std::string, int>& b)
			 {
				 return a.second > b.second;
			 });
		return std::vector<WordCountType>(allWords.begin(), allWords.begin() + count);
	}

	/**
	 * Get a count of the number of words currently in this WordAccumulator
	 * 
	 * @return	The number of words
	 */
	size_t GetUniqueWordCount() const
	{
		// Lock every bin
		for (auto &mutex : mBinMutexes)
			mutex.lock();

		// Count the words in each bin
		size_t totalWords = 0;
		for(const auto &bin : mBins)
		{
			totalWords += bin.size();
		}

		// Unlock every bin
		for (auto &mutex : mBinMutexes)
			mutex.unlock();

		return totalWords;
	}


private:
	static const size_t BIN_COUNT = 32767;  // Large number of bins to minimize lock contention and to keep the number of words per bin low
	std::vector<std::vector<WordCountType> > mBins;
	mutable std::vector<boost::mutex> mBinMutexes;
	std::hash<std::string> mHasher;

	// No copying
	WordAccumulator(const WordAccumulator&);
	WordAccumulator& operator=(const WordAccumulator& other);

};

#endif // WORDACCUMULATOR_H
