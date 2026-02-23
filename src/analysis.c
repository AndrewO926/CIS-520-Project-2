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

	if(strcmp(algorithm, FCFS) == 0){
			bool success = first_come_first_serve(load_process_control_blocks(file), &times);
			if(!success){
					printf("first come first serve failed idk why");
					return EXIT_FAILURE;
			}
	}

	printf("Average waiting time: %f\n", times.average_waiting_time);
	printf("Average turnaround time: %f\n", times.average_waiting_time);
	printf("Total run time: %ld\n", times.total_run_time);

	return EXIT_SUCCESS;
}
