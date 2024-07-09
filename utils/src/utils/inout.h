

#ifndef UTILS_INOUT_H_
#define UTILS_INOUT_H_

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>
#include <commons/log.h>
#include <commons/txt.h>
#include <commons/collections/queue.h>
#include <utils/cpu.h>
#include "client.h"


// ----- STRUCTURES -----

typedef struct 
{
    uint32_t length_name;
    char* name;
    uint32_t length_type;
    char* type;
} t_info;

typedef struct 
{
    uint32_t length_name;
    char* name;
    int connection;
    bool status;
} t_interface;

typedef struct 
{
    uint32_t pid;
    bool result;
} t_report;

typedef struct
{
    t_list* list;
    pthread_mutex_t mutex;
} t_interface_list;

typedef struct 
{
    t_queue* queue;
    pthread_mutex_t mutex;
}t_instruction_queue;

// ----- FUNCTIONS -----

// -- INTERFACE --
// CREATE / DELETE INFO

// Create a pointer to a struct t_info
t_info* create_info(char* name, char* type);

// Free the space of the struct t_info
void delete_info(t_info* info);


// CREATE / DELETE RESULT REPORT

// Create a report
t_report* create_report(uint32_t pid, bool result);

// Delete a report
void delete_report(t_report* report);

// Create a report from a list
t_report* list_to_report(t_list* list);


// PACKAGE

// Create a package to send the interface information 
t_paquete* info_to_package(t_info* info);

// Create a package to send a report
t_paquete* report_to_package(t_report* report);


// COMMUNICATION

// Send a confirmation to the connection
void send_confirmation(int connection, uint32_t status);

// Receive a confirmation from the connection
void receive_confirmation(int connection, uint32_t status);

// Send a report to the connection
void send_report(t_instruction* instruction, bool result, int connection);


// GETTERS

// Get the interface type from the struct t_config
char* type_from_config(t_config* config);

// Get type from operation code
char* type_from_code(op_code instruction_code);


// CONVERTION

char* int_to_string(int value);


// VALIDATION

// Validate the received instruction from the Kernel 
bool is_valid_instruction(op_code instruction, t_config* config);


// MANAGER INSTRUCTION QUEUE

// Create a Instruction queue
t_instruction_queue* create_instruction_queue();

// Add instruction to the queue
void add_instruction_to_queue(t_instruction_queue* queue, t_instruction* instruction);

//  Obtain the next instruction from the queue
t_instruction* get_next_instruction(t_instruction_queue* queue);




// -- KERNEL --
// CREATE / DELETE INTERFACE

// Create a pointer to a struct t_interface
t_interface* create_interface(char* name, int connection);

// Free the space of the struct t_interface
void delete_interface(t_interface* interface);

// Extract the information from a list and create a pointer to a struct t_interface 
t_interface* list_to_interface(t_list* list, int connection);

void delete_interface_from_list(t_interface_list* interface_list, char* name);

// GETTERS

// Get the interface name from the struct t_interface 
char* get_interface_name(t_interface* interface);

// Get the interface type from list
char* type_from_list(t_list* list);

// Get the interface connection number to send instructions
int get_interface_connection(t_interface* interface);


// LIST 

// Create the list to save interfaces
t_interface_list* create_interface_list();

// Add the interface to the list
void add_interface_to_list(t_interface_list* list, t_interface* interface);

// Find the interface by the name
t_interface* find_interface_by_name(char* name);

//void iterator(char* value);

#endif