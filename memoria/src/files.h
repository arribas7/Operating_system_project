#ifndef FILES_H
#define FILES_H
#include <stdio.h>
#include <stdint.h> 
#include <stdlib.h>
#include <string.h>
#include <utils/client.h>


typedef struct {
    char *complete_line;
} Instruction;

typedef struct {
    Instruction *instructions;
    int size;
    int capacity;
} InstructionDictionary;

typedef struct {
    uint32_t pid;
    InstructionDictionary *instruction_dict;
} PIDToDict;

#define MAX_PROCESSES 100

extern PIDToDict pid_dict_array[MAX_PROCESSES];

extern InstructionDictionary dict;

FILE *open_file(const char *file_path);
void handle_create_process(const char *file_path, uint32_t pid);
void instruction_dictionary_init(InstructionDictionary *dict, int capacity);
void instructions_put(InstructionDictionary *dict, const char *complete_line);
void instruction_dictionary_free(InstructionDictionary *dict);
void load_instructions_from_file(InstructionDictionary *dict, FILE *file);
const char *get_complete_instruction(const InstructionDictionary *dict, int index);
//const char *get_complete_instruction(uint32_t pid, int index);

FILE *open_file(const char *file_path);
void send_instruction(const char *instruction, int socket_cliente);
void add_pid_instruction_dict(uint32_t pid, InstructionDictionary *dict);



#endif // FILES_H