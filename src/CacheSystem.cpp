#include <iostream>
#include <math.h>

#include "CacheSystem.h"

using namespace std;

//Our default constructor
CacheSystem::CacheSystem()
{
	CacheSystem(8192, 1, 32768, 1);
}
CacheSystem::CacheSystem(int L1_size, int L1_assoc_val, int L2_size, int L2_assoc_val)
{
	//Not sure how to tackle this ATM, but we can set some values here anyway for later
	
	//This should be all we need- at least, in my current sleep-deprived state it seems
	//The BasicCache constructors will create the empty arrays & stuff we'll use later

	//Create the cache objects we'll be manipulating
	//BasicCache L1C(L1_size, L1_assoc_val, 32, 1, 1);
	//BasicCache L1D(L1_size, L1_assoc_val, 32, 1, 1);
	//BasicCache L2(L2_size, L2_assoc_val, 64, 5, 7);

	L1D = BasicCache(L1_size, L1_assoc_val, 32, 1, 1);
	L1I = BasicCache(L1_size, L1_assoc_val, 32, 1, 1);
	L2 = BasicCache(L2_size, L2_assoc_val, 64, 5, 7);
}

int CacheSystem::Execute(char inst, unsigned long long address, int numbytes)
{
	int exec_time = 0;
	/////////////////////////////////
	//What kind of instruction is this?
	if (inst == 'R') //Data Read
	{
		exec_time = Read(address, numbytes);
	}
	if (inst == 'I') //Instruction read
	{
		exec_time = InstRead(address, numbytes); //could probably use normal read(), but ehhhhhh
	}
	if (inst == 'W') //Write
	{
		exec_time = Write(address, numbytes);
	}

	//Clean the caches/check the caches for items that need to be evicted to the next level.
	exec_time = exec_time + Clean(); //later

	instruction_count = instruction_count + 1;
	if ((instruction_count % 380000) == 0)
	{
		exec_time = exec_time + flush();
	}
	return exec_time;
}

int CacheSystem::Read(unsigned long long address, int numbytes)
{
	int status = 0;
	//Step one: check the L1D cache
	status = L1D.Read(address, numbytes);
	L1D.transfers = L1D.transfers + 1; //cheating a little here- no matter what happens, L1D is going to transfer SOMETHING to the processor.
	if (status > 0) return status; //We found it! Hooray!
	else //We need to check the L2 cache
	{
		status = L2.Read(address, numbytes);
		if (status > 0)
		{
			//We found it!
			//Write it to the L1 cache, then return the time it took.
			int write_status = L1D.Write(address, numbytes, 0);
			L2.transfers = L2.transfers + 1;
			return (status + write_status + L1D.miss_time);
		}
		else
		{
			//Great. It's not in L1D OR L2. Time to "check" "main memory".
			//Write it to the L1 cache, return the time it took.
			int write_status = L1D.Write(address, numbytes, 0);
			int mem_time = mem_sendaddr + mem_ready + (mem_chunktime * (L2.block_size) / mem_chunksize);
			return (write_status + L1D.miss_time + L2.miss_time + mem_time);
		}
	}

}

int CacheSystem::InstRead(unsigned long long address, int numbytes)
{
	//Same as Read(), but we use the L1I cache instead
	int status = 0;
	//Step one: check the L1D cache
	status = L1I.Read(address, numbytes);
	L1I.transfers = L1I.transfers + 1; //cheating a little here- no matter what happens, L1I is going to transfer SOMETHING to the processor.
	if (status > 0) return status; //We found it! Hooray!
	else //We need to check the L2 cache
	{
		status = L2.Read(address, numbytes);
		if (status > 0)
		{
			//We found it!
			//Write it to the L1 cache, then return the time it took.
			int write_status = L1I.Write(address, numbytes, 0);
			return (status + write_status + L1I.miss_time);
		}
		else
		{
			//Great. It's not in L1D OR L2. Time to "check" "main memory".
			//Write it to the L1 cache, return the time it took.
			int write_status = L1I.Write(address, numbytes, 0);
			int mem_time = mem_sendaddr + mem_ready + (mem_chunktime * 64 / mem_chunksize);
			return (write_status + L1I.miss_time + L2.miss_time + mem_time);
		}
	}
}

