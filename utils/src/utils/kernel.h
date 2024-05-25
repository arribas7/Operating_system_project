// kernel.h tiene las estructuras y funciones relacionadas a la comunicación entre módulos.
#ifndef UTILS_KERNEL_H_
#define UTILS_KERNEL_H_

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <commons/log.h>
#include "client.h"

typedef struct {
    u_int32_t PC; 
    u_int8_t AX;
    u_int8_t BX;
    u_int_8_t CX;
    u_int_8_t DX;
    u_int_32_t EAX;
    u_int_32_t EBX;
    u_int_32_t ECX;
} t_register;

typedef struct {
    u_int32_t pid;
    u_int32_t pc;
    u_int32_t quantum;
    //char* path; 
    t_register *reg;
} t_pcb;

typedef struct {
    u_int32_t instruction;
} t_syscall;

/**
* @fn    serialize_pcb
* @brief Serialize pcb struct to be able to transport it to other services.
*/
void serialize_pcb(t_pcb* pcb, t_buffer* buffer);

/**
* @fn    deserialize_pcb
* @brief Deserialize pcb struct to be recognized in all the services.
*/
t_pcb* deserialize_pcb(void* stream);

t_pcb *new_pcb(u_int32_t pid, u_int32_t quantum, char* path);

void delete_pcb(t_pcb *pcb);

#endif
