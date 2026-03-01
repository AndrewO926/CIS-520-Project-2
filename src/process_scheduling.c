#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include "dyn_array.h"
#include "processing_scheduling.h"

static bool read_exact(int fd, void *buffer, size_t bytes);


// You might find this handy.  I put it around unused parameters, but you should
// remove it before you submit. Just allows things to compile initially.
#define UNUSED(x) (void)(x)

// Compares the arrival time of one pcb with another. results in increasing order
static int compare_arrival_ascending(const void* a, const void* b)
{
	int result1;
	int result2;

	if (a == NULL) { result1 = 0; }
	else
	{
		const ProcessControlBlock_t* p1 = (const ProcessControlBlock_t*)a;
		result1 = p1->arrival;
	}

	if (b == NULL) { result2 = 0; }
	else
	{
		const ProcessControlBlock_t* p2 = (const ProcessControlBlock_t*)b;
		result2 = p2->arrival;
	}
	
	if (result1 > result2) { return 1; }
	if (result1 < result2) {return -1; }
	else { return 0; }
}

static int compare_arrival_descending(const void* a, const void* b)
{
	return -1 * compare_arrival_ascending(a, b);
}

// Compares the remaining burst time of one pcb with another. results in increasing order
static int compare_remaining_time(const void* a, const void* b)
{
	int result1;
	int result2;

	if (a == NULL) { result1 = 0; }
	else
	{
		const ProcessControlBlock_t* p1 = (const ProcessControlBlock_t*)a;
		result1 = p1->remaining_burst_time;
	}

	if (b == NULL) { result2 = 0; }
	else
	{
		const ProcessControlBlock_t* p2 = (const ProcessControlBlock_t*)b;
		result2 = p2->remaining_burst_time;
	}

	if (result1 > result2) { return 1; }
	if (result1 < result2) { return -1; }
	else { return 0; }
}

//I needed a way to requeue objects for round robin where it factors in if the object has arrived or not.
//Basically, this should place the element in front of the stuff that isn't here yet and behind the stuff that is
static void requeue_correctly(dyn_array_t* array, size_t array_size, ProcessControlBlock_t* insertElement, float curTime){
    if(array_size == 0){ 
        dyn_array_push_back(array, insertElement);
        return; 
    }
    for(size_t i = 0; i < array_size; i++){
        ProcessControlBlock_t* ele = (ProcessControlBlock_t*)dyn_array_at(array, i);
        //If the element is already here and in queue, place element behind it
        if(ele->arrival <= curTime){
            dyn_array_insert(array, i, insertElement);
            return;
        }
    }
    dyn_array_push_back(array, insertElement);
}

// private function
void virtual_cpu(ProcessControlBlock_t *process_control_block) 
{
	// decrement the burst time of the pcb
	--process_control_block->remaining_burst_time;
}

static bool read_exact(int fd, void *buffer, size_t bytes)
{
    size_t total = 0;
    ssize_t n;

    while (total < bytes) {
        n = read(fd, (char*)buffer + total, bytes - total);
        if (n <= 0) {
            return false;  // error or EOF
        }
        total += (size_t)n;
    }

    return true;
} 
bool first_come_first_serve(dyn_array_t *ready_queue, ScheduleResult_t *result) 
{
	if (ready_queue == NULL || result == NULL) { return false; }
	else if (dyn_array_empty(ready_queue) || dyn_array_data_size(ready_queue) == 0) { return false; }
	size_t number_of_processes = dyn_array_size(ready_queue);

	dyn_array_sort(ready_queue, compare_arrival_descending); // Sort ready_queue by decreasing arrival time
	// This is done because popping from the back is less expensive than the front

	// Calculate and populate ScheduleResult_t fields
	float total_waiting_time = 0;
	float total_turnaround_time = 0;
	float current_time = 0;

	while (!dyn_array_empty(ready_queue))
	{
		ProcessControlBlock_t pcb;
		if (!dyn_array_extract_back(ready_queue, &pcb)) { return false; }

		// If the next process hasn't arrived yet, wait for it
		if (current_time < pcb.arrival) { current_time = pcb.arrival; }

		total_waiting_time += current_time - pcb.arrival;
		total_turnaround_time += current_time - pcb.arrival + pcb.remaining_burst_time;
		current_time += pcb.remaining_burst_time;
	}

	result->average_waiting_time = total_waiting_time / number_of_processes;
	result->average_turnaround_time = total_turnaround_time / number_of_processes;
	result->total_run_time = current_time;

	return true;
}

