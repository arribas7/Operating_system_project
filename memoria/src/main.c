#include <memoria.h>
#include <files.h>

t_memory memory;
t_config *config;
t_log* logger;

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

void handle_create_process(const char *file_path, uint32_t pid){
//OPEN FILE THAT CORRESPONDS TO PATH_INFO - PID FROM KERNEL
    FILE *file = fopen(file_path, "r");
         if (file != NULL) {
          printf("File succesfully opened.\n");
          fclose(file); 
             } else {
              perror("Error opening File");
             }
    // Inicialización del array de líneas
    char **lineas = NULL;
    int num_lineas = 0;
    size_t longitud = 0;
    char buffer[256]; 
    t_list *instructions = list_create();
    //  Read and store in a list
        while (fgets(buffer, sizeof(buffer), file) != NULL) { //fgets lee una línea del archivo y la almacena en el buffer, incluyendo el salto de línea (\n).
        // Después de leer la línea, se verifica si el último carácter es un salto de línea y, si es así, se reemplaza con el carácter nulo (\0).
        longitud = strlen(buffer);
        if (buffer[longitud - 1] == '\n') { 
            buffer[longitud - 1] = '\0';
        }
        //Add the instruction read from the file to the list
        list_add(instructions, strdup(buffer)); //int list_add(t_list *, void *element);
       
    }
    fclose(file);  
      //Add instruction list to dictionary
   // dictionary_put(dict_pcbs, &pid, instructions);
     // Imprimir las líneas para verificación
    for (int i = 0; i < num_lineas; i++) {
        printf("Posicion %d: %s\n", i, lineas[i]);
        free(lineas[i]); // Liberar cada línea
    }
    free(lineas); // Liberar el array de líneas
           
}

void agregar_instruccion_a_pcb(uint32_t pid, const char *instruccion) {
    t_list *instructions = dictionary_get(dict_pcbs, (void *)(intptr_t)pid);
    if (instructions == NULL) {
        instructions = list_create();
       // dictionary_put(dict_pcbs, (void *)(intptr_t)pid, instructions);
    }
    list_add(instructions, strdup(instruccion));
}

int correr_servidor(void *arg) {
    char *puerto = (char *) arg;

    int server_fd = iniciar_servidor(puerto);
    log_info(logger, "Servidor listo para recibir al cliente");

    t_list *lista;
    // TODO: While infinito para correr el servidor hasta signal SIGTERM/SIGINT
    // int cliente_fd = esperar_cliente(server_fd);
    // TODO: Posiblemente esto va a ir en un thread separado por cada cliente que se conecte.
    while (1) {
        int cliente_fd = esperar_cliente(server_fd); //SOLAMENTE PASANDO DESDE LA LINEA 55 HACIA AQUI LOGRO QUE EL SERVER QUEDE EN ESCUCHA PERMAMANENTE
        //aqui iria el semaforo esperando que un modulo envie la señal de que se conecta, ya sea el kernel o el cpu
        sleep(5);
        int cod_op = recibir_operacion(cliente_fd);
        switch (cod_op) {
            case CREATE_PROCESS:
             lista = recibir_paquete(cliente_fd);
             void *pcb_buffer;
             t_pcb *pcb;
             
             for(int i = 0; i < list_size(lista); i++) {
                pcb_buffer = list_get(lista, i);
                pcb = deserialize_pcb(pcb_buffer);
                log_info(logger, "pid: %d", pcb->pid);
                log_info(logger, "pc: %d", pcb->pc);               
                log_info(logger, "quantum: %d", pcb->quantum);
                log_info(logger, "path: %s", pcb->path);
                const char *path_info = pcb->path; 
                u_int32_t pid = pcb->pid; //KEY TO DICTIONARY?
                //u_int32_t pid = 20;
                //handle_create_process("path_info", pid); DESCOMENTAR CUANDO SE PUEDA RECIBIR PATH
                }
                free(pcb_buffer);
                break;
            /*
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
                //eliminar_pcb(pcb);
                enviar_mensaje("MEM: recibido OK",cliente_fd);
                break;*/
            case PC:
                lista = recibir_paquete(cliente_fd);
                void* reg_buffer;
                t_reg_cpu* reg;
                for(int i = 0; i< list_size(lista); i ++){
                    reg_buffer = list_get(lista, i);
                    reg = deserializar_reg(reg_buffer);
                    log_info(logger, "PC: %d", pcb->pc);     //pcb->pid
                    const char *instruction = get_complete_instruction(&dict, PC);
                    //send_instruction(instruction, cliente_fd);
                    enviar_mensaje((char *)instruction,cliente_fd);
                    }

                //aqui en base al PC recibido el modulo memoria debera buscar en los PCBs recibidos desde el kernel, para devolver a cpu en el siguiente enviar mensaje la proxima instruccion a ejecutar
              
                free(reg_buffer);
                eliminar_reg(reg);
                //enviar_mensaje("SET AX 1",cliente_fd); //simulo una instruccion cualquiera 
                //enviar_mensaje("JNZ AX 4",cliente_fd);
                enviar_mensaje("IO_GEN_SLEEP XXX 10",cliente_fd); //reemp las XXX por alguna interfaz, preg a nico q nombre les puso
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

void clean(t_config *config) {
    log_destroy(logger);
    config_destroy(config);
}

