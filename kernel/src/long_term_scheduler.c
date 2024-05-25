#include "long_term_scheduler.h"

void *lt_sched_new_ready(void *arg) {
    log_debug(logger, "Initializing long term scheduler...");
    while (1) {
        if (!list_is_empty(list_NEW)) {
            sem_wait(&sem_multiprogramming);
       
            int sem_value;
            sem_getvalue(&sem_multiprogramming, &sem_value);  // Get current value of the semaphore
            log_debug(logger, "Current semaphore value: %d", sem_value);  // Log the value

            t_pcb *pcb = (t_pcb*) list_pop(list_NEW);
            list_push(list_READY, pcb);
            log_debug(logger, "Logging list_new: ");
            log_list_contents(logger, list_NEW);
            log_debug(logger, "Logging list_ready: ");
            log_list_contents(logger, list_READY);
            log_info(logger, "Moved process %d from NEW to READY", pcb->pid);
        }
    }
}