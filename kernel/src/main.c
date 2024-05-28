#include <stdlib.h>
#include <stdio.h>
#include <utils/client.h>
#include <commons/log.h>
#include <commons/config.h>
#include <utils/server.h>
#include <utils/kernel.h>
#include <pthread.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <console.h>
#include <state_lists.h>
#include <stdatomic.h>
#include <semaphore.h>
#include <utils/inout.h>
#include <utils/cpu.h>
#include <quantum.h>

extern t_list *list_NEW;
extern t_list *list_READY;
extern t_list *list_BLOCKED;
extern t_list *list_EXIT;
t_pcb *pcb_RUNNING;

atomic_int pid_count;
sem_t sem_multiprogramming;
pthread_mutex_t mutex_multiprogramming;
sem_t sem_all_scheduler;
int scheduler_paused = 0;
atomic_int current_multiprogramming_grade;

t_log *logger;
t_config *config;

void clean(t_config *config);

int run_server();

void iterator(char *value);

void* dispatch(void* arg);

void *lt_sched_new_ready(void *arg);

int main(int argc, char *argv[]) {
    /* ---------------- Initial Setup ---------------- */
    t_config *config;
    logger = log_create("kernel.log", "kernel", true, LOG_LEVEL_DEBUG);
    if (logger == NULL) {
        return -1;
    }

    initialize_lists();


    config = config_create("kernel.config");
    if (config == NULL) {
        return -1;
    }

    atomic_init(&pid_count, 0);
    int multiprogramming_grade = config_get_int_value(config, "GRADO_MULTIPROGRAMACION");
    atomic_init(&current_multiprogramming_grade, multiprogramming_grade);
    sem_init(&sem_multiprogramming, 0, multiprogramming_grade);
    sem_init(&sem_all_scheduler, 0, 1);
    pthread_mutex_init(&mutex_multiprogramming, NULL);

    pthread_t server_thread, console_thread, lt_sched_new_ready_thread,hilo_cpu, quantum_counter_thread;

    char *puerto = config_get_string_value(config, "PUERTO_ESCUCHA");
    // TODO: Use a new log with other name for each thread?
    if (pthread_create(&server_thread, NULL, (void*) run_server, puerto) != 0) {
        log_error(logger, "Error creating server thread");
        return -1;
    }
/*
    // Creación y ejecución del hilo de la consola
    if (pthread_create(&hilo_consola, NULL, consola_interactiva, config) != 0) {
        log_error(logger, "Error al crear el hilo de la consola");
        return -1;
    }
*/
    // Creación y ejecución del hilo de CPU
    if (pthread_create(&hilo_cpu, NULL, dispatch, config) != 0) {
        log_error(logger, "Error al crear el hilo de la CPU");


    //TODO: Maybe change the logger for this?
    // Matias: Logger is only for debugging purposes. It should get commented out from within run_quantum_counter once it's working as intended.
    /*t_quantum_thread_params *q_params = get_quantum_params_struct(logger, config);

    if (pthread_create(&quantum_counter_thread, NULL, (void*) run_quantum_counter, q_params) != 0) {
        log_error(logger, "Error creating console thread");
        return -1;
    }*/

    if (pthread_create(&lt_sched_new_ready_thread, NULL, (void*) lt_sched_new_ready, NULL) != 0) {
        log_error(logger, "Error creating long term scheduler thread");
        return -1;
    }

    // Esperar a que los hilos terminen
    pthread_join(server_thread, NULL);
    pthread_join(hilo_cpu, NULL);
    pthread_join(console_thread, NULL);
    pthread_join(quantum_counter_thread, NULL);
    pthread_join(lt_sched_new_ready_thread, NULL);

    // TODO: Esperar a que los hilos den señal de terminado para limpiar la config.
    clean(config);
    return 0;
}

void clean(t_config *config) {
    log_destroy(logger);
    config_destroy(config);
    sem_destroy(&sem_multiprogramming);
    sem_destroy(&sem_all_scheduler);
    pthread_mutex_destroy(&mutex_multiprogramming);
    state_list_destroy(list_NEW);
    state_list_destroy(list_READY);
    state_list_destroy(list_BLOCKED);
    state_list_destroy(list_EXIT);
    free(pcb_RUNNING);
}

void iterator(char *value) {
    log_info(logger, "%s", value);
}

int run_server(void *arg) {
    char *puerto = (char *) arg;

    int server_fd = iniciar_servidor(puerto);
    log_info(logger, "Server ready to receive clients...");

    t_list *lista;
    int cliente_fd = esperar_cliente(server_fd);
    while (1) {
        int cod_op = recibir_operacion(cliente_fd);
        switch (cod_op) {
            case PAQUETE:
                lista = recibir_paquete(cliente_fd);
                log_info(logger, "I receive the following values:\n");
                list_iterate(lista, (void *) iterator);
                break;
            case IO:
                lista = recibir_paquete(cliente_fd);
                t_interface* new_io = list_to_IO(lista);
                char* name = get_IO_name(new_io);
                char* type = get_IO_type(new_io);
                log_info(logger, "NEW IO CONNECTED: Name: %s, Type: %s", name, type);
                delete_IO(new_io);
                log_info(logger, "IO Device disconnected");
                break;
            case -1:
                log_error(logger, "Client disconnected. Finishing server...");
                return EXIT_FAILURE;
            default:
                log_warning(logger, "Unknown operation.");
                break;
        }
    }
    return EXIT_SUCCESS;
}

void* dispatch(void* arg){
    log_debug(logger, "CPU connection running in a thread");
    
    t_config *config = (t_config *) arg;
    int conexion_cpu = conexion_by_config(config, "IP_CPU", "PUERTO_CPU_DISPATCH");

    t_pcb *pcb = new_pcb(22, 0, "/home/utn/");
    t_buffer *buffer = malloc(sizeof(t_buffer));
    serialize_pcb(pcb, buffer);

    t_paquete *paquete = crear_paquete(DISPATCH);
    agregar_a_paquete(paquete, buffer->stream, buffer->size);

    enviar_paquete(paquete, conexion_cpu);
    eliminar_paquete(paquete);
    delete_pcb(pcb);

    recibir_operacion(conexion_cpu); // va a ser cod_op: CPU ACK
    recibir_mensaje(conexion_cpu);

    while (1) {
        int cod_op = recibir_operacion(conexion_cpu);
        switch (cod_op) {
            case IO_GEN_SLEEP:
                t_list* lista = recibir_paquete(conexion_cpu);
                void *instruction_buffer;
                t_instruction* instruction;
                for(int i = 0; i< list_size(lista); i ++){
                    instruction_buffer = list_get(lista, i);
                    instruction = deserializar_instruction_IO(instruction_buffer);
                    log_info(logger, "PID: <%d> - Accion: <%s> - IO: <%s> - Unit: <%s>", instruction->pid , "IO_GEN_SLEEP", instruction->interfaz, instruction->job_unit);
                }
                free(instruction_buffer);
                
            case -1:
                log_error(logger, "el cliente se desconecto. Terminando servidor");
                return EXIT_FAILURE;
            default:
                log_warning(logger, "Operacion desconocida. No quieras meter la pata");
                break;
        }
        sleep(15);
    }

    liberar_conexion(conexion_cpu);
    log_debug(logger, "CPU connection released");
    return NULL;
}