#ifndef QUANTUM_H
#define QUANTUM_H

#include <commons/collections/list.h>
#include <stdio.h>
#include <time.h>
#include <commons/log.h>
#include <commons/config.h>

//Returns the quantum specified in kernel.config as an int
//Returns -1 in the case of error
uint32_t get_quantum_config(t_config *config);

//Gets the quantum of a certain pcb
uint32_t get_quantum_pcb(t_pcb *pcb);

//Returns the elapsed cpu time in milliseconds since the program started.
ulong get_current_clock();

//Code to run inside the quantum counter thread
//TODO: Finish it
int run_quantum_counter(t_log *logger);

#endif