int CacheSystem::Write(unsigned long long address, int numbytes)
{
	//Write to the L1 cache. IMMEDIATELY.
	int status = L1D.Write(address, numbytes, 0);
	//That's it! We'll push stuff to the next cache level later if we need to- that's the purpose of "Clean".
	return status;
	
}

int CacheSystem::Clean()
{
	//This is pretty simple. This will run at the end of every instruction "cycle"
	//and move evicted elements to the next cache level.

	//Since transferring stuff takes time, this will return the amount of time
	//taken to move everything.
	
	int time = 0;
	//Since the L2 stuff doesn't actually go anywhere, we just need to run this on L1I and L1D
	//We still need to check if something needs to be transferred, though.
	//L2
	if (L2.write_item != -1)
	{
		L2.write_item = -1;
		L2.transfers = L2.transfers + 1;

		int mem_time = mem_sendaddr + mem_ready + (mem_chunktime * (L2.block_size) / mem_chunksize);
		time = time + mem_time;
	}

	//L1
	if (L1I.write_item != -1)
	{
		int numbytes = L1I.block_size;
		L2.Write(L1I.write_item, numbytes, L1I.write_dirty);
		L1I.write_item = -1;
		L1I.transfers = L1I.transfers + 1;
		int cache_time = L2.hit_time * ((L2.block_size) / L2_bus_width);
		time = time + cache_time;
	}
	if (L1D.write_item != -1)
	{
		int numbytes = L1D.block_size;
		L2.Write(L1D.write_item, numbytes, L1D.write_dirty);
		L1D.write_item = -1;
		L1D.transfers = L1D.transfers + 1;

		int cache_time = L2.hit_time * ((L2.block_size) / L2_bus_width);
		time = time + cache_time;
	}

	return time;
}

int CacheSystem::flush()
{
	//Idea: Call "Evict()" on the L1D and L1I caches with a pointer towards L2. 
	//This command will evict dirty bits from L1D and L1I and do a bunch of writes
	//to L2 to move them over.

	//We'll run this on L2 first to "move" the bits to "main memory". With L2 freshly invalidated, we can move
	//up the stuff from the L1 caches easy.

	int L2_mem_count = L2.Evict(L2, 0); //(we're not going to actually write stuff so it's OK to use L2 as the first arg).
	int L1D_L2_count = L1D.Evict(L2, 1);
	int L1I_L2_count = L1I.Evict(L2, 1);


	int L2_mem_time = mem_sendaddr + mem_ready + (mem_chunktime * (L2_mem_count*L2.block_size) / mem_chunksize);
	int L1D_L2_time = L2.hit_time * (L1D_L2_count * (L1D.block_size)) / L2_bus_width;
	int L1I_L2_time = L2.hit_time * (L1I_L2_count * (L1I.block_size)) / L2_bus_width;

	int time = L2_mem_count + L1D_L2_time + L1I_L2_time;


	flush_count = flush_count + 1;
	return time; //I don't think we need to return anything here, but I'll leave this in case I change my mind.
}

int CacheSystem::GetL1Cost()
{
	//$100 for each 4kb, and an additional $100 for each
	//doubling in associativity beyond 1/direct
	//Ex: 4kb DM = $100, 8kb DM = $200, 4kb 2x = $200, 8kb 2x = $400
	int L1_assoc = L1D.getAssoc();
	int L1_cache_size = L1D.getCacheSize();

	int assoc_mult = int(log(L1_assoc)/log(2)) + 1; //Returns log_2(L1_assoc)
	int size_mult = L1_cache_size / 4096;

	int total = (100 * assoc_mult)*size_mult;

	return total;
}

