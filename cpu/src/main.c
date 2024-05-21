#include <stdlib.h>
#include <stdio.h>
#include <utils/client.h>
#include <commons/log.h>
#include <commons/config.h>
#include <utils/server.h>
#include <utils/kernel.h>
#include <pthread.h>
#include <utils/cpu.h>
#include <semaphore.h>
#include <math.h>
#include <commons/collections/queue.h>

t_log *logger;
t_log *loggerError;
t_config *config;
t_pcb *pcb_cpug;
char *instruccion_actual;
int instruccion_decodificada;
char **instr_decode;
int cant_parametros;
t_reg_cpu* reg_proceso_actual = NULL;
t_pcb* pcb_en_ejecucion;
int cliente_fd; //el kenel

int correr_servidor(void *arg);
void *conexion_MEM(void *arg);
void clean(t_config *config);
void ejecutar_ciclo_instruccion(t_pcb *pcb);
// void gestionesCPU (void* arg);
int conexion_mem = 0;
int conexionMemoria(t_config *config);
int numero_pagina = 0;

t_pcb *pcb_cpug;
t_log *cambiarNombre(t_log *logger, char *nuevoNombre);
void escucharAlKernel(void);
int ejecutarServidorCPU(int);
void fetch(t_pcb *pcb);
void recibir_instruccion(int socket_cliente);
int buscar(char *elemento, char **lista);

void agregar_a_TLB(int pid, int pagina, int marco);
uint32_t mmu(char* logicalAddress);
void init_reg_proceso_actual();
int requestFrameToMem (int numPag);
int recibir_frame(int socket_cliente);
void putRegValueToMem(int fisicalAddress, int valor);
int valueOfReg (char* reg);

#define cant_entradas_tlb() config_get_int_value("cpu.config","CANTIDAD_ENTRADAS_TLB")
#define tam_pag 32 //ESTO BORRAR, SE LO DEBO PEDIR A MEMORIA

//INSTRUCCIONES EXECUTE:
void set(char* registro, char* valor);
void mov_in(char* registro, char* si);

typedef enum
{
    SET,
    MOV_IN,
    MOV_OUT,
    SUM,
    SUB,
    JNZ,
    RESIZE,
    COPY_STRING,
    WAIT,
    SIGNAL,
    IO_GEN_SLEEP,
    IO_STDIN_READ,
    IO_STDOUT_WRITE,
    IO_FS_CREATE,
    IO_FS_DELETE,
    IO_FS_TRUNCATE,
    IO_FS_WRITE,
    IO_FS_READ,
    EXIT
} t_comando;

char *listaComandos[] = {
    [SET] = "SET",
    [MOV_IN] = "MOV_IN",
    [MOV_OUT] = "MOV_OUT",
    [SUM] = "SUM",
    [SUB] = "SUB",
    [JNZ] = "JNZ",
    [RESIZE] = "RESIZE",
    [COPY_STRING] = "COPY_STRING",
    [WAIT] = "WAIT",
    [SIGNAL] = "SIGNAL",
    [IO_GEN_SLEEP] = "IO_GEN_SLEEP",
    [IO_STDIN_READ] = "IO_STDIN_READ",
    [IO_STDOUT_WRITE] = "IO_STDOUT_WRITE",
    [IO_FS_CREATE] = "IO_FS_CREATE",
    [IO_FS_DELETE] = "IO_FS_DELETE",
    [IO_FS_TRUNCATE] = "IO_FS_TRUNCATE",
    [IO_FS_WRITE] = "IO_FS_WRITE",
    [IO_FS_READ] = "IO_FS_READ",
    [EXIT] = "EXIT"
};

//DEFINIENDO LA TLB:
typedef struct tlb{
    int pid;
    int pagina;
    int marco;
    // Otros campos que puedan ser necesarios
} TLBEntry;

TLBEntry TLB[32];
int tlb_size = 0; //para ir sabiendo cuantas entradas hay en la tlb
int tlb_index = 0; //indice para el algoritmo FIFO

TLBEntry* buscar_en_TLB(int pid, int pagina);

