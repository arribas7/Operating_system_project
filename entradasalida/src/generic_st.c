
#include <stdlib.h>
#include <stdio.h>
#include <commons/log.h>
#include <commons/config.h>
#include <utils/client.h>
#include <utils/server.h>
#include <utils/inout.h>
#include "generic_st.h"

// ----- DEFINITIONS -----

// -- SPECIFICS INTERFACE TYPE FUNCTIONS

// GENERIC IO FUNCTIONS

int interface_wait(t_instruction* instruction, t_config* config) 
{
    int work_unit_time = config_get_int_value(config, "TIEMPO_UNIDAD_TRABAJO");
    int time_to_wait = work_unit_time * instruction->time;

    return usleep(time_to_wait);
}

// STDIN / STDOUT FUNCTIONS

char* write_console() 
{
	char* word;
	while(1) 
	{
        word = readline("> ");
		break;
	}
    return word;
}

void write_in_memory(t_instruction* instruction, char* word, int connection) 
{

}

char* read_from_memory(t_instruction* instruction, int connection) 
{
    return "";
}