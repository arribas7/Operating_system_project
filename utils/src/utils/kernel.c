#include "kernel.h"
#include <stdio.h>
#include <stdlib.h>
#include<commons/string.h>
#include <commons/config.h>
#include <utils/client.h>

t_pcb *new_pcb(int pid) { // TODO: Una vez bien definida la struct. Pasar por param las props del pcb.
    t_pcb *pcb = malloc(sizeof(t_pcb));
    pcb->pid = pid;
    pcb->pc = 0;
    pcb->quantum = 0;

    t_register *reg = malloc(sizeof(t_register));
    reg->dato = 0;

    pcb->reg = reg;
    return pcb;
}

void delete_pcb(t_pcb *pcb) {
    free(pcb->reg);
    free(pcb);
}

uint32_t string_to_uint32(const char *str) {
    return (uint32_t)strtoul(str, NULL, 10);
}

void serialize_pcb(t_pcb* pcb, t_buffer* buffer){
    void* aux;

    buffer->size = sizeof(u_int32_t) * 4;

    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    memcpy(buffer->stream + buffer->offset, &(pcb->pid), sizeof(u_int32_t));
    buffer->offset += sizeof(u_int32_t);

    memcpy(buffer->stream + buffer->offset, &(pcb->pc), sizeof(u_int32_t));
    buffer->offset += sizeof(u_int32_t);

    memcpy(buffer->stream + buffer->offset, &(pcb->quantum), sizeof(u_int32_t));
    buffer->offset += sizeof(u_int32_t);

    if (pcb->reg != NULL) {
        memcpy(buffer->stream + buffer->offset, &(pcb->reg->dato), sizeof(u_int32_t));
        buffer->offset += sizeof(u_int32_t);
    }
}

t_pcb* deserialize_pcb(void* stream) {
    t_pcb* pcb = new_pcb(0);

    int offset = 0;
    memcpy(&(pcb->pid), stream + offset, sizeof(u_int32_t));
    offset += sizeof(u_int32_t);

    memcpy(&(pcb->pc), stream + offset, sizeof(u_int32_t));
    offset += sizeof(u_int32_t);
    
    memcpy(&(pcb->quantum), stream + offset, sizeof(u_int32_t));
    offset += sizeof(u_int32_t);

    pcb->reg = malloc(sizeof(t_register));
    if (pcb->reg == NULL) {
        return;
    }

    memcpy(&(pcb->reg->dato), stream + offset, sizeof(u_int32_t));

    return pcb;
}