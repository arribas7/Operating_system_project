#include "short_term_scheduler.h"
#include <communication_kernel_cpu.h>
#include <utils/cpu.h>
#include <commons/temporal.h>
#include <long_term_scheduler.h>
#include <resources_manager.h>
#include <utils/inout.h>

// Round Robin (RR) Priority:
// 1. RUNNING state PCBs have the highest priority.
// 2. BLOCKED state PCBs are prioritized over NEW state PCBs.
// 3. If the previous state of pcb1 is RUNNING and pcb2 is not RUNNING, pcb1 has higher priority.
// 4. If the previous state of pcb1 is BLOCKED and pcb2 is NEW, pcb1 has higher priority.
// 5. In all other cases, pcb2 is either equal or higher in priority.

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

bool rr_pcb_priority(void* pcb1, void* pcb2) { 
    t_pcb* a = (t_pcb*)pcb1;
    t_pcb* b = (t_pcb*)pcb2;
    // Priority: RUNNING > BLOCKED > NEW
    if (a->prev_state == RUNNING && b->prev_state != RUNNING) return true;
    if (a->prev_state == BLOCKED && b->prev_state == NEW) return true;
    if (a->prev_state == b->prev_state) {
        int a_index = list_pid_element_index(list_READY, a->pid);
        int b_index = list_pid_element_index(list_READY, b->pid);
        if(a_index < b_index) {
            return false;
        }else{
            return true;
        }
    }
    return false;
}

bool vrr_pcb_priority(void* pcb1, void* pcb2) {
    t_pcb* a = (t_pcb*)pcb1;
    t_pcb* b = (t_pcb*)pcb2;

    // BLOCKED > RUNNING > NEW
    // If PCB1 has been interrupted by IO and PCB2 hasn't
    if (a->prev_state == BLOCKED && b->prev_state != BLOCKED) return true;
    // If PCB1 came into READY from RUNNING and PCB2 was just created
    //if (a->prev_state == RUNNING && b->prev_state == NEW) return true;
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
    //list_sort(list_READY, rr_pcb_priority);

    t_pcb *next_pcb = list_pop(list_READY);
    pthread_mutex_unlock(&mutex_ready);

    return next_pcb;
}

t_pcb* virtual_round_robin(){
    pthread_mutex_lock(&mutex_ready);
    list_sort(list_READY, vrr_pcb_priority);

    t_pcb *next_pcb = list_pop(list_READY);
    pthread_mutex_unlock(&mutex_ready);
    return next_pcb;
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
    int quantum_time = *(args->quantum_time);
    log_debug(logger, "Quantum thread initialized with %s", args->selection_algorithm);
    //log_debug(logger, "Quantum time %d", quantum_time);
    while (1) {
        //Wait for initialization
        sem_wait(&sem_quantum);

        //Create and start timer
        t_temporal *timer = temporal_create();
        log_info(logger, "Quantum Iniciado con %d ms restantes", *(args->remaining_quantum));
        temporal_resume(timer);

        //Reset interrupt flag
        *(args->interrupted) = false;

        //If executing in Round Robin, we don't take into consideration the remaining quantum so we make it the default quantum time
        if (strcmp(args->selection_algorithm, "RR") == 0){
            *(args->remaining_quantum) = quantum_time;
        }

        //Check for the completion of the timer or interruption of the process
        while (temporal_gettime(timer) < *(args->remaining_quantum) && *(args->interrupted) == false) {
            usleep(1000); // sleep 1 ms to wait busy-waiting
        }

        //Pause the timer once it's not needed
        temporal_stop(timer);

        if (*(args->interrupted) == false) {
            //Quantum is finished
            log_info(logger, "Quantum Cumplido");

            //Reset the remaining quantum time to default
            *(args->remaining_quantum) = quantum_time;

            //Wait until no one is interacting with RUNNING and INTERRUPT_TIMEOUT
            pthread_mutex_lock(&mutex_running);
            if (pcb_RUNNING != NULL) {
                cpu_interrupt(config, INTERRUPT_TIMEOUT);
                pcb_RUNNING = NULL;
            }
            pthread_mutex_unlock(&mutex_running);
        } else {
            //Quantum got interrupted before completion
            log_info(logger, "Quantum Interrumpido");

            //Update the remaining time
            int remaining_time = *(args->remaining_quantum) - temporal_gettime(timer);
            
            *(args->remaining_quantum) = remaining_time;
        }
        //Destroy timer
        temporal_destroy(timer);
    }
}

void handle_resource(t_pcb* pcb, op_code code, t_ws* resp_ws){
    t_result result;
    switch (code) {
        case WAIT:
            result = resource_wait(pcb, resp_ws->recurso);
            break;
        case SIGNAL:
            result = resource_signal(pcb, resp_ws->recurso);
            break;
    }
            
    switch (result) {
        case RESOURCE_NOT_FOUND:
            exit_process(pcb, RUNNING, INVALID_RESOURCE);
            break;
        case RESOURCE_BLOCKED:
            move_pcb(pcb, RUNNING, BLOCKED, list_BLOCKED, &mutex_blocked);
            break;
        default: // success
            move_pcb(pcb, RUNNING, READY, list_READY, &mutex_ready);
            break;
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
            log_warning(logger, "TIMEOUT INTERRUPTION");
            move_pcb(ret_data->pcb_updated, RUNNING, READY, list_READY, &mutex_ready);
            break;
        case WAIT:
        case SIGNAL:
            handle_resource(ret_data->pcb_updated, ret_data->resp_code, ret_data->resp_ws);
            break;
        case IO_GEN_SLEEP:
        case IO_STDIN_READ:
        case IO_STDOUT_WRITE:
        case IO_FS_CREATE:
        case IO_FS_DELETE:
        case IO_FS_TRUNCATE:
        case IO_FS_WRITE:
        case IO_FS_READ:
            io_block(ret_data->pcb_updated, ret_data->instruction_IO);
            break;
        default:
            log_warning(logger, "Unknown operation for pid %d",ret_data->pcb_updated->pid);
            exit_process(ret_data->pcb_updated, RUNNING, INVALID_OPERATION);
            break;
    }
}