int CacheSystem::GetL2Cost()
{
	//L2 costs $50 per 64 KB of DM, with an additional $50 for each
	//doubling in associativity. 
	int L2_assoc = L2.getAssoc();
	int L2_cache_size = L2.getCacheSize();

	int assoc_mult = int(log(L2_assoc) / log(2)) + 1; //Returns log_2(L1_assoc)
	int size_mult = L2_cache_size / (64 * 1024);
	if (size_mult == 0) //less than 64 kb
	{
		size_mult = 1;
	}

	int total = (50 * assoc_mult)*size_mult;

	return total;
}

int CacheSystem::GetMMCost()
{

	//Bandwidth cost
	int bandwidth_mult = 0;
	if (mem_chunksize > 16)
	{ 
		bandwidth_mult = int(log(mem_chunksize) / log(2)) - 3;
	}
	int bandwidth_cost = 25 + 100 * bandwidth_mult;

	//Latency cost
	int latency_mult = 0;
	if (mem_ready < 50)
	{
		latency_mult = 50 / mem_ready;
		latency_mult = int(log(latency_mult) / log(2)) + 1;
	}
	int latency_cost = 50 + latency_mult * 200;

	int total = latency_cost + bandwidth_cost;
	return total;
}


//////////////////////////////////
//BasicCache
//////////////////////////////////

BasicCache::BasicCache() //default constructor
{
	BasicCache(8192, 1, 32, 1 , 1);
}

BasicCache::BasicCache(int size_val, int assoc_val, int block_size_val, int hit_time_val, int miss_time_val) //Advanced constructor
{
	cache_size = size_val;
	assoc = assoc_val;
	block_size = block_size_val;
	hit_time = hit_time_val;
	miss_time = miss_time_val;

	block_count = cache_size / (block_size); //Bytes / (bytes/block) = number of blocks we need

	tag_array = new unsigned long[block_count];
	valid_array = new int[block_count];
	dirty_array = new int[block_count];

	LRU_array = new long long[block_count];

	//Zero out the arrays so we don't have garbage messing with the sim.
	for (int i = 0; i < block_count; i++)
	{
		tag_array[i] = 0;
		valid_array[i] = 0;
		dirty_array[i] = 0;
		LRU_array[i] = -1; //0 may very well be a valid address! Maybe. I don't know.
	}

	//Update the number of bits we need from the address for tag, index, offset.
	offset_bits = GetOffsetBits();
	index_bits = GetIndexBits();
	tag_bits = GetTagBits();

}

int BasicCache::GetTagBits()
{
	//Computes the number of index bits in the address
	//Number of tag bits = 48 - index bits - offset bits
	//We're cheating here a little bit and assuming GetIndexBits()
	//and GetOffsetBits() have already been computed.

	int n = 48 - index_bits - offset_bits;
	return n;
}
int BasicCache::GetIndexBits()
{
	//Computes number of index bits in the address
	//2^n = number of blocks, where n is the number of bits we need so
	//n = log_2(block_count).

	int n = int(log(block_count) / log(2));

	//For X-way set-associative, we can divide this by X since each item
	//can go into several locations
	if (assoc != 0)
	{
		n = n / assoc;
	}

	return n;
}
int BasicCache::GetOffsetBits()
{
	//Computes the number of byte offest bits in the address
	//2^n = number of bytes in a block, where n is the number of bits we need so
	//n = log_2(block_size/8)

	int numbytes = block_size;
	int n = int(log(numbytes) / log(2));
	return n;
}

