#include <stdio.h>
#include "CacheSystem.h"
#include "cacheSim.h"

int main (int argc, char ** argv)
{

	char str[80];

	FILE * config;

	/* Default parameters */

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


	//if a config file has been passed in, change the cache parameters
	if (argc > 1)
	{

		config = fopen(argv[1], "r");

		if(config == NULL)
		{
			printf("Error opening config file\r\n");
			return -1;
		}

		

		fscanf(config, "%s %d %s %d\n", str, &L1_block_size, str, &L2_block_size);
		fscanf(config, "%s %d %s %d\n", str, &L1_cache_size, str, &L2_cache_size);
		fscanf(config, "%s %d %s %d\n", str, &L1_assoc, str, &L2_assoc);
		fscanf(config, "%s %d %s %d\n", str, &L1_hit_time, str, &L2_hit_time);
		fscanf(config, "%s %d %s %d\n", str, &L1_miss_time, str, &L2_miss_time);
		fscanf(config, "%s %d\n", str, &L2_transfer_time);
		fscanf(config, "%s %d\n", str, &L2_bus_width);
	
		fclose(config);
	}

	CacheSystem cache(L1_cache_size, L1_assoc, L1_block_size, L1_hit_time, L1_miss_time,
			  L2_cache_size, L2_assoc, L2_block_size, L2_hit_time, L2_miss_time,
			  L2_transfer_time, L2_bus_width); 

	FILE * log = fopen("Results.txt", "w+");
	
	fprintf(log,"D-cache size = %d : ways = %d : block size = %d\r\n", cache.L1D.getCacheSize(), cache.L1D.getAssoc(), cache.L1D.getBlockSize());
	fprintf(log,"I-cache size = %d : ways = %d : block size = %d\r\n", cache.L1I.getCacheSize(), cache.L1I.getAssoc(), cache.L1I.getBlockSize());
	fprintf(log,"L2-cache size = %d : ways = %d : block size = %d\r\n", cache.L2.getCacheSize(), cache.L2.getAssoc(), cache.L2.getBlockSize());

	char inst;
	unsigned long long addr;
	int numBytes;

	unsigned int execTime = 0, Rcycle = 0, Wcycle = 0, Icycle = 0;

	unsigned int Irefs = 0, Rrefs = 0, Wrefs = 0, Trefs = 0;

	while (scanf("%c %Lx %d\n", &inst, &addr, &numBytes) == 3)
	{
		int n;

		n = cache.Execute(inst, addr, numBytes);

		if (n < 0)
		{
			printf("CacheSystem.Execute Error. Terminating simulation\r\n");
			return -1;
		}

		switch (inst)
		{
			case 'I':
			{
				Irefs++;
				Icycle += n;
				break;
			}
			case 'R':
			{
				Rrefs++;
				Rcycle += n;
				break;
			}
			case 'W':
			{
				Wrefs++;
				Wcycle += n;
				break;
			}
			default:
				break;
		}

	}

	execTime = Rcycle + Wcycle + Icycle;

	Trefs = Irefs + Rrefs + Wrefs;

	float percent;


	fprintf(log,"Execution Time = %d;\tTotal refs:%d\r\n", execTime, Trefs);
	fprintf(log,"Flush Time = %d\r\n", 0);
	fprintf(log,"Inst refs = %d;\tData refs = %d\r\n\r\n", Irefs, Wrefs);

	if(Rrefs > 0) percent = (float)Rrefs / (float)Trefs * 100;
	else percent = 0;

	fprintf(log,"Number of reference types: [Percentage]\r\n");
	fprintf(log,"Reads = \t%d\t[%2.1f%]\r\n", Rrefs, percent);
	
	if(Wrefs > 0) percent = (float)Wrefs / (float)Trefs * 100;
	else percent = 0;

	fprintf(log,"Writes = \t%d\t[%2.1f%]\r\n", Wrefs, percent);

	if(Irefs > 0) percent = (float)Irefs / (float)Trefs * 100;
	else percent = 0;
	
	fprintf(log,"Inst. = \t%d\t[%2.1f%]\r\n", Irefs, percent);

	fprintf(log,"Total = \t%d\r\n\r\n", Trefs);

	fprintf(log,"Total cycles for activities: [Percentage]\r\n");

	
	if(Rcycle > 0) percent = (float)Rcycle / (float)execTime * 100;
	else percent = 0;

	fprintf(log,"Reads = \t%d\t[%2.1f%]\r\n", Rcycle, percent);
	
	if(Wcycle > 0) percent = (float)Wcycle / (float)execTime * 100;
	else percent = 0;
	
	fprintf(log,"Writes = \t%d\t[%2.1f%]\r\n", Wcycle, percent);

	if(Icycle > 0) percent = (float)Icycle / (float)execTime * 100;
	else percent = 0;

	fprintf(log,"Inst. = \t%d\t[%2.1f%]\r\n", Icycle, percent);

	fprintf(log,"Total = \t%d\r\n\r\n", execTime);

	float average;

	fprintf(log,"Average cycles per activity:\r\n");

	if(Rrefs > 0) average = Rcycle / Rrefs;
	else average = 0;

	fprintf(log,"\tRead = %2.1f;", average);
	
	if(Wrefs > 0) average = Wcycle / Wrefs;
	else average = 0;

	fprintf(log,"\tWrite = %2.1f;", average);
	
	if(Irefs > 0) average = Icycle / Irefs;
	else average = 0;

	fprintf(log,"\tInst. = %2.1f\r\n", average);
	
	int Ideal;

	Ideal = (Irefs*2) + Rrefs + Wrefs;

	average = (float)Ideal/(float)Irefs;
	
	fprintf(log,"Ideal: Exec. Time = %d; CPI = %2.1f\r\n", Ideal, average); 

	Ideal = (Irefs*2) + Rrefs + Wrefs;//need to figure out the mis-aligned case

	average = (float)Ideal/(float)Irefs;

	fprintf(log,"ideal Mis-aligned: Exec. Time = %d; CPI = %2.1f\r\n", Ideal, average);

	fclose(log);

return 0;
}
