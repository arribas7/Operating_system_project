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

extern t_list *list_BLOCKED;
extern pthread_mutex_t mutext_blocked;

extern t_pcb *pcb_RUNNING;
extern pthread_mutex_t mutex_running;

extern sem_t sem_all_scheduler;
extern sem_t sem_ready_process;
extern int scheduler_paused;
extern sem_t sem_quantum;
extern sem_t sem_cpu_dispatch;
extern sem_t sem_unblock;

void st_sched_ready_running(void* arg);

struct quantum_thread_args {
    int *quantum_time;
    bool *interrupted;
    u_int32_t *remaining_quantum;
    char *selection_algorithm;
};

#endif
