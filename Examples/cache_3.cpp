// Albert Coba and Shawn Nematbakhsh
// cs161 as3 - cache similator
//
// These are the function definitions for the cache block class.

#include "block.h"


// this is the constructor. It takes the size of the cache and the
// replacement policy(0=LRU,1=FIFO) and initializes the values.
block::block(int s,bool replacement)
{	
	// set size and replacement method
	size=s;
	replacement_method=replacement;

	// array for valid bits
	valid=new bool[size];
	
	// array for tags
	tag=new int[size];
	
	// array to keep track of LRU/FIFO
	lru=new int[size];
	
	// set arrays to zero initially
	reset();
}

// empty constructor, init has to be called if this is used. 
block::block()
{

}

// this is the destructor, it frees up the memory dynamically allocated
// in the arrays.
block::~block()
{
	// quit if arrays haven't been allocated
	if(!valid)
		return;

	// otherwise delete them
	delete valid;
	delete tag;
	delete lru;
}

// this function is the same as the first constructor. It exists so that
// arrays of blocks can be declared with different size/replacements.
void block::init(int s,bool replacement)
{
	// set size/replacement
	size=s;
	replacement_method=replacement;

	// set/initialize arrays
	valid=new bool[size];
	tag=new int[size];
	lru=new int[size];
	reset();
}

// this function sets all arrays used in the class to zero and sets
// the counts to zero
void block::reset()
{
	// set all data in the arrays to zero
	for(int i=0;i<size;++i)
	{
		valid[i]=0;
		lru[i]=0;
		tag[i]=0;
	}

	// reset counters
	misses=0;
	total=0;
}

// this function takes an address and the "way" of the cache(ie 2 for
// 2 way) and simulates a cache access, incrementing "misses" if there
// is a miss.
void block::access(unsigned int addr,int way)
{
	// increment total access count
	++total;
	
	// calculate index to place data
	int index=(int)(addr/16)%(size/way) * way;
	
	// calculate tag
	int target_tag=addr/size*16;
	
	// if the cache is hit then find the byte offset from the index
	int target_index=find_tag(index,way,target_tag);
	
	// if there's a cache hit
	if(valid[index] && target_index)
	{
		// if LRU replacement is chosen
		if(replacement_method==0)
		{
			// if the byte being accessed isn't already the most
			// recently used
			if(lru[index+target_index-1]!=way-1)
			{
				// decrement all values in the LRU array with values
				// greater than the value of the hit byte
				decrement_all(index,way,lru[index+target_index-1]);
				
				// set the byte to be the most recently used
				lru[index+target_index-1]=way-1;
			}
		}
	}
	// otherwise it's a miss
	else
	{
		// increment miss count
		++misses;

		// set to valid
		valid[index]=1;

		// set the tag and update the replacement array
		tag[index+fifo_offset(index,way)]=target_tag;
	}
}

// thos function takes a base index inside the cache and the "way" of
// the cache and the target tag to find and returns the offset from
// the index+1 of the tag data if it's found
int block::find_tag(int index,int way,int target)
{
	// for all data in the cache line
	for(int i=0;i<way;++i)
		// if tag is found
		if(tag[index+i]==target)
			// return it's offset from index
			return 1+i;
	
	// otherwise return false, it's not found	
	return 0;
}

// this function returns the miss rate of the cache
double block::rate()
{
	// misses/total = miss rate
	return (double)misses/(double)total*100;
}

// this function takes a base index within the cache and the "way"
// associativity and returns the offset from the index of the
// value to replace. The replacement array "LRU" is also updated 
int block::fifo_offset(int base,int way)
{
	// for all entries in the cache line
	for(int i=0;i<way;++i)
	{
		// if we run into a byte with LRU=0
		if(lru[i+base]==0)
		{
			// then mark that byte for replacement
			// set that byte as the most recently used
			lru[i+base]=way;

			// then decrement all values to reflect the add
			decrement_all(base,way,0);

			// return offset from index of byte to replace
			return i;				
		}
	}

	return 0;
}

// this function takes a base value within the cache and the "way"
// associativity and a value and decrements all values in the
// replacement array that are greater than the value
void block::decrement_all(int base,int way,int value)
{
	// for each byte in the subcache
	for(int j=0;j<way;++j)
		// decrement replacement value if > passed value
		if(lru[j+base] > value)
			--lru[j+base];
}
