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

// Comparator for sorting PCBs by arrival time (ascending)
static int compare_arrival_time(const void *a, const void *b)
{
	const ProcessControlBlock_t *pcb_a = (const ProcessControlBlock_t *)a;
	const ProcessControlBlock_t *pcb_b = (const ProcessControlBlock_t *)b;

	if (pcb_a->arrival < pcb_b->arrival) return -1;
	if (pcb_a->arrival > pcb_b->arrival) return 1;
	return 0;
}

bool first_come_first_serve(dyn_array_t *ready_queue, ScheduleResult_t *result) 
{
	// Parameter validation
	if (!ready_queue || !result)
	{
		return false;
	}

	size_t num_processes = dyn_array_size(ready_queue);
	if (num_processes == 0)
	{
		return false;
	}

	// Sort the ready queue by arrival time to ensure FCFS order
	dyn_array_sort(ready_queue, compare_arrival_time);

	unsigned long clock = 0;
	float total_wait_time = 0;
	float total_turnaround_time = 0;

	for (size_t i = 0; i < num_processes; i++)
	{
		ProcessControlBlock_t *pcb = (ProcessControlBlock_t *)dyn_array_at(ready_queue, i);
		if (!pcb)
		{
			return false;
		}

		// If the CPU is idle and the process hasn't arrived yet, advance the clock
		if (clock < pcb->arrival)
		{
			clock = pcb->arrival;
		}

		// Calculate wait time: time spent waiting before getting the CPU
		unsigned long wait_time = clock - pcb->arrival;
		total_wait_time += (float)wait_time;

		// Mark the process as started
		pcb->started = true;

		// Run the process on the virtual CPU until its burst is complete
		while (pcb->remaining_burst_time > 0)
		{
			virtual_cpu(pcb);
			clock++;
		}

		// Calculate turnaround time: completion time - arrival time
		unsigned long turnaround_time = clock - pcb->arrival;
		total_turnaround_time += (float)turnaround_time;
	}

	// Fill in the result structure
	result->average_waiting_time = total_wait_time / (float)num_processes;
	result->average_turnaround_time = total_turnaround_time / (float)num_processes;
	result->total_run_time = clock;

	return true;
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
	if (!input_file) 
	{
		return NULL;
	}

	int fd = open(input_file, O_RDONLY);
	if (fd == -1) 
	{
		return NULL;
	}

	uint32_t num_pcb;
	if (read(fd, &num_pcb, sizeof(uint32_t)) != sizeof(uint32_t)) 
	{
		close(fd);
		return NULL;
	}

	// No destructor needed: elements are stored inline in the dyn_array
	dyn_array_t *array = dyn_array_create((size_t)num_pcb, sizeof(ProcessControlBlock_t), NULL);
	if (!array)
	{
		close(fd);
		return NULL;
	}

	while (dyn_array_size(array) < (size_t)num_pcb) 
	{
		uint32_t burst_time, priority_val, arrival_time;

		// Read the next triple, representing the next process control block
		if (read(fd, &burst_time, sizeof(uint32_t)) == sizeof(uint32_t) && 
				read(fd, &priority_val, sizeof(uint32_t)) == sizeof(uint32_t) &&
				read(fd, &arrival_time, sizeof(uint32_t)) == sizeof(uint32_t))
		{
			// Use a stack-allocated PCB; push_back will copy it into the array
			ProcessControlBlock_t block;
			block.remaining_burst_time = burst_time;
			block.priority = priority_val;
			block.arrival = arrival_time;
			block.started = false;

			if (!dyn_array_push_back(array, &block)) 
			{
				dyn_array_destroy(array);
				close(fd);
				return NULL;
			}
		} 
		else 
		{
			dyn_array_destroy(array);
			close(fd);
			return NULL;
		}
	}

	close(fd);
	return array;
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
