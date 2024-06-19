#include <memoria.h>
#include <files.h>

t_memory memory;
t_config *config;
t_log* logger;

void handle_client(void *arg) {
    int cliente_fd = *(int*)arg;
    free(arg);

    log_info(logger, "Nuevo cliente conectado, socket fd: %d", cliente_fd);

    t_list *lista;
    while (1) {
        int cod_op = recibir_operacion(cliente_fd);
        t_pcb *pcb;
        switch (cod_op) {
            case CREATE_PROCESS:
                pcb = recibir_pcb(cliente_fd);
                log_info(logger, "pid: %d", pcb->pid);
                log_info(logger, "pc: %d", pcb->pc);               
                log_info(logger, "quantum: %d", pcb->quantum);
                log_info(logger, "path: %s", pcb->path);
                const char *path_info = pcb->path; 
                u_int32_t pid = pcb->pid; //KEY TO DICTIONARY?
                enviar_respuesta(cliente_fd, OK);
                break;
            case PC:
                pcb = recibir_pcb(cliente_fd);
                log_info(logger, "pid: %d", pcb->pid);
                log_info(logger, "pc: %d", pcb->pc);               
                log_info(logger, "quantum: %d", pcb->quantum);
                log_info(logger, "path: %s", pcb->path);
                /* TODO Jannet: uncomment this, I send a hardcoded data just for testing*/
                //const char *instruction = get_complete_instruction(&dict, pcb->pc);
                //enviar_mensaje((char *)instruction, cliente_fd);
                enviar_mensaje("IO_GEN_SLEEP XXX 10", cliente_fd);
                break;
            case FINISH_PROCESS:
                pcb = recibir_pcb(cliente_fd);
                log_info(logger, "pid: %d", pcb->pid);
                log_info(logger, "pc: %d", pcb->pc);               
                log_info(logger, "quantum: %d", pcb->quantum);
                log_info(logger, "path: %s", pcb->path);
                /* TODO Jannet: end process
                //enviar_mensaje((char *)instruction, cliente_fd);
                */
                //end_process();
                enviar_respuesta(cliente_fd, OK);
                break;
            case PAGE_REQUEST:
                pagina = recibir_pagina();
                marco = obtener_marco();
                enviar_marco(marco);
            break;
            case RESIZE:
                t_resize* resize = recibir_resize(socket_cpu);
                int caso = nuevo_tamaño_proceso(resize.tamanio) //deberiamos comparar este tamaño con el del proceso para ver si se amplia o se reduce
                if(caso == 0) enviar_mensaje("Out of memory",socket_cpu);
                if(caso == 1) ampliar_proceso(resize.pid);
                if(caso == 2) reducir_proceso(resize.pid);
            break;
            case -1:
                log_info(logger, "Connection finished. Client disconnected.");
                return;
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

void end_process(){
    int frameCount = memory.memory_size / memory.page_size; 

    memset(memory.frames_ocupados, 0, frameCount * sizeof(bool));
}

t_resize* recibir_resize(socket_cpu){
    int size;
    void *buffer = recibir_buffer(&size, socket_cpu);
    if (buffer == NULL) {
        return NULL;
    }

    t_resize* resize = deserializar_resize(buffer);
    free(buffer);

    return resize;
}