int BasicCache::Read(unsigned long long address, int numbytes)
{
	unsigned long tag = (address >> (index_bits + offset_bits)); //Shift right to get the tag bits
	//unsigned int index = (address << tag_bits) >> (tag_bits + offset_bits); //Shift left to get rid of the tag, shift right to get rid of offest bits
	unsigned int index = (address >> offset_bits);
	index = index << (32 - index_bits);
	index = index >> (32 - index_bits);

	//FA
	if (assoc == 0)
	{
		unsigned long fa_tag = (tag << (index_bits + offset_bits)) + index;
		for (int i = 0; i < block_count; i++)
		{
			if ((tag_array[i] == fa_tag) && (valid_array[i] == 1))
			{
				//hooray! it's here!
				//Update the LRU
				UpdateLRU(index);
				//update the hit count
				hit_count = hit_count + 1;
				return hit_time * numbytes;
			}
		}

		//If we're here, then what we're looking for... isn't here ;_;
		//Update miss count
		miss_count = miss_count + 1;
		return -1;
	}
	//Direct-mapped
	if (assoc == 1)
	{
		//Is it here?
		if ((tag_array[index] == tag) && (valid_array[index] == 1))
		{
			//It's here! Rejoice!
			//Update the LRU first.
			UpdateLRU(index);
			//Update hit count
			hit_count = hit_count + 1;
			return hit_time * numbytes;
		}
		else
		{
			//Oh. It's not here. ;_;
			miss_count = miss_count + 1;
			return -1;
		}
	}

	//2 or 4 or 8 or whatever-way SA
	if (assoc > 1)
	{
		index = index / assoc; //Shift the bits for a new, correct "base" index.

		//We need to check X locations, where X is the number of ways.
		int i = 0;
		for (i = 0; i < 2; i++)
		{			
			if ((tag_array[index] == tag) && (valid_array[index] == 1))
			{
				//It's here!
				//Same deal: need to update LRU
				UpdateLRU(index);
				hit_count = hit_count + 1;
				return hit_time * numbytes;
			}
			else
			{
				//Not here. ;_;
				miss_count = miss_count + 1;
				index = index + block_count / assoc;
			}
		}

		//We're not in the for-loop anymore, so I'm guessing that it's not here.
		return -1;
	}

}

