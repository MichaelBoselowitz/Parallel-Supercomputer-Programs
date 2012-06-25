#include <omp.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int processors, num_of_threads, num_blocks_cache, length_of_trace;
/* Bus Command:				Bus Valid:
 * 0 = Request Shared		0 = Not Valid
 * 1 = Request Exclusive	1 = Valid
 * 2 = Data Reply
 * 3 = Write to Memory
 * 4 = Invalidate Block
 */
int bus_command=0, bus_address=0, bus_valid=0;
int *all_requests, *all_priorities, current_head;
/*request=0 if are not making request, priority=0 for normal request, 1 for high priority*/
int arbitrate_bus(int request, int priority);
int simulate(int **operation, int **address);
int thread_print = 0;

int arbitrate_bus(int request, int priority)
{
	int thread_num = omp_get_thread_num(), i, 
		local_current_head = current_head;
	all_requests[thread_num] = request;
	all_priorities[thread_num] = priority;
	#pragma omp single
	bus_valid = 0;
	#pragma omp barrier
	//First check priorities
	for(i=0; i<num_of_threads; i++)
	{
		if(all_priorities[i])
		{
			if(thread_num == i)
				return 1;
			else
				return 0;
		}
	}
	//This catches the reaction cycle for the memory
	if(all_requests[num_of_threads-1])
	{
		if(thread_num == num_of_threads-1)
			return 1;
		else
			return 0;
	}
	//Then check round robin for which to give bus
	for(i=0; i<processors; i++)
	{
		if(all_requests[local_current_head])
		{
			#pragma omp single
			current_head = (local_current_head + 1) % processors;
			if(thread_num == local_current_head)
				return 1;
			else
				return 0;
		}
		local_current_head++;
		local_current_head %= processors;
	}
	return 0;
}

