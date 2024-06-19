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

    // Connect to the Kernel

    char* ip = config_get_string_value(config, "IP_KERNEL");
    char* port = config_get_string_value(config, "PUERTO_KERNEL");
    
    // I create the client connection
    
    int connection = crear_conexion(ip, port);
    log_info(logger, "La conexion es: %d", connection);

    // I send a package to Kernel to inform the interface created

    t_paquete* package = create_interface_package(name, config);
    enviar_paquete(package, connection);
    eliminar_paquete(package);

    uint32_t response;
    receive_confirmation_from_kernel(connection, &(response));

    if (response == 0) 
    {
        return EXIT_FAILURE;
    } else {
        txt_write_in_stdout("CONNECTION SUCESSFULL\n");
        log_info(logger, "CONNECTION SUCESSFULL");
    }

    /*
    while(1) 
    {
        // I hope I get an instruction from the Kernel
        //bytes = recv(connection, &result, sizeof(int32_t), MSG_WAITALL);
        
        //int client_fd = esperar_cliente(connection);
        log_debug(logger, "PASO 1: Cliente conectado");

        int cod_op = recibir_operacion(client_fd);
        log_debug(logger, "PASO 2: Operación recibida");

        // I manage the operation received
        if (is_valid_instruction(cod_op, config))
        {
            // I log the PID and the instruction
            log_debug(logger, "PASO 3: Instruccion validada");
            log_info(logger, "PID: %s - Operation: %d", "", cod_op);
            
            // Once the instruction has been validated and logged in, we execute it

            switch(cod_op) 
            {
                // Generic interface
                case IO_GEN_SLEEP:
                    int ret = generic_interface_wait(10, config);
                    inform_kernel(client_fd, ret);
                    break;
                // STDIN interface
                case IO_STDIN_READ:
                    break;
                // STDOUT interface
                case IO_STDOUT_WRITE:
                    break;
                default:
                    log_error(logger, "Invalid instruction");
                    break;
            }
        } else {
            log_error(logger, "Invalid instruction");
            enviar_mensaje("Invalid instruction", client_fd);
            break;
        }

        log_debug(logger, "PASO 4: Llego al fin. Vuelve al PASO 1");

    }
    */

    liberar_conexion(connection);

    return 0;
}