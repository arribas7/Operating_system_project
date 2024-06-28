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
extern pthread_mutex_t mutext_blocked;

void io_block_interfaz_fs(t_pcb* pcb, t_interfaz *interfaz_fs);
void io_block_instruction(t_pcb* pcb, t_instruction *instruction);
void io_block_stdin(t_pcb* pcb, t_io_stdin *io_stdin);

#endif 
