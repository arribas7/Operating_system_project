#ifndef COMMUNICATION_KERNEL_CPU_H
#define COMMUNICATION_KERNEL_CPU_H

#include <stdio.h>
#include <stdlib.h>
#include <utils/kernel.h>
#include <commons/config.h>
#include <utils/client.h>
#include <utils/cpu.h>

// t_return_dispatch could have or not instruction or resp_ws.
typedef struct {
    t_pcb *pcb_updated;
    op_code resp_code;
    t_instruction* instruction_IO;
    t_ws* resp_ws;
} t_return_dispatch;

//Serializes and then sends a pcb to the CPU module using the DISPATCH op code
//Port and Ip address are dependant on the config parameter
//Returns the response code passed by the CPU
t_return_dispatch *cpu_dispatch(t_pcb *pcb, t_config *config);

//Sends an empty package to the CPU module using the INTERRUPT op code
//Port and Ip address are dependant on the config parameter
//Returns the response code passed by the CPU
op_code cpu_interrupt(t_config *config, op_code reason);

#endif 
