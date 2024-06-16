#include "kernel.h"
#include <stdio.h>
#include <stdlib.h>
#include<commons/string.h>
#include <commons/config.h>
#include <utils/client.h>
#include "cpu.h"

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

t_instruction* new_instruction_IO(u_int32_t pid,char* interfaz, char* job_unit){
    t_instruction* instruction = malloc(sizeof(t_instruction));
    if (instruction == NULL) {
        return NULL; 
    }

    instruction->pid = pid;
    instruction->interfaz = malloc(string_length(interfaz) * sizeof(char) + 1);
    strcpy(instruction->interfaz, interfaz);

    instruction->job_unit = malloc(string_length(job_unit) * sizeof(char) + 1);
    strcpy(instruction->job_unit, job_unit);

    return instruction;
}

void serializar_instruccion_IO(t_instruction* IO, t_buffer* buffer){
    buffer->offset = 0;
    size_t size = sizeof(u_int32_t); //para pid
    if(IO->interfaz != NULL){
        size += sizeof(u_int32_t); //para la longitud de la cadena interfaz
        size += string_length(IO->interfaz) + 1;
    }
    if(IO->job_unit != NULL){
        size += sizeof(u_int32_t); //para la longitud de la cadena job_unit
        size += string_length(IO->job_unit) + 1;
    }

    buffer->size = size;
    buffer->stream = malloc(size);

    //serializo:
    memcpy(buffer->stream + buffer->offset, &(IO->pid), sizeof(u_int32_t));
    buffer->offset += sizeof(u_int32_t);

    u_int32_t interfaz_length = strlen(IO->interfaz) + 1;
    memcpy(buffer->stream + buffer->offset, &interfaz_length, sizeof(u_int32_t));
    buffer->offset += sizeof(u_int32_t);

    memcpy(buffer->stream + buffer->offset, IO->interfaz, interfaz_length);
    buffer->offset += interfaz_length;

    u_int32_t job_length = strlen(IO->job_unit) + 1;
    memcpy(buffer->stream + buffer->offset, &job_length, sizeof(u_int32_t));
    buffer->offset += sizeof(u_int32_t);

    memcpy(buffer->stream + buffer->offset, IO->job_unit, job_length);
    buffer->offset += job_length;

}

t_instruction* deserializar_instruction_IO (void* stream){
    t_instruction* instruction = malloc(sizeof(t_instruction));

    int offset = 0;

    memcpy(&(instruction->pid), stream + offset, sizeof(u_int32_t));
    offset += sizeof(u_int32_t);

    u_int32_t interf_length;
    memcpy(&interf_length, stream + offset, sizeof(u_int32_t));
    offset += sizeof(u_int32_t);

    instruction->interfaz = malloc(interf_length);
    memcpy(instruction->interfaz, stream + offset, interf_length);
    offset += interf_length;

    u_int32_t job_length;
    memcpy(&job_length, stream + offset, sizeof(u_int32_t));
    offset += sizeof(u_int32_t);

    instruction->job_unit = malloc(job_length);
    memcpy(instruction->job_unit, stream + offset, job_length);
    offset += job_length;

    return instruction;
}