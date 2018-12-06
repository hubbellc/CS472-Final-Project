/*
	Cache Simulator Implementation by Justin Goins
	Oregon State University
	Fall Term 2018
	
	Modified by Caleb Hubbell
	
	Sources referenced:
	https://www.geeksforgeeks.org/lru-cache-implementation/
	http://www.cs.cornell.edu/courses/cs3410/2013sp/lecture/18-caches3-w.pdf
	https://en.wikipedia.org/wiki/CPU_cache
*/

#include "CacheController.h"
#include <iostream>
#include <fstream>
#include <regex>
#include <cmath>
#include <vector>

using namespace std;

CacheController::CacheController(ConfigInfo ci, char* tracefile) {
	// store the configuration info
	this->ci = ci;
	this->inputFile = string(tracefile);
	this->outputFile = this->inputFile + ".out";
	// compute the other cache parameters
	this->numByteOffsetBits = log2(ci.blockSize);
	this->numSetIndexBits = log2(ci.numberSets);
	this->size = ci.blockSize;
	// initialize the counters
	this->globalCycles = 0;
	this->globalHits = 0;
	this->globalMisses = 0;
	this->globalEvictions = 0;
	
	validBit.resize(ci.numberSets); //TODO, using provided number of sets, verify, maybe move below, to constructor??

	for (unsigned long i = 0; i < (unsigned long)ci.numberSets; i++) 
	{ //TODO, not in simulator
		validBit[i].resize(ci.associativity);
		fill(validBit[i].begin(), validBit[i].end(), NULL);
	}

	cache.resize(ci.numberSets);
	for (unsigned long i = 0; i < (unsigned long)ci.numberSets; i++) 
	{
		cache[i].resize(ci.associativity);
		fill(cache[i].begin(), cache[i].end(), -1);
	}
	//TODO, is this the best way to solve the "tag is 0" issue?
	
	// to keep track of where we are
	LRU_keeper.resize(ci.numberSets);
	
	// to keep track of if the way if full
	isFull.resize(ci.numberSets);
	fill(isFull.begin(), isFull.end(), 0);
	
	// make a cache
	cout << endl << "Initial cache: " << endl;
	cout << "Sets: " << ci.numberSets << " Rows: " << ci.associativity << endl << endl;
	/*for (unsigned long i = 0; i < ci.numberSets; i++) 
	{
		cout << "idx: " << i << "|"; 
		for(unsigned long j=0; j < ci.associativity; j++)
		{
			cout << "V: " << validBit[i][j] << "| Tag: " << cache[i][j] << "|";
		}
		//TODO, concerning...
		
		cout << endl; 
	}*/
	
	// seed rand
	srand (time(NULL));

	// manual test code to see if the cache is behaving properly
	// will need to be changed slightly to match the function prototype
	/*
	cacheAccess(false, 0);
	cacheAccess(false, 128);
	cacheAccess(false, 256);

	cacheAccess(false, 0);
	cacheAccess(false, 128);
	cacheAccess(false, 256);
	*/
}

