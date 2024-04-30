#ifndef PROCESS_H
#define PROCESS_H

#include <commons/config.h>

typedef struct {
    char* pid;
    t_config* config;
} process_args;

void *start_process(void *arg);

#endif
