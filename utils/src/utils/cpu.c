#include "kernel.h"
#include <stdio.h>
#include <stdlib.h>
#include<commons/string.h>
#include <commons/config.h>
#include <utils/client.h>
#include "cpu.h"
#include "server.h"

t_reg_cpu* nuevo_reg(uint8_t pc) { // TODO: Una vez bien definida la struct. Pasar por param las props del pcb.
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

void serializar_reg(t_reg_cpu* reg, t_buffer* buffer){

    buffer->size = sizeof(u_int32_t) * 3 + sizeof(u_int8_t) * 7;

    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);

    memcpy(buffer->stream + buffer->offset, &(reg->PC), sizeof(u_int32_t));
    buffer->offset += sizeof(u_int32_t);

    memcpy(buffer->stream + buffer->offset, &(reg->AX), sizeof(u_int8_t));
    buffer->offset += sizeof(u_int8_t);

    memcpy(buffer->stream + buffer->offset, &(reg->BX), sizeof(u_int8_t));
    buffer->offset += sizeof(u_int8_t);

    memcpy(buffer->stream + buffer->offset, &(reg->CX), sizeof(u_int8_t));
    buffer->offset += sizeof(u_int8_t);

    memcpy(buffer->stream + buffer->offset, &(reg->DI), sizeof(u_int8_t));
    buffer->offset += sizeof(u_int8_t);

    memcpy(buffer->stream + buffer->offset, &(reg->EAX), sizeof(u_int8_t));
    buffer->offset += sizeof(u_int8_t);
    
    memcpy(buffer->stream + buffer->offset, &(reg->ECX), sizeof(u_int8_t));
    buffer->offset += sizeof(u_int8_t);

    memcpy(buffer->stream + buffer->offset, &(reg->EDX), sizeof(u_int8_t));
    buffer->offset += sizeof(u_int8_t);

    memcpy(buffer->stream + buffer->offset, &(reg->SI), sizeof(u_int32_t));
    buffer->offset += sizeof(u_int32_t);

    memcpy(buffer->stream + buffer->offset, &(reg->DI), sizeof(u_int32_t));
    buffer->offset += sizeof(u_int32_t);
}

t_reg_cpu* deserializar_reg(void* stream) {
    t_reg_cpu* reg = nuevo_reg(0);

    int offset = 0;
    memcpy(&(reg->PC), stream + offset, sizeof(u_int32_t));
    offset += sizeof(u_int32_t);

    memcpy(&(reg->AX), stream + offset, sizeof(u_int8_t));
    offset += sizeof(u_int8_t);
    
    memcpy(&(reg->BX), stream + offset, sizeof(u_int8_t));
    offset += sizeof(u_int8_t);

    memcpy(&(reg->CX), stream + offset, sizeof(u_int8_t));
    offset += sizeof(u_int8_t);

    memcpy(&(reg->DI), stream + offset, sizeof(u_int8_t));
    offset += sizeof(u_int8_t);

    memcpy(&(reg->EAX), stream + offset, sizeof(u_int8_t));
    offset += sizeof(u_int8_t);

    memcpy(&(reg->ECX), stream + offset, sizeof(u_int8_t));
    offset += sizeof(u_int8_t);

    memcpy(&(reg->EDX), stream + offset, sizeof(u_int8_t));
    offset += sizeof(u_int8_t);

    memcpy(&(reg->SI), stream + offset, sizeof(u_int32_t));
    offset += sizeof(u_int32_t);

    memcpy(&(reg->DI), stream + offset, sizeof(u_int32_t));
    offset += sizeof(u_int32_t);

    return reg;
}

void eliminar_reg(t_reg_cpu *reg) {
    free(reg);
}

// INSTRUCTION

t_instruction* create_instruction_IO(uint32_t pid, op_code code, char* name, uint32_t time, char* path) {
    t_instruction* instruction = malloc(sizeof(t_instruction));
    if (instruction == NULL) {
        return NULL; 
    }

    instruction->pid = pid;
    instruction->code = code;
    instruction->length_name = strlen(name) + 1;
    instruction->name = malloc(instruction->length_name);
    if (instruction->name == NULL) {
        free(instruction);
        return NULL;
    }
    strcpy(instruction->name, name);

    instruction->time = time;

    instruction->length_path = strlen(path) + 1;
    instruction->path = malloc(instruction->length_path);
    if (instruction->path == NULL) {
        free(instruction);
        return NULL;
    }
    strcpy(instruction->path, path);

    return instruction;
}

void serialize_instruccion_IO(t_instruction* instruction, t_buffer* buffer) {
    buffer->offset = 0;
    size_t size = sizeof(uint32_t) * 4 + sizeof(op_code) + instruction->length_name + instruction->length_path;
    buffer->size = size;
    buffer->stream = malloc(size);

    if (buffer->stream == NULL) {
        return;
    }

    // Serialize:
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

    memcpy(buffer->stream + buffer->offset, &(instruction->length_path), sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);

    memcpy(buffer->stream + buffer->offset, instruction->path, instruction->length_path);
    buffer->offset += instruction->length_path;
}

