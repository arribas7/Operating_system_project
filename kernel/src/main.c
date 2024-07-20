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
#include <short_term_scheduler.h>
#include <long_term_scheduler.h>
#include <communication_kernel_cpu.h>
#include <resources_manager.h>
#include <io_manager.h>

t_interface_list* interface_list;

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

void destroy_all() {
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
    destroy_resource_list();
    free(pcb_RUNNING);
}

void handle_client(void *arg) {
    int cliente_fd = *(int*)arg;
    free(arg);
    log_info(logger, "New client connected, socket fd: %d", cliente_fd);

    t_list *lista;
    char* interface_name;

    while (1) {
        int cod_op = recibir_operacion(cliente_fd);
        lista = recibir_paquete(cliente_fd);
        switch (cod_op) {
            case IO:
                uint32_t status;
                t_interface* interface = list_to_interface(lista, cliente_fd);                
                interface_name = get_interface_name(interface);

                if(find_interface_by_name(interface_list, interface_name) != NULL) {
                    log_error(logger, "THE INTERFACE %s ALREADY EXISTS", interface_name);
                    delete_interface(interface);
                    status = 0;
                } else {
                    char* type = type_from_list(lista);
                    log_info(logger, "NEW IO CONNECTED: NAME: %s, TYPE: %s", interface_name, type);
                    add_interface_to_list(interface_list, interface);
                    free(type);
                    status = 1;
                }

                char* mssg = mssg_log(status);
                log_info(logger, "STATUS %s: %s", interface_name, mssg);
                send_confirmation(cliente_fd, status);

                /*INSTRUCTION - 1 - TODO: Remove, only for tests.

                t_instruction* instruction_1 = create_instruction_IO(1, IO_GEN_SLEEP, "GENERICA", 200, 0, 0, "myPath", 0);
                send_instruction_IO(instruction_1, cliente_fd);
                log_info(logger, "INSTRUCTION_SENDED");
                delete_instruction_IO(instruction_1);

                // INSTRUCTION - 2

                t_instruction* instruction_2 = create_instruction_IO(2, IO_GEN_SLEEP, "GENERICA", 300, 0, 0, "myOtherPath", 0);
                send_instruction_IO(instruction_2, cliente_fd);
                log_info(logger, "INSTRUCTION_SENDED");
                delete_instruction_IO(instruction_2);

                // INSTRUCTION - 3

                t_instruction* instruction_3 = create_instruction_IO(3, IO_STDIN_READ, "prueba", 500, 0, 0, "myOtherPath", 0);
                send_instruction_IO(instruction_3, cliente_fd);
                log_info(logger, "INSTRUCTION_SENDED");
                delete_instruction_IO(instruction_3);*/

                break;
            case REPORT:
                t_report* report = list_to_report(lista);
                log_info(logger, "PID: %d", report->pid);
                log_info(logger, "OPERATION_RESULT: %d", report->result); // 0 - ERROR / 1 - OK
                io_unblock(report->pid);
                break;
            case -1:
                log_info(logger, "Client disconnected. Finishing client connection...");
                delete_interface_from_list(interface_list, interface_name);
                liberar_conexion(cliente_fd);
                return;
            default:
                log_warning(logger, "Unknown operation.");
                break;
        }
    }
}

void run_server(void *arg) {
    char *puerto = (char *) arg;

    int server_fd = iniciar_servidor(puerto);
    log_info(logger, "Server ready to receive clients...");

    while(1) {
        int cliente_fd = esperar_cliente(server_fd);

        pthread_t client_thread;
        int *new_sock = malloc(sizeof(int));
        *new_sock = cliente_fd;

        if(pthread_create(&(client_thread), NULL, (void*) handle_client, (void*) new_sock) != 0) {
            log_error(logger, "Error creating thread for the client.");
            free(new_sock);
        }
        pthread_detach(client_thread);
    }
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
    /* ---------------- Initial Setup ---------------- */
    logger = log_create("kernel.log", "kernel", true, LOG_LEVEL_DEBUG);
    if (logger == NULL) {
        return -1;
    }

    initialize_lists();
    interface_list = create_interface_list();
    
    config = config_create(argv[1]);
    if (config == NULL) {
        return -1;
    }
    initialize_resources();

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

    initialize_resources(config);
    scheduler_algorithm = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
    pthread_t server_thread, console_thread, lt_sched_new_ready_thread, st_sched_ready_running_thread;
    char *puerto = config_get_string_value(config, "PUERTO_ESCUCHA");

    if (pthread_create(&(server_thread), NULL, (void *) run_server, (void *) puerto) != 0) {
        log_error(logger, "Error creating server thread");
        return -1;
    }

    if (pthread_create(&(lt_sched_new_ready_thread), NULL, (void *) lt_sched_new_ready, NULL) != 0) {
        log_error(logger, "Error creating long term scheduler thread");
        return -1;
    }

    if (pthread_create(&(st_sched_ready_running_thread), NULL, (void *) st_sched_ready_running, (void *) scheduler_algorithm) != 0) {
        log_error(logger, "Error creating short term scheduler thread");
        return -1;
    }

    if (pthread_create(&(console_thread), NULL, interactive_console, (void *) config) != 0) {
        log_error(logger, "Error creating console thread");
        return -1;
    }

    // wait for threads to finish
    pthread_join(server_thread, NULL);
    pthread_join(console_thread, NULL);
    pthread_join(lt_sched_new_ready_thread, NULL);
    pthread_join(st_sched_ready_running_thread, NULL);

    destroy_all();
    return 0;
}