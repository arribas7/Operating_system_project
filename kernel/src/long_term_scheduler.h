#ifndef LT_SCHEDULER_H
#define LT_SCHEDULER_H
#include<commons/log.h>
#include <state_lists.h>
#include <utils/kernel.h>
#include <semaphore.h>

// lt scheduler doesn't use list_blocked;
extern t_log* logger;
extern t_list *list_NEW;
extern t_list *list_READY;
extern t_pcb *pcb_RUNNING;
extern t_list *list_EXIT;
extern sem_t sem_multiprogramming;

void *long_term_scheduler(void *arg);

#endif
