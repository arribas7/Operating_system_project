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

typedef enum {
    IO_SUCCESS_OP,
    IO_NOT_FOUND,
} t_io_block_return;

t_io_block_return io_block_instruction(t_pcb* pcb, t_instruction *instruction);

#endif 
