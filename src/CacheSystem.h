#ifndef CACHESYSTEM_H
#define CACHESYSTEM_H

class CacheSystem
{
	public:
		////////variables//////////
		unsigned long flush_count = 0;

		////////functions/////////
		CacheSystem(); //default constructor
		CacheSystem(int L1_size_kb, int L1_assoc_val, int L2_size_kb, int L2_assoc_val);
		//~CacheSystem(); //destructor?

		int Execute(char inst, unsigned long address, int numbytes);
		int GetL1Cost();
		int GetL2Cost();
		int GetMMCost();

		unsigned long L1D_Hits();
		unsigned long L1D_Misses();
		unsigned long L1D_Kickouts();
		unsigned long L1D_Kickouts_D();
		unsigned long L1D_Kickouts_Flush();
		unsigned long L1D_Transfers();

		unsigned long L1I_Hits();
		unsigned long L1I_Misses();
		unsigned long L1I_Kickouts();
		unsigned long L1I_Kickouts_D();
		unsigned long L1I_Kickouts_Flush();
		unsigned long L1I_Transfers();

		unsigned long L2_Hits();
		unsigned long L2_Misses();
		unsigned long L2_Kickouts();
		unsigned long L2_Kickouts_D();
		unsigned long L2_Kickouts_Flush();
		unsigned long L2_Transfers();

	
	private:
		////////variables//////////
		
		//L1/L2 Bus Parameters
		int L2_transfer_time = 5; //Transfer time L1 to/from L2
		int L2_bus_width = 16;	//Bus width from L1 to L2
		//Main Memory Bus Parameters
		int mem_sendaddr = 10; //Time to send address to memory
		int mem_ready = 30;	//time for the memory to be ready for start of transfer
		int mem_chunktime = 15; //Time to send/recieve a single bus-width of data
		int mem_chunksize = 8;	//Width of the bus interface to memory, in bytes

		BasicCache L1D;
		BasicCache L1I;
		BasicCache L2;

		int instruction_count = 0;

		////////functions/////////
		int Read(unsigned long address, int numbytes);
		int InstRead(unsigned long address, int numbytes);
		int Write(unsigned long address, int numbytes);
		int Clean(); //THIS WILL PROBABLY CHANGE
		int flush();


		CacheSystem() {} //private default constructor
};

class BasicCache
{
public:

	////////variables/////////
	unsigned long hit_count = 0;
	unsigned long miss_count = 0;
	unsigned long kickouts = 0;
	unsigned long kickouts_d = 0;
	unsigned long kickouts_flush = 0;
	unsigned long transfers = 0;

	int hit_time;
	int miss_time;

	long long write_item = -1;
	int write_dirty = 0;

	int block_size = 32;

	////////functions/////////
	BasicCache(); //Basic/default constructor
	BasicCache::BasicCache(int size_kb, int assoc_val, int block_size_val, int hit_time_val, int miss_time_val); //Advanced constructor
	~BasicCache(); //destructor

	int Read(unsigned long address, int numbytes); //Read the data according to the instruction, return the time it took.
	int Write(unsigned long address, int numbytes, int isDirty); //Write the data/instruction, same thing
	int Evict(BasicCache& input_cache, int real_evict);

	int getCacheSize();
	int getAssoc();

private:

	////////variables/////////
	int cache_size = 8192;
	int assoc = 1;	//associativity
	int block_count;


	//Tag and status bits
	unsigned long * tag_array; //32 bits!
	int * valid_array;
	int * dirty_array;

	//Bits used to denote how much of the address we use for what
	int tag_bits;
	int index_bits;
	int offset_bits;


	long long * LRU_array;


	////////functions/////////
	void UpdateLRU(int index);
	int GetAgeLRU(int index);

	//Get the number of bits for tag, index, offset.
	int GetTagBits();
	int GetIndexBits();
	int GetOffsetBits();

	BasicCache() {}//private default constructor


};

#endif