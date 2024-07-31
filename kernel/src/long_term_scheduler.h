#ifndef LT_SCHEDULER_H
#define LT_SCHEDULER_H
#include<commons/log.h>
#include <state_lists.h>
#include <utils/kernel.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <stdio.h>
#include <pthread.h>

typedef enum {
    SUCCESS,
    INVALID_RESOURCE,
    INVALID_INTERFACE,
    INVALID_OPERATION,
    INTERRUPTED_BY_USER,
    ERROR_INTERFACE,
    NUM_REASONS // This helps in determining the number of reasons
} exit_reason;

extern t_log* logger;

extern t_list *list_NEW;
extern pthread_mutex_t mutex_new;

extern t_list *list_READY;
extern pthread_mutex_t mutex_ready;

extern t_list *list_BLOCKED;
extern pthread_mutex_t mutex_blocked;

extern t_pcb *pcb_RUNNING;
extern pthread_mutex_t mutex_running;

extern t_list *list_EXIT;
extern pthread_mutex_t mutex_exit;

extern sem_t sem_multiprogramming;
extern pthread_mutex_t mutex_multiprogramming;

extern sem_t sem_all_scheduler;
extern sem_t sem_new_process;
extern int scheduler_paused;
extern atomic_int pid_count;
extern t_config *config;

void lt_sched_new_ready();
void start_process();
void exit_process_from_pid(int pid, exit_reason reason);
void exit_process(t_pcb *pcb, t_state prev_status, exit_reason reason);

#endif
