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

static ProcessControlBlock_t make_pcb(uint32_t arrival, uint32_t burst, uint32_t priority)
{
    ProcessControlBlock_t pcb;
    pcb.arrival = arrival;
    pcb.remaining_burst_time = burst;
    pcb.priority = priority;
    pcb.started = false;
    return pcb;
}

static dyn_array_t* make_queue(const ProcessControlBlock_t* pcbs, size_t n)
{
    dyn_array_t* q = dyn_array_create(n, sizeof(ProcessControlBlock_t), NULL);
    if (!q) return NULL;

    for (size_t i = 0; i < n; i++)
    {
        // push a COPY into the dyn_array
        if (!dyn_array_push_back(q, (void*)&pcbs[i]))
        {
            dyn_array_destroy(q);
            return NULL;
        }
    }
    return q;
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


//Test priority

TEST(priority, ChoosesHighestPriorityAmongArrived) {
    ProcessControlBlock_t pcbs[] = {
        make_pcb(0, 5, 10),
        make_pcb(0, 2,  1)
    };
    dyn_array_t* rq = make_queue(pcbs, 2);
    ASSERT_NE(nullptr, rq);

    ScheduleResult_t r;
    ASSERT_TRUE(priority(rq, &r));

    EXPECT_NEAR(r.total_run_time, 7.0f, 1e-5);
    EXPECT_NEAR(r.average_waiting_time, 1.0f, 1e-5);
    EXPECT_NEAR(r.average_turnaround_time, 4.5f, 1e-5);

    dyn_array_destroy(rq);
}

TEST(priority, NonPreemptiveHigherPriorityArrivesLater) {
    ProcessControlBlock_t pcbs[] = {
        make_pcb(0, 10, 5),
        make_pcb(3,  1, 1)
    };
    dyn_array_t* rq = make_queue(pcbs, 2);
    ASSERT_NE(nullptr, rq);

    ScheduleResult_t r;
    ASSERT_TRUE(priority(rq, &r));

    EXPECT_NEAR(r.total_run_time, 11.0f, 1e-5);
    EXPECT_NEAR(r.average_waiting_time, 3.5f, 1e-5);
    EXPECT_NEAR(r.average_turnaround_time, 9.0f, 1e-5);

    dyn_array_destroy(rq);
}


//Test SRTF
TEST(shortest_remaining_time_first, PreemptsWhenShorterArrives) {
    ProcessControlBlock_t pcbs[] = {
        make_pcb(0, 10, 5),
        make_pcb(3,  1, 5)
    };
    dyn_array_t* rq = make_queue(pcbs, 2);
    ASSERT_NE(nullptr, rq);

    ScheduleResult_t r;
    ASSERT_TRUE(shortest_remaining_time_first(rq, &r));

    EXPECT_NEAR(r.total_run_time, 11.0f, 1e-5);
    EXPECT_NEAR(r.average_turnaround_time, 6.0f, 1e-5);
    EXPECT_NEAR(r.average_waiting_time, 0.5f, 1e-5);

    dyn_array_destroy(rq);
}

TEST(shortest_remaining_time_first, HandlesIdleTime) {
    ProcessControlBlock_t pcbs[] = {
        make_pcb(5, 3, 1)
    };
    dyn_array_t* rq = make_queue(pcbs, 1);
    ASSERT_NE(nullptr, rq);

    ScheduleResult_t r;
    ASSERT_TRUE(shortest_remaining_time_first(rq, &r));

    EXPECT_NEAR(r.total_run_time, 8.0f, 1e-5);
    EXPECT_NEAR(r.average_turnaround_time, 3.0f, 1e-5);
    EXPECT_NEAR(r.average_waiting_time, 0.0f, 1e-5);

    dyn_array_destroy(rq);
}

//validation tests
TEST(priority, NullParams) {
    ScheduleResult_t r;
    ASSERT_FALSE(priority(nullptr, &r));
    ASSERT_FALSE(priority((dyn_array_t*)0x1, nullptr));
}

TEST(shortest_remaining_time_first, NullParams) {
    ScheduleResult_t r;
    ASSERT_FALSE(shortest_remaining_time_first(nullptr, &r));
    ASSERT_FALSE(shortest_remaining_time_first((dyn_array_t*)0x1, nullptr));
}

