#ifndef _CACHE_SIM_
#define _CACHE_SIM_

class CacheConfig
{

	public:
		int L1_block_size = 32;
		int L1_cache_size = 8192;
		int L1_assoc = 1;
		int L1_hit_time = 1;
		int L1_miss_time = 1;

		int L2_block_size = 64;
		int L2_cache_size = 32768;
		int L2_assoc = 1;
		int L2_hit_time = 5;
		int L2_miss_time = 7;
		int L2_transfer_time = 5;
		int L2_bus_width = 16;
};

#endif
