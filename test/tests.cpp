#include <fcntl.h>
#include <stdio.h>
#include <pthread.h>
#include "gtest/gtest.h"
#include "../include/processing_scheduling.h"

// Using a C library requires extern "C" to prevent function mangling
extern "C"
{
#include <dyn_array.h>
}

#define NUM_PCB 30
#define QUANTUM 5 // Used for Robin Round for process as the run time limit

/*
unsigned int score;
unsigned int total;

class GradeEnvironment : public testing::Environment
{
	public:
		virtual void SetUp()
		{
			score = 0;
			total = 210;
		}

		virtual void TearDown()
		{
			::testing::Test::RecordProperty("points_given", score);
			::testing::Test::RecordProperty("points_total", total);
			std::cout << "SCORE: " << score << '/' << total << std::endl;
		}
};
*/

TEST (load_process_control_blocks, CorrectFilename) 
{
	const char *query_filename = "pcb.bin"; 
	dyn_array_t *array = load_process_control_blocks(query_filename); 

	ASSERT_NE(array, nullptr); 
	ASSERT_EQ(dyn_array_size(array), (size_t)4);

	// Verify each element
	for (size_t i = 0; i < dyn_array_size(array); ++i) 
	{
		ProcessControlBlock_t *block = (ProcessControlBlock_t *)dyn_array_at(array, i);

		ASSERT_NE(block, nullptr);
		ASSERT_EQ(block->started, false);
	}

	dyn_array_destroy(array);
}

TEST (load_process_control_blocks, IncorrectFilename)
{
	const char *query_filename = "";

	dyn_array_t *array = load_process_control_blocks(query_filename);

	ASSERT_EQ(array, nullptr);
}

// FCFS Test 1: Verify correct scheduling with a known set of processes
TEST (first_come_first_serve, ValidProcesses)
{
	// Create 3 processes: (burst = 5, arrival = 0), (burst = 3, arrival = 1), (burst = 8, arrival = 2)
	// Expected timeline:
	//   t = 0-5:  P0 runs (wait = 0, turnaround = 5)
	//   t = 5-8:  P1 runs (wait = 4, turnaround = 7)
	//   t = 8-16: P2 runs (wait = 6, turnaround = 14)
	// Avg wait = (0+4+6)/3 = 3.33, Avg turnaround = (5+7+14)/3 = 8.67, Total = 16

	dyn_array_t *ready_queue = dyn_array_create(3, sizeof(ProcessControlBlock_t), NULL);
	ASSERT_NE(ready_queue, nullptr);

	ProcessControlBlock_t pcbs[3] = {
		{5, 0, 0, false},
		{3, 0, 1, false},
		{8, 0, 2, false}
	};

	dyn_array_push_back(ready_queue, &pcbs[0]);
	dyn_array_push_back(ready_queue, &pcbs[1]);
	dyn_array_push_back(ready_queue, &pcbs[2]);

	ScheduleResult_t result;
	bool success = first_come_first_serve(ready_queue, &result);

	ASSERT_EQ(success, true);
	EXPECT_NEAR(result.average_waiting_time, 3.33f, 0.1f);
	EXPECT_NEAR(result.average_turnaround_time, 8.67f, 0.1f);
	EXPECT_EQ(result.total_run_time, (unsigned long)16);

	dyn_array_destroy(ready_queue);
}

// FCFS Test 2: Verify that FCFS handles NULL inputs gracefully
TEST (first_come_first_serve, NullInputs)
{
	ScheduleResult_t result;
	dyn_array_t *ready_queue = dyn_array_create(3, sizeof(ProcessControlBlock_t), NULL);

	// Both NULL
	ASSERT_EQ(first_come_first_serve(NULL, NULL), false);

	// NULL queue
	ASSERT_EQ(first_come_first_serve(NULL, &result), false);

	// NULL result
	ASSERT_EQ(first_come_first_serve(ready_queue, NULL), false);

	dyn_array_destroy(ready_queue);
}

// SRT Test 1: Verify correct scheduling behavior
TEST(shortest_remaining_time_first, ValidProcesses)
{
	dyn_array_t* ready_queue = dyn_array_create(3, sizeof(ProcessControlBlock_t), NULL);
	ASSERT_NE(ready_queue, nullptr);

	ProcessControlBlock_t pcbs[3] = {
		{8, 0, 0, false},
		{4, 0, 1, false},
		{2, 0, 2, false}
	};

	dyn_array_push_back(ready_queue, &pcbs[0]);
	dyn_array_push_back(ready_queue, &pcbs[1]);
	dyn_array_push_back(ready_queue, &pcbs[2]);

	ScheduleResult_t result;
	bool success = shortest_remaining_time_first(ready_queue, &result);

	ASSERT_EQ(success, true);
	EXPECT_NEAR(result.average_waiting_time, 2.67f, 0.1f);
	EXPECT_NEAR(result.average_turnaround_time, 7.33f, 0.1f);
	EXPECT_EQ(result.total_run_time, (unsigned long)14);

	dyn_array_destroy(ready_queue);
}

// SRT Test 2: Verify NULL handling
TEST(shortest_remaining_time_first, NullInputs)
{
	ScheduleResult_t result;
	dyn_array_t* ready_queue = dyn_array_create(2, sizeof(ProcessControlBlock_t), NULL);

	ASSERT_EQ(shortest_remaining_time_first(NULL, NULL), false);
	ASSERT_EQ(shortest_remaining_time_first(NULL, &result), false);
	ASSERT_EQ(shortest_remaining_time_first(ready_queue, NULL), false);

	dyn_array_destroy(ready_queue);
}

// RR Test 1: Verify correct Round Robin scheduling
TEST(round_robin, ValidProcesses)
{

	dyn_array_t* ready_queue = dyn_array_create(2, sizeof(ProcessControlBlock_t), NULL);
	ASSERT_NE(ready_queue, nullptr);

	ProcessControlBlock_t pcbs[2] = {
		{5, 0, 0, false},
		{3, 0, 1, false}
	};

	dyn_array_push_back(ready_queue, &pcbs[0]);
	dyn_array_push_back(ready_queue, &pcbs[1]);

	ScheduleResult_t result;
	bool success = round_robin(ready_queue, &result, 2);

	ASSERT_EQ(success, true);
	EXPECT_NEAR(result.average_waiting_time, 0.5f, 0.1f);
	EXPECT_NEAR(result.average_turnaround_time, 7.0f, 0.1f);
	EXPECT_EQ(result.total_run_time, (unsigned long)8);

	dyn_array_destroy(ready_queue);
}

// RR Test 2: Verify NULL and invalid quantum handling
TEST(round_robin, NullInputs)
{
	ScheduleResult_t result;
	dyn_array_t* ready_queue = dyn_array_create(2, sizeof(ProcessControlBlock_t), NULL);

	ASSERT_EQ(round_robin(NULL, NULL, 2), false);
	ASSERT_EQ(round_robin(NULL, &result, 2), false);
	ASSERT_EQ(round_robin(ready_queue, NULL, 2), false);
	ASSERT_EQ(round_robin(ready_queue, &result, 0), false);

	dyn_array_destroy(ready_queue);
}

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	// ::testing::AddGlobalTestEnvironment(new GradeEnvironment);
	return RUN_ALL_TESTS();
}
