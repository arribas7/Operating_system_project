#include "client.h"

#include <stdlib.h>
#include <stdio.h>s
#include <utils/client.h>
#include <commons/log.h>
#include <commons/config.h>
#include <utils/server.h>

t_log* logger; 
t_config* config;

int init(t_config *config);
void clean(t_config *config);
int correr_servidor();
void iterator(char* value);
void *correr_consola(void *arg);

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

    // TODO: Podemos usar un nuevo log y otro name para loggear en el server
    if (pthread_create(&hilo_servidor, NULL, correr_servidor, NULL) != 0) {
        log_error(logger, "Error al crear el hilo del servidor");
        return -1;
    }

    // Creación y ejecución del hilo de la consola
    if (pthread_create(&hilo_consola, NULL, correr_consola, config) != 0) {
        log_error(logger, "Error al crear el hilo de la consola");
        return -1;
    }

    // Esperar a que los hilos terminen
    pthread_join(hilo_servidor, NULL);
    pthread_join(hilo_consola, NULL);

    clean(config);
    return 0;
}

int
init(t_config *config) {
    config = config_create("kernel.config");
    if (config == NULL) {
        return -1;
    }

    return 0;
}

void
clean(t_config *config) {
    /* Y por ultimo, hay que liberar lo que utilizamos (conexion, log y config)
      con las funciones de las commons y del TP mencionadas en el enunciado */
    log_destroy(logger);
    config_destroy(config);
}

void iterator(char* value) {
    log_info(logger,"%s", value);
}

int correr_servidor() {
    int server_fd = iniciar_servidor();
    log_info(logger, "Servidor listo para recibir al cliente");

    int cliente_fd = esperar_cliente(server_fd);

    recibir_operacion(cliente_fd); // int opcode

    t_list* lista = recibir_paquete(cliente_fd);
    log_info(logger, "Me llegaron los siguientes valores:\n");
    list_iterate(lista, (void*) iterator);

    return EXIT_SUCCESS;
}

void *correr_consola(void *arg) {
    // Código de la consola
    log_info(logger, "Consola corriendo en hilo separado");
    t_config *config = (t_config *) arg; // Castear el argumento de vuelta a t_config
    int conexion_memoria = conexion_by_config(config, "IP_MEMORIA", "PUERTO_MEMORIA");
    liberar_conexion(conexion_memoria);
    // int conexion_cpu_dispatch = conexion_by_config(config, "IP_CPU_DISPATCH", "PUERTO_CPU_DISPATCH");
    // int conexion_cpu_interrupt = conexion_by_config(config, "IP_CPU_INTERRUPT", "PUERTO_CPU_INTERRUPT");

    log_info(logger, "Conexion liberada");
    return NULL;
}