int simulate(int **operation, int **address)
{
	int i, j, cache_size = pow(2, num_blocks_cache), all_done = 0;
	int hits[processors], misses[processors], completed_cycles[processors];
	
	/*for(i=0; i < length_of_trace; i++)
	{
		for(j=0; j < processors; j++)
		{
			printf("row:%d col:%d op:%d addr:%d\t", i, j, operation[i][j], address[i][j]);
		}
		printf("\n");
	}*/
	
	memset(hits, 0, sizeof(hits[0]) * processors);
	memset(misses, 0, sizeof(misses[0]) * processors);
	memset(completed_cycles, 0, sizeof(completed_cycles[0]) * processors);
	
	#pragma omp parallel num_threads(num_of_threads) private(i)
	{
		int line, j, cycle, cache_address, current_op, current_addr, done = 0, made_request = 0, had_bus = 0;
		int thread_num = omp_get_thread_num();
		/*Column 1 stores tag, Column 2 stores state (invalid=0, shared=1, modified=2)*/
		int cache[cache_size][2];
		memset(cache, 0, sizeof(cache[0][0]) * cache_size * 2);
		
		for(cycle=1, line=0; ; cycle++)
		{
			if(line >= length_of_trace)
			{
				printf("Thread:%d Completed\n", thread_num);
				line = -1;
				done = 1;
				completed_cycles[thread_num] = cycle;
				#pragma omp atomic
				all_done++;
			}
			
			#pragma omp barrier
			
			if(all_done == num_of_threads-1)
			{
				#pragma omp barrier	
				break;
			}
			
			if(done)
			{
				arbitrate_bus(0, 0);
			}
			//If memory module
			else if(thread_num == (num_of_threads - 1))
			{	
				cache_address = (address[line][thread_num] / 4) % cache_size;
				current_op = operation[line][thread_num];
				current_addr = address[line][thread_num];
				arbitrate_bus(0, 0);
			}
			else
			{
				cache_address = (address[line][thread_num] / 4) % cache_size;
				current_op = operation[line][thread_num];
				current_addr = address[line][thread_num];
				//Cache hit
				if(cache[cache_address][1] && cache[cache_address][0] == current_addr)
				{
					//If a hit, but block is in shared and we want to writeback
					if(cache[cache_address][1] == 1 && current_op == 1)
					{
						if(arbitrate_bus(1, 0))
						{
							printf("Cycle:%d P:action\tTHD:%d CMD:invalidate\tADDR:%d PRI:normal\n", cycle, thread_num, current_addr);
							bus_command = 4;
							bus_address = current_addr;
							bus_valid = 1;
							hits[thread_num]++;
							had_bus = 1;
							cache[cache_address][1] = 2;
							line++;
						}
					}
					//True hit, don't need bus, can continue
					else
					{
						arbitrate_bus(0, 0);
						hits[thread_num]++;
						line++;
					}
				}
				//Something in cache at address that is modified, needs to be written back before proceeding
				else if(cache[cache_address][1] == 2)
				{
					if(arbitrate_bus(1, 0))
					{
						printf("Cycle:%d P:action\tTHD:%d CMD:writeback\tADDR:%d PRI:normal\n", cycle, thread_num, cache[cache_address][0]);
						bus_command = 3;
						bus_address = cache[cache_address][0];
						bus_valid = 1;
						had_bus = 1;
						cache[cache_address][1] = 0;
						cache[cache_address][0] = 0;
					}
				}
				//Cache miss
				else
				{
					if(arbitrate_bus(1, 0))
					{
						misses[thread_num]++;
						printf("Cycle:%d P:action\tTHD:%d CMD:%s\tADDR:%d PRI:normal\n", cycle, thread_num, (current_op ? "write" : "read"), current_addr);
						bus_command = current_op;
						bus_address = current_addr;
						bus_valid = 1;
						made_request = 1;
						had_bus = 1;
						line++;
					}
				}
			}
				
			//End of action cycle
			#pragma omp barrier
			
			if(thread_num == (num_of_threads - 1))
			{
				if(bus_command == 0 || bus_command == 1)
				{
					if(arbitrate_bus(1, 0))
					{
						printf("Cycle:%d P:reaction\tTHD:%d CMD:reply\tADDR:%d PRI:normal\n", cycle, thread_num, bus_address);
						bus_command = 2;
						bus_valid = 1;
					}
				}
				else
					arbitrate_bus(0, 0);
			}
			else
			{
				cache_address = (bus_address / 4) % cache_size;
				//Requested for reading
				if(bus_command == 0 || bus_command == 1)
				{
					//If it is modified, writeback data
					if(cache[cache_address][1] == 2 && cache[cache_address][0] == bus_address)
					{
						cache[cache_address][1] = (bus_command ? 0 : 1);
						cache[cache_address][0] = (bus_command ? 0 : cache[cache_address][0]);
						if(arbitrate_bus(1, 1))
						{
							printf("Cycle:%d P:reaction\tTHD:%d CMD:writeback/reply\tADDR:%d PRI:high\n", cycle, thread_num, bus_address);
							bus_command = 3;
							bus_valid = 1;
						}
					}
					else if(cache[cache_address][1] == 1 && cache[cache_address][0] == bus_address && bus_command == 1)
					{
						cache[cache_address][1] = 0;
						cache[cache_address][0] = 0;
						arbitrate_bus(0, 0);
					}
					else
					{
						arbitrate_bus(0, 0);
					}
				}
				//Received invalidate message
				else if(bus_command == 4)
				{
					if(cache[cache_address][1] && cache[cache_address][0] == bus_address && !had_bus)
					{
						cache[cache_address][1] = 0;
						cache[cache_address][0] = 0;
					}
					arbitrate_bus(0, 0);
				}
				else
				{
					arbitrate_bus(0, 0);
				}
			}
			
			//End reaction
			#pragma omp barrier
			
			//Received data
			if((bus_command == 2 || bus_command == 3) && made_request)
			{
				made_request = 0;
				cache_address = (bus_address / 4) % cache_size;
				cache[cache_address][0] = bus_address;
				if(current_op)
					cache[cache_address][1] = 2;
				else
					cache[cache_address][1] = 1;
			}
			
			had_bus = 0;
			
			if(processors <= 4 && num_blocks_cache <= 3)
			{
				while(thread_print != thread_num);
				if(thread_num != (num_of_threads - 1))
				{
					printf("Thread Num: %d\t", thread_num);
					for(j=0; j<cache_size; j++)
					{
						printf("%d %s\t", cache[j][0], (cache[j][1] == 1 ? "S" : (cache[j][1] == 2 ? "M" : "I")));
					}
					printf("\n");
					thread_print++;
				}
				else
				{
					thread_print = 0;
				}
			}
			#pragma omp barrier
		}
	}
	char strcycles[256] = "Completed Cycles: ";
	char strhits[256] = "Cache Hits: ";
	char strmisses[256] = "Cache Misses: ";
	char strmissrate[256] = "Cache Miss Rate: ";
	char temp[256];
	for(i=0; i<processors-1; i++)
	{
		sprintf(temp, "%d, ", completed_cycles[i]); 
		strcat(strcycles, temp);
		memset(temp, 0, 128);
		sprintf(temp, "%d, ", hits[i]);
		strcat(strhits, temp);
		memset(temp, 0, 128);
		sprintf(temp, "%d, ", misses[i]);
		strcat(strmisses, temp);
		memset(temp, 0, 128);
		sprintf(temp, "%f, ", (float)misses[i] / (float)(misses[i] + hits[i]));
		strcat(strmissrate, temp);
		memset(temp, 0, 128);
	}
	sprintf(temp, "%d\n", completed_cycles[i]); 
	strcat(strcycles, temp);
	memset(temp, 0, 128);
	sprintf(temp, "%d\n", hits[i]);
	strcat(strhits, temp);
	memset(temp, 0, 128);
	sprintf(temp, "%d\n", misses[i]);
	strcat(strmisses, temp);
	memset(temp, 0, 128);
	sprintf(temp, "%f\n", (float)misses[i] / (float)(misses[i] + hits[i]));
	strcat(strmissrate, temp);
	memset(temp, 0, 128);
	printf(strcycles);
	printf(strhits);
	printf(strmisses);
	printf(strmissrate);
	printf("All threads completed\n");
}

