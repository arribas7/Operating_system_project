#include <memoria.h>
#include <files.h>

t_memory memory;
t_config *config;
t_log* logger;

void *handle_client(void *arg) {
    int cliente_fd = *(int*)arg;
    free(arg);

    log_info(logger, "Nuevo cliente conectado, socket fd: %d", cliente_fd);

    t_list *lista;
    while (1) {
        int cod_op = recibir_operacion(cliente_fd);
        switch (cod_op) {
            case CREATE_PROCESS:
                lista = recibir_paquete(cliente_fd);
                void *pcb_buffer;
                t_pcb *pcb;
                for (int i = 0; i < list_size(lista); i++) {
                    pcb_buffer = list_get(lista, i);
                    pcb = deserialize_pcb(pcb_buffer);
                    log_info(logger, "pid: %d", pcb->pid);
                    log_info(logger, "pc: %d", pcb->pc);               
                    log_info(logger, "quantum: %d", pcb->quantum);
                    log_info(logger, "path: %s", pcb->path);
                    const char *path_info = pcb->path; 
                    u_int32_t pid = pcb->pid; //KEY TO DICTIONARY?
                }
                free(pcb_buffer);
                enviar_respuesta(cliente_fd, OK);
                break;
            case PC:
                lista = recibir_paquete(cliente_fd);
                void* reg_buffer;
                t_reg_cpu* reg;
                for (int i = 0; i < list_size(lista); i++) {
                    reg_buffer = list_get(lista, i);
                    reg = deserializar_reg(reg_buffer);
                    log_info(logger, "PC: %d", pcb->pc);     //pcb->pid
                    const char *instruction = get_complete_instruction(&dict, reg->PC);
                    enviar_mensaje((char *)instruction, cliente_fd);
                }
                free(reg_buffer);
                eliminar_reg(reg);
                enviar_mensaje("IO_GEN_SLEEP XXX 10", cliente_fd); //reemp las XXX por alguna interfaz
                break;
            case -1:
                log_error(logger, "El cliente se desconectó. Terminando conexión con el cliente");
                close(cliente_fd);
                pthread_exit(NULL);
                break;
            default:
                log_warning(logger, "Operación desconocida. No quieras meter la pata");
                break;
        }
    }

    return NULL;
}

int correr_servidor(void *arg) {
    char *puerto = (char *) arg;

    int server_fd = iniciar_servidor(puerto);
    log_info(logger, "Servidor listo para recibir clientes");

    while (1) {
        int cliente_fd = esperar_cliente(server_fd);
        if (cliente_fd < 0) {
            log_error(logger, "Error al aceptar el cliente");
            continue;
        }

        pthread_t client_thread;
        int *new_sock = malloc(sizeof(int));
        *new_sock = cliente_fd;

        if (pthread_create(&client_thread, NULL, handle_client, (void*)new_sock) != 0) {
            log_error(logger, "Error al crear el hilo para el cliente");
            free(new_sock);
        }
        pthread_detach(&client_thread);
    }

    return EXIT_SUCCESS;
}

void clean(t_config *config) {
    log_destroy(logger);
    config_destroy(config);
}

int main(int argc, char *argv[]) {
    /* ---------------- Setup inicial  ---------------- */
    
  
      config = config_create("memoria.config");
    if (config == NULL) {
        perror("memoria.config creation failed");
        exit(EXIT_FAILURE);
    }
           
	
    memory.memory_size = config_get_int_value(config,"TAM_MEMORIA");
    memory.page_size = config_get_int_value(config,"TAM_PAGINA");
    memory.port = config_get_string_value(config,"PUERTO");
    memory.ip = config_get_string_value(config,"IP");
    memory.respond_time = config_get_int_value(config,"RETARDO_RESPUESTA");
    
    logger = log_create("memoria.log", "memoria", true, LOG_LEVEL_INFO);
    if (logger == NULL) {
        return -1;
    }
     /*-------------------Pagination----------------------------*/

    initPaging();
  
    /*-------------------Test diccionary----------------------------*/
    const char *file_path="scripts-pruebas/file1";
    uint32_t TIPO=1;
    printf("Step 1: %s\n",file_path);
    handle_create_process(file_path,TIPO);
    //*FILE *open_file(const char *file_path);

    /* ---------------- Hilos ---------------- */

    pthread_t hilo_servidor;
    //t_config *config;
    char *puerto = config_get_string_value(config, "PUERTO_ESCUCHA");
    // El servidor se corre en un hilo
    if (pthread_create(&hilo_servidor, NULL, (void*)correr_servidor, puerto) != 0) {
        log_error(logger, "Error al crear el hilo del servidor");
        return -1;
    }

    pthread_join(hilo_servidor, NULL);
    clean(config);
         
    return 0;
}
