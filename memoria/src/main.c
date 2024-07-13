#include <memoria.h>
#include <files.h>
#include <utils/inout.h>
//#include "cpu/connections.h

t_memory memory;
t_config *config;
t_log* logger;

extern t_dictionary* listaTablasDePaginas;

void end_process(){
    int frameCount = memory.memory_size / memory.page_size; 

    memset(memory.frames_ocupados, 0, frameCount * sizeof(bool));
}

/*********************************************************//********INCLUIR EN ALGUN.H*******/
t_resize* deserializar_resize(void* stream){
    t_resize* resize = malloc(sizeof(t_resize));
    int offset = 0;

    memcpy(&(resize->pid), stream + offset, sizeof(u_int32_t));
    offset += sizeof(u_int32_t);

    memcpy(&(resize->tamanio), stream + offset, sizeof(u_int32_t));
    offset += sizeof(u_int32_t);

    return resize;
}
/*********************************************************/ 

t_resize* recibir_resize(int socket_cpu){
    int size;
    void *buffer = recibir_buffer(&size, socket_cpu);
    if (buffer == NULL) {
        return NULL;
    }

    t_resize* resize = deserializar_resize(buffer);
    free(buffer);

    return resize;
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


/*********************************************************/ /********INCLUIR EN ALGUN.H*******/
t_request* deserializar_request(void* stream){
    t_request* request = malloc(sizeof(t_request));
    int offset = 0;

    memcpy(&(request->pid), stream + offset, sizeof(u_int32_t));
    offset += sizeof(u_int32_t);

    memcpy(&(request->req), stream + offset, sizeof(int));
    offset += sizeof(int);

    return request;
}

t_request2* deserializar_request2(void* stream){
    t_request2* request = malloc(sizeof(t_request2));
    int offset = 0;

    memcpy(&(request->pid), stream + offset, sizeof(u_int32_t));
    offset += sizeof(u_int32_t);

    memcpy(&(request->req), stream + offset, sizeof(int));
    offset += sizeof(int);
    
    memcpy(&(request->val), stream + offset, sizeof(int));
    offset += sizeof(int);


    return request;
}


/*********************************************************/

void retardo_en_peticiones(){
    sleep(config_get_int_value(config,"RETARDO_RESPUESTA")/1000);
}

t_request* recibir_pagina(int socket_cpu){
    int size;
    void* buffer = recibir_buffer(&size,socket_cpu);
    if (buffer == NULL) {
        return NULL;
    }

    t_request* pagina = deserializar_request(buffer);
    free(buffer);

    return pagina;
}

t_request2* recibir_mov_out(int socket_cpu){
    int size;
    void* buffer = recibir_buffer(&size,socket_cpu);
    if (buffer == NULL) {
        return NULL;
    }

    t_request2* req = deserializar_request2(buffer);
    free(buffer);

    return req;
}

int tamanio_proceso(int pid){
    
} 

bool memoria_llena (int tam_actual,int nuevo_tam){
    return !hayEspacioEnBitmap(tam_actual - nuevo_tam);
}

void modificar_tamanio_proceso (int pid, int nuevo_tamanio,t_config* config){
    TablaPaginas* tablaAsoc = tablaDePaginasAsociada(pid);
    TablaPaginas tablaAux;
    tablaAux = *tablaAsoc;

    dictionary_remove(listaTablasDePaginas,string_itoa(pid));
    liberarTablaPaginas(*tablaAsoc);

    crearTablaPaginas(pid,nuevo_tamanio,config_get_int_value(config,"TAM_PAGINA"));

    TablaPaginas* tablaNew = dictionary_get(listaTablasDePaginas,string_itoa(pid));
    *tablaNew = tablaAux;
}

void nuevo_tamanio_proceso(t_resize* resize, int socket_cpu, t_config* config){
    int tam_actual_proceso = tamanio_proceso(resize->pid);
    int nuevo_tamanio;

    if(tam_actual_proceso > resize->tamanio){ //reduccion de tamanio de proceso
        nuevo_tamanio = resize->tamanio;
        modificar_tamanio_proceso(resize->pid,nuevo_tamanio,config);
    }
    else{
        nuevo_tamanio = tam_actual_proceso + (resize->tamanio - tam_actual_proceso);
        if (memoria_llena(tam_actual_proceso,nuevo_tamanio)) enviar_mensaje("Out of memory",socket_cpu); else modificar_tamanio_proceso(resize->pid,nuevo_tamanio,config);
    } 
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
                handle_create_process(pcb->path,pid,config); //funciona con scripts-pruebas/file1
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
                retardo_en_peticiones();
                enviar_mensaje(get_complete_instruction(pcb->pid, pcb->pc),cliente_fd);
                //enviar_mensaje((char *)instruction, cliente_fd);
                //enviar_mensaje("IO_GEN_SLEEP XXX 10", cliente_fd);
                break;
            case FINISH_PROCESS:
                retardo_en_peticiones();
                pcb = recibir_pcb(cliente_fd);
                log_info(logger, "pid: %d", pcb->pid);
                finish_process(pcb->pid);
                char mensaje[50];
                //snprintf formatea una cadena y escribirla en un búfer de caracteres
                snprintf(mensaje, sizeof(mensaje), "Process %d has finished", pcb->pid);
                enviar_mensaje((char *)mensaje, cliente_fd);
                enviar_respuesta(cliente_fd, OK);
                break;
                
            case PAGE_REQUEST:
                retardo_en_peticiones();
                t_request* request = recibir_pagina(cliente_fd);
                int pagina = request->req;
                enviar_marco(pagina,request->pid, cliente_fd); 
            break;
            case RESIZE:
                retardo_en_peticiones();
                t_resize* resize = recibir_resize(cliente_fd);
                nuevo_tamanio_proceso(resize,cliente_fd,config);
            break;
            case TAM_PAG:
                enviar_mensaje(config_get_string_value(config,"TAM_PAGINA"),cliente_fd);
            break;
            case WRITE: //dada una direccion fisica y un valor de registro, escribirlo (mov_out)
                retardo_en_peticiones();
                t_request2* write = recibir_mov_out(cliente_fd);
                escribir_en_direcc_fisica(write->pid,write->req,write->val);
            break;
            case W_REQ: // IO_STDIN_READ
                t_req_to_w* write_req = receive_req_to_w(cliente_fd);
                handle_paging(write_req->text, write_req->text_size, write_req->pid);
                break;
            case R_REQ:
                t_req_to_r* read_req = receive_req_to_r(cliente_fd);
                break;
            case TLB_MISS: //ESTE CODE OP ACTUA LITERALMENTE IGUAL A PAGE_REQUEST
                retardo_en_peticiones();
                t_request* tlb_request = recibir_pagina(cliente_fd);
                int pagina_tlb = tlb_request->req;
                enviar_marco(pagina_tlb,tlb_request->pid, cliente_fd); 
            break;
            //case INSTRUCTION: //nose para q es, creo que nunca lo use a este
            //break;
            case COPY_STRING: //recibo pid, tamanio, di(direccion fisica es un int) y si(direccion fisica), copio (bytes = tamanio) de si en di
                //recibis pid
                //con ese pid buscas la tabla de pagina asociada
                //la direccion fisica es el numero de pagina dentro de la tabla de paginas
                //entonces con la funcion marcoAsociado se obtendria el marco de esa pagina
                retardo_en_peticiones();
                pcb = recibir_pcb(cliente_fd);
                log_info(logger, "pid: %d", pcb->pid);
                log_info(logger, "pc: %d", pcb->pc);               
                log_info(logger, "quantum: %d", pcb->quantum);
                log_info(logger, "path: %s", pcb->path);
                int direc_fis_1;
                int direc_fis_2;
                copy_string(direc_fis_1, pcb->pid,direc_fis_2, cliente_fd, config);
            break;
            case REG_REQUEST: //debe devolver el valor de un registro dada una direccFisica
                retardo_en_peticiones();
                t_request* reg_request = recibir_pagina(cliente_fd);
                int direccion_fisica = reg_request->req;
                enviar_mensaje(obtener_valor(reg_request->pid,direccion_fisica),cliente_fd);
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

        if (pthread_create(&client_thread, NULL, (void*)handle_client, (void*)new_sock) != 0) {
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
    //const char* mensaje = "SOY UN MENSAJE DE PRUEBA PARA EL MÓDULO DE MEMORIA";
    const char* mensaje = "Procede a escribir en memoria entonces escribe soy un mensaje ESTHETIC";
    handle_paging(mensaje, strlen(mensaje) + 1, 1);

    pthread_mutex_destroy(&memory.mutex_espacio_usuario);
    pthread_mutex_destroy(&memory.mutex_frames_ocupados);
    //free(espacio_usuario); FALTA LIBERARLO. PERO VER DONDE.
    free(memory.frames_ocupados);
}


int main(int argc, char *argv[]) {
    /* ---------------- Setup inicial  ---------------- */
    config = config_create(argv[1]);
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