int main(int argc, char *argv[])
{
    /* ---------------- Setup inicial  ---------------- */
    logger = log_create("cpu.log", "cpu", true, LOG_LEVEL_INFO);
    if (logger == NULL)
    {
        return -1;
    }

    config = config_create("cpu.config");
    if (config == NULL)
    {
        return -1;
    }

    /* ---------------- Conexion a memoria ---------------- */

    log_info(logger, "Welcome to CPU!");

    conexionMemoria(config);

    escucharAlKernel();

    clean(config);
    return 0;
}

void escucharAlKernel(void)
{
    char *puerto = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
    int server_fd = iniciar_servidor(puerto);
    log_info(logger, "Listo para escuchar al Kernel!");
    ejecutarServidorCPU(server_fd);
}

//-------------------
// CICLO INSTRUCCION::

void fetch(t_pcb *pcb)
{
    t_buffer *buffer = malloc(sizeof(t_buffer));
    serializar_pcb(pcb, buffer);

    t_paquete *paquete = crear_paquete(PC);
    agregar_a_paquete(paquete, buffer->stream, buffer->size);

    enviar_paquete(paquete, conexion_mem);
    eliminar_paquete(paquete);

    recibir_operacion(conexion_mem);   // sera MENSAJE DESDE MEMORIA (LA INSTRUCCION A EJECUTAR)
    recibir_instruccion(conexion_mem); // LA INSTRUCCION
}

void decode(t_pcb *pcb)
{
    instr_decode = string_n_split(instruccion_actual, 4, " ");
    cant_parametros = string_array_size(instr_decode) - 1;
    instruccion_decodificada = buscar(instr_decode[0], listaComandos);
    log_info(logger, "Decode instruction fetched.. %s", listaComandos[instruccion_decodificada]);
    /*
    instr_decode = strtok(instruccion_actual, " ");
    instruccion_decodificada = buscar(instr_decode,listaComandos);
    log_info(logger, "Decode instruction fetched.. %s", instruccion_actual);
    */
}

void execute(t_pcb *pcb)
{
    pcb_en_ejecucion = pcb;
    switch(cant_parametros){
        case 0:
            log_info(logger, "PID: <%d> - Ejecutando: <%s> ", pcb->pid, listaComandos[instruccion_decodificada]);
        break;
        case 1:
            log_info(logger, "PID: <%d> - Ejecutando: <%s> - <%s>", pcb->pid, listaComandos[instruccion_decodificada], instr_decode[1]);
        break;
        case 2:
            log_info(logger, "PID: <%d> - Ejecutando: <%s> - <%s>, <%s>", pcb->pid, listaComandos[instruccion_decodificada], instr_decode[1], instr_decode[2]);
        break;
        case 3:
            log_info(logger, "PID: <%d> - Ejecutando: <%s> - <%s>, <%s>, <%s>", pcb->pid, listaComandos[instruccion_decodificada], instr_decode[1], instr_decode[2], instr_decode[3]);
        break;
    } 

    switch (instruccion_decodificada)
    {
        case SET:
            set(instr_decode[1],instr_decode[2]);
        break;
        case MOV_IN:
             
        break;
        case MOV_OUT:
             
        break;
        case SUM:
            sum(instr_decode[1],instr_decode[2]);
        break;
        case SUB:
            sub(instr_decode[1],instr_decode[2]);
        break;
        case JNZ:
             jnz(instr_decode[1],instr_decode[2]);
        break;
        case COPY_STRING:
             
        break;
        case WAIT:
             
        break;   
        case SIGNAL:
             
        break;
        case IO_GEN_SLEEP:
            io_gen_sleep(instr_decode[1],instr_decode[2]);
        break;
        case IO_STDIN_READ:
             
        break;
        case IO_STDOUT_WRITE:
             
        break;
        case IO_FS_CREATE:
             
        break;
        case IO_FS_DELETE:
             
        break;
        case IO_FS_TRUNCATE:
             
        break;
        case IO_FS_WRITE:
             
        break;
        case IO_FS_READ:
             
        break;
        case EXIT:
        
        break;
        default:
        break;
    }
}

void procesar_pcb(t_pcb *pcb)
{
    fetch(pcb);
    // Decodificar, ejecutar y verificar interrupciones...
    decode(pcb);
    execute(pcb);
    // Actualizar Program Counter (PC)
    pcb->pc++;
}

