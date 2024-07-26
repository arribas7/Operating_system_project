
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

int wait_time_units(uint32_t time, t_config* config) 
{
    int unit_time = config_get_int_value(config, "TIEMPO_UNIDAD_TRABAJO");
    int time_to_wait = unit_time * time;

    return usleep(time_to_wait * 1000);
}

// STDIN / STDOUT IO

char* write_console(t_instruction* instruction) 
{
    /*
    char* to_send = malloc(instruction->size);
    char* word = readline("--> ");
    strncpy(to_send, word, instruction->size);
    */
   
    char* to_send = malloc(instruction->size);
    char* word;
    log_info(logger, "Ingrese una palabra de %d bytes", instruction->size);
    while(1) 
    {
        word = readline("--> ");
        if (strlen(word) == instruction->size) 
        {
            strncpy(to_send, word, instruction->size);
            break;
        } else {
            log_warning(logger, "Longitud incorrecta");
            free(word);
        }
    }
    free(word);
    return to_send;
}

void send_write_request(t_instruction* instruction, int connection) 
{
    char* word_to_write = write_console(instruction);
    t_req_to_w* mem_req = req_to_write(instruction->pid, word_to_write, instruction->physical_address);
    send_req_to_w(mem_req, connection);
    free(word_to_write);
}

void send_read_request(t_instruction* instruction, int connection) 
{
    t_req_to_r* mem_req = req_to_read(instruction->pid, instruction->size, instruction->physical_address);
    send_req_to_r(mem_req, connection);
}

char* receive_word(int connection)
{
    char* word = "";
    int op_code = recibir_operacion(connection);
    switch(op_code) 
    {
        case MENSAJE:
            int size;
            word = recibir_buffer(&size, connection);
            break;
        default:
            log_error(logger, "ERROR RECEIVING WORD FROM MEMORY");
            break;
    }
    return word;
}