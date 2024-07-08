#include <io_manager.h>

void io_block_instruction(t_pcb* pcb, t_instruction *instruction) {
    t_interface* io_interface = find_interface_by_name(instruction->name);
    
    if(io_interface == NULL){ // interface non-available
        exit_process(pcb, RUNNING, INVALID_INTERFACE);
        return;
    }
    // TODO: send instruction to
    move_pcb(pcb, RUNNING, BLOCKED, list_BLOCKED, &mutex_blocked);
}