#include "short_term_scheduler.h"
#include <communication_kernel_cpu.h>
#include <utils/cpu.h>
#include <commons/temporal.h>
#include <long_term_scheduler.h>
#include <io_manager.h>

bool rr_pcb_priority(void* pcb1, void* pcb2) { // TODO: Check with ayudantes if we're managing well the fourth criteria, order.
    t_pcb* a = (t_pcb*)pcb1;
    t_pcb* b = (t_pcb*)pcb2;
    // Priority: RUNNING > BLOCKED > NEW
    if (a->prev_state == RUNNING && b->prev_state != RUNNING) return true;
    if (a->prev_state == BLOCKED && b->prev_state == NEW) return true;
    return false;
}

t_pcb* fifo(){
    pthread_mutex_lock(&mutex_ready);
    t_pcb *next_pcb = list_pop(list_READY);
    pthread_mutex_unlock(&mutex_ready);

    return next_pcb;
}

t_pcb* round_robin(){
    pthread_mutex_lock(&mutex_ready);
    list_sort(list_READY, rr_pcb_priority);

    t_pcb *next_pcb = list_pop(list_READY);
    pthread_mutex_unlock(&mutex_ready);

    return next_pcb;
}

t_pcb* virtual_round_robin(){ // TODO: Implement
    // order by RR priority
    // take a consideration an extra blocked list.
    return NULL;
}

t_pcb* get_next_pcb(char *selection_algorithm) {
    if (strcmp(selection_algorithm, "RR") == 0) {
        return round_robin();
    } else if (strcmp(selection_algorithm, "VRR") == 0) {
        return virtual_round_robin();
    }

    return fifo();
}

void run_quantum_counter(void* arg) {
    int quantum_time = *(int*) arg;
    
    while (1) {
        sem_wait(&sem_quantum);
        t_temporal *timer = temporal_create();

        log_info(logger, "Quantum Iniciado");
        temporal_resume(timer);
        while (temporal_gettime(timer) < quantum_time) {
            usleep(1000); // sleep 1 ms to wait busy-waiting
        }
        log_info(logger, "Quantum Cumplido");

        pthread_mutex_lock(&mutex_running);
        if (pcb_RUNNING != NULL) {
            cpu_interrupt(config, INTERRUPT_TIMEOUT);
            pcb_RUNNING = NULL;
        }
        pthread_mutex_unlock(&mutex_running);
        temporal_destroy(timer);
    }
}

void handle_dispatch_return_action(t_return_dispatch *ret_data){
   switch (ret_data->resp_code) {
        case RELEASE:
            exit_process(ret_data->pcb_updated, RUNNING, SUCCESS);
            break;
        case INTERRUPT_BY_USER:
            exit_process(ret_data->pcb_updated, RUNNING, INTERRUPTED_BY_USER);
            break;
        case INTERRUPT_TIMEOUT:
            move_pcb(ret_data->pcb_updated, RUNNING, READY, list_READY, &mutex_ready);
            break;
        case WAIT:
            // TODO: resource manager logic
            break;
        case SIGNAL:
            // TODO: resource manager logic
            break;
        case IO_GEN_SLEEP:
            io_block_instruction(ret_data->pcb_updated, ret_data->instruction_IO);
            break;
        case IO_STDIN_READ:
        case IO_STDOUT_WRITE:
            io_block_stdin(ret_data->pcb_updated, ret_data->resp_stdin);
            break;
        case IO_FS_CREATE:
        case IO_FS_DELETE:
        case IO_FS_TRUNCATE:
        case IO_FS_WRITE:
        case IO_FS_READ:
            io_block_interfaz_fs(ret_data->pcb_updated, ret_data->interfaz_fs);
            break;
        default:
            log_warning(logger, "Unknown operation");
            break;
    }
}

void st_sched_ready_running(void* arg) {
    char *selection_algorithm = (char *) arg;
    log_debug(logger, "Initializing short term scheduler (ready->running) with selection algorithm: %s...", selection_algorithm);

    if (strcmp(selection_algorithm, "RR") == 0 || strcmp(selection_algorithm, "VRR") == 0) {
        pthread_t quantum_counter_thread;
        int *quantum_time = config_get_int_value(config, "QUANTUM");

        if (pthread_create(&quantum_counter_thread, NULL, (void*) run_quantum_counter, quantum_time) != 0) {
            log_error(logger, "Error creating quantum thread");
            return;
        }
        pthread_detach(quantum_counter_thread);
    }

    while (1) {
        sem_wait(&sem_all_scheduler); // Wait for the semaphore to be available
        sem_post(&sem_all_scheduler); // Immediately release it for the next iteration
        if (!list_is_empty(list_READY)) { // TODO: Do we need to improve it to avoid intensive busy-waiting polling?
            sem_wait(&sem_st_scheduler);

            t_pcb *next_pcb = get_next_pcb(selection_algorithm);
            pthread_mutex_lock(&mutex_running);
            pcb_RUNNING = next_pcb;
            pthread_mutex_unlock(&mutex_running);

            if (strcmp(selection_algorithm, "RR") == 0 || strcmp(selection_algorithm, "VRR") == 0) {
                sem_post(&sem_quantum);
            }
            t_return_dispatch *ret = cpu_dispatch(pcb_RUNNING, config);
            if(ret->resp_code == GENERAL_ERROR || ret->resp_code == NOT_SUPPORTED){
                log_error(logger, "Error returned from cpu dispatch: %d", ret->resp_code);
                return;
            }

            log_debug(logger, "Processing cpu dispatch response_code: %d", ret->resp_code);

            pthread_mutex_lock(&mutex_running);
            handle_dispatch_return_action(ret);
            free(pcb_RUNNING); // free pcb because we used the updated pcb in other lists
            pcb_RUNNING = NULL;
            pthread_mutex_unlock(&mutex_running);

            sem_post(&sem_st_scheduler);
        }

        if (scheduler_paused) {
            sem_wait(&sem_all_scheduler); // Block until planning is resumed
            sem_post(&sem_all_scheduler);
        }
    }
}