#include <stdlib.h>
#include <stdio.h>
#include <commons/log.h>
#include <commons/config.h>
#include <utils/client.h>
#include <utils/server.h>
#include <utils/inout.h>

// -- Global variables

t_log* logger;
t_config* config;
char* IO_name;

int main(int argc, char* argv[]) {
    
    // -- Create log and config for the initial connection --

    logger = log_create("in_out.log", "IN_OUT", true, LOG_LEVEL_INFO);
    
    // -- In each instance of the module I can change the path of the configuration file --

    config = config_create(argv[1]);
    char* IO_name = argv[2];

    // -- I create the interface --

    /* 
    
    -- Procedure --
    
    // -- General (all interfaces)
    1 - I connect to the Kernel
    2 - I hope you send me a message (operation to be carried out)
    3 - I attend to the message (I perform the operation)
    4 - I reply that I'm done
    5 - I go back to step 2
    
    // -- Generic Interface
    1 - I wait the amount of time defined in the configuration file

    */

    // ----- Client Connection -----

    // -- Connect to the Kernel --

    char* ip = config_get_string_value(config, "IP_KERNEL");
    char* port = config_get_string_value(config, "PUERTO_KERNEL");
    
    // -- I create the client connection --
    
    int connection = crear_conexion(ip, port);

    // -- I send a package to Kernel to inform the IO created --

    t_paquete* package = create_IO_package(IO_name, config);
    enviar_paquete(package, connection);
    eliminar_paquete(package);

    /*

    while(1) 
    {
        // ----- De acuerdo a la interfaz hago x cosa
    }

    */

    liberar_conexion(connection);

    return 0;
}