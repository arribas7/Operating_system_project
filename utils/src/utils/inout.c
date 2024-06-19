
#include "inout.h"
#include <stdio.h>
#include <stdlib.h>
#include "client.h"
#include "server.h"

t_interface_list* interface_list;

// ----- DEFINITIONS -----

// -- TO KERNEL --
// CREATE INTERFACES

t_interface* create_interface(char* name, char* type, int conn) 
{
    t_interface* interface = malloc(sizeof(t_interface));
    interface->name = name;
    interface->type = type;
    interface->connection = conn;

    return interface;
}

t_interface* list_to_interface(t_list* list, int conn) 
{
    char* name = list_get(list, 0);
    char* type = list_get(list, 1);
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


// DELETE

void delete_interface(t_interface* interface) 
{
    free(interface);
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


// SEND

int send_confirmation_to_interface(int connection, uint32_t* response) 
{
    return send(connection, response, sizeof(uint32_t), 0);
}


// -- TO INTERFACE --
// SEND, RECEIVE AND INFORM

t_paquete* create_interface_package(char* name, t_config* config) 
{
    t_paquete* package = crear_paquete(IO);
    int length_name = strlen(name) + 1;

    char* type = type_from_config(config);
    int length_type = strlen(type) + 1;

    agregar_a_paquete(package, name, length_name);
    agregar_a_paquete(package, type, length_type);

    // No le paso la conexion porque este dato recien lo tengo en el Kernel,
    // cuando armo la estructura lo sumo.

    return package;

}

int receive_confirmation_from_kernel(int connection, uint32_t* received) 
{
    return recv(connection, received, sizeof(uint32_t), MSG_WAITALL);
}


// GETTERS

char* type_from_config(t_config* config) 
{
    return config_get_string_value(config, "TIPO_INTERFAZ");
}


// VALIDATION

bool is_valid_instruction(op_code instruction, t_config* config) 
{
    bool is_valid;
    char* type = type_from_config(config);
    switch(instruction) 
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



/*
void iterator(char* value) 
{
    log_info(logger, "%s", value);
}
*/