int main(int argc, char **argv)
{
	FILE *trace_file = fopen("traces", "r");
	char line[128], *token;
	int i, j;
	int **operation, **address;
	
	//Set variables
	//P
	fgets(line, 128, trace_file);
	token = strtok(line, " ");
	processors = atoi(token);
	num_of_threads = processors + 1;
	//printf("token = %s, processors = %d\n", token, processors);
	
	//k
	token = strtok(NULL, " ");
	num_blocks_cache = atoi(token);
	//printf("token = %s, num_blocks_cache = %d\n", token, num_blocks_cache);
	
	token = strtok(NULL, " ");
	length_of_trace = atoi(token);
	//printf("token = %s, length_of_trace = %d\n", token, length_of_trace);
	
	all_requests = (int *)malloc(num_of_threads * sizeof(int));
	all_priorities = (int *)malloc(num_of_threads * sizeof(int));
	
	operation = (int **)malloc(length_of_trace * sizeof(int *));
	address = (int **)malloc(length_of_trace * sizeof(int *));
	current_head = 0;
	
	for(i=0; fgets(line, 128, trace_file) != NULL; i++)
	{
		operation[i] = (int *)malloc(processors * sizeof(int));
		address[i] = (int *)malloc(processors * sizeof(int));
		
		token = strtok(line, " ");
		for(j=0; token != NULL; j++)
		{
			if(!strcmp(token, "\n"))
			{
				token = NULL;
				continue;
			}
				
			address[i][j] = atoi(token);
			//printf("token = %s, address = %d\n", token, address[i][j]);
			
			token = strtok(NULL, " ");
			operation[i][j] = atoi(token);
			//printf("token = %s, operation = %d\n", token, operation[i][j]);
			
			token = strtok(NULL, " ");
		}
	}
	
	simulate(operation, address);
	
	for(i=0; i<length_of_trace; i++)
	{
		free(operation[i]);
		free(address[i]);
	}
	free(operation);
	free(address);
	free(all_requests);
	free(all_priorities);
	fclose(trace_file);
	return 0;
}