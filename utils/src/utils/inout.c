
#include "inout.h"
#include <stdio.h>
#include <stdlib.h>
#include "client.h"
#include "server.h"

t_interface_list* interface_list;

// ----- DEFINITIONS -----

// -- GENERAL --
// CREATE / DELETE INTERFACES

t_interface* create_interface(char* name, char* type, int conn) 
{
    t_interface* interface = malloc(sizeof(t_interface));
    interface->length_name = strlen(name) + 1;
    interface->name = malloc(interface->length_name);
    strcpy(interface->name, name);
    interface->length_type = strlen(type) + 1;
    interface->type = malloc(interface->length_type);
    strcpy(interface->type, type);
    interface->connection = conn;

    return interface;
}

void delete_interface(t_interface* interface) 
{
    free(interface->name);
    free(interface->type);
    free(interface);
}

// INSTRUCTIONS

t_instruction* create_instruction(uint32_t pid, char* name, uint32_t job_unit, char* path) 
{
    t_instruction* new_instruction = malloc(sizeof(t_instruction));
    new_instruction->pid = pid;
    new_instruction->length_name = strlen(name) + 1;
    new_instruction->name = malloc(new_instruction->length_name);
    strcpy(new_instruction->name, name);
    new_instruction->job_unit = job_unit;
    new_instruction->length_path = strlen(path) + 1;
    new_instruction->path = malloc(new_instruction->length_path);
    strcpy(new_instruction->path, path);
    
    return new_instruction;
}

t_paquete* instruction_to_package(op_code code, t_instruction* instruction) 
{
    t_paquete* package = crear_paquete(code);

    agregar_a_paquete(package, &(instruction->pid), sizeof(uint32_t));
    agregar_a_paquete(package, &(instruction->length_name), sizeof(uint32_t));
    agregar_a_paquete(package, instruction->name, instruction->length_name);
    agregar_a_paquete(package, &(instruction->job_unit), sizeof(uint32_t));
    agregar_a_paquete(package, &(instruction->length_path), sizeof(uint32_t));
    agregar_a_paquete(package, instruction->path, instruction->length_path);

    return package;
}

void send_instruction(op_code code, t_instruction* instruction, int connection) 
{
    t_paquete* package = instruction_to_package(code, instruction);
    enviar_paquete(package, connection);
}

t_instruction* list_to_instruction(t_list* list) 
{
    uint32_t* pid = list_get(list, 0);
    char* name = list_get(list, 2);
    uint32_t* job_unit = list_get(list, 3);
    char* path = list_get(list, 5);

    return create_instruction(*pid, name, *job_unit, path);
}

t_instruction* receive_instruction(int connection) 
{
    t_list* list = recibir_paquete(connection);
    return list_to_instruction(list);
}

void delete_instruction(t_instruction* instruction)
{
    free(instruction->name);
    free(instruction->path);
    free(instruction);
}

// SEND

int send_confirmation(int connection, uint32_t* response) 
{
    return send(connection, response, sizeof(uint32_t), 0); 
}

int receive_confirmation(int connection, uint32_t* received) 
{
    return recv(connection, received, sizeof(uint32_t), MSG_WAITALL);
}

void result_of_operation(uint32_t result) 
{
    char* result_of_operation;
    if(result == 0) 
    {
        result_of_operation = "ERROR";
    } else {
        result_of_operation = "OK";
    }
    txt_write_in_stdout(result_of_operation);
}

// -- TO KERNEL --

t_interface* list_to_interface(t_list* list, int conn) 
{
    char* name = list_get(list, 1);
    char* type = list_get(list, 3);
    int connection = conn;

    return create_interface(name, type, connection);
}

// GETTERS

char* get_interface_name(t_interface* interface) 
{
    char* name = interface->name;
    return name;
}

char* get_interface_type(t_interface* interface) 
{
    char* type = interface->type;
    return type;
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

// -- TO INTERFACE --
// PACKAGE

t_paquete* interface_to_package(t_interface* interface) 
{
    t_paquete* package = crear_paquete(IO);

    agregar_a_paquete(package, &(interface->length_name), sizeof(uint32_t));
    agregar_a_paquete(package, interface->name, interface->length_name);
    agregar_a_paquete(package, &(interface->length_type), sizeof(uint32_t));
    agregar_a_paquete(package, interface->type, interface->length_type);
    agregar_a_paquete(package, &(interface->connection), sizeof(int));

    return package;
}

// GETTERS

char* type_from_config(t_config* config) 
{
    return config_get_string_value(config, "TIPO_INTERFAZ");
}

// VALIDATION

bool is_valid_instruction(op_code instruction, char* code, t_config* config) 
{
    bool is_valid;
    char* type = type_from_config(config);
    switch(instruction) 
    {
        case IO_GEN_SLEEP:
            is_valid = strcmp(type, "GENERICA") == 0;
            code = "IO_GEN_SLEEP";
            break;
        case IO_STDIN_READ:
            is_valid = strcmp(type, "STDIN") == 0;
            code = "IO_STDIN_READ";
            break;
        case IO_STDOUT_WRITE:
            is_valid = strcmp(type, "STDOUT") == 0;
            code = "IO_STDOUT_WRITE";
            break;
        case IO_FS_CREATE:
            is_valid = strcmp(type, "DIALFS") == 0;
            code = "IO_FS_CREATE";
            break;
        case IO_FS_DELETE:
            is_valid = strcmp(type, "DIALFS") == 0;
            code = "IO_FS_DELETE";
            break;
        case IO_FS_READ:
            is_valid = strcmp(type, "DIALFS") == 0;
            code = "IO_FS_READ";
            break;
        case IO_FS_TRUNCATE:
            is_valid = strcmp(type, "DIALFS") == 0;
            code = "IO_FS_TRUNCATE";
            break;
        case IO_FS_WRITE:
            is_valid = strcmp(type, "DIALFS") == 0;
            code = "IO_FS_WRITE";
            break;
        default:
            is_valid = false;
    }
    return is_valid;
}

/*
void iterator(char* value) 
{
    log_info(logger, "%s", value);
}
*/