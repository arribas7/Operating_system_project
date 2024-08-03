//#include <memoria.h>
#include <files.h>
#include <utils/inout.h>
//#include "cpu/connections.h
#include "pages.h"

#define SIZE_REG 5

t_memory memory;
t_config *config;
t_log* logger;

extern t_dictionary* listaTablasDePaginas;
int tam_pag;
int server_fd;

void destroy_page_table(void* table) {
    TablaPaginas* tabla = (TablaPaginas*)table;
    if (tabla == NULL) return;

    pthread_mutex_destroy(&tabla->mutex_tabla);
    free(tabla->paginas);
    free(tabla);
}

void destroy_all_page_tables(t_dictionary* listaTablasDePaginas) {
    if (listaTablasDePaginas == NULL) return;

    dictionary_destroy_and_destroy_elements(listaTablasDePaginas, destroy_page_table);
}

void destroy_all_mem(){
    destroy_all_page_tables(listaTablasDePaginas);
    config_destroy(config);
    log_destroy(logger);
}

void cleanup(){
    destroy_all_mem();
}


void end_process(){
    int frameCount = memory.memory_size / memory.page_size; 

    memset(memory.frames_ocupados, 0, frameCount * sizeof(bool));
}

/*********************************************************//********INCLUIR EN ALGUN.H*******/
t_resize* deserializar_resize(void* stream){
    t_resize* resize = malloc(sizeof(t_resize));
    int offset = 0;

    offset += sizeof(int);

    memcpy(&(resize->pid), stream + offset, sizeof(int));
    offset += sizeof(int);

    memcpy(&(resize->tamanio), stream + offset, sizeof(int));
    offset += sizeof(int);

    return resize;
}