int BasicCache::Write(unsigned long long address, int numbytes, int isDirty)
{
	//Idea: see if it's in the cache. If it is, we just need to set the dirty bit.
	//If it isn't, and the data is not valid, we return the hit time. Easy peasy.
	//If it isn't, and the data at the index is valid, we need to replace it.
		//Add the tag & index together, shift to form an address. Add to the write buffer.

	unsigned long tag = (address >> (index_bits + offset_bits)); //Shift right to get the tag bits
	//unsigned int index = (address << tag_bits) >> (tag_bits + offset_bits); //Shift left to get rid of the tag, shift right to get rid of offest bits
	unsigned int index = (address >> offset_bits);
	index = index << (32 - index_bits);
	index = index >> (32 - index_bits);
	

	//Fully Associative
	if (assoc == 0)
	{
		unsigned long fa_tag = (tag << (index_bits + offset_bits)) + index;
		
		//Idea: Try to find the current item- if that fails, an open spage.

		//Is our item already in the cache?
		for (int i = 0; i < block_count; i++)
		{
			if ((tag_array[i] == fa_tag) && (valid_array[i] == 1))
			{
				UpdateLRU(i);
				dirty_array[i] = 1;
				return hit_time * numbytes;
			}
		}

		//Okay, so our thing is not in the cache. Is there an open space?
		for (int i = 0; i < block_count; i++)
		{
			if (valid_array[i] == 0)
			{
				valid_array[i] = 1;
				tag_array[i] = fa_tag;
				UpdateLRU(i);

				dirty_array[i] = isDirty; //For when we move an open item up to another cache
				return hit_time * numbytes;
			}
		}

		//If that didn't work, get the oldest item and replace it.
		//Write that item to the write "buffer".

		
		int oldest_item = LRU_array[block_count - 1];	

		//update kickouts accordingly
		if (dirty_array[oldest_item] == 1)
		{
			kickouts_d = kickouts_d + 1;
		}
		else
		{
			kickouts = kickouts + 1;
		}

		long long old_tag = tag_array[oldest_item];
		old_tag = old_tag << offset_bits; //reconstruct the address. mostly. this may not work right.
		write_item = old_tag;
		write_dirty = dirty_array[oldest_item];

		tag_array[oldest_item] = fa_tag;
		dirty_array[oldest_item] = isDirty; //moving a dirty item up to another spot
		UpdateLRU(oldest_item); //I guess it's not the oldest anymore though
		return hit_time * numbytes;
	}

	//DM
	if (assoc == 1)
	{

		//Is our item there?
		if ((tag_array[index] == tag) && (valid_array[index] == 1))
		{
			dirty_array[index] = 1;
			UpdateLRU(index);
			return hit_time * numbytes;
		}
		//It's not there. Is the place we want to write to valid?
		if (valid_array[index] == 0)
		{
			//It's not valid, we can replace it no problem.
			tag_array[index] = tag;
			valid_array[index] = 1;
			UpdateLRU(index);
			return hit_time * numbytes;
		}
		else
		{
			//It's already valid. Oh dear. We need to get our hands dirty.
			//Update the kickouts appropriately.
			if (dirty_array[index] == 1)
			{
				kickouts_d = kickouts_d + 1;
			}
			else
			{
				kickouts = kickouts + 1;
			}

			long long old_tag = tag_array[index];
			old_tag = ((old_tag << index_bits) + index) << offset_bits; //now it's an old address!
			write_item = old_tag;
			write_dirty = dirty_array[index];

			tag_array[index] = tag;
			dirty_array[index] = 0;
			UpdateLRU(index);
			return hit_time * numbytes;
		}
	}

	//X-way set-associative
	if (assoc > 1)
	{
		index = index / assoc;

		//If it's set-associative, there are several places we need to check.

		//Obviously, let's first check if it's in the cache.
		int index_local = index;
		for (int i = 0; i < assoc; i++)
		{
			if ((tag_array[index_local] == tag) && (valid_array[index_local] == 1))
			{
				//It's here!
				//Update LRU, set dirty bit.
				UpdateLRU(index_local);
				dirty_array[index_local] = 1;
				return hit_time * numbytes;
			}
			else
			{
				//Not here. Keep checking. ;_;
				index_local = index_local + block_count / assoc;
			}
		}

		//It's not in the cache. We need to check the available spaces for an opening.
		index_local = index;
		for (int i = 0; i < assoc; i++)
		{
			if (valid_array[index_local] == 0)
			{
				//An empty space! Alright!
				UpdateLRU(index_local);
				valid_array[index_local] = 1;
				tag_array[index_local] = tag;
				return hit_time * numbytes;
			}
			else
			{
				index_local = index_local + block_count / assoc;
			}
		}

		
		//Shit. No openings, either. We need to compare the ages of the items in
		//the spaces taken and kick one of those jerks out.
		int max_age = -1;
		int max_age_index;
		for (int i = 0; i < assoc; i++)
		{
			int index_local = index + i*(block_count / assoc);
			int age_local = GetAgeLRU(index_local);
			if (age_local > max_age)
			{
				max_age = age_local;
				max_age_index = index_local;
			}
		}

		//Now we have the index of the item we want to replace. Let's do it.
		//Need to update kickouts or kickouts_d accordingly.
		if (dirty_array[max_age_index] == 1)
		{
			kickouts_d = kickouts_d + 1;
		}
		else
		{
			kickouts = kickouts + 1;
		}

		long long old_tag = tag_array[max_age_index];
		old_tag = ((old_tag << index_bits) + index) << offset_bits; //now it's an old address!
		write_item = old_tag;
		write_dirty = dirty_array[max_age_index];

		tag_array[max_age_index] = tag;
		dirty_array[max_age_index] = 0;
		UpdateLRU(max_age_index);
		return hit_time * numbytes;

	}

}

