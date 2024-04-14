#include <stdlib.h>
#include <stdio.h>
#include <utils/client.h>
#include <commons/log.h>
#include <commons/config.h>
#include <utils/server.h>
#include <utils/kernel.h>

t_log *logger;
t_config *config;

void clean(t_config *config);

int correr_servidor();

void iterator(char *value);

void *consola_interactiva(void *arg);

int main(int argc, char *argv[]) {
    /* ---------------- Setup inicial  ---------------- */
    t_config *config;
    logger = log_create("kernel.log", "kernel", true, LOG_LEVEL_INFO);
    if (logger == NULL) {
        return -1;
    }

    config = config_create("kernel.config");
    if (config == NULL) {
        return -1;
    }

    pthread_t hilo_servidor, hilo_consola;

    char *puerto = config_get_string_value(config, "PUERTO_ESCUCHA");
    // TODO: Podemos usar un nuevo log y otro name para loggear en el server
    if (pthread_create(&hilo_servidor, NULL, correr_servidor, puerto) != 0) {
        log_error(logger, "Error al crear el hilo del servidor");
        return -1;
    }

    // Creación y ejecución del hilo de la consola
    if (pthread_create(&hilo_consola, NULL, consola_interactiva, config) != 0) {
        log_error(logger, "Error al crear el hilo de la consola");
        return -1;
    }

    // Esperar a que los hilos terminen
    pthread_join(hilo_servidor, NULL);
    pthread_join(hilo_consola, NULL);

    // TODO: Esperar a que los hilos den señal de terminado para limpiar la config.
    clean(config);
    return 0;
}

void clean(t_config *config) {
    log_destroy(logger);
    config_destroy(config);
}

void iterator(char *value) {
    log_info(logger, "%s", value);
}

int correr_servidor(void *arg) {
    char *puerto = (char *) arg; // Castear el argumento de vuelta a t_config

    int server_fd = iniciar_servidor(puerto);
    log_info(logger, "Servidor listo para recibir al cliente");

    t_list *lista;
    int cliente_fd = esperar_cliente(server_fd);
    while (1) {
        int cod_op = recibir_operacion(cliente_fd);
        switch (cod_op) {
            case PAQUETE:
                lista = recibir_paquete(cliente_fd);
                log_info(logger, "Me llegaron los siguientes valores:\n");
                list_iterate(lista, (void *) iterator);
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

void *consola_interactiva(void *arg) {
    log_debug(logger, "Consola corriendo en hilo separado");
    t_config *config = (t_config *) arg;
    int conexion_memoria = conexion_by_config(config, "IP_MEMORIA", "PUERTO_MEMORIA");

    t_pcb *pcb = nuevo_pcb(15);

    t_buffer *buffer = malloc(sizeof(t_buffer));
    serializar_pcb(pcb, buffer);

    t_paquete *paquete = crear_paquete(PCB);
    agregar_a_paquete(paquete, buffer, buffer->offset);

    enviar_paquete(paquete, conexion_memoria);
    eliminar_paquete(paquete);
    eliminar_pcb(pcb);

    liberar_conexion(conexion_memoria);
    log_debug(logger, "Conexion liberada");
    return NULL;
}