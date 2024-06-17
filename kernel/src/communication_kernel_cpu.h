#ifndef COMMUNICATION_KERNEL_CPU_H
#define COMMUNICATION_KERNEL_CPU_H

#include <stdio.h>
#include <stdlib.h>
#include <utils/kernel.h>
#include <commons/config.h>
#include <utils/client.h>

//Serializes and then sends a pcb to the CPU module using the DISPATCH op code
//Port and Ip address are dependant on the config parameter
//Returns the response code passed by the CPU
op_code cpu_dispatch(t_pcb *pcb, t_config *config);

//Sends an empty package to the CPU module using the INTERRUPT op code
//Port and Ip address are dependant on the config parameter
//Returns the response code passed by the CPU
op_code cpu_interrupt(t_config *config);

#endif 
