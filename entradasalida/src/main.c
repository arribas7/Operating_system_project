#include <stdlib.h>
#include <stdio.h>
#include <commons/txt.h>
#include <commons/log.h>
#include <commons/config.h>
#include <utils/client.h>
#include <utils/server.h>
#include <utils/inout.h>
#include "generic.h"
#include "stdin_stdout.h"

// Global variables

t_list* process_list;
pthread_mutex_t mutex_process_list;
t_log* logger;
t_config* config;
char* name;

int main(int argc, char* argv[]) {
    
    // Create log and config for the initial connection

    logger = log_create("in_out.log", "IN_OUT", true, LOG_LEVEL_INFO);
    
    // In each instance of the module I can change the path of the configuration file

    config = config_create(argv[1]);
    char* name = argv[2];
    char* type = type_from_config(config);
    t_interface* interface = create_interface(name, type, 0);

    // Connect to the Kernel

    char* ip = config_get_string_value(config, "IP_KERNEL");
    char* port = config_get_string_value(config, "PUERTO_KERNEL");
    
    // I create the client connection
    
    int connection = crear_conexion(ip, port);
    log_info(logger, "La conexion es: %d", connection);

    // I send a package to Kernel to inform the interface created

    t_paquete* package = interface_to_package(interface);
    enviar_paquete(package, connection);
    eliminar_paquete(package);
    delete_interface(interface);

    uint32_t response;
    receive_confirmation(connection, &(response));

    if (response == 0) 
    {
        return EXIT_FAILURE;
    } else {
        log_info(logger, "CONNECTION SUCESSFULL");
    }
    
    while(1) 
    {
        int cod_op; char* code;
        cod_op = recibir_operacion(connection);

        if(is_valid_instruction(cod_op, code, config)) 
        {
            t_instruction* instruction = receive_instruction(connection);

            switch(cod_op) 
            {
                case IO_GEN_SLEEP:
                    log_info(logger, "PID: %i - Operación a realizar: %s", instruction->pid, code);
                    int result = generic_interface_wait(instruction->job_unit, config);
                    response = (result == 0) ? 1 : 0;
                    log_info(logger, "Operation finished");
                    send_confirmation(connection, &(response));
                    break;
                case IO_STDIN_READ:
                    break;
                case IO_STDOUT_WRITE:
                    break;
                default:
                    log_error(logger, "Invalid instruction");
                    break;
            }
        } else {
            log_error(logger, "Invalid instruction");
            enviar_mensaje("Invalid instruction", connection);
            break;
        }
    }

    liberar_conexion(connection);

    return 0;
}