void st_sched_ready_running(void* arg) {
    char *selection_algorithm = (char *) arg;
    log_debug(logger, "Initializing short term scheduler (ready->running) with selection algorithm: %s...", selection_algorithm);

    //Define quantum thread args
    struct quantum_thread_args *quantum_args = malloc(sizeof(struct quantum_thread_args));
        
    if (quantum_args == NULL) {
        log_error(logger, "Failed to allocate memory for quantum args");
        return;
    }
    // Check if the selection algorithm is Round Robin (RR) or Virtual Round Robin (VRR)
    if (strcmp(selection_algorithm, "RR") == 0 || strcmp(selection_algorithm, "VRR") == 0) {
        pthread_t quantum_counter_thread;

        quantum_args->quantum_time = malloc(sizeof(int));
        quantum_args->interrupted = malloc(sizeof(bool));
        quantum_args->remaining_quantum = malloc(sizeof(int));
        quantum_args->selection_algorithm = malloc(strlen(selection_algorithm) + 1);

        if (quantum_args->quantum_time == NULL || quantum_args->interrupted == NULL || quantum_args->remaining_quantum == NULL) {
            log_error(logger, "Failed to allocate memory for quantum args fields");
            free(quantum_args->quantum_time);
            free(quantum_args->interrupted);
            free(quantum_args->remaining_quantum);
            free(quantum_args->selection_algorithm);
            free(quantum_args);
            return;
        }

        int quantum_value = config_get_int_value(config, "QUANTUM");
        *(quantum_args->quantum_time) = quantum_value;
        *(quantum_args->interrupted) = false;
        strcpy(quantum_args->selection_algorithm, selection_algorithm);

        // Create a thread to handle the quantum counter
        if (pthread_create(&quantum_counter_thread, NULL, run_quantum_counter, quantum_args) != 0) {
            log_error(logger, "Error creating quantum thread");
            free(quantum_args->quantum_time);
            free(quantum_args->interrupted);
            free(quantum_args);
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
            log_info(logger, "“PID: <%d> - Estado Anterior: <READY> - Estado Actual: <RUNNING>”", next_pcb->pid);

            // If using RR or VRR, signal the quantum semaphore
            if (strcmp(selection_algorithm, "RR") == 0 || strcmp(selection_algorithm, "VRR") == 0) {
                //Update the remaining quantum
                //log_info(logger, "The quantum of the next pcb is %d", next_pcb->quantum)
                *(quantum_args->remaining_quantum) = next_pcb->quantum;
                sem_post(&sem_quantum);
            }

            // Dispatch the PCB to the CPU and get the result
            t_return_dispatch *ret = cpu_dispatch(pcb_RUNNING, config);
            log_info(logger, "||||||||||||||||||||READY LIST||||||||||||||||||||");
            log_list_contents(logger, list_READY, mutex_ready);
            log_info(logger, "Dispatched a PCB %s", next_pcb->path);

            // Handle errors from the CPU dispatch
            if(ret->resp_code == GENERAL_ERROR || ret->resp_code == NOT_SUPPORTED){
                log_error(logger, "Error returned from cpu dispatch: %d", ret->resp_code);
                return;
            }

            // Log the CPU dispatch response
            //log_debug(logger, "Processing cpu dispatch response_code: %d", ret->resp_code);

            // Lock the mutex to safely handle the PCB return
            pthread_mutex_lock(&mutex_running);
            // If using quantum, interrupt its execution
             if (strcmp(selection_algorithm, "RR") == 0 || strcmp(selection_algorithm, "VRR") == 0) {
                *(quantum_args->interrupted) = true;
                // Sleep 1 ms to sync up with the independent quantum thread
                usleep(1000);
                //Update the remaining quantum from the new pcb
                ret->pcb_updated->quantum = *(quantum_args->remaining_quantum);
             }
            handle_dispatch_return_action(ret);
            free(pcb_RUNNING); // free pcb because we used the updated pcb in other lists
            pcb_RUNNING = NULL;
            // Unlock the mutex
            pthread_mutex_unlock(&mutex_running);

            // Signal the short-term scheduler semaphore
            sem_post(&sem_st_scheduler);
            
            // TODO: free other rets
            if(ret->instruction_IO != NULL){
                delete_instruction_IO(ret->instruction_IO);
            }
            if(ret->resp_ws != NULL){
                destroy_ws(ret->resp_ws);
            }
            free(ret);
            log_info(logger, "||||||||||||||||||||PROCESO FINALIZADO||||||||||||||||||||");
        }

        // Check if the scheduler is paused
        if (scheduler_paused) {
            // Block until the scheduling is resumed
            sem_wait(&sem_all_scheduler); 
            sem_post(&sem_all_scheduler);
        }
    }
}
