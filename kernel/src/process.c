#include "process.h"
#include <stdlib.h>
#include <stdio.h>
#include <utils/client.h>
#include <commons/log.h>
#include <commons/config.h>
#include <utils/server.h>
#include <utils/kernel.h>

void *start_process(void *arg) {
    process_args* args = (process_args*) arg;
    char* pid_str = args->pid;
    int pid = atoi(pid_str);
    t_config* config = args->config;

    log_debug(logger, "[THREAD] Memory connection");
    int mem_conn = conexion_by_config(config, "IP_MEMORIA", "PUERTO_MEMORIA");

    t_pcb *pcb = new_pcb(pid);
    t_buffer *buffer = malloc(sizeof(t_buffer));
    serialize_pcb(pcb, buffer);

    t_paquete *paquete = crear_paquete(CREATE_PROCESS);
    agregar_a_paquete(paquete, buffer->stream, buffer->size);

    enviar_paquete(paquete, mem_conn);
    eliminar_paquete(paquete);
    delete_pcb(pcb);

	response_code code = esperar_respuesta(mem_conn);
    log_info(logger, "Response code: %d", code);

    liberar_conexion(mem_conn);
    log_info(logger, "Connection released");
    free(args);

}