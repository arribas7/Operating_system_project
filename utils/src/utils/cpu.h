// kernel.h tiene las estructuras y funciones relacionadas a la comunicación entre módulos.
#ifndef UTILS_CPU_H_
#define UTILS_CPU_H_

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
    u_int32_t dato; // TODO: TBD
} t_register;

typedef struct {
    u_int32_t pid;
    u_int32_t pc;
    u_int32_t quantum;
    t_register *reg;
} t_pcb;

/**
* @fn    serializar_pcb
* @brief Serializa la estructura pcb para poder transportarla a otros servicios.
*/
void serializar_pcb(t_pcb* pcb, t_buffer* buffer);

/**
* @fn    deserializar_pcb
* @brief Deserializa la estructura pcb para identificarla desde otros servicios.
*/
t_pcb* deserializar_pcb(void* stream);

t_pcb *nuevo_pcb(int pid);

void eliminar_pcb(t_pcb *pcb);

#endif
