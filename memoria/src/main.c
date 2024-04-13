#include <stdlib.h>
#include <stdio.h>
#include <utils/client.h>
#include <commons/log.h>
#include <commons/config.h>
#include <utils/server.h>
#include <utils/commons.h>

t_log* logger; 
t_config* config;

int correr_servidor(void *arg);
void iterator(char* value);
void clean(t_config* config);

int main(int argc, char* argv[]) {
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
    
    char* puerto = config_get_string_value(config, "PUERTO_ESCUCHA");
    // TODO: Podemos usar un nuevo log y otro name para loggear en el server
    if (pthread_create(&hilo_servidor, NULL, correr_servidor, puerto) != 0) {
        log_error(logger, "Error al crear el hilo del servidor");
        return -1;
    }

    // Esperar a que los hilos terminen
    pthread_join(hilo_servidor, NULL);

    // TODO: Esperar a que los hilos den señal de terminado para limpiar la config.
    clean(config);
    return 0;
}

int correr_servidor(void *arg) {
    char *puerto = (char *) arg; // Castear el argumento de vuelta a t_config

    int server_fd = iniciar_servidor(puerto);
    log_info(logger, "Servidor listo para recibir al cliente");

	t_pcb* pcb;
    int cliente_fd = esperar_cliente(server_fd);
	while (1) {
        int cod_op = recibir_operacion(cliente_fd);
		switch (cod_op) {
		case MENSAJE:
			recibir_mensaje(cliente_fd);
			break;
		case PAQUETE:
			pcb = recibir_pcb(cliente_fd);
			log_info(logger, "Me llegaron los siguientes valores:\n");
			log_info(logger, "pid: %d",pcb->pid);
			break;
		case -1:
			log_error(logger, "el cliente se desconecto. Terminando servidor");
			return EXIT_FAILURE;
		default:
			log_warning(logger,"Operacion desconocida. No quieras meter la pata");
			break;
        }
    }
    return EXIT_SUCCESS;
}

void iterator(char* value) {
    log_info(logger,"%s", value);
}

void
clean(t_config *config) {
    log_destroy(logger);
    config_destroy(config);
}