#include <stdlib.h>
#include <stdio.h>
#include <utils/client.h>
#include <commons/log.h>
#include <commons/config.h>
#include <utils/server.h>
#include <utils/kernel.h>
#include <pthread.h>
#include <utils/cpu.h>


t_log *logger;
t_config *config;

int correr_servidor(void *arg);

void* conexion_MEM(void *arg);

void clean(t_config *config);

int main(int argc, char *argv[]) {
    /* ---------------- Setup inicial  ---------------- */
    t_config *config;
    logger = log_create("cpu.log", "cpu", true, LOG_LEVEL_INFO);
    if (logger == NULL) {
        return -1;
    }

    config = config_create("cpu.config");
    if (config == NULL) {
        return -1;
    }

    /* ---------------- Hilos ---------------- */

    pthread_t hilo_servidor,hilo_cliente;

    char *puerto = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
    // creo un hilo server de escucha
    if (pthread_create(&hilo_servidor, NULL, correr_servidor, puerto) != 0) {
        log_error(logger, "Error al crear el hilo del servidor");
        return -1;
    }

    // creo un hilo client contra memoria
    if (pthread_create(&hilo_cliente, NULL, conexion_MEM, config) != 0) {
        log_error(logger, "Error al crear el hilo del servidor");
        return -1;
    }

    pthread_join(hilo_servidor, NULL);
    pthread_join(hilo_cliente, NULL);

    clean(config);
    return 0;
}

int correr_servidor(void *arg) {
    char *puerto = (char *) arg;

    int server_fd = iniciar_servidor(puerto);
    log_info(logger, "Servidor listo para recibir al cliente");

    t_list *lista;
    // TODO: While infinito para correr el servidor hasta signal SIGTERM/SIGINT
    //int cliente_fd = esperar_cliente(server_fd);
    // TODO: Posiblemente esto va a ir en un thread separado por cada cliente que se conecte.
    while (1) {
        int cliente_fd = esperar_cliente(server_fd);
        int cod_op = recibir_operacion(cliente_fd);
        switch (cod_op) {
            case DISPATCH:
                lista = recibir_paquete(cliente_fd);
                printf("%d",lista);
                void *pcb_buffer;
                t_pcb *pcb;
                for(int i = 0; i< list_size(lista); i ++){ // DESERIALIZO LA CANTIDAD DE PCB QUE RECIBI DESDE EL KERNEL OSEA LAS QUE TENDRE EN LA LISTA
                    pcb_buffer = list_get(lista, i);
                    pcb = deserializar_pcb(pcb_buffer);
                    log_info(logger, "pid: %d", pcb->pid);
                    log_info(logger, "pc: %d", pcb->pc);               
                    log_info(logger, "quantum: %d", pcb->quantum);
                    log_info(logger, "reg->dato: %d", pcb->reg->dato);
                }
                free(pcb_buffer);
                eliminar_pcb(pcb);
                enviar_respuesta(cliente_fd,OK);

                break;
            case -1:
                log_error(logger, "el cliente se desconecto. Terminando servidor");
                return EXIT_FAILURE;
            default:
                log_warning(logger, "Operacion desconocida. No quieras meter la pata");
                break;
        }
    }
    return EXIT_SUCCESS;
}

void* conexion_MEM (void* arg){

    log_debug(logger, "Conexion a MEM corriendo en hilo separado");
    
    t_config *config = (t_config *) arg;
    int conexion_mem = conexion_by_config(config, "IP_MEMORIA", "PUERTO_MEMORIA");


    //a continuacion genero, serializo y envio el registro T_REG_CPU
    t_reg_cpu* registros = nuevo_reg(12);

    t_buffer *buffer = malloc(sizeof(t_buffer));
    serializar_reg(registros, buffer);

    t_paquete *paquete = crear_paquete(PC);
    agregar_a_paquete(paquete, buffer->stream, buffer->size);

    enviar_paquete(paquete, conexion_mem);
    eliminar_paquete(paquete);
    eliminar_reg(registros);

    recibir_operacion(conexion_mem); //sera MENSAJE DESDE MEMORIA
    recibir_mensaje(conexion_mem);

    liberar_conexion(conexion_mem);
    log_debug(logger, "Conexion con CPU liberada");

    return NULL;
}

void clean(t_config *config) {
    log_destroy(logger);
    config_destroy(config);
}