void *procesar_pcb_thread(void *arg)
{
    t_pcb *pcb = (t_pcb *)arg;
    // Procesar el PCB y ejecutar el ciclo de instrucción
    log_info(logger, "Procesando el PCB con PID: %d", pcb->pid);
    init_reg_proceso_actual(); //luego de procesar el proceso actual, verificar instrucciones, antes de pasar al nuevo proceso del sig PC, acordarse hacer "free(reg_proceso_actual)"
    procesar_pcb(pcb);
    // Liberar memoria del PCB
    free(pcb);
    return NULL;
}

int ejecutarServidorCPU(int server_fd)
{
    while (1)
    {
        cliente_fd = esperar_cliente(server_fd);
        log_info(logger, "Recibi PCB desde el kernel...");
        int cod_op = recibir_operacion(cliente_fd);
        switch (cod_op)
        {
        case PCB:
            t_list *lista = recibir_paquete(cliente_fd);
            for (int i = 0; i < list_size(lista); i++)
            {
                void *pcb_buffer = list_get(lista, i);
                t_pcb *pcb = deserializar_pcb(pcb_buffer);
                free(pcb_buffer);
                if (pcb != NULL)
                {
                    // Crear un hilo para procesar el PCB
                    pthread_t thread;
                    pthread_create(&thread, NULL, procesar_pcb_thread, (void *)pcb);
                    pthread_detach(thread);
                }
                else
                {
                    log_error(logger, "Error al deserializar el PCB");
                }
            }
            enviar_mensaje("CPU: recibido PCBS del kernel OK", cliente_fd);
            break;
        case -1:
            log_error(logger, "El cliente se desconectó. Terminando servidor");
            return EXIT_FAILURE;
        default:
            log_warning(logger, "Operación desconocida. No quieras meter la pata");
            break;
        }
    }
    return EXIT_SUCCESS;
}

/*
void gestionesCPU (void* arg){
    sem_wait(&sem_cpu2); //fijarse pq solo entra el pcb 2009s
    pthread_mutex_lock(&mutex_pcb);
    if (pcb_cpug != NULL)
        log_info(logger, "Gestionando el PCB..... %d",pcb_cpug->pc); //pensar si queres como seria que en vez de pcb_cpug (var global) sea una cola

    sleep(10);

    //Hacemos el ciclo de instruccion

    pthread_mutex_unlock(&mutex_pcb);
    sem_post(&sem_cpu1);
}
*/

int conexionMemoria(t_config *config)
{

    log_info(logger, "CPU - MEMORIA");

    while (1)
    {
        conexion_mem = conexion_by_config(config, "IP_MEMORIA", "PUERTO_MEMORIA");
        if (conexion_mem != -1)
        {
            log_info(logger, "Memoria conectada!");
            return 0;
        }
        else
            log_error(logger, "No se pudo conectar al servidor, socket %d, esperando 5 segundos y reintentando.", conexion_mem);
        sleep(5);
    }

    return 0;
}

void clean(t_config *config)
{
    log_destroy(logger);
    config_destroy(config);
}
/*
t_log *cambiarNombre(t_log* logger, char *nuevoNombre) {
    t_log *nuevoLogger = logger;
    if (logger != NULL && nuevoNombre != NULL){
        free(logger->program_name);
        nuevoLogger->program_name = strdup(nuevoNombre);
    }

    return nuevoLogger;
}*/

//---------------

int buscar(char *elemento, char **lista)
{
    int i = 0;
    // for (; strcmp(lista[i], elemento) && i < string_array_size(lista); i++);
    while (i <= string_array_size(lista))
    {
        if (i < string_array_size(lista))
            if (!strcmp(elemento, lista[i]))
                return i;
        //    debug ("%s", lista[i]);
        i++;
    }
    return (i > string_array_size(lista)) ? -1 : i;
}

void recibir_instruccion(int socket_cliente)
{
    int size;
    instruccion_actual = recibir_buffer(&size, socket_cliente);
    log_info(logger, "Before fetching instruction.. %s", instruccion_actual);
}


//------- FUNCIONES DE EXECUTE:



