#ifndef ST_SCHEDULER_H
#define ST_SCHEDULER_H
#include <pthread.h>
#include <commons/log.h>
#include <state_lists.h>
#include <utils/kernel.h>
#include <semaphore.h>
#include <stdio.h>
#include <communication_kernel_cpu.h>

extern t_log* logger;
extern t_config *config;

extern t_list *list_READY;
extern pthread_mutex_t mutex_ready;

extern t_pcb *pcb_RUNNING;
extern pthread_mutex_t mutex_running;

extern sem_t sem_all_scheduler;
extern sem_t sem_st_scheduler;
extern int scheduler_paused;
extern sem_t sem_quantum;

void st_sched_ready_running(void* arg);

#endif
