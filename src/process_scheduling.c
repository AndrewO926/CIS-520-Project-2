#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "dyn_array.h"
#include "processing_scheduling.h"


// You might find this handy.  I put it around unused parameters, but you should
// remove it before you submit. Just allows things to compile initially.
#define UNUSED(x) (void)(x)

static int compare_arrival(const void* a, const void* b)
{
	const ProcessControlBlock_t* p1 = (const ProcessControlBlock_t*)a;
	const ProcessControlBlock_t* p2 = (const ProcessControlBlock_t*)b;

	if (p1->arrival < p2->arrival) { return -1; }
	if (p1->arrival > p2->arrival) { return 1; }
	return 0;
}

// private function
void virtual_cpu(ProcessControlBlock_t *process_control_block) 
{
	// decrement the burst time of the pcb
	--process_control_block->remaining_burst_time;
}

bool first_come_first_serve(dyn_array_t *ready_queue, ScheduleResult_t *result) 
{
	if (ready_queue == NULL || result == NULL) { return false; }
	else if (dyn_array_empty(ready_queue) || dyn_array_data_size(ready_queue) == 0) { return false; }
	size_t number_of_processes = dyn_array_size(ready_queue);

	dyn_array_sort(ready_queue, compare_arrival); // Sort ready_queue by arrival time

	// Calculate and populate ScheduleResult_t fields
	float total_waiting_time = 0;
	float total_turnaround_time = 0;
	float current_time = 0;
	for (size_t i = 0; i < number_of_processes; i++)
	{
		ProcessControlBlock_t* pcb = (ProcessControlBlock_t*)dyn_array_at(ready_queue, i);
		pcb->started = true;
		if (current_time < pcb->arrival) { current_time = pcb->arrival; }
		total_waiting_time += current_time - pcb->arrival;
		total_turnaround_time += current_time - pcb->arrival + pcb->remaining_burst_time;
		current_time += pcb->remaining_burst_time;
		pcb->remaining_burst_time = 0;
	}

	result->average_waiting_time = total_waiting_time / number_of_processes;
	result->average_turnaround_time = total_turnaround_time / number_of_processes;
	result->total_run_time = current_time;

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
	if (input_file == NULL) return NULL;
 
    int fd = open(input_file, O_RDONLY);
    if (fd < 0) return NULL;
 
    uint32_t n = 0;
    if (!read_exact(fd, &n, sizeof(uint32_t)))
    {
        close(fd);
        return NULL;
    }
 
    dyn_array_t *arr = dyn_array_create(n, sizeof(ProcessControlBlock_t), NULL);
    if (arr == NULL)
    {
        close(fd);
        return NULL;
    }
 
    for (uint32_t i = 0; i < n; i++)
    {
        ProcessControlBlock_t pcb;
 
        if (!read_exact(fd, &pcb.remaining_burst_time, sizeof(uint32_t)) ||
            !read_exact(fd, &pcb.priority,            sizeof(uint32_t)) ||
            !read_exact(fd, &pcb.arrival,             sizeof(uint32_t)))
        {
            dyn_array_destroy(arr);
            close(fd);
            return NULL;
        }
 
        pcb.started = false;
 
        if (!dyn_array_push_back(arr, &pcb))
        {
            dyn_array_destroy(arr);
            close(fd);
            return NULL;
        }
    }
 
    close(fd);
    return arr;
}

bool shortest_remaining_time_first(dyn_array_t *ready_queue, ScheduleResult_t *result) 
{
	UNUSED(ready_queue);
	UNUSED(result);
	return false;
}