#include <stdio.h>
#include "CacheSystem.h"
#include <time.h>
#include <math.h>

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
	int mem_chunksize = 8;


	time_t now;

	struct tm start;
	struct tm end;

	//if a config file has been passed in, change the cache parameters
	if (argc > 2)
	{

		config = fopen(argv[2], "r");

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
		fscanf(config, "%s %d\n", str, &mem_chunksize);
	
		fclose(config);
	}

	CacheSystem cache(L1_cache_size, L1_assoc, L1_block_size, L1_hit_time, L1_miss_time,
			  L2_cache_size, L2_assoc, L2_block_size, L2_hit_time, L2_miss_time,
			  L2_transfer_time, L2_bus_width, mem_chunksize); 

	FILE * log = fopen(argv[1], "w+");//open up file to log results with name passed in
	
	fprintf(log,"------------------------------------------------------------\r\n");
	fprintf(log,"\t%s\tSimulation Results\r\n", argv[2]);

	time(&now);

	start = *localtime(&now);

	fprintf(log, "\tStart time: %d-%d-%d %d:%d:%d\r\n", start.tm_year + 1900, start.tm_mon + 1, start.tm_mday,
		start.tm_hour, start.tm_min, start.tm_sec); 

	fprintf(log,"------------------------------------------------------------\r\n");
	
	fprintf(log, "Memory System:\r\n");

	fprintf(log,"D-cache size = %d : ways = %d : block size = %d\r\n", cache.L1D.getCacheSize(), cache.L1D.getAssoc(), cache.L1D.getBlockSize());
	fprintf(log,"I-cache size = %d : ways = %d : block size = %d\r\n", cache.L1I.getCacheSize(), cache.L1I.getAssoc(), cache.L1I.getBlockSize());
	fprintf(log,"L2-cache size = %d : ways = %d : block size = %d\r\n", cache.L2.getCacheSize(), cache.L2.getAssoc(), cache.L2.getBlockSize());
	fprintf(log,"Memory ready time = %d : chunksize = %d : chunktime = %d \r\n",cache.get_mem_ready(), cache.get_mem_chunksize(), cache.get_mem_chunktime());
	

	char inst;
	unsigned long long addr;
	int numBytes;

	unsigned long long execTime = 0, Rcycle = 0, Wcycle = 0, Icycle = 0;

	unsigned long long Rrefs = 0, Wrefs = 0, Irefs = 0, Trefs = 0;

	FILE * dbug;

	unsigned long long refs = 0;

	while (scanf("%c %Lx %d\n", &inst, &addr, &numBytes) == 3)
	{

		int n;

		n = cache.Execute(inst, addr, numBytes);

		
		if (refs > 10000000)
		{
			dbug = fopen("DBUG.txt", "a");
			fprintf(dbug,"On reference %lld\r\n",execTime);
			refs = 0;
			fclose(dbug);
		}
		else
		{
			refs++;
		}


		if (n < 0)
		{
			printf("CacheSystem.Execute Error. Terminating simulation\r\n");
			return -1;
		}

		execTime += n;
	}


	//populate variables for output
	Rrefs = cache.getRrefs();
	Wrefs = cache.getWrefs();
	Irefs = cache.getIrefs();

	Rcycle = cache.getRcycles();
	Wcycle = cache.getWcycles();
	Icycle = cache.getIcycles();

	Trefs = Irefs + Rrefs + Wrefs;

	float percent;

	/* Summary */

	fprintf(log,"Execution Time = %llu;\tTotal refs:%llu\r\n", execTime, Trefs);
	fprintf(log,"Flush Time = %llu\r\n", cache.flush_time);
	fprintf(log,"Inst refs = %llu;\tData refs = %llu\r\n\r\n", Irefs, Wrefs);
	
	/* # of refs */

	if(Rrefs > 0) percent = (float)Rrefs / (float)Trefs * 100;
	else percent = 0;

	fprintf(log,"Number of reference types: [Percentage]\r\n");
	fprintf(log,"Reads = \t%luu\t[%2.1f%]\r\n", Rrefs, percent);
	
	if(Wrefs > 0) percent = (float)Wrefs / (float)Trefs * 100;
	else percent = 0;

	fprintf(log,"Writes = \t%llu\t[%2.1f%]\r\n", Wrefs, percent);

	if(Irefs > 0) percent = (float)Irefs / (float)Trefs * 100;
	else percent = 0;
	
	fprintf(log,"Inst. = \t%llu\t[%2.1f%]\r\n", Irefs, percent);

	fprintf(log,"Total = \t%llu\r\n\r\n", Trefs);

	/* Total cycles*/

	fprintf(log,"Total cycles for activities: [Percentage]\r\n");

	if(Rcycle > 0) percent = (float)Rcycle / (float)execTime * 100;
	else percent = 0;

	fprintf(log,"Reads = \t%llu\t[%2.1f%]\r\n", Rcycle, percent);
	
	if(Wcycle > 0) percent = (float)Wcycle / (float)execTime * 100;
	else percent = 0;
	
	fprintf(log,"Writes = \t%llu\t[%2.1f%]\r\n", Wcycle, percent);

	if(Icycle > 0) percent = (float)Icycle / (float)execTime * 100;
	else percent = 0;

	fprintf(log,"Inst. = \t%llu\t[%2.1f%]\r\n", Icycle, percent);

	fprintf(log,"Total = \t%llu\r\n\r\n", execTime);

	float average;

	/*Averages*/

	fprintf(log,"Average cycles per activity:\r\n");

	if(Rrefs > 0) average = (float)Rcycle / Rrefs;
	else average = 0;

	fprintf(log,"\tRead = %2.1f;", average);
	
	if(Wrefs > 0) average = (float)Wcycle / Wrefs;
	else average = 0;

	fprintf(log,"\tWrite = %2.1f;", average);
	
	if(Irefs > 0) average = (float)execTime / Irefs;
	else average = 0;

	fprintf(log,"\tInst. = %2.1f\r\n", average);
	
	int Ideal;

	Ideal = (Irefs*2) + Rrefs + Wrefs;

	average = (float)Ideal/(float)Irefs;
	
	fprintf(log,"\tIdeal: Exec. Time = %llu; CPI = %2.1f\r\n", Ideal, average); 

	Ideal = Irefs*2 + ((cache.L1I_Hits() + cache.L1I_Misses()) - Irefs) + cache.L1D_Hits()
		+ cache.L1D_Misses();//need to figure out the mis-aligned case

	average = (float)Ideal/(float)Irefs;

	fprintf(log,"\tIdeal Mis-aligned: Exec. Time = %llu; CPI = %2.1f\r\n\r\n", Ideal, average);

	/*L1I stats*/

	fprintf(log, "Memory Level: L1I\r\n");
	fprintf(log, "\tHit Count = %llu\tMiss Count = %llu\r\n", cache.L1I_Hits(), cache.L1I_Misses());
	fprintf(log, "\tTotal Requests = %llu\r\n",cache.L1I_Hits() + cache.L1I_Misses());
	
	percent = (float)cache.L1I_Hits()/(float)(cache.L1I_Hits() + cache.L1I_Misses())*100;
	fprintf(log, "\tHit Rate = %2.1f\t",percent);
	percent = (float)cache.L1I_Misses()/(float)(cache.L1I_Hits() + cache.L1I_Misses())*100;
	fprintf(log, "Miss Rate = %2.1f\r\n", percent);

	fprintf(log, "\tKickouts = %llu;\tDirty Kickouts = %llu;\tTransfers = %llu\r\n",
		cache.L1I_Kickouts(), cache.L1I_Kickouts_D(), cache.L1I_Transfers());
	fprintf(log, "\tFlush Kickouts = %llu\r\n\r\n", cache.L1I_Kickouts_Flush());

	/* L1D stats*/
	
	fprintf(log, "Memory Level: L1D\r\n");
	fprintf(log, "\tHit Count = %llu\tMiss Count = %llu\r\n", cache.L1D_Hits(), cache.L1D_Misses());
	fprintf(log, "\tTotal Requests = %llu\r\n",cache.L1D_Hits() + cache.L1D_Misses());
	
	percent = (float)cache.L1D_Hits()/(float)(cache.L1D_Hits() + cache.L1D_Misses())*100;
	fprintf(log, "\tHit Rate = %2.1f\t",percent);
	percent = (float)cache.L1D_Misses()/(float)(cache.L1D_Hits() + cache.L1D_Misses())*100;
	fprintf(log, "Miss Rate = %2.1f\r\n", percent);

	fprintf(log, "\tKickouts = %llu;\tDirty Kickouts = %llu;\tTransfers = %llu\r\n",
		cache.L1D_Kickouts(), cache.L1D_Kickouts_D(), cache.L1D_Transfers());
	fprintf(log, "\tFlush Kickouts = %llu\r\n\r\n", cache.L1D_Kickouts_Flush());

	/* L2 stats*/

	
	fprintf(log, "Memory Level: L2\r\n");
	fprintf(log, "\tHit Count = %llu\tMiss Count = %llu\r\n", cache.L2_Hits(), cache.L2_Misses());
	fprintf(log, "\tTotal Requests = %llu\r\n",cache.L2_Hits() + cache.L2_Misses());
	
	if(cache.L2_Hits() > 0) percent = (float)cache.L2_Hits()/(float)(cache.L2_Hits() + cache.L2_Misses())*100;
	else percent = 0;
	fprintf(log, "\tHit Rate = %2.1f\t",percent);

	if(cache.L2_Misses() > 0) percent = (float)cache.L2_Misses()/(float)(cache.L2_Hits() + cache.L2_Misses())*100;
	else percent = 0;
	fprintf(log, "Miss Rate = %2.1f\r\n", percent);

	fprintf(log, "\tKickouts = %llu;\tDirty Kickouts = %llu;\tTransfers = %llu\r\n",
		cache.L2_Kickouts(), cache.L2_Kickouts_D(), cache.L2_Transfers());
	fprintf(log, "\tFlush Kickouts = %llu\r\n\r\n", cache.L2_Kickouts_Flush());

	/* Costs */

	fprintf(log, "L1 Cache Cost (Icache $%d) + (Dcache $%d) = $%d\r\n",cache.GetL1Cost(), 
		cache.GetL1Cost(), cache.GetL1Cost() * 2);
	fprintf(log, "L2 Cache Cost = $%d;\tMemory Cost = $%d\tTotal cost = $%d\r\n",
		cache.GetL2Cost(), cache.GetMMCost(), cache.GetL2Cost() + cache.GetMMCost() + (cache.GetL1Cost() * 2));
	fprintf(log, "Flushes = %llu :\tInvalidates = %llu\r\n\r\n", cache.flush_count, cache.L1I.invalidates + cache.L1D.invalidates + cache.L2.invalidates);//TODO: fill out

	fprintf(log,"------------------------------------------------------------\r\n\r\n");

	fprintf(log, "Cache Final Contents - Index and Tag values are in HEX\r\n\r\n");
	fprintf(log, "Memory Level:\tL1I\r\n");
	
	int i, j;
	int n = pow(2, cache.L1I.getIndexBits());

	int associativity = cache.L1I.getAssoc()?cache.L1I.getAssoc():cache.L1I.block_count;
	
	for (i = 0; i < n; i++)
	{
		if(cache.L1I.getValid(i))
		{
			fprintf(log, "Index : %x\t|",i);
			for (j = i; j < cache.L1I.block_count; j += cache.L1I.block_count / associativity )
			{
					fprintf(log, " V: %d\tD:%d\tTag:\t%llx\t|",cache.L1I.getValid(j), cache.L1I.getDirty(j), cache.L1I.getTag(j)); 
					if((j % 2) == 0) fprintf(log, "\r\n\t\t");
			}
			fprintf(log, "\r\n");
		}
	}
	fprintf(log,"\r\n");

	fprintf(log, "Memory Level:\tL1D\r\n");
	
	n = pow(2, cache.L1D.getIndexBits());

	associativity = cache.L1D.getAssoc()?cache.L1D.getAssoc():cache.L1D.block_count;
	
	for (i = 0; i < n; i++)
	{
		if(cache.L1D.getValid(i))
		{
			fprintf(log, "Index : %x\t|",i);
			for (j = i; j < cache.L1D.block_count; j += cache.L1D.block_count / associativity )
			{
					fprintf(log, " V: %d\tD:%d\tTag:\t%llx\t|",cache.L1D.getValid(j), cache.L1D.getDirty(j), cache.L1D.getTag(j)); 
				if((j % 2) == 0) fprintf(log, "\r\n\t\t");
			}
			fprintf(log, "\r\n");
		}
	}
	fprintf(log,"\r\n");

	fprintf(log, "Memory Level:\tL2\r\n");
	
	n = pow(2, cache.L2.getIndexBits());

	associativity = cache.L2.getAssoc()?cache.L2.getAssoc():cache.L2.block_count;
	
	for (i = 0; i < n; i++)
	{
		if(cache.L2.getValid(i))
		{
			fprintf(log, "Index : %x\t|",i);
			for (j = i; j < cache.L2.block_count; j += cache.L2.block_count / associativity )
			{
					fprintf(log, " V: %d\tD:%d\tTag:\t%llx\t|",cache.L2.getValid(j), cache.L2.getDirty(j), cache.L2.getTag(j)); 
				if((j % 2) == 0) fprintf(log, "\r\n\t\t");
			}
			fprintf(log, "\r\n");
		}
	}
	fprintf(log,"\r\n");
	time_t old;

	old = now;

	time(&now);

	end = *localtime(&now);

	fprintf(log, "End time: %d-%d-%d %d:%d:%d\r\n", end.tm_year + 1900, end.tm_mon + 1, end.tm_mday,
		end.tm_hour, end.tm_min, end.tm_sec); 

	fprintf(log, "Total run time: %d\r\n", now - old);

	fclose(log);

return 0;
}
