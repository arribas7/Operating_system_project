#include <stdio.h>
#include <commons/collections/list.h>
#include <commons/temporal.h>
#include <stdlib.h>
#include <utils/kernel.h>
#include <quantum.h>
#include <time.h>
#include <sys/time.h>
#include <commons/config.h>
#include <pthread.h>
#include <unistd.h>

void run_quantum_counter(void* arg)
{
    int* quantum_time = (int*) arg;
    
    while (1) {
        sem_wait(&sem_quantum);
        t_temporal *timer = temporal_create();

        log_info(logger, "Quantum Iniciado");
        temporal_resume(timer);
        while (temporal_gettime(timer) >= *quantum_time);
        log_info(logger, "Quantum Cumplido");

        // TODO: Enviar una interrupción al CPU después de que el quantum expire
    }
}