bool shortest_job_first(dyn_array_t *ready_queue, ScheduleResult_t *result) 
{
	if (ready_queue == NULL || result == NULL) { return false; }
	else if (dyn_array_empty(ready_queue) || dyn_array_data_size(ready_queue) == 0) { return false; }
	
	if (!dyn_array_sort(ready_queue, compare_remaining_time)) { return false; } // Sort ready_queue by increasing remaining burst time

	size_t number_of_processes = dyn_array_size(ready_queue);
	float total_waiting_time = 0;
	float total_turnaround_time = 0;
	float current_time = 0;
	bool process_ran;
	uint32_t earliest_arrival;

	while (!dyn_array_empty(ready_queue))
	{
		process_ran = false;
		earliest_arrival = UINT32_MAX;
		for (size_t i = 0; i < dyn_array_size(ready_queue); i++)
		{
			ProcessControlBlock_t* pcb = (ProcessControlBlock_t*)dyn_array_at(ready_queue, i);
			if (pcb == NULL) { return false; }
			earliest_arrival = earliest_arrival < pcb->arrival ? earliest_arrival : pcb->arrival;

			// A process is ready and can be run
			if (pcb->arrival <= current_time)
			{
				total_waiting_time += current_time - pcb->arrival;
				total_turnaround_time += current_time - pcb->arrival + pcb->remaining_burst_time;
				current_time += pcb->remaining_burst_time;
				process_ran = true;
				if (!dyn_array_erase(ready_queue, i)) { return false; }
				break;
			}
		}

		// A process wasn't run, therefore no processes have arrived. Skip ahead
		if (!process_ran)
		{
			current_time = earliest_arrival;
		}
	}
	result->average_waiting_time = total_waiting_time / number_of_processes;
	result->average_turnaround_time = total_turnaround_time / number_of_processes;
	result->total_run_time = current_time;

	return true;
}