void set(char* registro, char* valor){

    if (strcmp(registro, "AX") == 0)
        reg_proceso_actual->AX = (uint8_t)atoi(valor);
    if (strcmp(registro, "BX") == 0) 
        reg_proceso_actual->BX = atoi(valor);
    if (strcmp(registro, "CX") == 0) 
        reg_proceso_actual->CX = atoi(valor);
    if (strcmp(registro, "EAX") == 0)
        reg_proceso_actual->EAX = atoi(valor);
    if (strcmp(registro, "EBX") == 0)
        reg_proceso_actual->EBX = atoi(valor);
    if (strcmp(registro, "ECX") == 0)
        reg_proceso_actual->ECX = atoi(valor);
    if (strcmp(registro, "EDX") == 0)
        reg_proceso_actual->EDX = atoi(valor);
    if (strcmp(registro, "SI") == 0)
        reg_proceso_actual->SI = atoi(valor);
    if (strcmp(registro, "DI") == 0)
        reg_proceso_actual->DI = atoi(valor);

    log_info(logger, "%s in actual process: %d", registro, reg_proceso_actual->AX); //funciona
}

void mov_in(char* registro, char* logicalAddress){
    int valor = 0; //quitar luego de hacer el siguiente TO DO
    int fisicalAddr = mmu(logicalAddress);
    //request to mem
    //int valor = requestRegToMem(fisicalAddr); //TO DO

    if (strcmp(registro, "AX") == 0)
        reg_proceso_actual->AX = valor;
    if (strcmp(registro, "BX") == 0) 
        reg_proceso_actual->BX = valor;
    if (strcmp(registro, "CX") == 0) 
        reg_proceso_actual->CX = valor;
    if (strcmp(registro, "EAX") == 0)
        reg_proceso_actual->EAX = valor;
    if (strcmp(registro, "EBX") == 0)
        reg_proceso_actual->EBX = valor;
    if (strcmp(registro, "ECX") == 0)
        reg_proceso_actual->ECX = valor;
    if (strcmp(registro, "EDX") == 0)
        reg_proceso_actual->EDX = valor;
}

void mov_out(char* logicalAddr, char* reg){

    int valor;
    int fisicalAddress = mmu(logicalAddr);

    if (strcmp(reg, "AX") == 0)
        valor = reg_proceso_actual->AX;
    if (strcmp(reg, "BX") == 0) 
        valor = reg_proceso_actual->BX;
    if (strcmp(reg, "CX") == 0) 
        valor = reg_proceso_actual->CX;
    if (strcmp(reg, "EAX") == 0)
        valor = reg_proceso_actual->EAX;
    if (strcmp(reg, "EBX") == 0)
        valor = reg_proceso_actual->EBX;
    if (strcmp(reg, "ECX") == 0)
        valor = reg_proceso_actual->ECX;
    if (strcmp(reg, "EDX") == 0)
        valor = reg_proceso_actual->EDX;

    putRegValueToMem(fisicalAddress,valor); 
}

void sum(char* destReg, char* origReg){                 

    int valor1,valor2,suma;

    valor1 = valueOfReg(destReg);
    valor2 = valueOfReg(origReg);

    suma = valor1 + valor2;

    set(destReg,string_itoa(suma));  
}

void sub(char* destReg, char* origReg){

    int valor1,valor2,resta;

    valor1 = valueOfReg(destReg);
    valor2 = valueOfReg(origReg);

    resta = valor1 - valor2;

    set(destReg,string_itoa(resta));   
}

void jnz(char* reg, char* inst){
    //log_info(logger, "PC in actual process before: %d", reg_proceso_actual->PC); //funciona

    if(valueOfReg(reg))
        reg_proceso_actual->PC = atoi(inst);

    //log_info(logger, "PC in actual process after: %d", reg_proceso_actual->PC); //funciona probe init reg proceso actual con AX = 1
}
/*
void resize(int tamanio){
    //solicitar a mem ajustar el tamaño del proceso a tamanio, deberia ser un opCode que reciba mem

    //armar paquete con opcode RESIZE
    t_paquete* resize = crear_paquete(RESIZE);

    //if out of memory como respuesta
    //devolver contexto ej a kernel informando esto.
}
*/

