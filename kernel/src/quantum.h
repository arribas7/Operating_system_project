#ifndef QUANTUM_H
#define QUANTUM_H

#include <commons/collections/list.h>
#include <stdio.h>
#include <time.h>
#include <commons/log.h>
#include <commons/config.h>

typedef struct {
    t_log *logger;
    t_config *config;
    bool stop;
    bool reset;
} t_quantum_thread_params;

//Returns the quantum specified in kernel.config as an int
//Returns -1 in the case of error
uint32_t get_quantum_config(t_config *config);

//Gets the quantum of a certain pcb
uint32_t get_quantum_pcb(t_pcb *pcb);

//Returns the elapsed cpu time in milliseconds since the program started.
ulong get_current_clock();

//Pauses the current quantum counter until resumed
void stop_quantum_counter(t_quantum_thread_params *params);

//Makes it so "time" is set back to 0 for the quantum counter
//This needs to be done each time you want to start counting again
void reset_quantum_counter(t_quantum_thread_params *params);

//Pauses and resets the quantum counter
void interrupt_quantum_counter(t_quantum_thread_params *params);

//Resumes the quantum counter
void resume_quantum_counter(t_quantum_thread_params *params);

//Code to run inside the quantum counter thread
//TODO: Finish it
int run_quantum_counter(t_quantum_thread_params *params);

//Returns the struct to be passed to run_quantum_counter
t_quantum_thread_params *get_quantum_params_struct(t_log *input_log, t_config *input_config);

#endif