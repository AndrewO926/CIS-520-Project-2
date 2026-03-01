#include <stdio.h>
#include <stdlib.h>

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
			printf("%s <pcb file> <schedule algorithm> [quantum]\n", argv[0]);
			return EXIT_FAILURE;
	}

	char* file = argv[1];
	char* algorithm = argv[2];

	ScheduleResult_t times;
	dyn_array_t* ready_queue = load_process_control_blocks(file);

	if(strcmp(algorithm, FCFS) == 0){
			bool success = first_come_first_serve(ready_queue, &times);
			if(!success){
					printf("first come first serve failed idk why");
					dyn_array_destroy(ready_queue);
					return EXIT_FAILURE;
			}
	}
	else if(strcmp(algorithm, RR) == 0){
		if(argc < 4){
			printf("not enough arguments for RR");
			dyn_array_destroy(ready_queue);
			return EXIT_FAILURE;
		}
		bool success = round_robin(ready_queue, &times, strtoul(argv[3], NULL, 10));
		if(!success){
				printf("round robin failed");
				dyn_array_destroy(ready_queue);
				return EXIT_FAILURE;
		}
	}
	else if(strcmp(algorithm, SJF) == 0)
	{
		bool success = shortest_job_first(ready_queue, &times);
		if (!success)
		{
			printf("Shortest job first failed\n");
			dyn_array_destroy(ready_queue);
			return EXIT_FAILURE;
		}
	}
	else if(strcmp(algorithm, P) == 0){
		bool success = priority(ready_queue, &times);
		if (!success)
		{
			printf("Shortest job first failed\n");
			dyn_array_destroy(ready_queue);
			return EXIT_FAILURE;
		}
	}
	else if(strcmp(algorithm, SRT) == 0){
		bool success = shortest_remaining_time_first(ready_queue, &times);
		if (!success)
		{
			printf("Shortest job first failed\n");
			dyn_array_destroy(ready_queue);
			return EXIT_FAILURE;
		}
	}

	printf("Average waiting time: %f\n", times.average_waiting_time);
	printf("Average turnaround time: %f\n", times.average_turnaround_time);
	printf("Total run time: %ld\n", times.total_run_time);

	dyn_array_destroy(ready_queue);
	return EXIT_SUCCESS;
}
