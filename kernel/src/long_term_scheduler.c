#include "long_term_scheduler.h"
#include <communication_kernel_cpu.h>
#include <resources_manager.h>

extern sem_t sem_ready_process;
// Array of strings corresponding to the enum values
const char *exit_reason_strings[] = {
    "SUCCESS",
    "INVALID_RESOURCE",
    "INVALID_INTERFACE",
    "OUT_OF_MEMORY",
    "INTERRUPTED_BY_USER"
};

const char *get_exit_reason_str(exit_reason reason) {
    if (reason >= 0 && reason < NUM_REASONS) {
        return exit_reason_strings[reason];
    } else {
        return "UNKNOWN_REASON";
    }
}

// start_process creates a pcb and push it into list NEW.
void start_process(char* path, t_config *config) {
    int pid = atomic_load(&pid_count) + 1;
    log_info(logger, "Se crea el proceso <%d> en NEW", pid);

    log_debug(logger, "[THREAD] Memory connection");
    int mem_conn = conexion_by_config(config, "IP_MEMORIA", "PUERTO_MEMORIA");

    u_int32_t quantum = config_get_int_value(config, "QUANTUM");
    t_pcb *pcb = new_pcb(pid, quantum, path, NEW);
    t_buffer *buffer = malloc(sizeof(t_buffer));
    serialize_pcb(pcb, buffer);

    t_paquete *paquete = crear_paquete(CREATE_PROCESS);
    agregar_a_paquete(paquete, buffer->stream, buffer->size);
    enviar_paquete(paquete, mem_conn);
    eliminar_paquete(paquete);
    free(buffer->stream);
    free(buffer);

	op_code code = recibir_operacion(mem_conn);
    log_debug(logger, "Process created on memory: %d", code);

    liberar_conexion(mem_conn);
    atomic_fetch_add(&pid_count, 1); // Increment after a successful response
    
    pthread_mutex_lock(&mutex_new);
    list_push(list_NEW, pcb);
    pthread_mutex_unlock(&mutex_new);
    sem_post(&sem_new_process); 
}

// exit_process_memory calls memory to release process resources.
void exit_process_memory(t_pcb* pcb) {
    log_debug(logger, "[THREAD] Memory connection");
    int mem_conn = conexion_by_config(config, "IP_MEMORIA", "PUERTO_MEMORIA");

    t_buffer *buffer = malloc(sizeof(t_buffer));
    serialize_pcb(pcb, buffer);

    t_paquete *paquete = crear_paquete(FINISH_PROCESS);
    agregar_a_paquete(paquete, buffer->stream, buffer->size);
    enviar_paquete(paquete, mem_conn);
    eliminar_paquete(paquete);
    free(buffer->stream);
    free(buffer);

	op_code code = esperar_respuesta(mem_conn);
    log_debug(logger, "Response code: %d", code);
    liberar_conexion(mem_conn);
    log_debug(logger, "Connection released");
}

void sem_post_multiprogramming(){
    pthread_mutex_lock(&mutex_multiprogramming);
    sem_post(&sem_multiprogramming);
    pthread_mutex_unlock(&mutex_multiprogramming);
}

void exit_process(t_pcb *pcb, t_state prev_status, exit_reason reason){
    log_info(logger, "Finaliza el proceso <%d> - Motivo: %s", pcb->pid, get_exit_reason_str(reason));
    move_pcb(pcb, prev_status, EXIT, list_EXIT, &mutex_exit);
    release_all_resources(pcb);
    exit_process_memory(pcb);
    sem_post_multiprogramming();
    return;
}

void exit_process_from_pid(int pid, exit_reason reason) {
    // Check in all lists 
    pthread_mutex_lock(&mutex_running);
    t_pcb *pcb = pcb_RUNNING;
    if(pcb_RUNNING != NULL && pcb_RUNNING->pid == pid) {
        cpu_interrupt(config, INTERRUPT_BY_USER); // interrupt should answer dispatch and move it to ready.
        pthread_mutex_unlock(&mutex_running);
        return;
    } else {
        pthread_mutex_unlock(&mutex_running);
    }

    pthread_mutex_lock(&mutex_ready);
    pcb = list_pid_element(list_READY, pid);
    if (pcb != NULL){
        list_remove_by_pid(list_READY, pid);
        sem_wait(&sem_ready_process); // reduce one
        pthread_mutex_unlock(&mutex_ready);
        exit_process(pcb, READY, reason);
        return;
    } else {
        pthread_mutex_unlock(&mutex_ready);
    }

    pthread_mutex_lock(&mutex_blocked);
    pcb = list_pid_element(list_BLOCKED, pid);
    if (pcb != NULL){
        list_remove_by_pid(list_BLOCKED, pid);
        pthread_mutex_unlock(&mutex_blocked);
        exit_process(pcb, BLOCKED, reason);
        return;
    } else {
        pthread_mutex_unlock(&mutex_blocked);
    }

    pthread_mutex_lock(&mutex_new);
    pcb = list_pid_element(list_NEW, pid);
    if (pcb != NULL){
        list_remove_by_pid(list_NEW, pid);
        pthread_mutex_unlock(&mutex_new);
        exit_process(pcb, NEW, reason);
        return;
    } else {
        pthread_mutex_unlock(&mutex_new);
    }
    log_error(logger, "Process with pid: <%d> doesn't exist or it is already finished.", pid);
}

void set_actual_multiprogramming_value(){
    pthread_mutex_lock(&mutex_multiprogramming);
    int sem_value;
    sem_getvalue(&sem_multiprogramming, &sem_value);
    pthread_mutex_unlock(&mutex_multiprogramming);
    log_debug(logger, "Current MULTIPROGRAMMING value: %d", sem_value);
}

void lt_sched_new_ready() {
    log_debug(logger, "Initializing long term scheduler (new->ready)...");
    while (1) {
        sem_wait(&sem_all_scheduler); // Wait for the semaphore to be available
        sem_post(&sem_all_scheduler); // Immediately release it for the next iteration
        
        sem_wait(&sem_new_process);
        sem_wait(&sem_multiprogramming);
        set_actual_multiprogramming_value();

        pthread_mutex_lock(&mutex_new);
        if (!list_is_empty(list_NEW)) {
            t_pcb *pcb = (t_pcb*) list_pop(list_NEW);
            pthread_mutex_unlock(&mutex_new);

            move_pcb(pcb, NEW, READY, list_READY, &mutex_ready);
        } else {
            pthread_mutex_unlock(&mutex_new);
            sem_post(&sem_multiprogramming); // Revertir el semáforo si no hay procesos para mover
            set_actual_multiprogramming_value();
        }
    }
}