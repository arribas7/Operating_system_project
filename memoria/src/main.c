#include <memoria.h>


t_log *logger;
t_config *config;

int main(int argc, char *argv[]) {
    
    /* ---------------- Setup inicial  ---------------- */
    
    t_config *config; // // Declarar un puntero para almacenar la configuración
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


void *manejar_cliente(void *arg){
   //char *puerto = (char *) arg;
   int cliente_fd = *((int *)arg);
  // int server_fd = iniciar_servidor(puerto);
   t_list *lista;
   //int cliente_fd = esperar_cliente(server_fd);
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
                break;
            case -1:
                log_error(logger, "el cliente se desconecto. Terminando servidor");
                return EXIT_FAILURE;
            default:
                log_warning(logger, "Operacion desconocida. No quieras meter la pata");
                break;
        }
    }
 }   



int correr_servidor(void *arg) {
    char *puerto = (char *) arg;

    int server_fd = iniciar_servidor(puerto);
    log_info(logger, "Servidor listo para recibir al cliente");

     while (1) {
        int cliente_fd = esperar_cliente(server_fd);
        
        // Crear un nuevo hilo para manejar la conexión con el cliente por separado
        pthread_t hilo_cliente;
        if (pthread_create(&hilo_cliente, NULL, manejar_cliente, (void *)&cliente_fd) != 0) {
            log_error(logger, "Error al crear el hilo para manejar al cliente");
            close(cliente_fd);
            continue; // Continuar esperando la conexión de otros clientes
        }
        // Detach el hilo para que libere sus recursos automáticamente al terminar
        pthread_detach(hilo_cliente);
    }
   /*
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
                break;
            case -1:
                log_error(logger, "el cliente se desconecto. Terminando servidor");
                return EXIT_FAILURE;
            default:
                log_warning(logger, "Operacion desconocida. No quieras meter la pata");
                break;
        }
    }
    */
    return EXIT_SUCCESS;
}





void clean(t_config *config) {
   // free(memoria);
    log_destroy(logger);
    config_destroy(config);
}

