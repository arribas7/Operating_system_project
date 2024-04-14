// model.h tiene las estructuras y funciones relacionadas a la comunicación entre módulos.
#ifndef UTILS_MODEL_H_
#define UTILS_MODEL_H_

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <commons/log.h>
#include <commons/config.h>

typedef struct {
    int dato; // TODO: TBD
} t_register;

typedef struct {
    int pid;
    int pc;
    int quantum;
    t_register *reg;
} t_pcb;

/**
* @fn    serializar_pcb
* @brief Serializa la estructura pcb para poder transportarla a otros servicios.
*/
void serializar_pcb(t_pcb *pcb, uint8_t *buffer, int *offset);
/**
* @fn    deserializar_pcb
* @brief Deserializa la estructura pcb para identificarla desde otros servicios.
*/
void deserializar_pcb(uint8_t *buffer, t_pcb *pcb, int *offset);
t_pcb *nuevo_pcb(void);
#endif
