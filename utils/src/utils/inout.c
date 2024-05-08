
#include "inout.h"
#include <stdio.h>
#include <stdlib.h>
#include "client.h"
#include "server.h"

// ----- Definitions -----

t_interface* create_IO(char* IO_name, char* IO_type) 
{
    t_interface* interface = malloc(sizeof(t_interface));
    interface->IO_name = IO_name;
    interface->IO_type = IO_type;

    return interface;
}

char* get_IO_type(t_config* config) 
{
    return config_get_string_value(config, "TIPO_INTERFAZ");
}

t_paquete* create_IO_package(char* IO_name, t_config* config) 
{
    t_paquete* package = crear_paquete(IO);

    int length_IO_name = strlen(IO_name) + 1;

    char* IO_type = get_IO_type(config);
    int length_IO_type = strlen(IO_type) + 1;

    agregar_a_paquete(package, IO_name, length_IO_name);
    agregar_a_paquete(package, IO_type, length_IO_type);

    return package;

}

t_interface* list_to_IO(t_list* list) 
{
    char* name = list_get(list, 0);
    char* type = list_get(list, 1);

    return create_IO(name, type);
}

char* get_IO_info(t_interface* interface) 
{
    char* name = interface->IO_name;
    char* type = interface->IO_type;

    return strcat(strcat(name, " - "), type);
}

void delete_IO(t_interface* interface) 
{
    free(interface);
}

bool is_valid_instruction(op_code instruction, t_config* config) 
{
    bool is_valid;
    char* IO_type = get_IO_type(config);
    switch(instruction) 
    {
        case IO_GEN_SLEEP:
            is_valid = strcmp(IO_type, "GENERICA") == 0;
            break;
        case IO_STDIN_READ:
            is_valid = strcmp(IO_type, "STDIN") == 0;
            break;
        case IO_STDOUT_WRITE:
            is_valid = strcmp(IO_type, "STDOUT") == 0;
            break;
        case IO_FS_CREATE:
            is_valid = strcmp(IO_type, "DIALFS") == 0;
            break;
        case IO_FS_DELETE:
            is_valid = strcmp(IO_type, "DIALFS") == 0;
            break;
        case IO_FS_READ:
            is_valid = strcmp(IO_type, "DIALFS") == 0;
            break;
        case IO_FS_TRUNCATE:
            is_valid = strcmp(IO_type, "DIALFS") == 0;
            break;
        case IO_FS_WRITE:
            is_valid = strcmp(IO_type, "DIALFS") == 0;
            break;
        default:
            is_valid = false;
    }
    return is_valid;
}

int generic_IO_wait(int time, t_config* config) 
{
    int work_unit_time = config_get_int_value(config, "TIEMPO_UNIDAD_TRABAJO");
    int time_to_wait = work_unit_time * time;

    return usleep(time_to_wait);
}

/*
void iterator(char* value) 
{
    log_info(logger, "%s", value);
}
*/