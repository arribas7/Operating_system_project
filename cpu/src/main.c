#include <stdlib.h>
#include <stdio.h>
#include <utils/client.h>
#include <commons/log.h>
#include <commons/config.h>
//#include <utils/server.h>
//#include <utils/kernel.h>
#include <pthread.h>
//#include <utils/cpu.h>
#include <semaphore.h>
//#include <math.h>
#include <commons/collections/queue.h>
#include "connections.h"
#include "instructions.h"

t_log *logger;
t_log *loggerError;
t_config *config;

//t_pcb *pcb_cpug;
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
//int numero_pagina = 0;

t_pcb *pcb_cpug;
t_log *cambiarNombre(t_log *logger, char *nuevoNombre);
void escucharAlKernel(void);
int ejecutarServidorCPU(int);
void fetch(t_pcb *pcb);
void recibir_instruccion(int socket_cliente);
int buscar(char *elemento, char **lista);

t_list* interruptions_list; //nodo [pid,interrupcion]


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

void procesar_pcb(t_pcb *pcb)
{
    fetch(pcb);
    decode(pcb);
    execute(pcb);
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
    //hacer hilo para checkear las interrupciones, envio el pcb actualizado y el motivo del desalojo
    check_interrupt();
    //el motivo sera el codeop del paquete que contenga el pcb actualizado
    return NULL;
}

int ejecutarServidorCPU(int server_fd)
{
    interruptions_list = list_create();
    while (1)
    {
        cliente_fd = esperar_cliente(server_fd);
        log_info(logger, "Recibi PCB desde el kernel...");
        int cod_op = recibir_operacion(cliente_fd);
        switch (cod_op){
        case DISPATCH:
            t_list *lista = recibir_paquete(cliente_fd);
            for (int i = 0; i < list_size(lista); i++)
            {
                void *pcb_buffer = list_get(lista, i);
                t_pcb *pcb = deserialize_pcb(pcb_buffer);
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
        case INTERRUPT: //aqui debe ser el motivo de la interrupcion
                t_list *lista = recibir_paquete(cliente_fd);
                for (int i = 0; i < list_size(lista); i++)
                {
                    void *int_buffer = list_get(lista, i);
                    t_interrupt* interrupt = deserializar_interrupcion(int_buffer);
                    free(int_buffer);
                    if (interrupt != NULL)
                    {
                        sem_wait(&interruptions_list_sem);
                        list_add(interruptions_list,interrupt);
                        sem_post(&interruptions_list_sem);
                    }
                    else
                    {
                        log_error(logger, "Error al deserializar interrupcion");
                    }
            }
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