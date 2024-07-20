
#include <stdlib.h>
#include <stdio.h>
#include <commons/log.h>
#include <commons/config.h>
#include <readline/readline.h>
#include <utils/client.h>
#include <utils/server.h>
#include <utils/inout.h>
#include <utils/cpu.h>

// ----- FUNCTIONS -----

// GENERIC

int wait_time_units(uint32_t time, t_config* config);

// STDIN - STDOUT

char* write_console(t_instruction* instruction);
void send_write_request(t_instruction* instruction, int connection);
void send_read_request(t_instruction* instruction, int connection);
char* receive_word(int connection);