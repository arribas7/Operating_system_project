#include <communication_kernel_cpu.h>
#include <stdio.h>
#include <stdlib.h>
#include <utils/kernel.h>
#include <utils/server.h>
#include <utils/client.h>
#include <commons/config.h>

t_return_dispatch *handle_dispatch_deserialization(int cpu_connection){
    t_return_dispatch *ret = malloc(sizeof(t_return_dispatch));

    op_code resp_code = (op_code) recibir_operacion(cpu_connection);
    t_ws *resp_ws = NULL;
    t_instruction *instruction_IO = NULL;

    t_list* list_package = recibir_paquete(cpu_connection);
    void *buffer = list_get(list_package, 0);

    switch (resp_code)
    {
        case WAIT:
        case SIGNAL:
            resp_ws = deserializar_wait_o_signal(buffer);
            break;
        case IO_GEN_SLEEP:
        case IO_STDIN_READ:
        case IO_STDOUT_WRITE:
            instruction_IO = deserialize_instruction_IO(buffer);
            break;
        case IO_FS_CREATE:
        case IO_FS_DELETE:
        case IO_FS_TRUNCATE:
        case IO_FS_WRITE:
        case IO_FS_READ:
            resp_interfaz_fs = deserializar_interfaz(buffer);
            break;
    }
    void *pcb_updated_buffer = list_get(list_package, 1);
    t_pcb *pcb_updated = deserialize_pcb(pcb_updated_buffer, 0);

    ret->pcb_updated = pcb_updated;
    ret->resp_code = resp_code;
    ret->resp_ws = resp_ws;
    ret->instruction_IO = instruction_IO;
    ret->interfaz_fs = resp_interfaz_fs;

    log_debug(logger, "PCB PC updated: %d",pcb_updated->pc);

    return ret;
}

t_return_dispatch *cpu_dispatch(t_pcb *pcb, t_config *config){
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

    t_return_dispatch *ret = handle_dispatch_deserialization(cpu_connection);

    liberar_conexion(cpu_connection);
    log_debug(logger, "CPU dispatch connection released");
    return ret;
}

op_code cpu_interrupt(t_config *config, op_code reason){
    int cpu_connection = conexion_by_config(config, "IP_CPU", "PUERTO_CPU_INTERRUPT");

    t_paquete *paquete = crear_paquete(reason);
    enviar_paquete(paquete, cpu_connection);
    eliminar_paquete(paquete);

    op_code response_code = (op_code) recibir_operacion(cpu_connection);
    liberar_conexion(cpu_connection);
    log_debug(logger, "CPU interrupt connection released");

    return response_code;
}