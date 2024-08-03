#include "kernel.h"
#include <stdio.h>
#include <stdlib.h>
#include<commons/string.h>
#include <commons/config.h>
#include <utils/client.h>
#include "cpu.h"
#include "server.h"

extern t_pcb* pcb_en_ejecucion;
extern int conexion_mem;

t_reg_cpu* nuevo_reg(uint8_t pc) {
    t_reg_cpu *reg_cpu = malloc(sizeof(t_reg_cpu));

    reg_cpu->AX = 0;
    reg_cpu->BX = 0;
    reg_cpu->CX = 0;
    reg_cpu->DI = 0;
    reg_cpu->EAX = 0;
    reg_cpu->EBX = 0;
    reg_cpu->ECX = 0;
    reg_cpu->EDX = 0;
    reg_cpu->PC = pc;
    reg_cpu->SI = 0;    

    return reg_cpu;
}

/*void serializar_pcb(t_pcb *pcb, uint8_t *buffer, int *offset) {
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
}*/

void eliminar_reg(t_reg_cpu *reg) {
    free(reg);
}

// INSTRUCTION

t_instruction* create_instruction_IO(uint32_t pid, op_code code, char* name, uint32_t time, uint32_t physical_address, uint32_t size, char* f_name, uint32_t f_pointer) 
{
    t_instruction* instruction = malloc(sizeof(t_instruction));
    if (instruction == NULL) 
    {
        return NULL; 
    }

    instruction->pid = pid;
    instruction->code = code;
    
    instruction->length_name = strlen(name) + 1;
    instruction->name = malloc(instruction->length_name);
    if (instruction->name == NULL) 
    {
        free(instruction);
        return NULL;
    }
    strcpy(instruction->name, name);

    instruction->time = time;
    instruction->physical_address = physical_address;
    instruction->size = size;

    instruction->length_f_name = strlen(f_name) + 1;
    instruction->f_name = malloc(instruction->length_f_name);
    if (instruction->f_name == NULL) 
    {
        free(instruction);
        return NULL;
    }
    strcpy(instruction->f_name, f_name);

    instruction->f_pointer = f_pointer;

    return instruction;
}

void serialize_instruction_IO(t_instruction* instruction, t_buffer* buffer) {
    buffer->offset = 0;
    size_t size = sizeof(uint32_t) * 7 + sizeof(op_code) + instruction->length_name + instruction->length_f_name;
    buffer->size = size;
    buffer->stream = malloc(size);

    if (buffer->stream == NULL) 
    {
        return;
    }

    // Serialize
    memcpy(buffer->stream + buffer->offset, &(instruction->pid), sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);

    memcpy(buffer->stream + buffer->offset, &(instruction->code), sizeof(op_code));
    buffer->offset += sizeof(op_code);

    memcpy(buffer->stream + buffer->offset, &(instruction->length_name), sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);

    memcpy(buffer->stream + buffer->offset, instruction->name, instruction->length_name);
    buffer->offset += instruction->length_name;

    memcpy(buffer->stream + buffer->offset, &(instruction->time), sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);

    memcpy(buffer->stream + buffer->offset, &(instruction->physical_address), sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);

    memcpy(buffer->stream + buffer->offset, &(instruction->size), sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);

    memcpy(buffer->stream + buffer->offset, &(instruction->length_f_name), sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);

    memcpy(buffer->stream + buffer->offset, instruction->f_name, instruction->length_f_name);
    buffer->offset += instruction->length_f_name;

    memcpy(buffer->stream + buffer->offset, &(instruction->f_pointer), sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);
}

t_instruction* deserialize_instruction_IO(void* stream) 
{    
    t_instruction* instruction = malloc(sizeof(t_instruction));
    if (instruction == NULL) 
    {
        return NULL;
    }

    int offset = 0;

    memcpy(&(instruction->pid), stream + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(&(instruction->code), stream + offset, sizeof(op_code));
    offset += sizeof(op_code);

    memcpy(&(instruction->length_name), stream + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    instruction->name = malloc(instruction->length_name);
    if (instruction->name == NULL) 
    {
        free(instruction);
        return NULL;
    }
    memcpy(instruction->name, stream + offset, instruction->length_name);
    offset += instruction->length_name;

    memcpy(&(instruction->time), stream + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(&(instruction->physical_address), stream + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(&(instruction->size), stream + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t); 

    memcpy(&(instruction->length_f_name), stream + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    instruction->f_name = malloc(instruction->length_f_name);
    if (instruction->f_name == NULL) 
    {
        free(instruction->name);
        free(instruction);
        return NULL;
    }
    memcpy(instruction->f_name, stream + offset, instruction->length_f_name);
    offset += instruction->length_f_name;

    memcpy(&(instruction->f_pointer), stream + offset, sizeof(uint32_t));

    return instruction;
}

t_instruction* receive_instruction_IO(int socket_cliente) {
    int size;
    void *buffer = recibir_buffer(&size, socket_cliente);
    if (buffer == NULL) {
        return NULL;
    }

    t_instruction* instruction = deserialize_instruction_IO(buffer);
    free(buffer);
    return instruction;
}

void send_instruction_IO(t_instruction* instruction, int socket_cliente) {
    t_paquete* package = crear_paquete(instruction->code);
    serialize_instruction_IO(instruction, package->buffer);
    enviar_paquete(package, socket_cliente);
    eliminar_paquete(package);   
}

void delete_instruction_IO(t_instruction* instruction) {
    free(instruction->name);
    free(instruction->f_name);
    free(instruction);
}

// ----------

t_ws* new_ws(char* recurso){
    t_ws* ws = malloc(sizeof(t_ws));

    ws->recurso_length = string_length(recurso);
    ws->recurso = strdup(recurso);

    return ws;
}

void serializar_wait_o_signal(t_ws* ws, t_buffer* buffer){
    buffer->offset = 0;
    u_int32_t recurso_length = strlen(ws->recurso) + 1;
    u_int32_t size = sizeof(u_int32_t) + recurso_length;
    buffer->size = size;
    buffer->stream = malloc(size);

    //serializo:
    
    memcpy(buffer->stream + buffer->offset, &(recurso_length), sizeof(u_int32_t));
    buffer->offset += sizeof(u_int32_t);

    memcpy(buffer->stream + buffer->offset, ws->recurso, recurso_length);
    buffer->offset += recurso_length;
}

t_ws* deserializar_wait_o_signal(void* stream){
    t_ws* ws = malloc(sizeof(t_ws));
    int offset = 0;

    u_int32_t recurso_length;
    memcpy(&recurso_length, stream + offset, sizeof(u_int32_t));
    offset += sizeof(u_int32_t);

    ws->recurso_length = recurso_length;

    ws->recurso = malloc(recurso_length);
    memcpy(ws->recurso, stream + offset, recurso_length);
    offset += recurso_length;

    return ws;
}

t_ws* recibir_wait_o_signal(int socket_cliente) {
    int size;
    void *buffer = recibir_buffer(&size, socket_cliente);
    if (buffer == NULL) {
        return NULL;
    }

    t_ws* ws = deserializar_wait_o_signal(buffer);
    free(buffer);
    return ws;
}

void destroy_ws(t_ws* ws) {
    free(ws->recurso);
    free(ws);
}