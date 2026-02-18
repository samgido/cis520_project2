#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dyn_array.h"
#include "processing_scheduling.h"

#define FCFS "FCFS"
#define P "P"
#define RR "RR"
#define SJF "SJF"
#define SRT "SRT"

// Add and comment your analysis code in this function.
// THIS IS NOT FINISHED.
int main(int argc, char **argv) 
{
	if (argc < 3) 
	{
		printf("Usage: %s <pcb file> <schedule algorithm> [quantum]\n", argv[0]);
		return EXIT_FAILURE;
	}

	const char *pcb_file = argv[1];
	const char *algorithm = argv[2];

	// Load process control blocks from the binary file
	dyn_array_t *ready_queue = load_process_control_blocks(pcb_file);
	if (!ready_queue)
	{
		fprintf(stderr, "Error: Failed to load process control blocks from '%s'\n", pcb_file);
		return EXIT_FAILURE;
	}

	ScheduleResult_t result;
	bool success = false;

	// Dispatch to the appropriate scheduling algorithm
	if (strncmp(algorithm, FCFS, 5) == 0)
	{
		success = first_come_first_serve(ready_queue, &result);
	}
	else if (strncmp(algorithm, SJF, 4) == 0)
	{
		success = shortest_job_first(ready_queue, &result);
	}
	else if (strncmp(algorithm, P, 2) == 0)
	{
		success = priority(ready_queue, &result);
	}
	else if (strncmp(algorithm, RR, 3) == 0)
	{
		if (argc < 4)
		{
			fprintf(stderr, "Error: Round Robin requires a time quantum argument\n");
			dyn_array_destroy(ready_queue);
			return EXIT_FAILURE;
		}
		size_t quantum = 0;
		if (sscanf(argv[3], "%zu", &quantum) != 1 || quantum == 0)
		{
			fprintf(stderr, "Error: Invalid time quantum '%s'\n", argv[3]);
			dyn_array_destroy(ready_queue);
			return EXIT_FAILURE;
		}
		success = round_robin(ready_queue, &result, quantum);
	}
	else if (strncmp(algorithm, SRT, 4) == 0)
	{
		success = shortest_remaining_time_first(ready_queue, &result);
	}
	else
	{
		fprintf(stderr, "Error: Unknown scheduling algorithm '%s'\n", algorithm);
		fprintf(stderr, "Valid options: FCFS, SJF, P, RR, SRT\n");
		dyn_array_destroy(ready_queue);
		return EXIT_FAILURE;
	}

	if (success)
	{
		printf("Algorithm: %s\n", algorithm);
		printf("Average Waiting Time: %.2f\n", result.average_waiting_time);
		printf("Average Turnaround Time: %.2f\n", result.average_turnaround_time);
		printf("Total Clock Time: %lu\n", result.total_run_time);
	}
	else
	{
		fprintf(stderr, "Error: Scheduling algorithm '%s' failed\n", algorithm);
		dyn_array_destroy(ready_queue);
		return EXIT_FAILURE;
	}

	// Clean up
	dyn_array_destroy(ready_queue);

	return EXIT_SUCCESS;
}