/*
	Starts reading the tracefile and processing memory operations.
*/
void CacheController::runTracefile() {
	cout << "Input tracefile: " << inputFile << endl;
	cout << "Output file name: " << outputFile << endl;
	
	// process each input line
	string line;
	// define regular expressions that are used to locate commands
	regex commentPattern("==.*");
	regex instructionPattern("I .*");
	regex loadPattern(" (L )(.*)(,[[:digit:]]+)$");
	regex storePattern(" (S )(.*)(,[[:digit:]]+)$");
	regex modifyPattern(" (M )(.*)(,[[:digit:]]+)$");

	// open the output file
	ofstream outfile(outputFile);
	// open the output file
	ifstream infile(inputFile);
	// parse each line of the file and look for commands
	while (getline(infile, line)) {
		// these strings will be used in the file output
		string opString, activityString;
		smatch match; // will eventually hold the hexadecimal address string
		unsigned long int address;
		// create a struct to track cache responses
		CacheResponse response;

		// ignore comments
		if (std::regex_match(line, commentPattern) || std::regex_match(line, instructionPattern)) {
			// skip over comments and CPU instructions
			continue;
		} else if (std::regex_match(line, match, loadPattern)) {
			cout << "Found a load op!" << endl;
			istringstream hexStream(match.str(2));
			hexStream >> std::hex >> address;
			outfile << match.str(1) << match.str(2) << match.str(3);
			cacheAccess(&response, false, address);
			outfile << " " << response.cycles << (response.hit ? " hit" : " miss") << (response.eviction ? " eviction" : "");
		} else if (std::regex_match(line, match, storePattern)) {
			cout << "Found a store op!" << endl;
			istringstream hexStream(match.str(2));
			hexStream >> std::hex >> address;
			outfile << match.str(1) << match.str(2) << match.str(3);
			cacheAccess(&response, true, address);
			outfile << " " << response.cycles << (response.hit ? " hit" : " miss") << (response.eviction ? " eviction" : "");
		} else if (std::regex_match(line, match, modifyPattern)) {
			cout << "Found a modify op!" << endl;
			istringstream hexStream(match.str(2));
			hexStream >> std::hex >> address;
			outfile << match.str(1) << match.str(2) << match.str(3);
			// first process the read operation
			cacheAccess(&response, false, address);
			string tmpString; // will be used during the file output
			tmpString.append(response.hit ? " hit" : " miss");
			tmpString.append(response.eviction ? " eviction" : "");
			unsigned long int totalCycles = response.cycles; // track the number of cycles used for both stages of the modify operation
			// now process the write operation
			cacheAccess(&response, true, address);
			tmpString.append(response.hit ? " hit" : " miss");
			tmpString.append(response.eviction ? " eviction" : "");
			totalCycles += response.cycles;
			outfile << " " << totalCycles << tmpString;
		} else {
			throw runtime_error("Encountered unknown line format in tracefile.");
		}
		outfile << endl;
	}
	// add the final cache statistics
	outfile << "Hits: " << globalHits << " Misses: " << globalMisses << " Evictions: " << globalEvictions << endl;
	outfile << "Cycles: " << globalCycles << endl;
	
	// make a cache
	/*cout << endl << "Final cache: " << endl;
	cout << "Sets: " << ci.numberSets << " Rows: " << ci.associativity << endl << endl;
	for (unsigned long i = 0; i < ci.numberSets; i++) 
	{
		cout << "idx: " << i << "|"; 
		for(unsigned long j=0; j < ci.associativity; j++)
		{
			cout << "V: " << validBit[i][j] << "| Tag: " << cache[i][j] << "|";
		}
		cout << endl; 
	}*/

	infile.close();
	outfile.close();
}

/*
	Calculate the block index and tag for a specified address.
*/
CacheController::AddressInfo CacheController::getAddressInfo(unsigned long int address) {
	// address is the byte address!
	AddressInfo ai;

	// get the tag
	// (left most bits of addr after byte offset and idx used)
	int shift = numByteOffsetBits + numSetIndexBits;
	//cout << "shift " << shift << endl;
	int cur_tag = address >> shift;
	//cout << "tag " << cur_tag << endl;
	ai.tag = cur_tag;

	// get the index
	// (block addr % num of indicies)
	int cur_addr = floor(address / size);
	//cout << "addr " << cur_addr << endl;
	int cur_idx = cur_addr % size;
	//cout << "idx " << cur_idx << endl;
	ai.setIndex = cur_idx;
	return ai;
}

