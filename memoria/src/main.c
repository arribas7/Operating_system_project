#include <memoria.h>
#include <files.h>
//#include "cpu/connections.h"

t_memory memory;
t_config *config;
t_log* logger;

void end_process(){
    int frameCount = memory.memory_size / memory.page_size; 

    memset(memory.frames_ocupados, 0, frameCount * sizeof(bool));
}

t_resize* recibir_resize(int socket_cpu){
    int size;
    void *buffer = recibir_buffer(&size, socket_cpu);
    if (buffer == NULL) {
        return NULL;
    }

    //t_resize* resize = deserializar_resize(buffer);
    free(buffer);

    //return resize;
}
/* HABILITAR CUANDO INCLUYA CONNECTIONS.H
t_request* recibir_pagina(int socket_cpu){
    int size;
    void *buffer = recibir_buffer(&size, socket_cpu);
    if (buffer == NULL) {
        return NULL;
    }

    t_request* pagina = deserializar_request(buffer);
    free(buffer);

    return pagina;
}
*/
/*
void enviar_marco(int pagina,int pid){
    int marco = buscar_marco_en_tabla_de_pagina(pid,pagina); //TO DO
    enviar_mensaje(string_itoa(marco),cliente_fd); //send frame
}
*/

void retardo_en_peticiones(){
    sleep(config_get_int_value(config,"RETARDO_RESPUESTA")/1000);
}

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
                u_int32_t pid = pcb->pid; 
                handle_create_process(pcb->path,pid); //funciona con scripts-pruebas/file1
                printf("Path recibido: %s", pcb->path);
                enviar_respuesta(cliente_fd, OK);
                break;
            case PC:
                pcb = recibir_pcb(cliente_fd);
                log_info(logger, "pid: %d", pcb->pid);
                log_info(logger, "pc: %d", pcb->pc);               
                log_info(logger, "quantum: %d", pcb->quantum);
                log_info(logger, "path: %s", pcb->path);
                /* TODO Jannet: uncomment this, I send a hardcoded data just for testing*/
                //const char *instruction = get_complete_instruction(&dict, pcb->pc,pcb->pid);
                //const char *instruction = get_complete_instruction(pcb->pid, pcb->pc);
                enviar_mensaje(get_complete_instruction(pcb->pid, pcb->pc),cliente_fd);
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
                retardo_en_peticiones();
                //t_request* request = recibir_pagina(cliente_fd);
                //int pid = request->pid;
                //int pagina = request->req;
                //enviar_marco(pagina,pid);
            break;
            case RESIZE:
                retardo_en_peticiones();
                //t_resize* resize = recibir_resize(cliente_fd);
                //int caso = nuevo_tamaño_proceso(resize.tamanio) //deberiamos comparar este tamaño con el del proceso para ver si se amplia o se reduce
                //if(caso == 0) enviar_mensaje("Out of memory",socket_cpu);
                //if(caso == 1) ampliar_proceso(resize.pid);
                //if(caso == 2) reducir_proceso(resize.pid);
            break;
            case TAM_PAG:
                enviar_mensaje(config_get_string_value(config,"TAM_PAGINA"),cliente_fd);
            break;
            case WRITE: //dada una direccion fisica y un valor de registro, escribirlo (mov_out)
            break;
            case TLB_MISS: //deserializar el request, dado un numero de pagina y pid debo enviar el frame asociado
                //este caso es lo mismo que PAGE_REQUEST, dejar solo uno y modificar el code op cargado en la serializacion de cpu
            break;
            //case INSTRUCTION: //nose para q es, creo que nunca lo use a este
            //break;
            case COPY_STRING: //recibo pid, tamanio, di y si, copio bytes tamanio de si en di

            break;
            case REG_REQUEST: //debe devolver el valor de un registro dada una direccFisica
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

void testing_paging(void) {
    const char* mensaje = "Este es un mensaje para el proceso.";
    handle_paging(mensaje, strlen(mensaje) + 1, 1);

  //  pthread_mutex_destroy(&mutex_espacio_usuario);
  //  pthread_mutex_destroy(&memory.mutex_frames_ocupados);
   // free(espacio_usuario);
    free(memory.frames_ocupados);
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

    if (!initPaging()) {
        return EXIT_FAILURE;
    }

    testing_paging();

    /*-------------------Test diccionary----------------------------*/
    /*
    const char *file_path="scripts-pruebas/file1";
    //recibir_path();
    uint32_t TIPO1=1; //tipo es el PID1
     uint32_t TIPO2=2; //tipo es el PID2
    printf("Step PID1: %s\n",file_path);
    handle_create_process(file_path,TIPO1);
    printf("Step PID2: %s\n",file_path);
    handle_create_process(file_path,TIPO2);
    
*/

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
