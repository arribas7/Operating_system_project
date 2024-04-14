#include "commons.h"
#include<stdio.h>
#include<stdlib.h>
#include<commons/string.h>
#include <commons/config.h>
#include <utils/client.h>

t_pcb *nuevo_pcb(void){ // TODO: Una vez bien definida la struct. Pasar por param las props del pcb.
    t_pcb* pcb = malloc(sizeof(t_pcb));
    pcb->pid = 0;
    pcb->pc = 0;
    pcb->quantum = 0;

    t_register* reg = malloc(sizeof(t_register));
    reg->dato = 0;

    pcb->reg = reg;
    return pcb;
}

void serializar_pcb(t_pcb *pcb, uint8_t *buffer, int *offset) {
    memcpy(buffer + *offset, &(pcb->pid), sizeof(int));
    *offset += sizeof(int);

    memcpy(buffer + *offset, &(pcb->pc), sizeof(int));
    *offset += sizeof(int);

    memcpy(buffer + *offset, &(pcb->quantum), sizeof(int));
    *offset += sizeof(int);

    if (pcb->reg != NULL) {
        memcpy(buffer + *offset, &(pcb->reg->dato), sizeof(int));
        *offset += sizeof(int);
    }
}

void deserializar_pcb(uint8_t *buffer, t_pcb *pcb, int *offset) {
    if (pcb == NULL || buffer == NULL) {
        return;
    }

    memcpy(&(pcb->pid), buffer + *offset, sizeof(int));
    *offset += sizeof(int);

    memcpy(&(pcb->pc), buffer + *offset, sizeof(int));
    *offset += sizeof(int);

    memcpy(&(pcb->quantum), buffer + *offset, sizeof(int));
    *offset += sizeof(int);

    pcb->reg = malloc(sizeof(t_register));
    if (pcb->reg == NULL) {
        return;
    }

    memcpy(&(pcb->reg->dato), buffer + *offset, sizeof(int));
    *offset += sizeof(int);
}



int conexion_by_config(t_config *config, char *ip_config, char *puerto_config) {
    char *ip = config_get_string_value(config, ip_config);
    char *puerto = config_get_string_value(config, puerto_config);
    return crear_conexion(ip, puerto);
}
