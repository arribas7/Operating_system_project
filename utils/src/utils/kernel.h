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
    uint32_t ECX;
} t_register;

typedef struct {
    u_int32_t pid;
    u_int32_t pc;
    u_int32_t quantum;
    char* path; 
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
/*
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
    u_int32_t dato; // TODO: TBD, aqui deberia esta la lista de instrucciones a ejecutar
} t_register;

typedef struct {
    u_int32_t pid;
    u_int32_t pc;
    u_int32_t quantum;
    t_register *reg;
    u_int32_t instruccionesLength;
    char** instrucciones; //instrucciones a ejecutar, ej: MOV, RESIZE
} t_pcb;


/**
* @fn    serializar_pcb
* @brief Serializa la estructura pcb para poder transportarla a otros servicios.
*/
//void serializar_pcb(t_pcb* pcb, t_buffer* buffer);

//t_pcb* deserializar_pcb(void* stream);

//t_pcb* nuevo_pcb(int pid, int pc, int quantum, char** instrucciones, int instruccionesLength);



//void eliminar_pcb(t_pcb *pcb);
