
#ifndef UTILS_INOUT_H_
#define UTILS_INOUT_H_

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <commons/log.h>
#include "client.h"

// ----- Structures -----
typedef struct 
{
    char* IO_name;
    char* IO_type;
} t_interface;

// ----- Declaraciones -----

t_interface* create_IO(char* IO_name, char* IO_type);

t_paquete* create_IO_package(char* IO_name, t_config* config);

t_interface* list_to_IO(t_list* list);

char* get_IO_info(t_interface* interface);

void delete_IO(t_interface* interface);

int valid_instruction(op_code instruction, t_config* config);

// ----- Generic Interface Functions

int generic_IO_wait(int time, t_config* config);

//void iterator(char* value);

#endif
