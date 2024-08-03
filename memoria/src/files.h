#ifndef FILES_H
#define FILES_H
#include <stdio.h>
#include <stdint.h> 
#include <stdlib.h>
#include <string.h>
#include <utils/client.h>
#include <pages.h>

typedef struct {
    char *complete_line;
} Instruction;

//Archivo de Instrucciones y Diccionario:
typedef struct {
    Instruction *instructions;
    int size;
    int capacity;
} InstructionDictionary;

// Tabla de Asociación PID a Diccionario:
typedef struct {
    uint32_t pid;
    InstructionDictionary *instruction_dict;
  } PIDToDict;

#define MAX_PROCESSES 100

extern PIDToDict pid_dict_array[MAX_PROCESSES];

extern InstructionDictionary dict;
void handle_create_process(const char *file_path, uint32_t pid,t_config* config);
FILE *open_file(const char *file_path);
void instruction_dictionary_init(InstructionDictionary *dict, int capacity);
void add_pid_instruction_dict(uint32_t pid, InstructionDictionary *dict);
const char *get_complete_instruction(uint32_t pid, int pc);
void load_instructions_from_file(InstructionDictionary *dict, FILE *file);
//void instructions_put(InstructionDictionary *dict, const char *complete_line);
void instruction_dictionary_free(InstructionDictionary *dict);
void send_instruction(const char *instruction, int socket_cliente);

extern void destroy_page_table(void* table);

#endif // FILES_H