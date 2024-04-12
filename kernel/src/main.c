#include <stdlib.h>
#include <stdio.h>
#include <../utils/client.h>
#include <commons/log.h>
#include <commons/config.h>
#include <../utils/server.h>
#include "client.h"

int
init(t_log *logger, t_config *config) {
    logger = log_create("kernel.log", "kernel", true, LOG_LEVEL_INFO);
    if (logger == NULL) {
        return -1;
    }

    config = config_create("kernel.config");
    if (config == NULL) {
        return -1;
    }

    return 0;
}

void
clean(t_log *logger, t_config *config) {
    /* Y por ultimo, hay que liberar lo que utilizamos (conexion, log y config)
      con las funciones de las commons y del TP mencionadas en el enunciado */
    log_destroy(logger);
    config_destroy(config);
}

void iterator(char* value) {
    log_info(logger,"%s", value);
}

int correr_servidor(t_log *logger) {
    int server_fd = iniciar_servidor();
    log_info(logger, "Servidor listo para recibir al cliente");

    int cliente_fd = esperar_cliente(server_fd);

    recibir_operacion(cliente_fd); // int opcode

    t_list* lista = recibir_paquete(cliente_fd);
    log_info(logger, "Me llegaron los siguientes valores:\n");
    list_iterate(lista, (void*) iterator);

    return EXIT_SUCCESS;
}


int main(int argc, char *argv[]) {
    /* ---------------- Setup inicial  ---------------- */
    t_log *logger;
    t_config *config;

    int err = init(logger, config);
    if (err != 0) {
        return err;
    }

    // TODO: Podemos usar un nuevo log y otro name para loggear
    correr_servidor(logger);
    // correr servidor en thread
    // correr consola en thread

    int conexion_memoria = conexion_by_config(config, "IP_MEMORIA", "PUERTO_MEMORIA");
    liberar_conexion(conexion_memoria);
    // int conexion_cpu_dispatch = conexion_by_config(config, "IP_CPU_DISPATCH", "PUERTO_CPU_DISPATCH");
    // int conexion_cpu_interrupt = conexion_by_config(config, "IP_CPU_INTERRUPT", "PUERTO_CPU_INTERRUPT");



    clean(logger, config);
}







