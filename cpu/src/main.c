#include <stdlib.h>
#include <stdio.h>
#include <utils/client.h>
#include <commons/log.h>
#include <commons/config.h>
//#include <utils/server.h>
//#include <utils/kernel.h>
#include <pthread.h>
//#include <utils/cpu.h>
//#include <math.h>
#include <commons/collections/queue.h>
#include "connections.h"
#include "instructions.h"

t_log *logger;
t_log *loggerError;
t_config *config;

//t_pcb *pcb_cpug;
char *instruccion_actual = NULL;
int instruccion_decodificada;
char **instr_decode = NULL;
int cant_parametros;
char* ack;

t_register* reg_proceso_actual = NULL;
t_pcb* pcb_en_ejecucion;
int kernel_dispatch_socket;
int kernel_interrupt_socket;

int correr_servidor(void *arg);
void *conexion_MEM(void *arg);
void clean(t_config *config);
void ejecutar_ciclo_instruccion(t_pcb *pcb);
// void gestionesCPU (void* arg);
int conexion_mem = 0;
int conexionMemoria(t_config *config);
//int numero_pagina = 0;

t_pcb *pcb_cpug;
t_log *cambiarNombre(t_log *logger, char *nuevoNombre);
void run_dispatch_server();
void run_interrupt_server();
int handle_dispatch(int);
int handle_interrupt(int);
void fetch(t_pcb *pcb);
void recibir_instruccion(int socket_cliente);
int buscar(char *elemento, char **lista);

op_code interrupted_reason = 0;
int server_fd_dispatch;
int server_fd_interrupt;
/*
void destroy_all_cpu(){
    delete_pcb(pcb_en_ejecucion);
    free(reg_proceso_actual);
    free(instruccion_actual);
    //free_all_global_variables();
    config_destroy(config);
    log_destroy(logger);
}
*/
void cleanup(){
    liberar_conexion(server_fd_dispatch);
    liberar_conexion(server_fd_interrupt);
    liberar_conexion(conexion_mem);
    //destroy_all_cpu();
    //free(instruccion_actual);
}
/*
void free_all_global_variables(){
    free(ack);
    free(instruccion_actual);
    free_double_pointer(instr_decode,sizeof(instr_decode));
}
*/

//agregar aqui en esta funcion los frees
void handle_graceful_shutdown(int sig) {
    close(server_fd_dispatch);
    close(server_fd_interrupt);
    printf("Socket for dispatch: %d closed\n", server_fd_dispatch);
    printf("Socket for interrupt: %d closed\n", server_fd_interrupt);
    exit(0);
}

int main(int argc, char *argv[])
{
    atexit(cleanup);
    // Manage signals
    signal(SIGINT, handle_graceful_shutdown);
    signal(SIGTERM, handle_graceful_shutdown);

    /* ---------------- Setup inicial  ---------------- */
    logger = log_create("cpu.log", "cpu", true, LOG_LEVEL_DEBUG);
    if (logger == NULL)
    {
        return -1;
    }

    config = config_create(argv[1]);
    if (config == NULL)
    {
        return -1;
    }

    /* ---------------- Conexion a memoria ---------------- */

    log_info(logger, "Welcome to CPU!");

    conexionMemoria(config);
    pthread_t dispatch_thread;
    pthread_t interrupt_thread;

    if(pthread_create(&(dispatch_thread), NULL, (void*) run_dispatch_server, NULL) != 0) {
        log_error(logger, "Error creating thread for dispatch server.");
    }
    if(pthread_create(&(interrupt_thread), NULL, (void*) run_interrupt_server, NULL) != 0) {
        log_error(logger, "Error creating thread for interrupt server.");
    }

    pthread_join(dispatch_thread, NULL);
    pthread_join(interrupt_thread, NULL);

    clean(config);
    return 0;
}

void run_dispatch_server(void){
    char *puerto = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
    server_fd_dispatch = iniciar_servidor(puerto);
    log_info(logger, "Dispatch server ready on socket %d.", server_fd_dispatch);
    handle_dispatch(server_fd_dispatch);
}

