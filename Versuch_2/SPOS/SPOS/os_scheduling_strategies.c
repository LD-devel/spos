#include "os_scheduling_strategies.h"
#include "defines.h"
#include "os_process.h"

#include <stdlib.h>

SchedulingInformation schedulingInfo; 

/*!
 *  Reset the scheduling information for a specific strategy
 *  This is only relevant for RoundRobin and InactiveAging
 *  and is done when the strategy is changed through os_setSchedulingStrategy
 *
 * \param strategy  The strategy to reset information for
 */
void os_resetSchedulingInformation(SchedulingStrategy strategy) {
    // This is a presence task
	switch(strategy) {
		case OS_SS_ROUND_ROBIN :
		schedulingInfo.timeSlice = os_getProcessSlot(os_getCurrentProc())->priority;
		
		break;
		
		case OS_SS_INACTIVE_AGING : 
		for (uint8_t i=0; i<MAX_NUMBER_OF_PROCESSES; i++){
			schedulingInfo.age[i] = 0;
		}
		break;
		
		default:
		break;
	}
}

/*!
 *  Reset the scheduling information for a specific process slot
 *  This is necessary when a new process is started to clear out any
 *  leftover data from a process that previously occupied that slot
 *
 *  \param id  The process slot to erase state for
 */
void os_resetProcessSchedulingInformation(ProcessID id) {
    // This is a presence task
	schedulingInfo.age[id]=0; 
}

/*!
 * Wrapper function to link SchedulingStrategy-values and their respective
 * implementations.
 */
ProcessID os_Scheduler_byStrategy(Process const processes[], ProcessID current, SchedulingStrategy strat) {
    switch(strat) {
        case OS_SS_EVEN:
            return os_Scheduler_Even(os_processes, currentProc);
        case OS_SS_RANDOM:
            return os_Scheduler_Random(os_processes, currentProc);
        case OS_SS_RUN_TO_COMPLETION:
            return os_Scheduler_RunToCompletion(os_processes, currentProc);
        case OS_SS_ROUND_ROBIN:
            return os_Scheduler_RoundRobin(os_processes, currentProc);
        case OS_SS_INACTIVE_AGING:
            return os_Scheduler_InactiveAging(os_processes, currentProc);
        default:
            os_error("Nonexisting scheduling strat"); // this should never happen
            return current;
    }
}

/*!
 *  This function implements the even strategy. Every process gets the same
 *  amount of processing time and is rescheduled after each scheduler call
 *  if there are other processes running other than the idle process.
 *  The idle process is executed if no other process is ready for execution
 *
 *  \param processes An array holding the processes to choose the next process from.
 *  \param current The id of the current process.
 *  \return The next process to be executed determined on the basis of the even strategy.
 */
ProcessID os_Scheduler_Even(Process const processes[], ProcessID current) {
	uint8_t i = current;
	for (uint8_t j=0;j < MAX_NUMBER_OF_PROCESSES ;j++){
		if (i+1 < MAX_NUMBER_OF_PROCESSES) i++;
		else{
			i=1;
		}
		if (processes[i].state == OS_PS_READY) return i;
	}
	return 0;
}

/*!
 *  This function implements the random strategy. The next process is chosen based on
 *  the result of a pseudo random number generator.
 *
 *  \param processes An array holding the processes to choose the next process from.
 *  \param current The id of the current process.
 *  \return The next process to be executed determined on the basis of the random strategy.
 */
ProcessID os_Scheduler_Random(Process const processes[], ProcessID current) {
	uint8_t readyCount = 0; 
	//Count all ready Processes except for "Leerlaufprozess" at PID = 0
	for (uint8_t i=1; i < MAX_NUMBER_OF_PROCESSES; i++){
		if (processes[i].state == OS_PS_READY) readyCount++;
	}
	if (readyCount == 0) return 0;
	//List all PIDs of ready processes
	uint8_t readyPIDs[readyCount];
	uint8_t j = 0; 
	for (uint8_t i=1; i < MAX_NUMBER_OF_PROCESSES; i++){
		if (processes[i].state == OS_PS_READY){
			readyPIDs[j] = i; 
			j++;
		}
	}
	return readyPIDs[rand() % readyCount];
}

/*!
 *  This function implements the round-robin strategy. In this strategy, process priorities
 *  are considered when choosing the next process. A process stays active as long its time slice
 *  does not reach zero. This time slice is initialized with the priority of each specific process
 *  and decremented each time this function is called. If the time slice reaches zero, the even
 *  strategy is used to determine the next process to run.
 *
 *  \param processes An array holding the processes to choose the next process from.
 *  \param current The id of the current process.
 *  \return The next process to be executed determined on the basis of the round robin strategy.
 */
ProcessID os_Scheduler_RoundRobin(Process const processes[], ProcessID current) {
    // This is a presence task
	if (schedulingInfo.timeSlice > 0){
		schedulingInfo.timeSlice--;
		return current;
	}
	ProcessID next = os_Scheduler_Even(processes,current);
	schedulingInfo.timeSlice = processes[next].priority -1;	
    return next;
}

/*!
 *  This function realizes the inactive-aging strategy. In this strategy a process specific integer ("the age") is used to determine
 *  which process will be chosen. At first, the age of every waiting process is increased by its priority. After that the oldest
 *  process is chosen. If the oldest process is not distinct, the one with the highest priority is chosen. If this is not distinct
 *  as well, the one with the lower ProcessID is chosen. Before actually returning the ProcessID, the age of the process who
 *  is to be returned is reset to its priority.
 *
 *  \param processes An array holding the processes to choose the next process from.
 *  \param current The id of the current process.
 *  \return The next process to be executed, determined based on the inactive-aging strategy.
 */
ProcessID os_Scheduler_InactiveAging(Process const processes[], ProcessID current) {
    // This is a presence task
	uint8_t next = 1;
	for(uint8_t i=0;i<MAX_NUMBER_OF_PROCESSES;i++){
		
		if (i != current)schedulingInfo.age[i] += processes[i].priority;
		
		if (schedulingInfo.age[i]>schedulingInfo.age[next]){
			next = i;
		} else if (schedulingInfo.age[i] == schedulingInfo.age[next]){
			if (processes[i].priority > processes[next].priority){
				next = i; 
			}
		}
	}
    return next;
}

/*!
 *  This function realizes the run-to-completion strategy.
 *  As long as the process that has run before is still ready, it is returned again.
 *  If  it is not ready, the even strategy is used to determine the process to be returned
 *
 *  \param processes An array holding the processes to choose the next process from.
 *  \param current The id of the current process.
 *  \return The next process to be executed, determined based on the run-to-completion strategy.
 */
ProcessID os_Scheduler_RunToCompletion(Process const processes[], ProcessID current) {
    // This is a presence task
	if (processes[current].state == OS_PS_READY) return current;
    return os_Scheduler_Even(processes, current);
}