t_copy_string* deserializar_copy_string(void* stream){
    t_copy_string* copy_string = malloc(sizeof(t_copy_string));
    int offset = sizeof(int); // tamanio

    memcpy(&(copy_string->pid), stream + offset, sizeof(u_int32_t));
    offset += sizeof(u_int32_t);

    memcpy(&(copy_string->tamanio), stream + offset, sizeof(int));
    offset += sizeof(int);

    memcpy(&(copy_string->fisical_si), stream + offset, sizeof(int));
    offset += sizeof(int);

    memcpy(&(copy_string->fisical_di), stream + offset, sizeof(int));
    offset += sizeof(int);

    return copy_string;
}
t_copy_string* recibir_copy_string (int socket_cpu){
    int size;
    void *buffer = recibir_buffer(&size, socket_cpu);
    if (buffer == NULL) {
        return NULL;
    }

    t_copy_string* cs = deserializar_copy_string(buffer);
    free(buffer);

    return cs;
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

void recibir_tam_pag(int socket_cpu){
    int size;
    void *buffer = recibir_buffer(&size, socket_cpu);
    if (buffer == NULL) {
        return NULL;
    }

    free(buffer);
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

    offset += sizeof(int);

    memcpy(&(request->pid), stream + offset, sizeof(u_int32_t));
    offset += sizeof(u_int32_t);

    memcpy(&(request->req), stream + offset, sizeof(int));
    offset += sizeof(int);

    return request;
}

t_request2* deserializar_request2(void* stream){
    t_request2* request = malloc(sizeof(t_request2));
    int offset = 0;

    offset += sizeof(int);

    memcpy(&(request->pid), stream + offset, sizeof(u_int32_t));
    offset += sizeof(u_int32_t);

    memcpy(&(request->req), stream + offset, sizeof(int));
    offset += sizeof(int);
    
    memcpy(&(request->val), stream + offset, sizeof(int));
    offset += sizeof(int);


    return request;
}


/*********************************************************/

void retardo_en_peticiones() {
    unsigned int retardo_ms = config_get_int_value(config, "RETARDO_RESPUESTA");
    usleep(1000 * retardo_ms);
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
/*
int tamanio_proceso(int pid, int tam_pag){
    TablaPaginas* tablaAsoc = tablaDePaginasAsociada(pid);
    int tamanio_proceso = tablaAsoc->num_paginas;
    return tamanio_proceso;
} 

bool memoria_llena (int tam_actual,int nuevo_tam){
    return !hayEspacioEnBitmap(tam_actual - nuevo_tam);
}

void modificar_tamanio_proceso (int pid, int nuevo_tamanio,t_config* config, int tam_pag){
    TablaPaginas* tablaAsoc = tablaDePaginasAsociada(pid);
    TablaPaginas* tablaAux = NULL;
    tablaAux = tablaAsoc;

    dictionary_remove(listaTablasDePaginas,string_itoa(pid));
    liberarTablaPaginas(*tablaAsoc);

    crearTablaPaginas(pid,nuevo_tamanio,tam_pag);

    TablaPaginas* tablaNew = NULL;
    tablaNew = dictionary_get(listaTablasDePaginas,string_itoa(pid));
    tablaNew = tablaAux;
}
void nuevo_tamanio_proceso(t_resize* resize){

    resize_process(resize->pid,resize->tamanio)
    int tam_actual_proceso = tamanio_proceso(resize->pid,tam_pag);
    int nuevo_tamanio;

    if(tam_actual_proceso > resize->tamanio){ //reduccion de tamanio de proceso
        nuevo_tamanio = resize->tamanio;
        modificar_tamanio_proceso(resize->pid,nuevo_tamanio,config,tam_pag);
    }
    else{
        nuevo_tamanio = tam_actual_proceso + (resize->tamanio - tam_actual_proceso);
        if (memoria_llena(tam_actual_proceso,nuevo_tamanio)) 
            enviar_mensaje("Out of memory",socket_cpu);
        else 
            modificar_tamanio_proceso(resize->pid,nuevo_tamanio,config,tam_pag);
    } 
}
*/


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
                log_debug(logger, "Creating process...");
                pcb = recibir_pcb(cliente_fd);
                log_debug(logger, "pid: %d", pcb->pid);
                log_debug(logger, "pc: %d", pcb->pc);
                u_int32_t pid = pcb->pid; 
                handle_create_process(pcb->path,pid,config); //funciona con scripts-pruebas/file1
                printf("Path recibido: %s", pcb->path);
                enviar_respuesta(cliente_fd, OK);
                delete_pcb(pcb);
                break;
            case PC:
                retardo_en_peticiones();
                log_debug(logger, "Processing next PC...");
                pcb = recibir_pcb(cliente_fd);
                log_debug(logger, "pid: %d", pcb->pid);
                log_debug(logger, "pc: %d", pcb->pc);
                /* TODO Jannet: uncomment this, I send a hardcoded data just for testing*/
                //const char *instruction = get_complete_instruction(&dict, pcb->pc,pcb->pid);
                //const char *instruction = get_complete_instruction(pcb->pid, pcb->pc);
                enviar_mensaje(get_complete_instruction(pcb->pid, pcb->pc),cliente_fd);
                //enviar_mensaje((char *)instruction, cliente_fd);
                //enviar_mensaje("IO_GEN_SLEEP XXX 10", cliente_fd);
                delete_pcb(pcb);
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
                delete_pcb(pcb);
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
                //20-7 modifique ampliar tam proceso y actualizar bitmap
                if (!resize_process(resize->pid,resize->tamanio))
                    enviar_mensaje("Out_of_memory",cliente_fd);
                else
                    enviar_mensaje("OK",cliente_fd);
            break;
            case TAM_PAG:
                retardo_en_peticiones();
                recibir_tam_pag(cliente_fd);
                enviar_mensaje(string_itoa(tam_pag),cliente_fd);
            break;
            case WRITE: //dada una direccion fisica y un valor de registro, escribirlo (mov_out)
                log_debug(logger,"Processing WRITE");
                retardo_en_peticiones();
                t_request2* write = recibir_mov_out(cliente_fd);
                char asciiValue = (char)write->val;
                //escribir_en_direcc_fisica(write->pid,write->req,write->val);

                escribirEnDireccionFisica2(write->req, &asciiValue, sizeof(asciiValue), write->pid);
                //escribirEnDireccionFisica2(write->req, string_itoa(write->val), strlen(string_itoa(write->val)), write->pid);
            break;
            case W_REQ: // IO_STDIN_READ
                log_debug(logger,"Processing W_REQ");
                t_req_to_w* to_write = receive_req_to_w(cliente_fd);
                log_debug(logger, "PID: %d - Cant. Bytes: %d", to_write->pid, to_write->text_size);
                escribirEnDireccionFisica2(to_write->physical_address, to_write->text, to_write->text_size, to_write->pid);
                uint32_t status = 1;
                send_confirmation(cliente_fd, &(status));
                break;
            case R_REQ: // IO_STDOUT_WRITE
                log_debug(logger, "Processing R_REQ");
                t_req_to_r* to_read = receive_req_to_r(cliente_fd);
                // char* to_send = leerDeDireccionFisica(to_read->physical_address, to_read->text_size, to_read->pid);
                char* to_send = malloc(sizeof(char) * (to_read->text_size + 1)); 
                leerDeDireccionFisica3(to_read->physical_address, to_read->text_size, to_send, to_read->pid);
                to_send[to_read->text_size] = '\0';
                log_debug(logger, "Buffer leído de la posicion fisica: %d.\nBuffer:%s\n Bytes leídos: %d", to_read->physical_address, to_send, to_read->text_size);
                enviar_mensaje(to_send, cliente_fd);
                free(to_send);
                break;
            case TLB_MISS: //ESTE CODE OP ACTUA LITERALMENTE IGUAL A PAGE_REQUEST
                log_debug(logger,"Processing TLB_MISS");
                retardo_en_peticiones();
                t_request* tlb_request = recibir_pagina(cliente_fd);
                int pagina_tlb = tlb_request->req;
                enviar_marco(pagina_tlb,tlb_request->pid, cliente_fd); 
            break;
            //case INSTRUCTION: //nose para q es, creo que nunca lo use a este
            //break;
            case COPY_STRING: //recibo pid, tamanio, di(direccion fisica es un int) y si(direccion fisica), copio (bytes = tamanio) de si en di
                log_debug(logger,"Processing COPY_STRING");
                //recibis pid
                //con ese pid buscas la tabla de pagina asociada
                //la direccion fisica es el numero de pagina dentro de la tabla de paginas
                //entonces con la funcion marcoAsociado se obtendria el marco de esa pagina
                retardo_en_peticiones();
                t_copy_string* cs = recibir_copy_string(cliente_fd);
                copy_string(cs->fisical_si,cs->fisical_di,cs->tamanio,cs->pid);
            break;
            case REG_REQUEST: //debe devolver el valor de un registro dada una direccFisica (MOV_IN)
                log_debug(logger, "Processing REG_REQUEST");
                retardo_en_peticiones();
                t_request* reg_request = recibir_pagina(cliente_fd);

                int direccion_fisica = reg_request->req;
                char* leido = malloc(sizeof(char));

                // Leer el valor de un registro que está dentro de un marco
                leerDeDireccionFisica3(direccion_fisica, sizeof(char), leido, reg_request->pid);

                char read_char = leido[0];
                int number;
                if (read_char >= '0' && read_char <= '9') 
                    number = read_char - '0';  //si es un num del 0 al 0 transformamos de una
                else
                    number = (int)read_char; //sino tomamos el ascii
                
                //printf("numero convertido: %d\n", number);
                enviar_mensaje(string_itoa(number), cliente_fd);

                free(leido);
            break;
            case -1:
                log_info(logger, "Connection finished. Client disconnected.");
                return;
            default:
                log_warning(logger, "Operación desconocida. No quieras meter la pata");
               break;
        }
    }

    close(cliente_fd);

    /*return NULL;*/
}

