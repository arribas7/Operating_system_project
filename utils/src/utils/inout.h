
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

// ----- Statements -----

// -- To Kernel

/**
 * @fn create_IO
 * @brief Create a pointer to a struct t_interface
**/
t_interface* create_IO(char* IO_name, char* IO_type);

/**
 * @fn list_to_IO
 * @brief Extract the information from a list and create a pointer to a struct t_interface 
**/
t_interface* list_to_IO(t_list* list);

/**
 * @fn get_IO_name
 * @brief Get the IO name from the struct t_interface
**/ 
char* get_IO_name(t_interface* interface);

/**
 * @fn get_IO_type
 * @brief Get the IO type from the struct t_interface
**/
char* get_IO_type(t_interface* interface);

/**
 * @fn delete_IO
 * @brief free the space of the struct t_interface
**/
void delete_IO(t_interface* interface);

// -- To IO

/**
 * @fn create_IO_package
 * @brief create a package to send the IO information
**/ 
t_paquete* create_IO_package(char* IO_name, t_config* config);

/**
 * @fn IO:type_from_config
 * @brief get the IO type from the struct t_config
**/ 
char* IO_type_from_config(t_config* config);

/**
 * @fn is_valid_instructiom
 * @brief validate the received instruction from the Kernel
**/ 
bool is_valid_instruction(op_code instruction, t_config* config);

/**
 * @fn IO_inform_kernel
 * @brief send a message to Kernel about the event status
**/ 
void IO_inform_kernel(int connection, int ret);

// ----- Generic Interface Functions

/**
 * @fn generic_IO_wait
 * @brief Wait time microseconds * unit_work_time
**/ 
int generic_IO_wait(int time, t_config* config);

//void iterator(char* value);

#endif