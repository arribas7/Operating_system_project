#include <stdio.h>
#include <commons/collections/list.h>
#include <commons/temporal.h>
#include <stdlib.h>
#include <state_lists.h>
#include <utils/kernel.h>
#include <quantum.h>
#include <time.h>
#include <sys/time.h>
#include <commons/config.h>
#include <pthread.h>
#include <unistd.h>

t_quantum_thread_params *get_quantum_params_struct(t_log *input_log, t_config *input_config)
{
    t_quantum_thread_params *params = malloc(sizeof(t_quantum_thread_params));
    if (params == NULL)
    {
        // Handle memory allocation failure
        return NULL;
    }
    params->logger = input_log;
    params->config = input_config;
    params->stop = false;
    params->reset = false; 
    return params;
}

uint32_t get_quantum_config(t_config *config)
{
    if (config == NULL)
    {
        return -1;
    }

    char *char_quantum = config_get_string_value(config, "QUANTUM");
    return string_to_uint32(char_quantum);
}

uint32_t get_quantum_pcb(t_pcb *pcb)
{
    return pcb->quantum;
}

ulong get_current_clock()
{
    clock_t time = clock();
    double ms = ((double)time / CLOCKS_PER_SEC) * 1000.0;
    return ms;
}

long long get_current_time_ms()
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    return (((long long)tv.tv_sec) * 1000) + (tv.tv_usec / 1000);
}

void stop_quantum_counter(t_quantum_thread_params *params)
{
    params->stop = true;
}

void reset_quantum_counter(t_quantum_thread_params *params)
{
    params->reset = true;
}

void interrupt_quantum_counter(t_quantum_thread_params *params)
{
    stop_quantum_counter(params);
    reset_quantum_counter(params);
}

void resume_quantum_counter(t_quantum_thread_params *params)
{
    params->stop = false;
}

int run_quantum_counter(t_quantum_thread_params *params)
{
    t_log *logger = params->logger;
    t_config *config = params->config;
    t_temporal *timer_variable;
    int max_ms = get_quantum_config(config);

    timer_variable = temporal_create();
    log_info(logger, "Timer Iniciado");
    while (1)
    {
        if (!params->stop)
        {
            temporal_resume(timer_variable);
            if (params->reset)
            {
                // Resets the timer
                temporal_destroy(timer_variable);
                timer_variable = temporal_create();

                // Restore the reset flag to its default state
                params->reset = false;
            }
            if (temporal_gettime(timer_variable) >= max_ms)
            {
                // Resets the timer
                temporal_destroy(timer_variable);
                timer_variable = temporal_create();

                // Send a message somehow to change the current running pcb in here
                log_trace(logger, "Quantum Cumplido");
            }
        }
        else
        {
            temporal_stop(timer_variable);
        }
    }

    return 0; // Return statement to avoid a warning about missing return value
}