/*
	This function allows us to read or write to the cache.
	The read or write is indicated by isWrite.
*/
void CacheController::cacheAccess(CacheResponse* response, bool isWrite, unsigned long int address) {
	
	// determine the index and tag
	AddressInfo ai = getAddressInfo(address);

	cout << "\tSet index: " << ai.setIndex << ", tag: " << ai.tag << endl;

	// need to hit our stored cache and look for a match, if a match, decide what to do. If not, add this element
	// also, update the global counters that track the number of hits, misses, and evictions
	// this is why we need a cache structure (vector?)

	// look! accessing a cache structure!
	// see if our tag bit matches one in the cache by iterating through each option looking for a match
	/*for(unsigned long i=0; i < ci.associativity; i++)
	{
		cout << "\t\tSet index: " << ai.setIndex << ", way: " << i << ", tag: " << cache[ai.setIndex][i] << endl;
		if(cache[ai.setIndex][i] == ai.tag && validBit[ai.setIndex][i]==1)
		{
			response->hit = true;
			ai.blockOffset = i;
			break;
		}
		else
			response->hit = false;
	}*/
	unsigned long i=0;
	response->hit = false;
	// use a while loop to only go while our way exists
	while((cache[ai.setIndex][i] != -1) && i < ci.associativity)
	{
		cout << "\t\tSet index: " << ai.setIndex << ", way: " << i << ", tag: " << cache[ai.setIndex][i] << endl;
		if(cache[ai.setIndex][i] == ai.tag && validBit[ai.setIndex][i]==1)
		{
			response->hit = true;
			break;
		}
		i++;
	}
	
	ai.blockOffset = i - 1;
	
	// init
	response->eviction = false;

	if(!response->hit) // miss
	{
		// need to write our friend into the cache, replacing something?
		
		// not full yet? just add that baby in there
		if(!isFull[ai.setIndex])
		{
			cache[ai.setIndex][i] = ai.tag;
			validBit[ai.setIndex][i] = 1;
			
			cout << "not full" << endl;
		}		
		// do random replacement if our way is full
		else if(ci.rp == ReplacementPolicy::Random)
		{
			// get a random number
			long randNum = rand() % ci.associativity;

			// replace that way
			cache[ai.setIndex][randNum] = ai.tag;
			validBit[ai.setIndex][randNum] = 1;
			response->eviction = true;
		}
		// do LRU replacement if our cache way is full
		else
		{
			cout << "in LRU replace" << " LRU: " << LRU_keeper[ai.setIndex].back() << endl;
			
			// replace the LRU way
			cache[ai.setIndex][LRU_keeper[ai.setIndex].back()] = ai.tag;
			validBit[ai.setIndex][LRU_keeper[ai.setIndex].back()] = 1;
			response->eviction = true;
		}
		
		// block has been evicted, see what to do
		if(isWrite && ci.wp == WritePolicy::WriteThrough)
		{
			// always writes back to main memory!!
			cout << "write through" << endl;
			//TODO, counter??
		}
		else
		{
			// writes when replaced, extra credit
			//cout << "write back not implemented" << endl;
		}
		
		// stats for nerds
		cout << "\t On: " << i << " full: " << isFull[ai.setIndex] << " size: " << LRU_keeper[ai.setIndex].size() << endl;
	}
	
	else // hit
	{
		// if we hit on a write, nothing happens, but see if we need to write to main mem
		if(ci.wp == WritePolicy::WriteThrough)
		{
			// always writes back to main memory!!
			cout << "write through" << endl;
		}
		else
		{
			// writes when replaced, extra credit
			cout << "write back not implemented" << endl;
		}
		
		// if we hit on a read, nothing happens. We found it!
		
		// detach from where it is, so we can add it back to the beginning
		// TODO, right? seems like we would just want to pop the back again
		LRU_keeper[ai.setIndex].remove(ai.blockOffset);
	}
	
	// always push the most recently used to the LRU_keeper
	cout << "Pushing: " << ai.blockOffset << endl;
	LRU_keeper[ai.setIndex].push_front(ai.blockOffset);
	
	// once we are full, keep track of the LRU
	if(LRU_keeper[ai.setIndex].size() == ci.associativity) // -1 because we check before it is actually added
	{ 
		cout << "full" << endl;
		isFull[ai.setIndex] = true;
		
		//delete the least recently used element 
		LRU_keeper[ai.setIndex].pop_back(); 
	}
	else
		cout << "not full" << endl;
	
	updateCycles(response, isWrite);

	if (response->hit)
	{
		globalHits++;
		cout << "Address " << address << " was a hit." << endl;
	}
	else
	{
		globalMisses++;
		cout << "Address " << address << " was a miss." << endl;
	}
	if (response->eviction)
	{
		globalEvictions++;
		cout << "Address " << address << " was evicted and replaced." << endl;
		// if want hex use this: cout << "Address " << std::hex << address << " was evicted and replaced." << endl;
	}
	

	cout << "-----------------------------------------" << endl;

	return;
}

/*
	Compute the number of cycles used by a particular memory operation.
	This will depend on the cache write policy.
*/
void CacheController::updateCycles(CacheResponse* response, bool isWrite) {
	// your code should calculate the proper number of cycles
	// init!
	response->cycles = 0;
	
	if(response->hit) // just the cache access
	{
		response->cycles += ci.cacheAccessCycles;
		
		//writing back?
		if(isWrite && ci.wp == WritePolicy::WriteThrough)
			response->cycles += ci.memoryAccessCycles;
	}
	else // miss... has to read from mem
	{
		response->cycles += (ci.cacheAccessCycles + ci.memoryAccessCycles);
		// no evection penalty for write-through
		
		// for extra cred
		/*if(response->eviction)
		{
			if(ci.wp == WritePolicy::WriteThrough){} // no evection penalty for write-through
			else // write back
				globalCycles += ci.memoryAccessCycles;
		}*/
	}
	
	// keep track of global cycles too
	globalCycles += response->cycles;
}