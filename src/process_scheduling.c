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

static int compare_remaining_burst_time(const void *a, const void *b)
{
	const ProcessControlBlock_t *pcb_a = (const ProcessControlBlock_t *)a;
	const ProcessControlBlock_t *pcb_b = (const ProcessControlBlock_t *)b;

	if (pcb_a->remaining_burst_time < pcb_b->remaining_burst_time) return -1;
	if (pcb_a->remaining_burst_time > pcb_b->remaining_burst_time) return 1;
	return 0;
}

static int compare_priority(const void *a, const void *b)
{
	const ProcessControlBlock_t *pcb_a = (const ProcessControlBlock_t *)a;
	const ProcessControlBlock_t *pcb_b = (const ProcessControlBlock_t *)b;

	if (pcb_a->priority < pcb_b->priority) return -1;
	if (pcb_a->priority > pcb_b->priority) return 1;
	return 0;
}

// If two processes have the same arrival, check their remaining burst time
static int compare_arrival_with_burst_fallback(const void *a, const void*b) 
{
	const int arrival_comp = compare_arrival_time(a, b);

	if (arrival_comp != 0)
		return arrival_comp;

	return compare_remaining_burst_time(a, b);
}

// If two processes have the same arrival, check their priority
static int compare_arrival_with_priority_fallback(const void *a, const void*b) 
{
	const int arrival_comp = compare_arrival_time(a, b);

	if (arrival_comp != 0)
		return arrival_comp;

	return compare_priority(a, b);
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
	if (!ready_queue || !result)
	{
		return false;
	}

	size_t num_processes = dyn_array_size(ready_queue);
	if (num_processes == 0)
	{
		return false;
	}

	unsigned long clock = 0;
	float total_wait_time = 0;
	float total_turnaround_time = 0;

	size_t completed = 0;

	dyn_array_sort(ready_queue, compare_remaining_burst_time);
	while (completed < num_processes) 
	{
		ProcessControlBlock_t *pcb = NULL;

		// In case no PCB have arrived, also keep track of 
		// the soonest arrival
		ProcessControlBlock_t *soonest_arrival = NULL;

		// Search for an arrived process in the queue sorted by burst
		for (size_t i = 0; i < dyn_array_size(ready_queue); ++i) 
		{
			ProcessControlBlock_t *p = (ProcessControlBlock_t *)dyn_array_at(ready_queue, i);
			if (!p)
			{
				return false;
			}

			// If this process has arrived, extract remove it from the queue
			// and stop searching
			if (p->arrival <= clock && p->remaining_burst_time > 0)
			{
				pcb = p;
				break;
			}

			if (!soonest_arrival || compare_arrival_with_burst_fallback(soonest_arrival, p) < 0)
			{
				soonest_arrival = p;
			}
		}

		if (pcb == NULL) // No arrived PCBs, take the soonest arriving process
		{
			pcb = soonest_arrival;
			clock = pcb->arrival;
		}

		unsigned long wait_time = clock - pcb->arrival;
		total_wait_time += (float)wait_time;

		pcb->started = true;
		while (pcb->remaining_burst_time > 0)
		{
			virtual_cpu(pcb);
			clock++;
		}
		completed++;

		unsigned long turnaround_time = clock - pcb->arrival;
		total_turnaround_time += (float)turnaround_time;
	}

	result->average_waiting_time = total_wait_time / (float)num_processes;
	result->average_turnaround_time = total_turnaround_time / (float)num_processes;
	result->total_run_time = clock;

	return true;
}

bool priority(dyn_array_t *ready_queue, ScheduleResult_t *result) 
{
	if (!ready_queue || !result)
	{
		return false;
	}

	size_t num_processes = dyn_array_size(ready_queue);
	if (num_processes == 0)
	{
		return false;
	}

	unsigned long clock = 0;
	float total_wait_time = 0;
	float total_turnaround_time = 0;

	size_t completed = 0;

	dyn_array_sort(ready_queue, compare_priority);
	while (completed < num_processes) 
	{
		ProcessControlBlock_t *pcb = NULL;

		// In case no PCB have arrived, also keep track of 
		// the soonest arrival
		ProcessControlBlock_t *soonest_arrival = NULL;

		// Search for an arrived process in the queue sorted by priority
		for (size_t i = 0; i < dyn_array_size(ready_queue); ++i) 
		{
			ProcessControlBlock_t *p = (ProcessControlBlock_t *)dyn_array_at(ready_queue, i);
			if (!p)
			{
				return false;
			}

			// If this process has arrived, extract remove it from the queue
			// and stop searching
			if (p->arrival <= clock && p->remaining_burst_time > 0)
			{
				pcb = p;
				break;
			}

			if (!soonest_arrival || compare_arrival_with_priority_fallback(soonest_arrival, p) < 0) 
			{
				soonest_arrival = p;
			}
		}

		if (pcb == NULL) // No arrived PCBs, take the soonest arriving process
		{
			pcb = soonest_arrival;
			clock = pcb->arrival;
		}

		unsigned long wait_time = clock - pcb->arrival;
		total_wait_time += (float)wait_time;

		pcb->started = true;
		while (pcb->remaining_burst_time > 0)
		{
			virtual_cpu(pcb);
			clock++;
		}
		completed++;

		unsigned long turnaround_time = clock - pcb->arrival;
		total_turnaround_time += (float)turnaround_time;
	}

	result->average_waiting_time = total_wait_time / (float)num_processes;
	result->average_turnaround_time = total_turnaround_time / (float)num_processes;
	result->total_run_time = clock;

	return true;
}

