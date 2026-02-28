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
	ASSERT_FALSE(first_come_first_serve(ptr, nullptr)) << "FCFS method should fail on null result";
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

TEST(first_come_first_serve, CalculationCheck1) {
  ProcessControlBlock_t one;
	one.arrival = 0;
	one.remaining_burst_time = 5;
	one.started = false;
	one.priority = 1;

	ProcessControlBlock_t two;
	two.arrival = 0;
	two.remaining_burst_time = 3;
	two.started = false;
	two.priority = 1;

	ProcessControlBlock_t three;
	three.arrival = 0;
	three.remaining_burst_time = 8;
	three.started = false;
	three.priority = 1;
  
  dyn_array_t *ptr = dyn_array_create(3, sizeof(ProcessControlBlock_t), NULL);
	dyn_array_push_back(ptr, &one);
	dyn_array_push_back(ptr, &two);
	dyn_array_push_back(ptr, &three);

	ScheduleResult_t times;
	ASSERT_TRUE(first_come_first_serve(ptr, &times)) << "fcfs was false expected true";
	ASSERT_NEAR(9.67, times.average_turnaround_time, .01);
	ASSERT_NEAR(4.33, times.average_waiting_time, .01);
  ASSERT_NEAR(16, times.total_run_time, .01);
	dyn_array_destroy(ptr);
}

TEST(first_come_first_serve, CalculationCheck2) {
  ProcessControlBlock_t one;
	one.arrival = 2;
	one.remaining_burst_time = 5;
	one.started = false;
	one.priority = 1;

	ProcessControlBlock_t two;
	two.arrival = 0;
	two.remaining_burst_time = 3;
	two.started = false;
	two.priority = 1;

	ProcessControlBlock_t three;
	three.arrival = 4;
	three.remaining_burst_time = 4;
	three.started = false;
	three.priority = 1;
  
  dyn_array_t *ptr = dyn_array_create(3, sizeof(ProcessControlBlock_t), NULL);
	dyn_array_push_back(ptr, &one);
	dyn_array_push_back(ptr, &two);
	dyn_array_push_back(ptr, &three);

	ScheduleResult_t times;
	ASSERT_TRUE(first_come_first_serve(ptr, &times)) << "fcfs was false expected true";
	ASSERT_NEAR(5.67, times.average_turnaround_time, .01);
	ASSERT_NEAR(1.67, times.average_waiting_time, .01);
  ASSERT_NEAR(12, times.total_run_time, .01);
	dyn_array_destroy(ptr);
}

//Test round robin
TEST(round_robin, emptyQueue) {
	ScheduleResult_t times;
	ASSERT_FALSE(round_robin(nullptr, &times, 10)) << "should return false for empty queue";
}
TEST(round_robin, emptySchedule) {
	const char filename[] = "../pcb.bin";
	dyn_array_t* ptr = load_process_control_blocks(filename);
	ASSERT_NE(nullptr, ptr) << "process control block method failed or Andrew fucked up the filename or the file doesn't exist";
	ASSERT_FALSE(round_robin(ptr, nullptr, 10)) << "RR method should fail on null result";
	dyn_array_destroy(ptr);
}

TEST(round_robin, noTimeQuantum) {
	const char filename[] = "../pcb.bin";
	dyn_array_t *ptr = load_process_control_blocks(filename);
	ASSERT_NE(nullptr, ptr) << "process control block method failed or Andrew fucked up the filename or the file doesn't exist";

	ScheduleResult_t times;
	ASSERT_FALSE(round_robin(ptr, &times, 0)) << "RR was true expected false but didn't get it";
	dyn_array_destroy(ptr);
}

TEST(round_robin, ValidInput){
	const char filename[] = "../pcb.bin";
	dyn_array_t *ptr = load_process_control_blocks(filename);
	ASSERT_NE(nullptr, ptr) << "process control block method failed or Andrew fucked up the filename or the file doesn't exist";

	ScheduleResult_t times;
	ASSERT_TRUE(round_robin(ptr, &times, 10)) << "RR was false expected true but didn't get it";
	dyn_array_destroy(ptr);
}

