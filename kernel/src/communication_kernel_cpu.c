#include <stdio.h>
#include <stdlib.h>
#include <utils/kernel.h>
#include <utils/server.h>
#include <utils/client.h>
#include <commons/config.h>

op_code cpu_dispatch(t_pcb *pcb, t_config *config){
    int cpu_connection = conexion_by_config(config, "IP_CPU", "PUERTO_CPU_DISPATCH");

    log_debug(logger, "CPU connection running in a thread");
    t_buffer *buffer = malloc(sizeof(t_buffer));
    serialize_pcb(pcb, buffer);

    t_paquete *paquete = crear_paquete(DISPATCH);
    agregar_a_paquete(paquete, buffer->stream, buffer->size);
    enviar_paquete(paquete, cpu_connection);
    eliminar_paquete(paquete);
    free(buffer->stream);
    free(buffer);

    op_code response_code = (op_code) recibir_operacion(cpu_connection);

    liberar_conexion(cpu_connection);
    log_debug(logger, "CPU dispatch connection released");
    return response_code;
}

op_code cpu_interrupt(t_config *config){
    int cpu_connection = conexion_by_config(config, "IP_CPU", "PUERTO_CPU_INTERRUPT");

    t_paquete *paquete = crear_paquete(INTERRUPT);
    enviar_paquete(paquete, cpu_connection);
    eliminar_paquete(paquete);

    op_code response_code = (op_code) recibir_operacion(cpu_connection);
    liberar_conexion(cpu_connection);
    log_debug(logger, "CPU interrupt connection released");

    return response_code;
}