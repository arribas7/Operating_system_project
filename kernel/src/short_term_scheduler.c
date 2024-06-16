#include "short_term_scheduler.h"
#include <utils/cpu.h>
#include <quantum.h>

t_pcb* fifo(){
    pthread_mutex_lock(&mutex_ready);
    t_pcb *next_pcb = list_pop(list_READY);
    pthread_mutex_unlock(&mutex_ready);

    return next_pcb;
}

bool rr_pcb_priority(void* pcb1, void* pcb2) { // TODO: Check if we're managing well the fourth criteria, order.
    t_pcb* a = (t_pcb*)pcb1;
    t_pcb* b = (t_pcb*)pcb2;
    // Priority: RUNNING > BLOCKED > NEW
    if (a->prev_state == RUNNING && b->prev_state != RUNNING) return true;
    if (a->prev_state == BLOCKED && b->prev_state == NEW) return true;
    return false;
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

void* cpu_dispatch(void* arg){
    log_debug(logger, "CPU connection running in a thread");
    
    t_config *config = (t_config *) arg;
    int conexion_cpu = conexion_by_config(config, "IP_CPU", "PUERTO_CPU_DISPATCH");

    t_pcb *pcb = new_pcb(22, 0, "/home/utn/");
    t_buffer *buffer = malloc(sizeof(t_buffer));
    serialize_pcb(pcb, buffer);

    t_paquete *paquete = crear_paquete(DISPATCH);
    agregar_a_paquete(paquete, buffer->stream, buffer->size);

    enviar_paquete(paquete, conexion_cpu);
    eliminar_paquete(paquete);
    delete_pcb(pcb);

    recibir_operacion(conexion_cpu); // va a ser cod_op: CPU ACK
    recibir_mensaje(conexion_cpu);

    int cod_op = recibir_operacion(conexion_cpu);
    switch (cod_op) {
        case IO_GEN_SLEEP:
            t_list* lista = recibir_paquete(conexion_cpu);
            void *instruction_buffer;
            t_instruction* instruction;
            for(int i = 0; i< list_size(lista); i ++){
                instruction_buffer = list_get(lista, i);
                instruction = deserializar_instruction_IO(instruction_buffer);
                log_info(logger, "PID: <%d> - Accion: <%s> - IO: <%s> - Unit: <%s>", instruction->pid , "IO_GEN_SLEEP", instruction->interfaz, instruction->job_unit);
            }
            free(instruction_buffer);
            
        case -1:
            log_error(logger, "el cliente se desconecto. Terminando servidor");
            return EXIT_FAILURE;
        default:
            log_warning(logger, "Operacion desconocida. No quieras meter la pata");
            break;
    }
    sleep(15);

    liberar_conexion(conexion_cpu);
    log_debug(logger, "CPU connection released");
    return NULL;
}

void handle_pcb_dispatch_return(t_pcb* pcb){
    // TODO:
}

void* st_sched_ready_running(void* arg) {
    char *selection_algorithm = (char *) arg;
    log_debug(logger, "Initializing short term scheduler (ready->running) with selection algorithm: %s...", selection_algorithm);

    if (strcmp(selection_algorithm, "RR") == 0 || strcmp(selection_algorithm, "VRR") == 0) {
        pthread_t quantum_counter_thread;
        int *quantum_time = config_get_int_value(config, "QUANTUM");

        if (pthread_create(&quantum_counter_thread, NULL, (void*) run_quantum_counter, quantum_time) != 0) {
            log_error(logger, "Error creating quantum thread");
            return;
        }
        pthread_join(&quantum_counter_thread, NULL);
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
            
            pthread_t cpu_dispatch_thread;
            if (pthread_create(&cpu_dispatch_thread, NULL, cpu_dispatch, config) != 0) {
                log_error(logger, "Error creating cpu dispatch thread");
            }

            if (strcmp(selection_algorithm, "RR") == 0 || strcmp(selection_algorithm, "VRR") == 0) {
                sem_post(&sem_quantum);
            }

            pthread_join(cpu_dispatch_thread, NULL);

            pthread_mutex_lock(&mutex_running);
            if (pcb_RUNNING != NULL) {
                handle_pcb_dispatch_return(pcb_RUNNING);
                pcb_RUNNING = NULL;
            }
            pthread_mutex_unlock(&mutex_running);
            sem_post(&sem_st_scheduler);
        }

        if (scheduler_paused) {
            sem_wait(&sem_all_scheduler); // Block until planning is resumed
            sem_post(&sem_all_scheduler);
        }
    }
}