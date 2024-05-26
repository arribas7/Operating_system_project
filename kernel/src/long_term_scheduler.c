#include "long_term_scheduler.h"

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

// start_process_on_new creates a pcb and push it into list NEW.
void start_process(char* path, t_config *config) {
    int pid = atomic_load(&pid_count) + 1;
    log_info(logger, "Se crea el proceso <%d> en NEW\n", pid);

    log_debug(logger, "[THREAD] Memory connection");
    int mem_conn = conexion_by_config(config, "IP_MEMORIA", "PUERTO_MEMORIA");

    u_int32_t quantum = config_get_int_value(config, "QUANTUM");
    t_pcb *pcb = new_pcb(pid, quantum, path);
    t_buffer *buffer = malloc(sizeof(t_buffer));
    serialize_pcb(pcb, buffer);

    t_paquete *paquete = crear_paquete(CREATE_PROCESS);
    agregar_a_paquete(paquete, buffer->stream, buffer->size);
    enviar_paquete(paquete, mem_conn);
    eliminar_paquete(paquete);
    free(buffer->stream);
    free(buffer);

	response_code code = esperar_respuesta(mem_conn);
    log_debug(logger, "Response code: %d", code);
    // TODO: Manage error response codes.
    liberar_conexion(mem_conn);
    log_debug(logger, "Connection released");
    atomic_fetch_add(&pid_count, 1); // Increment after a successful response
    list_push(list_NEW, pcb);
}

void move_to_exit(t_pcb* pcb, char* status_prev, exit_reason reason){
    log_info(logger, "Finaliza el proceso <%d> - Motivo: %s", pcb->pid, get_exit_reason_str(reason));
    log_info(logger, "“PID: <%d> - Estado Anterior: <%s> - Estado Actual: <EXIT>”", pcb->pid, status_prev);
    list_push(list_EXIT, pcb);
}

void exit_process(int pid, exit_reason reason){
    // Check in all lists
    t_pcb *pcb = pcb_RUNNING;
    if(pcb_RUNNING != NULL && pcb_RUNNING->pid == pid) {
        pcb_RUNNING = NULL;
        move_to_exit(pcb, "RUNNING", reason);
        sem_post(&sem_multiprogramming);
        return;
    } 

    pcb = list_pid_element(list_READY, pid);
    if (pcb != NULL){
        list_remove_by_pid(list_READY, pid);
        move_to_exit(pcb, "READY", reason);
        sem_post(&sem_multiprogramming);
        return;
    }

    pcb = list_pid_element(list_BLOCKED, pid);
    if (pcb != NULL){
        list_remove_by_pid(list_BLOCKED, pid);
        move_to_exit(pcb, "BLOCKED", reason);
        sem_post(&sem_multiprogramming);
        return;
    }

    pcb = list_pid_element(list_NEW, pid);
    if (pcb != NULL){
        list_remove_by_pid(list_NEW, pid);
        move_to_exit(pcb, "NEW", reason);
        return;
    }
    log_error(logger, "Process with pid: <%d> doesn't exist or it is already finished.", pid);
}

void lt_sched_new_ready() {
    log_debug(logger, "Initializing long term scheduler (new->ready)...");
    while (1) {
        sem_wait(&sem_all_scheduler); // Wait for the semaphore to be available
        sem_post(&sem_all_scheduler); // Immediately release it for the next iteration
        if (!list_is_empty(list_NEW)) { // TODO: Do we need to improve it to avoid intensive busy-waiting polling?
            sem_wait(&sem_multiprogramming);
       
            int sem_value;
            sem_getvalue(&sem_multiprogramming, &sem_value);
            log_debug(logger, "Current semaphore value: %d", sem_value);  // Log the value

            t_pcb *pcb = (t_pcb*) list_pop(list_NEW);
            list_push(list_READY, pcb);
            log_info(logger, "“PID: <%d> - Estado Anterior: <NEW> - Estado Actual: <READY>”", pcb->pid);

            log_debug(logger, "Logging list_new: ");
            log_list_contents(logger, list_NEW);

            log_info(logger, "Cola Ready <%d>:", pcb->pid);
            log_list_contents(logger, list_READY);
        }
        // Check if planning is paused
        if (scheduler_paused) {
            sem_wait(&sem_all_scheduler); // Block until planning is resumed
            sem_post(&sem_all_scheduler);
        }
    }
}