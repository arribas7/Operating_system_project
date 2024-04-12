#ifndef KERNEL_MODEL_H_
#define KERNEL_MODEL_H_

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

} t_register;

typedef struct {
    int pid;
    int pc;
    int quantum;
    t_register reg;
} t_pcb;

/**
* @fn    serializar_pcb
* @brief Serializa la estructura pcb para poder transportarla a otros servicios.
*/
void *serializar_pcb(t_pcb *pcb, int bytes);
int conexion_by_config(t_config *config, char *ip_config, char *puerto_config);

#endif