int correr_servidor(void *arg) {
    char *puerto = (char *) arg;
    int cliente_fd = 0;

    server_fd = iniciar_servidor(puerto);
    log_info(logger, "Servidor listo para recibir clientes");

    while (1) {
        cliente_fd = esperar_cliente(server_fd);
        if (cliente_fd < 0) {
            log_error(logger, "Error al aceptar el cliente");
            break;
        }

        pthread_t client_thread;
        int *new_sock = malloc(sizeof(int));
        *new_sock = cliente_fd;

        if(pthread_create(&(client_thread), NULL, (void*) handle_client, (void*) new_sock) != 0) {
            log_error(logger, "Error al crear el hilo para el cliente");
            free(new_sock);
        }
        pthread_detach(client_thread);
    }

    close(cliente_fd);

    return EXIT_SUCCESS;
}

void clean(t_config* config){
    log_destroy(logger);
    config_destroy(config);
    close(server_fd);
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

/*
void test1WriteRead(){
    /*handle_create_process("scripts_memoria/test1WriteRead", 1, config);
    // SET EAX 16 -> CPU
    // SET EBX 20 -> CPU
    // RESIZE 128
    resize_process(1,128);
    // MOV_OUT y MOV_IN escriben solo de a un byte.

    escribirEnDireccionFisica2(write->req, string_itoa(write->val), strlen(string_itoa(write->val)), write->pid);
    leerDeDireccionFisica3(to_read->physical_address, to_read->text_size, to_send, to_read->pid);
    
}

void test2WriteReadTwoProcesses(){

}

void test3IOReplicated(){

}
*/


void handle_graceful_shutdown(int sig) {
    close(server_fd);
    printf("Socket %d closed\n", server_fd);
    exit(0);
}

int main(int argc, char *argv[]) {
    atexit(cleanup);
    // Manage signals
    signal(SIGINT, handle_graceful_shutdown);
    signal(SIGTERM, handle_graceful_shutdown);
    /* ---------------- Setup inicial  ---------------- */
    config = config_create(argv[1]);
    if (config == NULL) {
        perror("memoria.config creation failed");
        exit(EXIT_FAILURE);
    }

    tam_pag = config_get_int_value(config,"TAM_PAGINA");


    memory.memory_size = config_get_int_value(config,"TAM_MEMORIA");
    memory.page_size = config_get_int_value(config,"TAM_PAGINA");
    memory.port = config_get_string_value(config,"PUERTO");
    memory.ip = config_get_string_value(config,"IP");
    memory.respond_time = config_get_int_value(config,"RETARDO_RESPUESTA");

    //iniciar_semaforos();
    
    logger = log_create("memoria.log", "memoria", true, LOG_LEVEL_DEBUG);
    if (logger == NULL) {
        return -1;
    }
     /*-------------------Pagination----------------------------*/

    if (!initPaging()) {
        return EXIT_FAILURE;
    }

    //testing_paging();

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
    destroy_all_mem();
    return 0;
}