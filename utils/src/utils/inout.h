

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
#include <commons/txt.h>
#include <pthread.h>
#include "client.h"

// ----- STRUCTURES -----
typedef struct 
{
    uint32_t length_name;
    char* name;
    uint32_t length_type;
    char* type;
    int connection;
} t_interface;

typedef struct
{
    t_list* list;
    pthread_mutex_t mutex;
} t_interface_list;

typedef struct 
{
    uint32_t pid;
    uint32_t length_name;
    char* name;
    uint32_t job_unit;
    uint32_t length_path;
    char* path;
} t_instruction;

// ----- FUNCTIONS -----

// -- GENERAL --
// CREATE / DELETE INTERFACES

// Create a pointer to a struct t_interface
t_interface* create_interface(char* name, char* type, int connection);

// Free the space of the struct t_interface
void delete_interface(t_interface* interface);

// INSTRUCTION

// Create a instruction struct
t_instruction* create_instruction(uint32_t pid, char* name, uint32_t job_unit, char* path);

// Serialize instruction
t_buffer* serialize_instruction(t_instruction* instruction);

// Deserialize instruction
t_instruction* desearialize_instruction(t_buffer* buffer);

// Send a instruction
void send_instruction(op_code code, t_instruction* instruction, int connection);

// Receive a instruction
t_instruction* receive_instruction(int connection);

// Delete a instruction
void delete_instruction(t_instruction* instruction);

// SEND

// Send a confirmation to the connection
int send_confirmation(int connection, uint32_t* response);

// Receive a confirmation from the connection
int receive_confirmation(int connection, uint32_t* received);

// Inform by screen the result of the operation
void result_of_operation(uint32_t);

// -- TO KERNEL --

// Extract the information from a list and create a pointer to a struct t_interface 
t_interface* list_to_interface(t_list* list, int connection);

// GETTERS

// Get the interface name from the struct t_interface 
char* get_interface_name(t_interface* interface);

// Get the interface type from the struct t_interface
char* get_interface_type(t_interface* interface);

// Get the interface connection number to send instructions
int get_interface_connection(t_interface* interface);

// LIST 

// Create the list to save interfaces
t_interface_list* create_interface_list();

// Add the interface to the list
void add_interface_to_list(t_interface_list* list, t_interface* interface);

// Find the interface by the name
t_interface* find_interface_by_name(char* name);

// -- TO INTERFACE --
// PACKAGE

// Create a package to send the interface information 
t_paquete* interface_to_package(t_interface* interface);

// GETTERS

// Get the interface type from the struct t_config
char* type_from_config(t_config* config);

// VALIDATION

// Validate the received instruction from the Kernel 
bool is_valid_instruction(op_code instruction, char* code, t_config* config);

//void iterator(char* value);

#endif