#include "kernel.h"
#include <stdio.h>
#include <stdlib.h>
#include <commons/string.h>
#include <commons/config.h>
#include <utils/client.h>

t_pcb *new_pcb(u_int32_t pid, u_int32_t quantum, char *path)
{
    // Check if path is NULL
    if (path == NULL || *path == '\0')
    {
        // Handle the case where path is empty
        return NULL;
    }

    t_pcb *pcb = malloc(sizeof(t_pcb));
    if (pcb == NULL)
    {
        return NULL;
    }

    pcb->pid = pid;
    pcb->pc = 0;
    pcb->quantum = quantum;

    pcb->path = malloc(strlen(path) + 1);
    if (pcb->path == NULL)
    {
        free(pcb);
        return NULL;
    }
    strcpy(pcb->path, path);

    t_register *reg = malloc(sizeof(t_register));
    if (reg == NULL)
    {
        free(pcb->path);
        free(pcb);
        return NULL;
    }

    reg->PC = 0;
    reg->AX = 0;
    reg->BX = 0;
    reg->CX = 0;
    reg->DX = 0;
    reg->EAX = 0;
    reg->EBX = 0;
    reg->ECX = 0;

    pcb->reg = reg;
    return pcb;
}

void delete_pcb(t_pcb *pcb)
{
    if (pcb != NULL)
    {
        free(pcb->path);
        free(pcb->reg);
        free(pcb);
    }
}

void serialize_pcb(t_pcb *pcb, t_buffer *buffer)
{
    void *aux;

    // Calculate the size needed for serialization
    buffer->size = sizeof(u_int32_t) * 4  // pid, pc, quantum, and path length
                    + strlen(pcb->path) + 1 // path string and null terminator
                    + sizeof(t_register); // entire t_register structure

    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    memcpy(buffer->stream + buffer->offset, &(pcb->pid), sizeof(u_int32_t));
    buffer->offset += sizeof(u_int32_t);

    memcpy(buffer->stream + buffer->offset, &(pcb->pc), sizeof(u_int32_t));
    buffer->offset += sizeof(u_int32_t);

    memcpy(buffer->stream + buffer->offset, &(pcb->quantum), sizeof(u_int32_t));
    buffer->offset += sizeof(u_int32_t);

    // Serialize the length of the path string
    u_int32_t path_length = strlen(pcb->path) + 1; // include null terminator
    memcpy(buffer->stream + buffer->offset, &path_length, sizeof(u_int32_t));
    buffer->offset += sizeof(u_int32_t);

    // Serialize the path string
    memcpy(buffer->stream + buffer->offset, pcb->path, path_length);
    buffer->offset += path_length;

    // Serialize the registers
    memcpy(buffer->stream + buffer->offset, pcb->reg, sizeof(t_register));
    buffer->offset += sizeof(t_register);
}

t_pcb *deserialize_pcb(void *stream)
{
    t_pcb *pcb = malloc(sizeof(t_pcb));

    int offset = 0;
    memcpy(&(pcb->pid), stream + offset, sizeof(u_int32_t));
    offset += sizeof(u_int32_t);

    memcpy(&(pcb->pc), stream + offset, sizeof(u_int32_t));
    offset += sizeof(u_int32_t);

    memcpy(&(pcb->quantum), stream + offset, sizeof(u_int32_t));
    offset += sizeof(u_int32_t);

    // Deserialize the length of the path string
    u_int32_t path_length;
    memcpy(&path_length, stream + offset, sizeof(u_int32_t));
    offset += sizeof(u_int32_t);

    // Allocate memory and deserialize the path string
    pcb->path = malloc(path_length);
    if (pcb->path == NULL) {
        free(pcb);
        return NULL;
    }
    memcpy(pcb->path, stream + offset, path_length);
    offset += path_length;

    // Allocate memory for the registers
    pcb->reg = malloc(sizeof(t_register));
    if (pcb->reg == NULL)
    {
        free(pcb->path);
        free(pcb);
        return NULL;
    }

    // Deserialize the registers
    memcpy(pcb->reg, stream + offset, sizeof(t_register));
    offset += sizeof(t_register);

    return pcb;
}
