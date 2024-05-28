#ifndef COMMUNICATION_KERNEL_CPU_H
#define COMMUNICATION_KERNEL_CPU_H

#include <stdio.h>
#include <stdlib.h>
#include <utils/kernel.h>
#include <commons/config.h>

//Serializes and then sends a pcb to the CPU module using the DISPATCH op code
//Port and Ip address are dependant on the config parameter
void *KERNEL_DISPATCH(t_pcb *pcb, t_config *config);

#endif 
