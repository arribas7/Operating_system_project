#include <stdlib.h>
#include <stdio.h>
#include <commons/log.h>
#include <commons/config.h>
#include <utils/client.h>
#include <utils/server.h>
#include <utils/entradasalida.h>

t_log* logger;
t_config* config;

void conexion_cliente(char* ip, char* puerto);
int levantar_servidor(char* puerto);

int main(int argc, char* argv[]) {
    
    // ----- Creo el log y el config -----
    logger = log_create("in-out.log", "IN-OUT", true, LOG_LEVEL_INFO);
    config = config_create("entradasalida.config");

    /*    
    Las conexiones se haran a los modulos:
        - Kernel (Servidor: recibo instrucciones)
        - Memoria (Cliente: leo direcciones | Servidor: envio informacion)
    */

   // ----- Conexiones cliente -----

   // -- Kernel --

   char* ip_k = config_get_string_value(config, "IP_KERNEL");
   char* puerto_k = config_get_string_value(config, "PUERTO_KERNEL");
   //conexion_cliente(ip_k, puerto_k);

   // -- Memoria --

   //char* ip_m = config_get_string_value(config, "IP_MEMORIA");
   //char* puerto_m = config_get_string_value(config, "PUERTO_MEMORIA");
   //conexion_cliente(ip_m, puerto_m);   

    // ----- Fin conexiones cliente -----


    // ----- Levantar servidor -----

    levantar_servidor(puerto_k);

    return 0;
}

// ----- Funciones de conexion - servidor -----

void conexion_cliente(char* ip, char* puerto) 
{
    // -- Creo la conexion con el Kernel --
    int conexion = crear_conexion(ip, puerto);

    // -- Preparo un paquete --
    t_paquete* paquete = crear_paquete(PAQUETE);
    char* mssg = strdup("Me voy al Kernel");
    int tamanio = strlen(mssg) + 1;
    agregar_a_paquete(paquete, mssg, tamanio);

    // -- Envio paquete y elimino lo creado
    enviar_paquete(paquete, conexion);

    free(mssg);
    eliminar_paquete(paquete);
    close(conexion);
};

int levantar_servidor(char* puerto) 
{
    // Creo el socket servidor y espero al cliente
    int socket_servidor = iniciar_servidor(puerto);
    log_info(logger, "Servidor listo para recibir al cliente");

    int socket_cliente = esperar_cliente(socket_servidor);
    
    t_list* list;
    while(1) 
    {
        // Espero a recibir el cod_op y ver que debo hacer
        /*
            En esta parte se recibiran las instrucciones del Kernel con los codigos de instruccion
            Los mismos estan definidos en entradasalida.h
            NECESARIO: ver como los enviara el Kernel
        */
        int cod_op = recibir_operacion(socket_cliente);
        switch(cod_op) 
        {
            case MENSAJE:
                recibir_mensaje(socket_cliente);
                break;
            case PAQUETE:
                list = recibir_paquete(socket_cliente);
                log_info(logger, "Me llego lo siguiente: \n");
                list_iterate(list, (void*) iterator);
                break;
            case -1:
                log_error(logger, "El cliente se desconecto. Terminando servidor");
                return EXIT_FAILURE;
            default:
                log_warning(logger, "Operacion desconocida");
                break;
        }

        return EXIT_SUCCESS;
    };
}