
#include <stdlib.h>
#include <stdio.h>
#include <commons/log.h>
#include <commons/config.h>
#include <utils/client.h>
#include <utils/server.h>
#include <utils/inout.h>
#include "generic_st.h"

// ---------- DEFINITIONS ----------

// ----- FUNCTIONS -----

// Generic IO

int interface_wait(t_instruction* instruction, t_config* config) 
{
    int work_unit_time = config_get_int_value(config, "TIEMPO_UNIDAD_TRABAJO");
    int time_to_wait = work_unit_time * instruction->time;

    return sleep(time_to_wait);
}

// STDIN / STDOUT IO

char* write_console(t_instruction* instruction) 
{
    char* word_to_send = malloc((instruction->size) + 1);
	while(1) 
	{
            char* word = readline("> ");
            strncpy(word_to_send, word, instruction->size);
            free(word);
            break;
	}
    return strcat(word_to_send, "\0");
}

void send_write_request(t_instruction* instruction, int connection) 
{
    char* word_to_write = write_console(instruction);
    t_req_to_w* mem_req = req_to_write(instruction->pid, word_to_write, instruction->physical_address);
    send_req_to_w(mem_req, connection);
    delete_req_to_w(mem_req);
    free(word_to_write);
}

void send_read_request(t_instruction* instruction, int connection) 
{
    t_req_to_r* mem_req = req_to_read(instruction->pid, instruction->size, instruction->physical_address);
    send_req_to_r(mem_req, connection);
    delete_req_to_r(mem_req);
}

char* receive_word(int socket_cliente)
{
    int size;
    char* word = recibir_buffer(&size, socket_cliente);
    return word;
}