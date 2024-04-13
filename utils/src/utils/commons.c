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
        //log_trace(logger, "Puntero nulo proporcionado a deserializar_pcb");
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
        //log_trace(logger, "Error al asignar memoria para t_register");
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

t_pcb* recibir_pcb(int socket_cliente) {
    int size;
    void *buffer = recibir_buffer(&size, socket_cliente);
    
    if (buffer == NULL) {
        //log_trace(logger, "Error recibiendo el buffer");
        return NULL;
    }

    // Iniciar deserialización de t_pcb
    t_pcb *pcb = malloc(sizeof(t_pcb));
    if (pcb == NULL) {
        free(buffer);
        //log_trace(logger, "Error al asignar memoria para pcb");
        return NULL;
    }

    int offset = 0;
    deserializar_pcb(buffer, pcb, &offset);
    free(buffer); // Liberar el buffer una vez que hemos deserializado los datos

    return pcb;
}
