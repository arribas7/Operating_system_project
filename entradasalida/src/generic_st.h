
#include <stdlib.h>
#include <stdio.h>
#include <commons/log.h>
#include <commons/config.h>
#include <readline/readline.h>
#include <utils/client.h>
#include <utils/server.h>
#include <utils/inout.h>
#include <utils/cpu.h>

// ----- STRUCTURES -----

// GENERIC IO FUNCTION

// Wait time microseconds * unit_work_time 
int interface_wait(t_instruction* instruction, t_config* config);

// STDIN - STDOUT

// Write a word in console
char* write_console();

// Write the word in memory using the address
void write_in_memory(t_instruction* instruction, char* word, int connection);

// Read a word from the memory using the adress
char* read_from_memory(t_instruction* instruction, int connection);