//ejemplo de interfaz: GENERICA
void io_gen_sleep(char* interfaz, char* job_unit){

    t_paquete* peticion = crear_paquete(IOSLEEP); //this opcode receive in KERNEL
    int tamInterfaz = string_length(interfaz);
    int tamJobUnit = string_length(job_unit);

    agregar_a_paquete(peticion, &(pcb_en_ejecucion->pid), sizeof(uint32_t));
    agregar_a_paquete(peticion, interfaz, sizeof(char) * tamInterfaz + 1);
    agregar_a_paquete(peticion, job_unit, sizeof(char) * tamJobUnit + 1); 

    enviar_paquete(peticion, cliente_fd);
    eliminar_paquete(peticion);

    log_info(logger, "PID: <%d> - Accion: <%s> - IO: <%s>", pcb_en_ejecucion->pid, "IO_GEN_SLEEP", interfaz);
    /*
    recibir_operacion(cliente_fd);
    recibir_mensaje(cliente_fd); //receive ack from kermel
    */
}
//ARMAR FUNCION DESERIALIZADORA EN EL KERNEL:


int valueOfReg (char* reg){
    if (strcmp(reg, "AX") == 0)
        return reg_proceso_actual->AX;
    if (strcmp(reg, "BX") == 0)
        return reg_proceso_actual->BX;
    if (strcmp(reg, "CX") == 0)
        return reg_proceso_actual->CX;
    if (strcmp(reg, "EAX") == 0)
        return reg_proceso_actual->EAX;
    if (strcmp(reg, "EBX") == 0)
        return reg_proceso_actual->EBX;
    if (strcmp(reg, "ECX") == 0)
        return reg_proceso_actual->ECX;
    if (strcmp(reg, "EDX") == 0)
        return reg_proceso_actual->EDX;

    return 0;
}


//-------------MMU con la TLB:

uint32_t mmu(char* logicalAddress){
    int direccion_fisica;
    int direccion_logica = atoi(logicalAddress);
    TLBEntry* tlbEntry;

    //int tam_pag = solicitar_tam_pag();//a memoria (32)  TO DO

    numero_pagina = floor(direccion_logica / tam_pag);
    int desplazamiento = direccion_logica - (numero_pagina * tam_pag);

    log_debug(logger, "nroPagina: %d", numero_pagina);
    log_debug(logger, "desplazamiento: %d", desplazamiento);


    tlbEntry = buscar_en_TLB(pcb_en_ejecucion->pid,numero_pagina);

    if (tlbEntry == NULL){ //actualizo la TLB
        log_info(logger, "PID: <%d> - TLB MISS - Pagina: <%d> ", pcb_en_ejecucion->pid, numero_pagina);
        //aqui no la calculo sino que se la pido a memoria
        direccion_fisica = requestFrameToMem(numero_pagina);
        //direccion_fisica = (numero_pagina * tam_pag) + desplazamiento;
        agregar_a_TLB(pcb_en_ejecucion->pid,numero_pagina,direccion_fisica);
    }
    else{
        log_info(logger, "PID: <%d> - TLB HIT - Pagina: <%d> ", pcb_en_ejecucion->pid, numero_pagina);
        direccion_fisica = tlbEntry->marco;
    }
    return direccion_fisica;
}

TLBEntry* buscar_en_TLB(int pid, int pagina){
    for (int i=0; i < cant_entradas_tlb() ; i++){
        if (TLB[i].pid == pid && TLB[i].pagina == pagina){
            return &TLB[i];
        }  
    }
    return NULL; //si no encuentra en la tlb
}

void agregar_a_TLB(int pid, int pagina, int marco) {
    // Verificar si la TLB está llena
    if (tlb_size < cant_entradas_tlb()) {
        // Si hay espacio disponible, agregar la entrada al final de la TLB
        TLB[tlb_size].pid = pid;
        TLB[tlb_size].pagina = pagina;
        TLB[tlb_size].marco = marco;
        tlb_size++;        // Incrementar el tamaño de la TLB
    } else {
        // Si la TLB está llena, se necesita aplicar un algoritmo de reemplazo
        // En este caso, utilizaremos FIFO (Primero en entrar, primero en salir)
        // Se sobrescribirá la entrada más antigua (la que está en el índice tlb_index)
        TLB[tlb_index].pid = pid;
        TLB[tlb_index].pagina = pagina;
        TLB[tlb_index].marco = marco;
        // Incrementar el índice para la próxima entrada
        tlb_index = (tlb_index + 1) % cant_entradas_tlb(); // Incremento circular del índice
    }
}


