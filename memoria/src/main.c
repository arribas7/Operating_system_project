#include <memoria.h>
t_log *logger;
t_config *config;
//FALTA MUTEX EN VARIABLES GLOBALES
char ***vector = NULL;//GLOBAL
int *tamano = 0;

 t_dictionary *dict_pcbs;
 pthread_mutex_t mutex_dict_pcbs;

int correr_servidor(void *arg);

void clean(t_config *config);

int main(int argc, char *argv[]) {
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
    /* ---------------- Dictionary ---------------- */
    dict_pcbs = dictionary_create();
    pthread_mutex_init(&mutex_dict_pcbs, NULL);

    /* ---------------- Hilos ---------------- */

    pthread_t hilo_servidor;
  
    char *puerto = config_get_string_value(config, "PUERTO_ESCUCHA");
    // TODO: Podemos usar un nuevo log y otro name para loggear en el server
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
                //log_info(logger, "reg->dato: %d", pcb->reg->dato);
                //log_info(logger, "path: %d", pcb->path);
                //const char *path_info = pcb->path; 
                //u_int32_t pid = pcb->pid; //KEY TO DICTIONARY
                const char *path_info = "file1";//memory.config contains relative path
                u_int32_t pid = 20;
                // Simulación de operación CREATE_PROCESS
                handle_create_process("path_info", pid);
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
                    log_info(logger, "PC: %d", reg->PC);
                    uint32_t PC=reg->PC;
                    t_list *instructions_list = dictionary_get(dict_pcbs, pcb->pid);
                    list_get(instructions_list, pcb->pc);
                }

                //aqui en base al PC recibido el modulo memoria debera buscar en los PCBs recibidos desde el kernel, para devolver a cpu en el siguiente enviar mensaje la proxima instruccion a ejecutar
               // obtenerinstruccion(PC); //CONTAINS PC TO MEMORY      
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

