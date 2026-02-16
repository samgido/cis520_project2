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
	ASSERT_E(dyn_array_size(array), (size_t)4);

	// Verify each element
	for (size_t i = 0; i < dyn_array_size(array); ++i) 
	{
		ProcessControlBlock_t *block = (ProcessControlBlock_t *)dyn_array_at(array, i);

		ASSERT_NE(block, nullptr);
		ASSERT_EQ(block->started, false);
	}
}

TEST (load_process_control_blocks, IncorrectFilename)
{
	const char *query_filename = "";

	dyn_array_t *array = load_process_control_blocks(query_filename);

	ASSERT_EQ(array, nullptr);
}

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	// ::testing::AddGlobalTestEnvironment(new GradeEnvironment);
	return RUN_ALL_TESTS();
}
