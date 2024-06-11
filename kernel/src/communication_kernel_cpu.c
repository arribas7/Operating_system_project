#include <stdio.h>
#include <stdlib.h>
#include <utils/kernel.h>
#include <utils/server.h>
#include <utils/client.h>
#include <commons/config.h>

response_code KERNEL_DISPATCH(t_pcb *pcb, t_config *config){
    int cpu_connection = conexion_by_config(config, "IP_CPU", "PUERTO_CPU_DISPATCH");

    t_buffer *buffer = malloc(sizeof(t_buffer));
    serialize_pcb(pcb, buffer);

    t_paquete *paquete = crear_paquete(DISPATCH);
    agregar_a_paquete(paquete, buffer->stream, buffer->size);
    enviar_paquete(paquete, cpu_connection);
    eliminar_paquete(paquete);
    free(buffer->stream);
    free(buffer);

    response_code code = esperar_respuesta(cpu_connection);

    return code;
}

response_code KERNEL_INTERRUPT(t_config *config){
    int cpu_connection = conexion_by_config(config, "IP_CPU", "PUERTO_CPU_DISPATCH");

    t_paquete *paquete = crear_paquete(INTERRUPT);
    enviar_paquete(paquete, cpu_connection);
    eliminar_paquete(paquete);

    response_code code = esperar_respuesta(cpu_connection);

    return code;
}