bool priority(dyn_array_t *ready_queue, ScheduleResult_t *result) 
{
    // 1) Basic parameter validation (matches your FCFS/RR style)
    if (ready_queue == NULL || result == NULL) { return false; }
    else if (dyn_array_empty(ready_queue) || dyn_array_data_size(ready_queue) == 0) { return false; }

    size_t number_of_processes = dyn_array_size(ready_queue);

    float total_waiting_time = 0.0f;
    float total_turnaround_time = 0.0f;
    float current_time = 0.0f;
    size_t finished = 0;

    // 2) Keep scheduling until every process has remaining_burst_time == 0
    while (finished < number_of_processes)
    {
        int best_index = -1;

        // 3) Find the "best" process that has arrived and is not finished
        for (size_t i = 0; i < number_of_processes; i++)
        {
            ProcessControlBlock_t *pcb = (ProcessControlBlock_t*)dyn_array_at(ready_queue, i);

            // Skip finished processes
            if (pcb->remaining_burst_time == 0) { continue; }

            // Skip processes that haven't arrived yet
            if (pcb->arrival > current_time) { continue; }

            // First candidate becomes the best by default
            if (best_index == -1)
            {
                best_index = (int)i;
            }
            else
            {
                ProcessControlBlock_t *best = (ProcessControlBlock_t*)dyn_array_at(ready_queue, (size_t)best_index);

                // Choose highest priority (lowest number)
                // Tie-breakers make results deterministic for tests:
                // 1) earlier arrival
                // 2) smaller burst
                // 3) lower index
                if (pcb->priority < best->priority ||
                    (pcb->priority == best->priority && pcb->arrival < best->arrival) ||
                    (pcb->priority == best->priority && pcb->arrival == best->arrival && pcb->remaining_burst_time < best->remaining_burst_time) ||
                    (pcb->priority == best->priority && pcb->arrival == best->arrival && pcb->remaining_burst_time == best->remaining_burst_time && i < (size_t)best_index))
                {
                    best_index = (int)i;
                }
            }
        }

        // 4) If nothing is available, CPU is idle.
        // Jump current_time to the next arrival among unfinished processes.
        if (best_index == -1)
        {
            uint32_t next_arrival = UINT32_MAX;

            for (size_t i = 0; i < number_of_processes; i++)
            {
                ProcessControlBlock_t *pcb = (ProcessControlBlock_t*)dyn_array_at(ready_queue, i);
                if (pcb->remaining_burst_time == 0) { continue; }
                if (pcb->arrival < next_arrival) { next_arrival = pcb->arrival; }
            }

            current_time = (float)next_arrival;
            continue;
        }

        // 5) Run the chosen process to completion (non-preemptive)
        ProcessControlBlock_t *run = (ProcessControlBlock_t*)dyn_array_at(ready_queue, (size_t)best_index);
        run->started = true;

        // If CPU is behind the arrival time, fast-forward to arrival
        if (current_time < run->arrival) { current_time = (float)run->arrival; }

        float start_time = current_time;
        float burst = (float)run->remaining_burst_time;
        float finish_time = start_time + burst;

        // Waiting time = time spent waiting in ready queue before starting
        total_waiting_time += start_time - (float)run->arrival;

        // Turnaround time = completion - arrival
        total_turnaround_time += finish_time - (float)run->arrival;

        // Advance clock and mark process finished
        current_time = finish_time;
        run->remaining_burst_time = 0;

        finished++;
    }

    result->average_waiting_time = total_waiting_time / (float)number_of_processes;
    result->average_turnaround_time = total_turnaround_time / (float)number_of_processes;
    result->total_run_time = current_time;

    return true;
}
bool round_robin(dyn_array_t *ready_queue, ScheduleResult_t *result, size_t quantum) 
{
    if (ready_queue == NULL || result == NULL || quantum == 0) { return false; }
	else if (dyn_array_empty(ready_queue) || dyn_array_data_size(ready_queue) == 0) { return false; }
	size_t number_of_processes = dyn_array_size(ready_queue);

    dyn_array_sort(ready_queue, compare_arrival_descending); // Sort ready_queue in reverse by arrival time

	float total_turnaround_time = 0;
    float total_burst_time = 0;
	float current_time = 0;

    // We want to iterate and run each process until all processes are done
    while(!dyn_array_empty(ready_queue)){
        ProcessControlBlock_t pcb;
        dyn_array_extract_back(ready_queue, &pcb);

        //rev our engines (start)
        pcb.started = true;

        //Calculate wait time
        if(pcb.arrival > current_time) { current_time = pcb.arrival; }

        // Variable to check if the current process needs to run again
        bool keepRunning = true;

        while(keepRunning){
            // If the burst time is lower than the quantum, use burst time
            if(pcb.remaining_burst_time <= quantum){

                current_time += pcb.remaining_burst_time;

                //Need this to calculate waiting time
                total_burst_time += pcb.remaining_burst_time;

                //Turnaround should just be total time since it arrived
                total_turnaround_time += current_time - pcb.arrival;

                pcb.started = false;
                keepRunning = false;
            } else {
                //Increment current_time, total burst time, and decrement remaining burst
                current_time += quantum;
                total_burst_time += quantum;
                pcb.remaining_burst_time -= quantum;

                // Need to check array size so as to not grab at an empty array
                size_t arr_size = dyn_array_size(ready_queue);
                if(arr_size > 0) {
                    //Check if anyone is waiting to go.
                    ProcessControlBlock_t* nextEle = (ProcessControlBlock_t*)dyn_array_at(ready_queue, arr_size - 1);
                    if(nextEle->arrival <= current_time){
                        pcb.started = false;
                        //Add node back to the front of the queue
                        requeue_correctly(ready_queue, arr_size, &pcb, current_time);
                        keepRunning = false;
                    }
                }
            }
        }
    }

	result->average_turnaround_time = total_turnaround_time / number_of_processes;
    //Calculate wait time based off total turnaround and burst time
    result->average_waiting_time = (total_turnaround_time - total_burst_time) / number_of_processes;
	result->total_run_time = current_time;
    
	return true;
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
    if (ready_queue == NULL || result == NULL) { return false; }
    else if (dyn_array_empty(ready_queue) || dyn_array_data_size(ready_queue) == 0) { return false; }

    size_t number_of_processes = dyn_array_size(ready_queue);

    // 1) Total original burst time (BEFORE we start decrementing)
    float total_burst_time = 0.0f;
    for (size_t i = 0; i < number_of_processes; i++)
    {
        ProcessControlBlock_t *pcb = (ProcessControlBlock_t*)dyn_array_at(ready_queue, i);
        total_burst_time += (float)pcb->remaining_burst_time;
        pcb->started = false;
    }

    float total_turnaround_time = 0.0f;
    unsigned long current_time = 0;
    size_t finished = 0;

    // 2) Run until all processes complete
    while (finished < number_of_processes)
    {
        int best_index = -1;

        // 3) Find arrived process with smallest remaining time
        for (size_t i = 0; i < number_of_processes; i++)
        {
            ProcessControlBlock_t *pcb = (ProcessControlBlock_t*)dyn_array_at(ready_queue, i);

            if (pcb->remaining_burst_time == 0) { continue; }     // done
            if (pcb->arrival > current_time) { continue; }        // not arrived

            if (best_index == -1)
            {
                best_index = (int)i;
            }
            else
            {
                ProcessControlBlock_t *best = (ProcessControlBlock_t*)dyn_array_at(ready_queue, (size_t)best_index);

                // Smallest remaining time wins
                // Tie-breakers for determinism:
                // 1) earlier arrival
                // 2) higher priority (smaller number)
                // 3) lower index
                if (pcb->remaining_burst_time < best->remaining_burst_time ||
                    (pcb->remaining_burst_time == best->remaining_burst_time && pcb->arrival < best->arrival) ||
                    (pcb->remaining_burst_time == best->remaining_burst_time && pcb->arrival == best->arrival && pcb->priority < best->priority) ||
                    (pcb->remaining_burst_time == best->remaining_burst_time && pcb->arrival == best->arrival && pcb->priority == best->priority && i < (size_t)best_index))
                {
                    best_index = (int)i;
                }
            }
        }

        // 4) If nothing arrived yet, jump to next arrival
        if (best_index == -1)
        {
            uint32_t next_arrival = UINT32_MAX;
            for (size_t i = 0; i < number_of_processes; i++)
            {
                ProcessControlBlock_t *pcb = (ProcessControlBlock_t*)dyn_array_at(ready_queue, i);
                if (pcb->remaining_burst_time == 0) { continue; }
                if (pcb->arrival < next_arrival) { next_arrival = pcb->arrival; }
            }
            current_time = next_arrival;
            continue;
        }

        // 5) Run the best process for ONE time unit (preemptive behavior)
        ProcessControlBlock_t *run = (ProcessControlBlock_t*)dyn_array_at(ready_queue, (size_t)best_index);
        run->started = true;

        virtual_cpu(run);       // remaining_burst_time--
        current_time += 1;      // one tick passed

        // 6) If it just finished, record turnaround time
        if (run->remaining_burst_time == 0)
        {
            total_turnaround_time += (float)(current_time - run->arrival);
            finished++;
        }
    }

    // 7) Fill result: wait = turnaround - burst
    result->average_turnaround_time = total_turnaround_time / (float)number_of_processes;
    result->average_waiting_time = (total_turnaround_time - total_burst_time) / (float)number_of_processes;
    result->total_run_time = (float)current_time;

    return true;
}
