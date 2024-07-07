
#include "inout.h"
#include <stdio.h>
#include <stdlib.h>
#include "client.h"
#include "server.h"

t_interface_list* interface_list;

// ----- DEFINITIONS -----

// -- INTERFACE --
// CREATE / DELETE INFO

t_info* create_info(char* name, char* type)
{
    t_info* info = malloc(sizeof(t_info));
    info->length_name = strlen(name) + 1;
    info->name = malloc(info->length_name);
    strcpy(info->name, name);
    info->length_type = strlen(type) + 1;
    info->type = malloc(info->length_type);
    strcpy(info->type, type);

    return info;
}

void delete_info(t_info* info) 
{
    free(info->name);
    free(info->type);
    free(info);
}

// CREATE / DELETE RESULT REPORT

t_report* create_report(uint32_t pid, bool result) 
{
    t_report* report = malloc(sizeof(t_report));
    report->pid = pid;
    report->result = result;
    return report;
}

void delete_report(t_report* report) 
{
    free(report);
} 

t_report* list_to_report(t_list* list) 
{
    uint32_t pid = *((uint32_t *) list_get(list, 0));
    bool result = *((bool *) list_get(list, 1));

    return create_report(pid, result);
}

// PACKAGE

t_paquete* info_to_package(t_info* info) 
{
    t_paquete* package = crear_paquete(IO);

    agregar_a_paquete(package, &(info->length_name), sizeof(uint32_t));
    agregar_a_paquete(package, info->name, info->length_name);
    agregar_a_paquete(package, &(info->length_type), sizeof(uint32_t));
    agregar_a_paquete(package, info->type, info->length_type);

    return package;
}

t_paquete* report_to_package(t_report* report) 
{
    t_paquete* package = crear_paquete(REPORT);
    agregar_a_paquete(package, &(report->pid), sizeof(uint32_t));
    agregar_a_paquete(package, &(report->result), sizeof(bool));
    return package;
}

// GETTERS

char* type_from_config(t_config* config) 
{
    return config_get_string_value(config, "TIPO_INTERFAZ");
}

// CONVERTION

char* int_to_string(int value) 
{
    int length = snprintf(NULL, 0, "%d", value);
    char* str = malloc(length + 1);
    snprintf(str, length + 1, "%d", value);
    return str;
}

// VALIDATION

bool is_valid_instruction(op_code code, t_config* config) 
{
    bool is_valid;
    char* type = type_from_config(config);
    switch(code) 
    {
        case IO_GEN_SLEEP:
            is_valid = strcmp(type, "GENERICA") == 0;
            break;
        case IO_STDIN_READ:
            is_valid = strcmp(type, "STDIN") == 0;
            break;
        case IO_STDOUT_WRITE:
            is_valid = strcmp(type, "STDOUT") == 0;
            break;
        case IO_FS_CREATE:
            is_valid = strcmp(type, "DIALFS") == 0;
            break;
        case IO_FS_DELETE:
            is_valid = strcmp(type, "DIALFS") == 0;
            break;
        case IO_FS_READ:
            is_valid = strcmp(type, "DIALFS") == 0;
            break;
        case IO_FS_TRUNCATE:
            is_valid = strcmp(type, "DIALFS") == 0;
            break;
        case IO_FS_WRITE:
            is_valid = strcmp(type, "DIALFS") == 0;
            break;
        default:
            is_valid = false;
    }
    return is_valid;
}

char* type_from_code(op_code instruction_code) 
{
    char* code;
    switch(instruction_code) 
    {
        case IO_GEN_SLEEP:
            code = "IO_GEN_SLEEP";
            break;
        case IO_STDIN_READ:
            code = "IO_STDIN_READ";
            break;
        case IO_STDOUT_WRITE:
            code = "IO_STDOUT_WRITE";
            break;
        case IO_FS_CREATE:
            code = "IO_FS_CREATE";
            break;
        case IO_FS_DELETE:
            code = "IO_FS_DELETE";
            break;
        case IO_FS_READ:
            code = "IO_FS_READ";
            break;
        case IO_FS_TRUNCATE:
            code = "IO_FS_TRUNCATE";
            break;
        case IO_FS_WRITE:
            code = "IO_FS_WRITE";
            break;
        default:
            code = "";
    }
    return code;
}

// COMMUNICATION

void send_confirmation(int connection, uint32_t status) 
{
    send(connection, &(status), sizeof(uint32_t), 0); 
}

void receive_confirmation(int connection, uint32_t status) 
{
    recv(connection, &(status), sizeof(uint32_t), MSG_WAITALL);
}

void send_report(t_instruction* instruction, bool result, int connection) 
{
    t_report* report = create_report(instruction->pid, result);
    t_paquete* package = report_to_package(report);
    enviar_paquete(package, connection);
    eliminar_paquete(package);
    delete_report(report);
    delete_instruction_IO(instruction);
}

// MANAGER INSTRUCTION QUEUE

t_instruction_queue* create_instruction_queue() 
{
    t_instruction_queue* i_queue = malloc(sizeof(t_instruction_queue));
    i_queue->queue = queue_create();
    pthread_mutex_init(&(i_queue->mutex), NULL);

    return i_queue;
}

void add_instruction_to_queue(t_instruction_queue* i_queue, t_instruction* instruction) 
{
    pthread_mutex_lock(&(i_queue->mutex));
    queue_push(i_queue->queue, instruction);
    pthread_mutex_unlock(&(i_queue->mutex));
}

t_instruction* get_next_instruction(t_instruction_queue* i_queue) 
{
    t_instruction* instruction = NULL;
    if(!(queue_is_empty(i_queue->queue))) 
    {
        instruction = queue_pop(i_queue->queue);
    }

    return instruction;
}



// -- KERNEL --
// CREATE / DELETE INTERFACE

t_interface* create_interface(char* name, int conn) 
{
    t_interface* interface = malloc(sizeof(t_interface));
    interface->length_name = strlen(name) + 1;
    interface->name = malloc(interface->length_name);
    strcpy(interface->name, name);
    interface->connection = conn;
    interface->status = false;

    return interface;
}

void delete_interface(t_interface* interface) 
{
    free(interface->name);
    free(interface);
}

t_interface* list_to_interface(t_list* list, int conn) 
{
    char* name = list_get(list, 1);
    int connection = conn;

    return create_interface(name, connection);
}

// GETTERS

char* get_interface_name(t_interface* interface) 
{
    char* name = interface->name;
    return name;
}

char* type_from_list(t_list* list) 
{
    return list_get(list, 3);
}

int get_interface_connection(t_interface* interface) 
{
    return interface->connection;
}

// LIST

t_interface_list* create_interface_list() 
{
    t_interface_list* interface_list = malloc(sizeof(t_interface_list));
    interface_list->list = list_create();
    pthread_mutex_init(&(interface_list->mutex), NULL);

    return interface_list;
}

void add_interface_to_list(t_interface_list* interface_list, t_interface* interface) 
{
    pthread_mutex_lock(&(interface_list->mutex));
    list_add(interface_list->list, interface);
    pthread_mutex_unlock(&(interface_list->mutex));
}

t_interface* find_interface_by_name(char* name) 
{
    bool _is_interface_searched(void *interface) 
    {
        return (strcmp(((t_interface *)interface)->name, name) == 0);
    }

    pthread_mutex_lock(&(interface_list->mutex));
    t_interface* ret_interface = list_find(interface_list->list, (void*) _is_interface_searched);
    pthread_mutex_unlock(&(interface_list->mutex));
    return ret_interface;
}

/*
void iterator(char* value) 
{
    log_info(logger, "%s", value);
}
*/