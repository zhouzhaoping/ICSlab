//ID:guest377


#include "cachelab.h"


#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

struct
{
	int valid;
	unsigned tag;
}cache[32][16];

int nHit = 0, nMiss = 0, nEvic = 0;
int E, S, s, b;
int v = 0, h = 0;
int marsk;



void try_hit(unsigned long long addr)
{
	unsigned set_id = (addr >> b) & marsk, tag_id = addr >> (b + s);

	int i = 0;
	while (i < E && cache[set_id][i].valid)
	{
		if (cache[set_id][i].tag == tag_id)
		{
			++nHit;
			if (v) printf(" hit");
			for (int j = i; j > 0; --j)
				cache[set_id][j].tag = cache[set_id][j - 1].tag;
			cache[set_id][0].tag = tag_id;
			return;
		}
		++i;
	}

	++nMiss;
	if (v) printf(" miss");
	if (i == E)
	{
		++nEvic;
		if (v) printf(" eviction");
		i = E - 1;
	}
	else
		cache[set_id][i].valid = 1;

	for (; i >= 1; --i)
		cache[set_id][i].tag = cache[set_id][i - 1].tag;
	cache[set_id][0].tag = tag_id;

	return;
}
int main(int argc, char * argv[])
{
	int type;
	FILE *pFILE;
	while ((type = getopt(argc, argv, "hvs:E:b:t:")) != -1)
	{
		switch (type)
		{
			case 'h':h = 1;break;
			case 'v':v = 1;break;
			case 's':s = atoi(optarg);marsk = (1 << s) - 1;S = 1 << s;break;
			case 'E':E = atoi(optarg);break;
			case 'b':b = atoi(optarg);break;
			case 't':pFILE = fopen(optarg, "r");break;
		}
	}


	for (int i = 0; i < S; ++i)
		for (int j = 0; j < E; ++j)
			cache[i][j].valid = 0;


	unsigned long long int addr;
	unsigned size;
	char opt, tail[5];
	while (fscanf(pFILE, "%c", &opt) != EOF)
	{
		if (opt == 'I')
		{
			fscanf(pFILE, " %llx,%u", &addr, &size);
			fgets(tail, 4, pFILE);
		}
		else
		{
			fscanf(pFILE, "%c %llx,%u", &opt, &addr, &size);
			fgets(tail, 4, pFILE);
			if (opt == 'M' || opt == 'L' || opt == 'S')
			{
				if (v) printf("%c %llx,%u", opt, addr, size);
				try_hit (addr);
				if (opt == 'M')
				{
					nHit++;
					if (v) printf(" hit");
				}
			}
		}
		if (v) printf("\n");
	}


	printSummary(nHit, nMiss, nEvic);
	fclose(pFILE);
	return 0;
}
