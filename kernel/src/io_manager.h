#ifndef IO_MANAGER_H
#define IO_MANAGER_H

#include <stdio.h>
#include <stdlib.h>
#include <utils/kernel.h>
#include <commons/config.h>
#include <utils/client.h>
#include <state_lists.h>
#include <long_term_scheduler.h>
#include <utils/inout.h>

extern t_list *list_BLOCKED;
extern pthread_mutex_t mutex_blocked;
extern t_list *list_READY;
extern pthread_mutex_t mutex_ready;
extern sem_t sem_unblock;

void io_block(t_pcb* pcb, t_instruction *instruction);
void io_unblock(int pid);

#endif 