t_instruction* deserialize_instruction_IO(void* stream) {
    t_instruction* instruction = malloc(sizeof(t_instruction));
    if (instruction == NULL) {
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
    if (instruction->name == NULL) {
        free(instruction);
        return NULL;
    }
    memcpy(instruction->name, stream + offset, instruction->length_name);
    offset += instruction->length_name;

    memcpy(&(instruction->time), stream + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(&(instruction->length_path), stream + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    instruction->path = malloc(instruction->length_path);
    if (instruction->path == NULL) {
        free(instruction->name);
        free(instruction);
        return NULL;
    }
    memcpy(instruction->path, stream + offset, instruction->length_path);

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
    serialize_instruccion_IO(instruction, package->buffer);
    enviar_paquete(package, socket_cliente);
}

void delete_instruction_IO(t_instruction* instruction) {
    free(instruction->name);
    free(instruction->path);
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
    size_t size;
    if(ws->recurso != NULL){
        size+= sizeof(u_int32_t);
        size+= string_length(ws->recurso) + 1;
    }

    buffer->size = size;
    buffer->stream = malloc(size);

    //serializo:
    u_int32_t recurso_length = strlen(ws->recurso) + 1;
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
    memcpy(&(ws->recurso), stream + offset, recurso_length); //esta bien el primer argumento o es sin el &
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

//IO_FS:

void serializar_interfaz(t_interfaz* interfaz, t_buffer* buffer){
    buffer->offset = 0;
    size_t size;
    if(interfaz->interfaz != NULL){
        size+= sizeof(u_int32_t);
        size+= string_length(interfaz->interfaz) + 1;
    }

    if(interfaz->nombre_archivo != NULL){
        size+= sizeof(u_int32_t);
        size+= string_length(interfaz->nombre_archivo) + 1;
    }

    size+= sizeof(u_int32_t) * 3;

    buffer->size = size;
    buffer->stream = malloc(size);

    //serializo:
    u_int32_t interfaz_length = strlen(interfaz->interfaz) + 1;
    memcpy(buffer->stream + buffer->offset, &(interfaz_length), sizeof(u_int32_t));
    buffer->offset += sizeof(u_int32_t);

    memcpy(buffer->stream + buffer->offset, interfaz->interfaz, interfaz_length);
    buffer->offset += interfaz_length;

    u_int32_t nombre_archivo_length = strlen(interfaz->nombre_archivo) + 1;
    memcpy(buffer->stream + buffer->offset, &(nombre_archivo_length), sizeof(u_int32_t));
    buffer->offset += sizeof(u_int32_t);

    memcpy(buffer->stream + buffer->offset, interfaz->nombre_archivo, nombre_archivo_length);
    buffer->offset += nombre_archivo_length;

    memcpy(buffer->stream + buffer->offset, &(interfaz->direccion_fisica), sizeof(u_int32_t));
    buffer->offset += sizeof(u_int32_t);

    memcpy(buffer->stream + buffer->offset, &(interfaz->tamanio_bytes), sizeof(u_int32_t));
    buffer->offset += sizeof(u_int32_t);

    memcpy(buffer->stream + buffer->offset, &(interfaz->puntero_archivo), sizeof(u_int32_t));
    buffer->offset += sizeof(u_int32_t);
}

t_interfaz* deserializar_interfaz(void* stream){
    t_interfaz* interfaz = malloc(sizeof(t_interfaz));
    int offset = 0;

    u_int32_t interfaz_length;
    memcpy(&interfaz_length, stream + offset, sizeof(u_int32_t));
    offset += sizeof(u_int32_t);

    interfaz->interfaz_length = interfaz_length;

    interfaz->interfaz = malloc(interfaz_length);
    memcpy(&(interfaz->interfaz), stream + offset, interfaz_length);
    offset += interfaz_length;

    u_int32_t nombre_archivo_length;
    memcpy(&nombre_archivo_length, stream + offset, sizeof(u_int32_t));
    offset += sizeof(u_int32_t);

    interfaz->nombre_archivo_length = nombre_archivo_length;

    interfaz->nombre_archivo = malloc(nombre_archivo_length);
    memcpy(&(interfaz->nombre_archivo), stream + offset, nombre_archivo_length);
    offset += nombre_archivo_length;

    memcpy(&(interfaz->direccion_fisica), stream + offset, sizeof(u_int32_t));
    offset += sizeof(u_int32_t);

    memcpy(&(interfaz->tamanio_bytes), stream + offset, sizeof(u_int32_t));
    offset += sizeof(u_int32_t);

    memcpy(&(interfaz->puntero_archivo), stream + offset, sizeof(u_int32_t));
    offset += sizeof(u_int32_t);

    return interfaz;
}

//en caso de no necesitar algun argumento poner 0 o NULL
t_interfaz* new_interfaz(char* interfazs, char* nombre_archivo, u_int32_t direccion_fisica, u_int32_t tamanio_bytes, u_int32_t puntero_archivo){
    t_interfaz* interfaz = malloc(sizeof(t_interfaz));
    if (interfaz == NULL) {
        return NULL; 
    }

    interfaz->interfaz_length = string_length(interfazs);
    interfaz->interfaz = strdup(interfazs);
    interfaz->nombre_archivo_length = string_length(nombre_archivo);
    interfaz->nombre_archivo = strdup(nombre_archivo);
    interfaz->direccion_fisica = direccion_fisica;
    interfaz->tamanio_bytes = tamanio_bytes;
    interfaz->puntero_archivo = puntero_archivo;
    
    return interfaz;
}

t_interfaz* recibir_interfaz(int socket_cliente) {
    int size;
    void *buffer = recibir_buffer(&size, socket_cliente);
    if (buffer == NULL) {
        return NULL;
    }

    t_interfaz* interfaz = deserializar_interfaz(buffer);
    free(buffer);
    return interfaz;
}