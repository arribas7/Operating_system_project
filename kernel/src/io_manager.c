#include <io_manager.h>
#include <utils/cpu.h>

t_io_block_return io_block_instruction(t_pcb* pcb, t_instruction *instruction) {
    t_interface* io_interface = find_interface_by_name(instruction->name);
    
    if(io_interface == NULL){ // interface non-available
        return IO_NOT_FOUND;
    }
    t_paquete* io_instruction_paq = crear_paquete(instruction->code);
    t_buffer* buffer = malloc(sizeof(t_buffer));

    serialize_instruccion_IO(instruction, buffer);
    agregar_a_paquete(io_instruction_paq, buffer->stream, buffer->size);
    enviar_paquete(io_instruction_paq, io_interface->connection);
    free(buffer);
    return IO_SUCCESS_OP;
}