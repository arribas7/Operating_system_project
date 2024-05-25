#include "process.h"
#include <stdlib.h>
#include <stdio.h>
#include <utils/client.h>
#include <commons/log.h>
#include <commons/config.h>
#include <utils/server.h>
#include <utils/kernel.h>
#include <stdatomic.h>

// start_process_on_new creates a pcb and push it into list NEW.
void *start_process_on_new(char* path, t_config *config) {
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
    log_info(logger, "Response code: %d", code);
    // TODO: Manage error response codes.
    liberar_conexion(mem_conn);
    log_info(logger, "Connection released");
    atomic_fetch_add(&pid_count, 1); // Increment after a successful response
    list_push(list_NEW, pcb);
}