int BasicCache::Evict(BasicCache& input_cache, int real_evict)
{
	//Invalidates all items in the cache.
	//if one of the items is dirty, it's written to the next level.
	//If real_evict is 1, then we're going to actually write. If not, we'll just pretend and say we did.
	//Returns number of evicts.

	int evict_count = 0;
	for (int i = 0; i < block_count; i++)
	{
		valid_array[i] = 0;
		if (dirty_array[i] == 1)
		{
			evict_count = 1;
			//Write it to the next level
			if (real_evict == 1)
			{
				//Reconstruct the address
				long long tag = tag_array[i];
				tag = (tag << (index_bits + offset_bits)) + (i << offset_bits);
				int numbytes = (block_size); //number of bytes in a block

				input_cache.Write(tag, numbytes, 1);
				kickouts_flush = kickouts_flush + 1;
			}
			dirty_array[i] = 0;
		}
	}
	return evict_count; //vOv
}

void BasicCache::UpdateLRU(int index)
{
	
	//Updates the LRU

	//First: check if the item is in the array.
	//If it is, we shift starting from that point.
	//If it isn't, we shift from the end.
	int index_pos = block_count - 1;
	for (int i = 0; i < block_count; i++)
	{
		if (LRU_array[i] == index)
		{
			index_pos = i;
			break;
		}
	}

	//Second: start shifting stuff from the index position
	for (int i = index_pos; i > 0; i--)
	{
		LRU_array[i] = LRU_array[i - 1];
	}

	//Last: update the first element with the index we were given
	LRU_array[0] = index;

}

int BasicCache::GetAgeLRU(int index)
{
	//Returns the "age" of the item in the LRU. Returns -1 if the item was not found.
	int age = -1;
	for (int i = 0; i < block_count; i++)
	{
		if (index == LRU_array[i])
		{
			age = i;
		}
	}

	return age;
}

int BasicCache::getCacheSize()
{
	return cache_size;
}

int BasicCache::getAssoc()
{
	return assoc;
}

BasicCache::~BasicCache()
{
	//delete tag_array;
	//delete valid_array;
	//delete dirty_array;
	//delete LRU_array;
}


///////////////////////
//Cache system data functions
//Putting these down here so the important
//stuff up there is easier to find.
/////////////////////

unsigned long CacheSystem::L1D_Hits()
{
	return L1D.hit_count;
}
unsigned long CacheSystem::L1D_Misses()
{
	return L1D.miss_count;
}
unsigned long CacheSystem::L1D_Kickouts()
{
	return L1D.kickouts;
}
unsigned long CacheSystem::L1D_Kickouts_D()
{
	return L1D.kickouts_d;
}
unsigned long CacheSystem::L1D_Kickouts_Flush()
{
	return L1D.kickouts_flush;
}
unsigned long CacheSystem::L1D_Transfers()
{
	return L1D.transfers;
}

////////////////
//L1I
unsigned long CacheSystem::L1I_Hits()
{
	return L1I.hit_count;
}
unsigned long CacheSystem::L1I_Misses()
{
	return L1I.miss_count;
}
unsigned long CacheSystem::L1I_Kickouts()
{
	return L1I.kickouts;
}
unsigned long CacheSystem::L1I_Kickouts_D()
{
	return L1I.kickouts_d;
}
unsigned long CacheSystem::L1I_Kickouts_Flush()
{
	return L1I.kickouts_flush;
}
unsigned long CacheSystem::L1I_Transfers()
{
	return L1I.transfers;
}

////////////
//L2

unsigned long CacheSystem::L2_Hits()
{
	return L2.hit_count;
}
unsigned long CacheSystem::L2_Misses()
{
	return L2.miss_count;
}
unsigned long CacheSystem::L2_Kickouts()
{
	return L2.kickouts;
}
unsigned long CacheSystem::L2_Kickouts_D()
{
	return L2.kickouts_d;
}
unsigned long CacheSystem::L2_Kickouts_Flush()
{
	return L2.kickouts_flush;
}
unsigned long CacheSystem::L2_Transfers()
{
	return L2.transfers;
}