#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "dyn_array.h"
#include "processing_scheduling.h"


// You might find this handy.  I put it around unused parameters, but you should
// remove it before you submit. Just allows things to compile initially.
#define UNUSED(x) (void)(x)

// private function
void virtual_cpu(ProcessControlBlock_t *process_control_block) 
{
	// decrement the burst time of the pcb
	--process_control_block->remaining_burst_time;
}

bool first_come_first_serve(dyn_array_t *ready_queue, ScheduleResult_t *result) 
{
	UNUSED(ready_queue);
	UNUSED(result);
	return false;
}

bool shortest_job_first(dyn_array_t *ready_queue, ScheduleResult_t *result) 
{
	UNUSED(ready_queue);
	UNUSED(result);
	return false;
}

bool priority(dyn_array_t *ready_queue, ScheduleResult_t *result) 
{
	UNUSED(ready_queue);
	UNUSED(result);
	return false;
}

bool round_robin(dyn_array_t *ready_queue, ScheduleResult_t *result, size_t quantum) 
{
	UNUSED(ready_queue);
	UNUSED(result);
	UNUSED(quantum);
	return false;
}

dyn_array_t *load_process_control_blocks(const char *input_file) 
{
	if (input_file) 
	{
		int fd = open(input_file, O_RDONLY);
		if (fd != -1) 
		{
			uint32_t num_pcb;
			if (read(fd, &num_pcb, sizeof(uint32_t)) == sizeof(uint32_t)) 
			{
				dyn_array_t *array = dyn_array_create((size_t)num_pcb, sizeof(ProcessControlBlock_t), &process_control_block_destruct);

				while (dyn_array_size(array) < (size_t)num_pcb) 
				{
					uint32_t burst_time, priority, arrival_time;

					// Read the next triple, representing the next process control block
					if (read(fd, &burst_time, sizeof(uint32_t)) == sizeof(uint32_t) && 
							read(fd, &priority, sizeof(uint32_t)) == sizeof(uint32_t) &&
							read(fd, &arrival_time, sizeof(uint32_t)) == sizeof(uint32_t))
					{
						ProcessControlBlock_t *block = malloc(sizeof(ProcessControlBlock_t));
						if (block) 
						{
							block->remaining_burst_time = burst_time;
							block->priority = priority;
							block->arrival = arrival_time;
							block->started = false;

							bool res = dyn_array_push_back(array, block);
							
							if (!res) return NULL;
						} 
						else return NULL;
					} 
					else return NULL;
				}

				close(fd);

				return array;
			}
		}
	}

	return NULL;
}

bool shortest_remaining_time_first(dyn_array_t *ready_queue, ScheduleResult_t *result) 
{
	UNUSED(ready_queue);
	UNUSED(result);
	return false;
}

void process_control_block_destruct(void *element) 
{ 
	free(element);
}
