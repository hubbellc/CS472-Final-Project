/*
	Cache Simulator Implementation by Justin Goins
	Oregon State University
	Fall Term 2018
*/

#ifndef _CACHECONTROLLER_H_
#define _CACHECONTROLLER_H_

#include "CacheStuff.h"
#include <string>
#include <vector>
#include <list>

using namespace std;

class CacheController {
	private:
		struct AddressInfo {
			long tag; // tag initalized to -1, so it must be an unsigned long
			unsigned int setIndex;
			long blockOffset; // represents way
		};
		unsigned int numByteOffsetBits;
		unsigned int numSetIndexBits;
		unsigned int size;
		unsigned int globalCycles;
		unsigned int globalHits;
		unsigned int globalMisses;
		unsigned int globalEvictions;
		std::string inputFile, outputFile;

		ConfigInfo ci;

		vector<vector<long>> cache;
		vector<vector<bool>> validBit;
		vector<list<long>> LRU_keeper; 
		vector<bool> isFull;		

		// function to allow read or write access to the cache
		void cacheAccess(CacheResponse*, bool, unsigned long int);
		// function that can compute the index and tag matching a specific address
		AddressInfo getAddressInfo(unsigned long int);
		// compute the number of clock cycles used to complete a memory access
		void updateCycles(CacheResponse*, bool);

	public:
		CacheController(ConfigInfo, char *);
		void runTracefile();
};

#endif //CACHECONTROLLER
