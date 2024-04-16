#include <stdlib.h>
#include <stdio.h>
#include <utils/client.h>
#include <commons/log.h>
#include <commons/config.h>
#include <utils/server.h>
#include <utils/kernel.h>

t_log *logger;
t_config *config;

int correr_servidor(void *arg);

void clean(t_config *config);

int main(int argc, char *argv[]) {
    /* ---------------- Setup inicial  ---------------- */
    t_config *config;
    logger = log_create("memoria.log", "memoria", true, LOG_LEVEL_INFO);
    if (logger == NULL) {
        return -1;
    }

    config = config_create("memoria.config");
    if (config == NULL) {
        return -1;
    }

    /* ---------------- Hilos ---------------- */

    pthread_t hilo_servidor;

    char *puerto = config_get_string_value(config, "PUERTO_ESCUCHA");
    // TODO: Podemos usar un nuevo log y otro name para loggear en el server
    if (pthread_create(&hilo_servidor, NULL, correr_servidor, puerto) != 0) {
        log_error(logger, "Error al crear el hilo del servidor");
        return -1;
    }

    pthread_join(hilo_servidor, NULL);

    clean(config);
    return 0;
}

int correr_servidor(void *arg) {
    char *puerto = (char *) arg;

    int server_fd = iniciar_servidor(puerto);
    log_info(logger, "Servidor listo para recibir al cliente");

    t_list *lista;
    // TODO: While infinito para correr el servidor hasta signal SIGTERM/SIGINT
    int cliente_fd = esperar_cliente(server_fd);
    // TODO: Posiblemente esto va a ir en un thread separado por cada cliente que se conecte.
    while (1) {
        int cod_op = recibir_operacion(cliente_fd);
        switch (cod_op) {
            case PCB:
                lista = recibir_paquete(cliente_fd);
                void *pcb_buffer;
                t_pcb *pcb;
                for(int i = 0; i< list_size(lista); i ++){
                    pcb_buffer = list_get(lista, i);
                    pcb = deserializar_pcb(pcb_buffer);
                    log_info(logger, "pid: %d", pcb->pid);
                    log_info(logger, "pc: %d", pcb->pc);               
                    log_info(logger, "quantum: %d", pcb->quantum);
                    log_info(logger, "reg->dato: %d", pcb->reg->dato);
                }
                free(pcb_buffer);
                eliminar_pcb(pcb);
                enviar_mensaje("ok", cliente_fd);
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

void
clean(t_config *config) {
    log_destroy(logger);
    config_destroy(config);
}