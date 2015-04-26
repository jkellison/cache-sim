#include <stdio.h>
#include <cacheSim.h>
#include <CacheSystem.h>

int main (int argc, char ** argv)
{

	CacheConfig cacheParams;

	char str[80];

	FILE * config;

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
	
	}

	printf("L1_block_size: %d\tL2_block_size: %d\r\n", cacheParams.L1_block_size, cacheParams.L2_block_size);
	printf("L1_cache_size: %d\tL2_cache_size: %d\r\n", cacheParams.L1_cache_size, cacheParams.L2_cache_size);
	printf("L1_assoc: %d\t\tL2_assoc: %d\r\n", cacheParams.L1_assoc, cacheParams.L2_assoc);
	printf("L1_hit_time: %d\t\tL2_hit_time: %d\r\n", cacheParams.L1_hit_time, cacheParams.L2_hit_time);
	printf("L1_miss_time: %d\t\tL2_miss_time: %d\r\n", cacheParams.L1_miss_time, cacheParams.L2_miss_time);
	printf("L2_transfer_time: %d\r\n", cacheParams.L2_transfer_time);
	printf("L2_bus_width: %d\r\n", cacheParams.L2_bus_width);

	char inst;
	unsigned long long addr;
	int numBytes;

	CacheSystem(cacheParams.L1_cache_size, cacheParams.L1_assoc, cacheParams.L2_cache_size, cacheParams.L2_assoc); 

	CacheSystem cache;

	while (scanf("%c %Lx %d\n", &inst, &addr, &numBytes) == 3)
	{

		if (cache.Execute(inst, addr, numBytes))
		{
			printf("CacheSystem.Execute Error. Terminating simulation\r\n");
			return -1;
		}

	printf("Instr: %c Addr: %Lx Bytes: %d\r\n", inst, addr, numBytes);	

	}

return 0;
}
