

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

// ---------- STRUCTURES ----------

typedef struct 
{
    uint32_t length_name;
    char* name;
    uint32_t length_type;
    char* type;
} t_info; // Informara a Kernel la conexion de una nueva interfaz

typedef struct 
{
    uint32_t length_name;
    char* name;
    int connection;
    bool status;
} t_interface; // La interfaz como tal, con la conexion y un byte de estado

typedef struct 
{
    uint32_t pid;
    bool result;
} t_report; // Informara el logro / error de las instrucciones

typedef struct 
{
    uint32_t pid;
    uint32_t text_size;
    char* text;
    uint32_t physical_address;
} t_req_to_w; // Peticion de escritura a memoria

typedef struct 
{
    uint32_t pid;
    uint32_t text_size;
    uint32_t physical_address;
} t_req_to_r; // Peticion de lectura a memoria

typedef struct
{
    t_list* list;
    pthread_mutex_t mutex;
} t_interface_list; // Guardara las interfaces que se vayan sumando al kernel

typedef struct 
{
    t_queue* queue;
    pthread_mutex_t mutex;
}t_instruction_queue; // Cola de instrucciones para la interfaz

// ---------- FUNCTIONS ----------

// ----- INTERFACE -----
// Create / Delete info

t_info* create_info(char* name, char* type);
void delete_info(t_info* info);

// Create / Delete report

t_report* create_report(uint32_t pid, bool result);
void delete_report(t_report* report);
t_report* list_to_report(t_list* list);

// Create / Delete memory requeriment

t_req_to_w* req_to_write(uint32_t pid, char* text, uint32_t physical_address);
t_req_to_r* req_to_read(uint32_t pid, uint32_t size, uint32_t physical_address);
void delete_req_to_w(t_req_to_w* mem_req);
void delete_req_to_r(t_req_to_r* mem_req);
t_req_to_w* list_to_req_to_w(t_list* list);
t_req_to_r* list_to_req_to_r(t_list* list);

// Paquete

t_paquete* info_to_package(t_info* info);
t_paquete* report_to_package(t_report* report);
t_paquete* req_to_w_package(t_req_to_w* mem_req);
t_paquete* req_to_r_package(t_req_to_r* mem_req);

// Communication

void send_confirmation(int connection, uint32_t status);
void receive_confirmation(int connection, uint32_t status);
void send_info(t_info* info, int connection);
void send_report(t_instruction* instruction, bool result, int connection);
void send_req_to_w(t_req_to_w* mem_req, int connection);
void send_req_to_r(t_req_to_r* mem_req, int connection);
t_req_to_w* receive_req_to_w(int connection);
t_req_to_r* receive_req_to_r(int connection);

// Getters

char* type_from_config(t_config* config);
char* type_from_code(op_code instruction_code);
char* path_from_config(t_config* config);

// Convertion

char* int_to_string(int value);

// Validation

bool is_valid_instruction(op_code instruction, t_config* config);

// Instruction Queue

t_instruction_queue* create_instruction_queue();
void add_instruction_to_queue(t_instruction_queue* queue, t_instruction* instruction);
t_instruction* get_next_instruction(t_instruction_queue* queue);

// Log

void generate_log_from_instruction(t_instruction* instruction);

// ----- KERNEL -----
// Create / Delete interface

t_interface* create_interface(char* name, int connection);
void delete_interface(t_interface* interface);
t_interface* list_to_interface(t_list* list, int connection);

// Getters

char* get_interface_name(t_interface* interface);
char* type_from_list(t_list* list);
int get_interface_connection(t_interface* interface);

// List

t_interface_list* create_interface_list();
void add_interface_to_list(t_interface_list* list, t_interface* interface);
t_interface* delete_interface_from_list(t_interface_list* list, char* name);
t_interface* find_interface_by_name(t_interface_list* interface_list, char* name);

#endif