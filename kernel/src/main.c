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
#include <short_term_scheduler.h>
#include <long_term_scheduler.h>
#include <quantum.h>
#include <communication_kernel_cpu.h>

extern t_list *list_NEW;
pthread_mutex_t mutex_new;

extern t_list *list_READY;
pthread_mutex_t mutex_ready;

extern t_list *list_BLOCKED;
pthread_mutex_t mutex_blocked;

extern t_list *list_EXIT;
pthread_mutex_t mutex_exit;

t_pcb *pcb_RUNNING;
pthread_mutex_t mutex_running;

atomic_int pid_count;

sem_t sem_multiprogramming;
pthread_mutex_t mutex_multiprogramming;
sem_t sem_all_scheduler;
sem_t sem_st_scheduler;
sem_t sem_quantum;

int scheduler_paused = 0;
atomic_int current_multiprogramming_grade;
char* scheduler_algorithm;

t_log *logger;
t_config *config;

void iterator(char *value) {
    log_info(logger, "%s", value);
}

void clean(t_config *config) {
    log_destroy(logger);
    config_destroy(config);
    sem_destroy(&sem_multiprogramming);
    sem_destroy(&sem_all_scheduler);
    sem_destroy(&sem_st_scheduler);
    sem_destroy(&sem_quantum);
    pthread_mutex_destroy(&mutex_multiprogramming);
    pthread_mutex_destroy(&mutex_new);
    pthread_mutex_destroy(&mutex_running);
    pthread_mutex_destroy(&mutex_ready);
    pthread_mutex_destroy(&mutex_exit);
    pthread_mutex_destroy(&mutex_blocked);
    state_list_destroy(list_NEW);
    state_list_destroy(list_READY);
    state_list_destroy(list_BLOCKED);
    state_list_destroy(list_EXIT);
    free(pcb_RUNNING);
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
    sem_init(&sem_st_scheduler,0, 1);
    sem_init(&sem_quantum,0, 0);
    
    pthread_mutex_init(&mutex_multiprogramming, NULL);
    pthread_mutex_init(&mutex_new, NULL);
    pthread_mutex_init(&mutex_ready, NULL);
    pthread_mutex_init(&mutex_running, NULL);
    pthread_mutex_init(&mutex_blocked, NULL);
    pthread_mutex_init(&mutex_exit, NULL);

    scheduler_algorithm = config_get_string_value(config, "ALGORITMO_PLANIFICACION");

    pthread_t server_thread, console_thread, lt_sched_new_ready_thread, st_sched_ready_running_thread;

    char *puerto = config_get_string_value(config, "PUERTO_ESCUCHA");
    if (pthread_create(&server_thread, NULL, (void*) run_server, puerto) != 0) {
        log_error(logger, "Error creating server thread");
        return -1;
    }

    if (pthread_create(&lt_sched_new_ready_thread, NULL, (void*) lt_sched_new_ready, NULL) != 0) {
        log_error(logger, "Error creating long term scheduler thread");
        return -1;
    }

    if (pthread_create(&st_sched_ready_running_thread, NULL, (void*) st_sched_ready_running, scheduler_algorithm) != 0) {
        log_error(logger, "Error creating short term scheduler thread");
        return -1;
    }

    if (pthread_create(&console_thread, NULL, (void*) interactive_console, config) != 0) {
        log_error(logger, "Error creating console thread");
        return -1;
    }

    // wait for threads to finish
    pthread_join(server_thread, NULL);
    pthread_join(console_thread, NULL);
    pthread_join(lt_sched_new_ready_thread, NULL);
    pthread_join(st_sched_ready_running_thread, NULL);

    clean(config);
    return 0;
}