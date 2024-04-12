#include "client.h"
#include<stdio.h>
#include<stdlib.h>
#include<commons/string.h>
#include <commons/config.h>
#include <utils/client.h>

void *serializar_pcb(t_pcb *pcb, int bytes) {
    void *magic = malloc(bytes);
    int desplazamiento = 0;

    memcpy(magic + desplazamiento, &(pcb->pid), sizeof(int));
    desplazamiento += sizeof(int);

    memcpy(magic + desplazamiento, &(pcb->pc), sizeof(int));
    desplazamiento += sizeof(int);

    memcpy(magic + desplazamiento, &(pcb->quantum), sizeof(int));
    desplazamiento += sizeof(int);

    memcpy(magic + desplazamiento, &(pcb->reg), sizeof(t_register));
    desplazamiento += sizeof(t_register);

    return magic;
}

int conexion_by_config(t_config *config, char *ip_config, char *puerto_config) {
    char *ip = config_get_string_value(config, ip_config);
    char *puerto = config_get_string_value(config, puerto_config);
    return crear_conexion(ip, puerto);
}
