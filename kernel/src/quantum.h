#ifndef QUANTUM_H
#define QUANTUM_H

#include <stdio.h>
#include <time.h>
#include <commons/log.h>
#include <commons/config.h>
#include <semaphore.h>

extern t_log *logger;
extern sem_t sem_quantum;

void run_quantum_counter(void *arg);

#endif