TEST(round_robin, CalculationCheck1){
	ProcessControlBlock_t one;
	one.arrival = 0;
	one.remaining_burst_time = 5;
	one.started = false;
	one.priority = 1;

	ProcessControlBlock_t two;
	two.arrival = 4;
	two.remaining_burst_time = 2;
	two.started = false;
	two.priority = 1;

	ProcessControlBlock_t three;
	three.arrival = 5;
	three.remaining_burst_time = 4;
	three.started = false;
	three.priority = 1;

	dyn_array_t *ptr = dyn_array_create(3, sizeof(ProcessControlBlock_t), NULL);
	dyn_array_push_back(ptr, &one);
	dyn_array_push_back(ptr, &two);
	dyn_array_push_back(ptr, &three);

	ScheduleResult_t times;
	ASSERT_TRUE(round_robin(ptr, &times, 2)) << "RR was false expected true but didn't get it";
	ASSERT_EQ(5, times.average_turnaround_time);
	ASSERT_NEAR(1.33, times.average_waiting_time, .01);
	dyn_array_destroy(ptr);
}

TEST(round_robin, CalculationCheck2){
	ProcessControlBlock_t one;
	one.arrival = 0;
	one.remaining_burst_time = 4;
	one.started = false;
	one.priority = 1;

	ProcessControlBlock_t two;
	two.arrival = 1;
	two.remaining_burst_time = 5;
	two.started = false;
	two.priority = 1;

	ProcessControlBlock_t three;
	three.arrival = 2;
	three.remaining_burst_time = 3;
	three.started = false;
	three.priority = 1;

	dyn_array_t *ptr = dyn_array_create(3, sizeof(ProcessControlBlock_t), NULL);
	dyn_array_push_back(ptr, &one);
	dyn_array_push_back(ptr, &two);
	dyn_array_push_back(ptr, &three);

	ScheduleResult_t times;
	ASSERT_TRUE(round_robin(ptr, &times, 2)) << "RR was false expected true but didn't get it";
	ASSERT_NEAR(9.33, times.average_turnaround_time, .01);
	ASSERT_NEAR(5.33, times.average_waiting_time, .01);
	dyn_array_destroy(ptr);
}

//Test shortest job first
TEST(shortest_job_first, sjfEmptyQueue) {
	ScheduleResult_t times;
	ASSERT_FALSE(shortest_job_first(nullptr, &times)) << "should return false for empty queue";
}

TEST(shortest_job_first, sjfNullSchedule) {
  ProcessControlBlock_t one;
	one.arrival = 0;
	one.remaining_burst_time = 4;
	one.started = false;
	one.priority = 1;

	ProcessControlBlock_t two;
	two.arrival = 1;
	two.remaining_burst_time = 5;
	two.started = false;
	two.priority = 1;

	ProcessControlBlock_t three;
	three.arrival = 2;
	three.remaining_burst_time = 3;
	three.started = false;
	three.priority = 1;
  
  dyn_array_t *ptr = dyn_array_create(3, sizeof(ProcessControlBlock_t), NULL);
	dyn_array_push_back(ptr, &one);
	dyn_array_push_back(ptr, &two);
	dyn_array_push_back(ptr, &three);

	ASSERT_FALSE(shortest_job_first(ptr, nullptr)) << "should fail on null schedule pointer";
	dyn_array_destroy(ptr);
}

TEST(shortest_job_first, CalculationCheck1){
	ProcessControlBlock_t one;
	one.arrival = 2;
	one.remaining_burst_time = 6;
	one.started = false;
	one.priority = 1;

	ProcessControlBlock_t two;
	two.arrival = 5;
	two.remaining_burst_time = 2;
	two.started = false;
	two.priority = 1;

	ProcessControlBlock_t three;
	three.arrival = 1;
	three.remaining_burst_time = 8;
	three.started = false;
	three.priority = 1;
  
  ProcessControlBlock_t four;
  four.arrival = 0;
  four.remaining_burst_time = 3;
  four.started = false;
  four.priority = 1;
  
  ProcessControlBlock_t five;
  five.arrival = 4;
  five.remaining_burst_time = 4;
  five.started = false;
  five.priority = 1;
  

	dyn_array_t *ptr = dyn_array_create(3, sizeof(ProcessControlBlock_t), NULL);
	dyn_array_push_back(ptr, &one);
	dyn_array_push_back(ptr, &two);
	dyn_array_push_back(ptr, &three);
  dyn_array_push_back(ptr, &four);
  dyn_array_push_back(ptr, &five);

	ScheduleResult_t times;
	ASSERT_TRUE(shortest_job_first(ptr, &times)) << "sjf was false, expected true";
	ASSERT_NEAR(9.8, times.average_turnaround_time, .01);
	ASSERT_NEAR(5.2, times.average_waiting_time, .01);
  ASSERT_NEAR(23, times.total_run_time, .01);
	dyn_array_destroy(ptr);
}