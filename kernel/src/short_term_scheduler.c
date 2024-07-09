#include "short_term_scheduler.h"
#include <communication_kernel_cpu.h>
#include <utils/cpu.h>
#include <commons/temporal.h>
#include <long_term_scheduler.h>

// Round Robin (RR) Priority:
// 1. RUNNING state PCBs have the highest priority.
// 2. BLOCKED state PCBs are prioritized over NEW state PCBs.
// 3. If the previous state of pcb1 is RUNNING and pcb2 is not RUNNING, pcb1 has higher priority.
// 4. If the previous state of pcb1 is BLOCKED and pcb2 is NEW, pcb1 has higher priority.
// 5. In all other cases, pcb2 is either equal or higher in priority.
bool rr_pcb_priority(void* pcb1, void* pcb2) { // TODO: Check with ayudantes if we're managing well the fourth criteria, order.
    t_pcb* a = (t_pcb*)pcb1;
    t_pcb* b = (t_pcb*)pcb2;
    // Priority: RUNNING > BLOCKED > NEW
    if (a->prev_state == RUNNING && b->prev_state != RUNNING) return true;
    if (a->prev_state == BLOCKED && b->prev_state == NEW) return true;
    return false;
}

/*
    VRR Handles priority by first selecting PCBS that have not been able to complete their quantum 
    This means that a process that has been interrupted for any reason and sent to BLOCKED was unable to complete its quantum
    This is because if a process completes its quantum it's sent back to ready, not BLOCKED
    Therefore any process that goes from BLOCKED to READY will have priority over EXECUTING to READY
*/
// Virtual Round Robin (VRR) Priority:
// 1. BLOCKED state PCBs have the highest priority since they haven't completed their quantum.
// 2. RUNNING state PCBs are prioritized over NEW state PCBs.
// 3. If pcb1 was previously BLOCKED and pcb2 wasn't, pcb1 has higher priority.
// 4. If pcb1 was previously RUNNING and pcb2 was NEW, pcb1 has higher priority.
// 5. In all other cases, pcb2 is either equal or higher in priority.
bool vrr_pcb_priority(void* pcb1, void* pcb2) {
    t_pcb* a = (t_pcb*)pcb1;
    t_pcb* b = (t_pcb*)pcb2;

    // BLOCKED > RUNNING > NEW
    // If PCB1 has been interrupted by IO and PCB2 hasn't
    if (a->prev_state == BLOCKED && b->prev_state != BLOCKED) return true;
    // If PCB1 came into READY from RUNNING and PCB2 was just created
    if (a->prev_state == RUNNING && b->prev_state == NEW) return true;
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
    // Matias: There's no need for an extra blocked list since PCBS that completed their quantum go back to READY
    pthread_mutex_lock(&mutex_ready);
    list_sort(list_READY, vrr_pcb_priority);

    t_pcb *next_pcb = list_pop(list_READY);
    pthread_mutex_unlock(&mutex_ready);
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

void *run_quantum_counter(void *arg) {
    struct quantum_thread_args *args = (struct quantum_thread_args *)arg;
    int quantum_time = args->quantum_time;

    while (1) {
        //Wait for initialization
        sem_wait(&sem_quantum);

        //Create and start timer
        t_temporal *timer = temporal_create();
        log_info(logger, "Quantum Iniciado");
        temporal_resume(timer);

        //Check for the completion of the timer
        while (temporal_gettime(timer) < quantum_time) {
            usleep(1000); // sleep 1 ms to wait busy-waiting
        }

        //Quantum is finished
        log_info(logger, "Quantum Cumplido");

        //Wait until no one is interacting with RUNNING
        pthread_mutex_lock(&mutex_running);
        if (pcb_RUNNING != NULL) {
            cpu_interrupt(config, INTERRUPT_TIMEOUT);
            pcb_RUNNING = NULL;
        }
        pthread_mutex_unlock(&mutex_running);

        //Destroy timer
        temporal_destroy(timer);
    }
}

void handle_pcb_dispatch_return(t_pcb* pcb, op_code resp_code){
   switch (resp_code) {
        case RELEASE:
            exit_process(pcb, RUNNING, SUCCESS);
            break;
        case INTERRUPT_BY_USER:
            exit_process(pcb, RUNNING, INTERRUPTED_BY_USER);
            break;
        case INTERRUPT_TIMEOUT:
            log_info(logger, "KERNEL DETECTED A TIMEOUT");
            move_pcb(pcb, RUNNING, READY, list_READY, &mutex_ready);
            break;
        case WAIT:
            // TODO: resource manager logic
            break;
        case SIGNAL:
            // TODO: resource manager logic
            break;
        case IO_GEN_SLEEP:
            // TODO: handle instructions call to IO + updated pcb 
            log_info(logger, "KERNEL DETECTED A IO INTERRUPTION");
            move_pcb(pcb, RUNNING, BLOCKED, list_BLOCKED, &mutex_blocked);
            break;
        /*TODO Other IO Cases*/
        default:
            log_warning(logger, "Unknown operation");
            break;
    }
}

void st_sched_ready_running(void* arg) {
    
    char *selection_algorithm = (char *) arg;
    log_debug(logger, "Initializing short term scheduler (ready->running) with selection algorithm: %s...", selection_algorithm);

    // Check if the selection algorithm is Round Robin (RR) or Virtual Round Robin (VRR)
    if (strcmp(selection_algorithm, "RR") == 0 || strcmp(selection_algorithm, "VRR") == 0) {
        pthread_t quantum_counter_thread;
        struct quantum_thread_args quantum_args;
        
        int quantum_value = config_get_int_value(config, "QUANTUM");
        quantum_args.quantum_time = (int *)malloc(sizeof(int));
        quantum_args.interrupted = (bool *)malloc(sizeof(bool));;

        if (quantum_args.interrupted == NULL) {
            log_error(logger, "Failed to allocate memory for quantum");
            free(quantum_args.quantum_time);
            return;
        }
        if (quantum_args.quantum_time == NULL) {
            log_error(logger, "Failed to allocate memory for quantum");
            return;
        }

        *(quantum_args.quantum_time) = quantum_value;
        *(quantum_args.interrupted) = false;

        // Create a thread to handle the quantum counter
        if (pthread_create(&quantum_counter_thread, NULL, (void*) run_quantum_counter, &quantum_args) != 0) {
            log_error(logger, "Error creating quantum thread");
            free(quantum_args.quantum_time);
            return;
        }
        // Detach the thread to let it run independently
        pthread_detach(quantum_counter_thread);
    }

    // Main loop to continuously check for ready processes to schedule
    while (1) {
        // Wait for the semaphore indicating scheduling is allowed
        sem_wait(&sem_all_scheduler); 
        // Immediately release the semaphore for the next iteration
        sem_post(&sem_all_scheduler); 

        if (!list_is_empty(list_READY)) { 
            sem_wait(&sem_st_scheduler);

            // Get the next process control block (PCB) based on the selection algorithm
            t_pcb *next_pcb = get_next_pcb(selection_algorithm);

            // Lock the mutex to safely update the running PCB
            pthread_mutex_lock(&mutex_running);
            pcb_RUNNING = next_pcb;
            pthread_mutex_unlock(&mutex_running);

            // If using RR or VRR, signal the quantum semaphore
            if (strcmp(selection_algorithm, "RR") == 0 || strcmp(selection_algorithm, "VRR") == 0) {
                sem_post(&sem_quantum);
            }

            // Dispatch the PCB to the CPU and get the result
            t_return_dispatch *ret = cpu_dispatch(pcb_RUNNING, config);
            log_info(logger, "Dispatched a PCB %s", next_pcb->path);

            // Handle errors from the CPU dispatch
            if(ret->resp_code == GENERAL_ERROR || ret->resp_code == NOT_SUPPORTED){
                log_error(logger, "Error returned from cpu dispatch: %d", ret->resp_code);
                return;
            }

            // Log the CPU dispatch response
            log_debug(logger, "Processing cpu dispatch response_code: %d", ret->resp_code);

            // Lock the mutex to safely handle the PCB return
            pthread_mutex_lock(&mutex_running);
            // Handle the returned PCB and update based on the response code
            handle_pcb_dispatch_return(ret->pcb_updated, ret->resp_code);
            // Free the running PCB as it has been handled
            free(pcb_RUNNING); 
            // Set the running PCB to NULL
            pcb_RUNNING = NULL;
            // Unlock the mutex
            pthread_mutex_unlock(&mutex_running);

            // Signal the short-term scheduler semaphore
            sem_post(&sem_st_scheduler);
        }

        // Check if the scheduler is paused
        if (scheduler_paused) {
            // Block until the scheduling is resumed
            sem_wait(&sem_all_scheduler); 
            sem_post(&sem_all_scheduler);
        }
    }
}
