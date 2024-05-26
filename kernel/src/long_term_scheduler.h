#ifndef LT_SCHEDULER_H
#define LT_SCHEDULER_H
#include<commons/log.h>
#include <state_lists.h>
#include <utils/kernel.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <stdio.h>

typedef enum {
    SUCCESS,
    INVALID_RESOURCE,
    INVALID_INTERFACE,
    OUT_OF_MEMORY,
    INTERRUPTED_BY_USER,
    NUM_REASONS // This helps in determining the number of reasons
} exit_reason;

extern t_log* logger;
extern t_list *list_NEW;
extern t_list *list_READY;
extern t_list *list_BLOCKED;
extern t_pcb *pcb_RUNNING;
extern t_list *list_EXIT;
extern sem_t sem_multiprogramming;
extern sem_t sem_all_scheduler;
extern int scheduler_paused;
extern atomic_int pid_count;

void lt_sched_new_ready();
void start_process();
void exit_process(int pid, exit_reason reason);

#endif
