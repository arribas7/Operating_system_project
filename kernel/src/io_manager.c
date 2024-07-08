#include <io_manager.h>
#include <utils/cpu.h>

void io_block_instruction(t_pcb* pcb, op_code code, t_instruction *instruction) {
    t_interface* io_interface = find_interface_by_name(instruction->name);
    
    if(io_interface == NULL){ // interface non-available
        exit_process(pcb, RUNNING, INVALID_INTERFACE);
        return;
    }
    t_paquete* io_instruction_paq = crear_paquete(code);
    t_buffer* buffer = malloc(sizeof(t_buffer));

    serialize_instruccion_IO(instruction, buffer);
    agregar_a_paquete(io_instruction_paq, buffer->stream, buffer->size);
    enviar_paquete(io_instruction_paq, io_interface->connection);
    free(buffer);
    move_pcb(pcb, RUNNING, BLOCKED, list_BLOCKED, &mutex_blocked);
}