void run_interrupt_server(void){
    char *puerto = config_get_string_value(config, "PUERTO_ESCUCHA_INTERRUPT");
    server_fd_interrupt = iniciar_servidor(puerto);
    log_info(logger, "Interrupt server ready on socket %d.", server_fd_interrupt);
    handle_interrupt(server_fd_interrupt);
}

t_paquete *procesar_pcb(t_pcb *pcb){
    // Procesar el PCB y ejecutar el ciclo de instrucción
    log_info(logger, "Procesando el PCB con PID: %d", pcb->pid);
    
    //init_reg_proceso_actual(); //luego de procesar el proceso actual, verificar instrucciones, antes de pasar al nuevo proceso del sig PC, acordarse hacer "free(reg_proceso_actual)"
    reg_proceso_actual = pcb->reg;
    t_paquete *response = NULL;
    
    pcb_en_ejecucion = pcb;

    while(true){
        fetch(pcb_en_ejecucion);
        decode(pcb_en_ejecucion);
        response = execute(pcb_en_ejecucion);
        if (instr_decode != NULL) string_array_destroy(instr_decode);
        if (instruccion_actual != NULL) free(instruccion_actual);
        if(reg_proceso_actual->PC != pcb_en_ejecucion->pc) {
            pcb_en_ejecucion->pc = reg_proceso_actual->PC;
        } else {
            pcb_en_ejecucion->pc++;
            reg_proceso_actual->PC = pcb_en_ejecucion->pc;
        }
        pcb_en_ejecucion->reg = reg_proceso_actual;

        if(response != NULL){
            break;
        }

        op_code actual_interrupt_reason = check_interrupt();
        if(actual_interrupt_reason > 0){
            log_debug(logger, "Enviando response por INTERRUPT al kernel..."); 
            response = crear_paquete(actual_interrupt_reason);
            break;
        }
    }

    if(response != NULL){
        t_buffer* buffer = malloc(sizeof(t_buffer));

        log_debug(logger, "Enviando response OK al kernel..."); 
        serialize_pcb(pcb_en_ejecucion, buffer); // We always need to return pcb updated
        agregar_a_paquete(response,buffer->stream,buffer->size);
        free(buffer->stream);
        free(buffer);
    }
    return response;
}

int handle_dispatch(int server_fd)
{
    while (1)
    {
        kernel_dispatch_socket = esperar_cliente(server_fd);
        log_info(logger, "Received PCB from kernel...");
        
        int cod_op = recibir_operacion(kernel_dispatch_socket);
        if(cod_op != DISPATCH){
            log_error(logger, "Invalid operation code or disconnected from client for dispatch handler. cod_op: %s", cod_op);
            enviar_respuesta(kernel_dispatch_socket, NOT_SUPPORTED);
            continue;
        }

        t_pcb *pcb = recibir_pcb(kernel_dispatch_socket);
        if (pcb == NULL) {
            log_error(logger, "Error deserializing PCB.");
            enviar_respuesta(kernel_dispatch_socket, GENERAL_ERROR);
            continue;
        }
        // Process dispatch and get the updated pcb
        
        t_paquete *response = procesar_pcb(pcb);
        enviar_paquete(response, kernel_dispatch_socket);
        eliminar_paquete(response);
        delete_pcb(pcb);
        liberar_conexion(kernel_dispatch_socket);
    }
    return EXIT_SUCCESS;
}

int handle_interrupt(int server_fd)
{
    while (1)
    {
        kernel_interrupt_socket = esperar_cliente(server_fd);
        log_info(logger, "Received interrupt signal from kernel...");
        int cod_op = recibir_operacion(kernel_interrupt_socket);
        switch (cod_op)
        {
            case INTERRUPT_BY_USER:
            case INTERRUPT_TIMEOUT:
                interrupted_reason = cod_op;
                break;
            
            default:
                log_error(logger, "Invalid operation code or disconnected from client for interrupt handler. cod_op: %s", cod_op);
                enviar_respuesta(kernel_interrupt_socket, NOT_SUPPORTED);
                break;
        }

        enviar_respuesta(kernel_interrupt_socket, OK);
    }
    return EXIT_SUCCESS;
}

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

int obtenerTamanioReg(char* registro){
    if(string_starts_with(registro, "E")) return 3;
    else return 2;
}