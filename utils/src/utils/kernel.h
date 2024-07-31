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
    uint32_t PC;
    uint8_t AX;
    uint8_t BX;
    uint8_t CX;
    uint8_t DX;
    uint32_t EAX;
    uint32_t EBX;
    uint32_t ECX;//a continuacion los registros que agrego
    uint32_t EDX;
    uint32_t SI;
    uint32_t DI;
} t_register;

typedef enum {
    NEW,
    READY,
    RUNNING,
    BLOCKED,
    EXIT
} t_state;

typedef struct {
    u_int32_t pid;
    u_int32_t pc;
    u_int32_t quantum;
    char* path; 
    t_register *reg;
    t_state prev_state;
} t_pcb;

/**
* @fn    serialize_pcb
* @brief Serialize pcb struct to be able to transport it to other services.
*/
void serialize_pcb(t_pcb* pcb, t_buffer* buffer);

/**
* @fn    deserialize_pcb
* @brief Deserialize pcb struct to be recognized in all the services.
*/
t_pcb* deserialize_pcb(void* stream, int offset);

t_pcb *new_pcb(u_int32_t pid, u_int32_t quantum, char* path, t_state prev_state);

void delete_pcb(t_pcb *pcb);

//Does exactly what it says it does. Doesn't validate input so make sure it's valid beforehand.
uint32_t string_to_uint32(const char *str);

const char* t_state_to_string(t_state state);

#endif