bool round_robin(dyn_array_t *ready_queue, ScheduleResult_t *result, size_t quantum) 
{
	if (!ready_queue || !result || quantum == 0)
		return false;

	size_t n = dyn_array_size(ready_queue);
	if (n == 0)
		return false;

	unsigned long clock = 0;
	size_t completed = 0;

	float total_wait_time = 0;
	float total_turnaround_time = 0;

	while (completed < n)
	{
		bool progress_made = false;

		for (size_t i = 0; i < n; i++)
		{
			ProcessControlBlock_t* pcb = dyn_array_at(ready_queue, i);
			if (!pcb)
				return false;

			if (pcb->remaining_burst_time > 0 && pcb->arrival <= clock)
			{
				progress_made = true;

				if (!pcb->started)
				{
					total_wait_time += (float)(clock - pcb->arrival);
					pcb->started = true;
				}

				size_t time_slice = 0;

				while (time_slice < quantum && pcb->remaining_burst_time > 0)
				{
					virtual_cpu(pcb);
					clock++;
					time_slice++;
				}

				if (pcb->remaining_burst_time == 0)
				{
					completed++;
					total_turnaround_time += (float)(clock - pcb->arrival);
				}
			}
		}

		// If no process was ready, advance clock
		if (!progress_made)
			clock++;
	}

	result->average_waiting_time = total_wait_time / (float)n;
	result->average_turnaround_time = total_turnaround_time / (float)n;
	result->total_run_time = clock;

	return true;
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
	if (!ready_queue || !result)
		return false;

	size_t n = dyn_array_size(ready_queue);
	if (n == 0)
		return false;

	unsigned long clock = 0;
	size_t completed = 0;

	float total_wait_time = 0;
	float total_turnaround_time = 0;

	unsigned long* original_bursts = malloc(sizeof(unsigned long) * n);
	if (!original_bursts) return false;

	for (size_t i = 0; i < n; i++)
	{
		ProcessControlBlock_t* pcb = dyn_array_at(ready_queue, i);
		original_bursts[i] = pcb->remaining_burst_time;
	}

	while (completed < n)
	{
		ProcessControlBlock_t* shortest = NULL;
		size_t shortest_index = 0;

		// Find process with smallest remaining time among the current processes
		for (size_t i = 0; i < n; i++)
		{
			ProcessControlBlock_t* pcb = dyn_array_at(ready_queue, i);
			if (!pcb) {
				free(original_bursts);
				return false;
			}

			if (pcb->arrival <= clock && pcb->remaining_burst_time > 0)
			{
				if (!shortest ||
					pcb->remaining_burst_time < shortest->remaining_burst_time)
				{
					shortest = pcb;
					shortest_index = i;
				}
			}
		}

		// If no process is ready advance clock
		if (!shortest)
		{
			clock++;
			continue;
		}

		// First time scheduled: compute waiting time
		if (!shortest->started)
		{
			total_wait_time += (float)(clock - shortest->arrival);
			shortest->started = true;
		}

		// Run for 1 time unit
		virtual_cpu(shortest);
		clock++;

		// If finished
		if (shortest->remaining_burst_time == 0)
		{
			completed++;
			unsigned long turnaround = clock - shortest->arrival;
			total_turnaround_time += (float)turnaround;

			unsigned long burst = original_bursts[shortest_index];
			total_wait_time += (float)(turnaround - burst);
		}
	}

	result->average_waiting_time = total_wait_time / (float)n;
	result->average_turnaround_time = total_turnaround_time / (float)n;
	result->total_run_time = clock;

	free(original_bursts);
		
	return true;
}

void process_control_block_destruct(void *element) 
{ 
	free(element);
}
