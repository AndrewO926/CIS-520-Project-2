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


int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	::testing::AddGlobalTestEnvironment(new GradeEnvironment);
	return RUN_ALL_TESTS();
}

// Test load processing
TEST(load_processing, EmptyInput) {
	dyn_array_t *ptr = load_process_control_blocks(nullptr);
	ASSERT_EQ(nullptr, ptr);
}

TEST(load_processing, ValidInput) {
	const char filename[] = "../pcb.bin";
	dyn_array_t *ptr = load_process_control_blocks(filename);
	ASSERT_NE(nullptr, ptr);
	EXPECT_FALSE(dyn_array_empty(ptr));
	dyn_array_destroy(ptr);
}

//Test first come first serve
TEST(first_come_first_serve, emptyQueue) {
	ScheduleResult_t times;
	ASSERT_FALSE(first_come_first_serve(nullptr, &times)) << "should return false for empty queue";
}
TEST(first_come_first_serve, emptySchedule) {
	const char filename[] = "../pcb.bin";
	dyn_array_t* ptr = load_process_control_blocks(filename);
	ASSERT_NE(nullptr, ptr) << "process control block method failed or Andrew fucked up the filename or the file doesn't exist";
	ASSERT_FALSE(first_come_first_serve(ptr, nullptr)) << "pcb method should fail on null result";
	dyn_array_destroy(ptr);
}
TEST(first_come_first_serve, ValidInput){
	const char filename[] = "../pcb.bin";
	dyn_array_t *ptr = load_process_control_blocks(filename);
	ASSERT_NE(nullptr, ptr) << "process control block method failed or Andrew fucked up the filename or the file doesn't exist";

	ScheduleResult_t times;
	ASSERT_TRUE(first_come_first_serve(ptr, &times)) << "FCFS was false expected true but didn't get it";
	dyn_array_destroy(ptr);
}