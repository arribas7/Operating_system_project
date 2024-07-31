#include <io_manager.h>
#include <utils/cpu.h>
#include <long_term_scheduler.h>

extern t_interface_list* interface_list;

void io_block(t_pcb* pcb, t_instruction *instruction) {
    t_interface* io_interface = find_interface_by_name(interface_list ,instruction->name);

    if(io_interface == NULL){
        exit_process(pcb, RUNNING, INVALID_INTERFACE);
        return;
    }
    send_instruction_IO(instruction, io_interface->connection);
    log_info(logger, "PID: <%d> - Bloqueado por: <%s>", pcb->pid, instruction->name);
    move_pcb(pcb, RUNNING, BLOCKED, list_BLOCKED, &mutex_blocked);
}

void io_unblock(int pid) {
    sem_wait(&sem_unblock);
    sem_post(&sem_unblock);
    move_pcb_from_to_by_pid(pid, BLOCKED, list_BLOCKED, &mutex_blocked, READY, list_READY, &mutex_ready);
}