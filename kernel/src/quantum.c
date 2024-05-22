#include <stdio.h>
//#include <commons/log.h>
#include <commons/collections/list.h>
#include <stdlib.h>
#include <state_lists.h>
#include <utils/kernel.h>
#include <quantum.h>
#include <time.h>
#include <commons/config.h>

uint32_t get_quantum_config(t_config *config) {
    if (config == NULL) {
        return -1;
    }

    char* char_quantum = config_get_string_value(config, "QUANTUM");
    return string_to_uint32(char_quantum);
}

uint32_t get_quantum_pcb(t_pcb *pcb) {
    return pcb->quantum;
}

ulong get_current_clock() {
    clock_t time = clock();
    double ms = ((double)time / CLOCKS_PER_SEC) * 1000.0;
    return ms;
}

int run_quantum_counter(t_log *logger) {
    log_info(logger, "Counting Quantum");

    bool flag = false;
    unsigned long start_time = get_current_clock();

    while (1) {
        ulong current_time = get_current_clock();
        ulong elapsed_time = current_time - start_time;

        log_info(logger, "Elapsed Time: %lu", current_time);

        // Check if 2000 milliseconds have passed or the flag is true
        if (elapsed_time >= 2000 || flag) {
            log_info(logger, "Either 2000 milliseconds have passed or the flag is true");
            break;
        }

        // Simulate some condition to set the flag
        // For example, let's set the flag to true after 1 second for demonstration
        if (elapsed_time == 1000) {
            flag = true;
        }
        continue;
    return EXIT_SUCCESS;
}
}