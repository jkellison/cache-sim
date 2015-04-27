#include <stdio.h>
#include "CacheSystem.h"
#include "cacheSim.h"

int main (int argc, char ** argv)
{

	CacheConfig cacheParams;

	char str[80];

	FILE * config;

	CacheSystem cache;

	//if a config file has been passed in, change the cache parameters
	if (argc > 1)
	{

		config = fopen(argv[1], "r");

		if(config == NULL)
		{
			printf("Error opening config file\r\n");
			return -1;
		}

		fscanf(config, "%s %d %s %d\n", str, &cacheParams.L1_block_size, str, &cacheParams.L2_block_size);
		fscanf(config, "%s %d %s %d\n", str, &cacheParams.L1_cache_size, str, &cacheParams.L2_cache_size);
		fscanf(config, "%s %d %s %d\n", str, &cacheParams.L1_assoc, str, &cacheParams.L2_assoc);
		fscanf(config, "%s %d %s %d\n", str, &cacheParams.L1_hit_time, str, &cacheParams.L2_hit_time);
		fscanf(config, "%s %d %s %d\n", str, &cacheParams.L1_miss_time, str, &cacheParams.L2_miss_time);
		fscanf(config, "%s %d\n", str, &cacheParams.L2_transfer_time);
		fscanf(config, "%s %d\n", str, &cacheParams.L2_bus_width);
	
		fclose(config);
	}

	cache = CacheSystem(cacheParams.L1_cache_size, cacheParams.L1_assoc, cacheParams.L1_block_size,
				  cacheParams.L1_hit_time, cacheParams.L1_miss_time,
				  cacheParams.L2_cache_size, cacheParams.L2_assoc, cacheParams.L2_block_size,
				  cacheParams.L2_hit_time, cacheParams.L2_miss_time); 

	printf("D-cache size = %d : ways = %d : block size = %d\r\n", cache.L1D.getCacheSize(), cache.L1D.getAssoc(), cache.L1D.getBlockSize());
	printf("I-cache size = %d : ways = %d : block size = %d\r\n", cache.L1I.getCacheSize(), cache.L1I.getAssoc(), cache.L1I.getBlockSize());
	printf("L2-cache size = %d : ways = %d : block size = %d\r\n", cache.L2.getCacheSize(), cache.L2.getAssoc(), cache.L2.getBlockSize());


	char inst;
	unsigned long long addr;
	int numBytes;

	unsigned int execTime = 0;

	unsigned int refs = 0;

	while (scanf("%c %Lx %d\n", &inst, &addr, &numBytes) == 3)
	{


		printf("Instr: %c Addr: %Lx Bytes: %d\r\n", inst, addr, numBytes);	

		execTime += cache.Execute(inst, addr, numBytes);
		
		if (execTime < 0)
		{
			printf("CacheSystem.Execute Error. Terminating simulation\r\n");
			return -1;
		}

		refs++;//increment ref counter


	}

	printf("Execution Time: %d\tTotal refs:%d\r\n", execTime, refs);

return 0;
}
