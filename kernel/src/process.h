#ifndef PROCESS_H
#define PROCESS_H

#include <commons/config.h>
#include <commons/log.h>
#include <stdatomic.h>
#include <semaphore.h>

extern t_log* logger;
extern atomic_int pid_count;
extern t_list *list_NEW;

void *start_process_on_new(char *path, t_config *config);

#endif
