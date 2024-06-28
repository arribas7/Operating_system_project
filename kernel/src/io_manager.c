#include <io_manager.h>

void io_block_interfaz_fs(t_pcb* pcb, t_interfaz *interfaz_fs) {
    // open a detached_thread:
        // move PCB to blocked
        // busy -> send and they will wait on i/o
    // move from blocked -> ready with prev_state
    t_interface* io_interface = find_interface_by_name(interfaz_fs->interfaz);
    
    if(io_interface == NULL){ // interface non-available
        exit_process(pcb, RUNNING, INVALID_INTERFACE);
        return;
    }
    // TODO: send interfaz_fs to io_interface.connection
    // TODO: Wait for response and if it's OK, send pcb to blocked
    // TODO: if response is "BUSY" -> Queue. queue should be on IO module or kernel module?
    move_pcb(pcb, RUNNING, BLOCKED, list_BLOCKED, &mutex_blocked);
   
}

void io_block_instruction(t_pcb* pcb, t_instruction *instruction) {
    t_interface* io_interface = find_interface_by_name(instruction->name);
    
    if(io_interface == NULL){ // interface non-available
        exit_process(pcb, RUNNING, INVALID_INTERFACE);
        return;
    }
    // TODO: send instruction
    move_pcb(pcb, RUNNING, BLOCKED, list_BLOCKED, &mutex_blocked);
}

void io_block_stdin(t_pcb* pcb, t_io_stdin *io_stdin) {
    t_interface* io_interface = find_interface_by_name(io_stdin->interfaz);
    
    if(io_interface == NULL){ // interface non-available
        exit_process(pcb, RUNNING, INVALID_INTERFACE);
        return;
    }
    // TODO: send io_stdin
    move_pcb(pcb, RUNNING, BLOCKED, list_BLOCKED, &mutex_blocked);
}