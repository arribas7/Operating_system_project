

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
#include <pthread.h>
#include "client.h"

// ----- STRUCTURES -----
typedef struct 
{
    char* name;
    char* type;
    int connection;
} t_interface;

typedef struct
{
    t_list* list;
    pthread_mutex_t mutex;
} t_interface_list;

// ----- FUNCTIONS -----

// -- TO KERNEL --
// CREATE INTERFACES

// Create a pointer to a struct t_interface
t_interface* create_interface(char* name, char* type, int connection);

// Extract the information from a list and create a pointer to a struct t_interface 
t_interface* list_to_interface(t_list* list, int connection);


// GETTERS

// Get the interface name from the struct t_interface 
char* get_interface_name(t_interface* interface);

// Get the interface type from the struct t_interface
char* get_interface_type(t_interface* interface);

// Get the interface connection number to send instructions
int get_interface_connection(t_interface* interface);


// DELETE

// Free the space of the struct t_interface
void delete_interface(t_interface* interface);


// LIST 

// Create the list to save interfaces
t_interface_list* create_interface_list();

// Add the interface to the list
void add_interface_to_list(t_interface_list* list, t_interface* interface);

// Find the interface by the name
t_interface* find_interface_by_name(char* name);

// SEND

// Send a confirmation to interface about the connection
int send_confirmation_to_interface(int connection, uint32_t* response);


// -- TO INTERFACE --
// SEND, RECEIVE AND INFORM

// Receive a confirmation about the interface connection
int receive_confirmation_from_kernel(int connection, uint32_t* received);

// Create a package to send the interface information 
t_paquete* create_interface_package(char* name, t_config* config);


// GETTERS

// Get the interface type from the struct t_config
char* type_from_config(t_config* config);


// VALIDATION

// Validate the received instruction from the Kernel 
bool is_valid_instruction(op_code instruction, t_config* config);

//void iterator(char* value);

#endif