void putRegValueToMem(int fisicalAddress, int valor){
    t_paquete* peticion = crear_paquete(WRITE); //this opcode receive in mem
    //int tamReg = obtenerTamanioReg(reg);

    agregar_a_paquete(peticion, &(pcb_en_ejecucion->pid), sizeof(uint32_t));
    agregar_a_paquete(peticion, &fisicalAddress, sizeof(int));
    agregar_a_paquete(peticion, &valor, sizeof(int)); 
    //armar la funcion deserializadora de este paquete para recibirlo bien en memoria
    enviar_paquete(peticion, conexion_mem);
    eliminar_paquete(peticion);

    recibir_operacion(conexion_mem);
    recibir_mensaje(conexion_mem); //receive ack from mem "guarde lo q me pediste"

    log_info(logger, "PID: <%d> - Accion: <%s> - Pagina: <%d> - Direccion Fisica: <%d> - Valor: <%s>", pcb_en_ejecucion->pid, "WRITE", numero_pagina, fisicalAddress, (char *)valor);
}

/*
	int size, desplazamiento=0, pid, tamanio;
	int32_t direccionFisica;

	void* buffer = recibirBuffer(socketCPU, &size);
	desplazamiento += sizeof(uint32_t);
	memcpy(&(pid), buffer + desplazamiento, sizeof(int));
	desplazamiento += sizeof(uint32_t) + sizeof (int);
	memcpy(&(direccionFisica), buffer + desplazamiento, sizeof(int32_t));
	desplazamiento += sizeof(uint32_t) + sizeof(int); 
	memcpy(&(tamanio),buffer+desplazamiento,sizeof(int)); 

	valorLeido = leer(direccionFisica, tamanio); 
	valorLeido = realloc (valorLeido, tamanio + 1);
	valorLeido[tamanio] = '\0';
    */

int obtenerTamanioReg(char* registro){
    if(string_starts_with(registro, "E")) return 3;
    else return 2;
}

void init_reg_proceso_actual(){
    reg_proceso_actual = (t_reg_cpu *)malloc(sizeof(t_reg_cpu));
    if (reg_proceso_actual == NULL) {
        fprintf(stderr, "Error al asignar memoria\n");
        return 1;
    }

    reg_proceso_actual->AX = 0;
    reg_proceso_actual->BX = 0;
    reg_proceso_actual->CX = 0;
    reg_proceso_actual->DI = 0;
    reg_proceso_actual->EAX = 0;
    reg_proceso_actual->EBX = 0;
    reg_proceso_actual->ECX = 0;
    reg_proceso_actual->EDX = 0;
    reg_proceso_actual->PC = 0;
    reg_proceso_actual->SI = 0;
}

int requestFrameToMem (int numPag){
    t_paquete* peticion = crear_paquete(TLBMISS); //this opcode receive in mem

    agregar_a_paquete(peticion, &(pcb_en_ejecucion->pid), sizeof(uint32_t));
    agregar_a_paquete(peticion, &numPag, sizeof(int));
    //armar la funcion deserializadora de este paquete para recibirlo bien en memoria
    enviar_paquete(peticion, conexion_mem);
    eliminar_paquete(peticion);

    recibir_operacion(conexion_mem); //OPCODE: FRAME
    return recibir_frame(conexion_mem); //receive ack from mem "guarde lo q me pediste"
}

int recibir_frame(int socket_cliente)
{
    int size;
    char* frame;
    frame = recibir_buffer(&size, socket_cliente);
    log_info(logger, "Frame received... %s", frame);
    return atoi(frame);
}

/*
case WRITE:
    recibir_paquete();
    deserializar(); //aca recibo la direccion fisica y valor
    fisicalAddress la transformo en un idx
    escribir(idx, valor); 
    aumentaras el puntero de la tabla de memoria 

    valor   
    ----    *
    ----
    ----
*/