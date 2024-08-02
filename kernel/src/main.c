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

t_list* resources_list;
pthread_mutex_t mutex_resources;

t_list *list_NEW;
pthread_mutex_t mutex_new;

t_list *list_READY;
pthread_mutex_t mutex_ready;

t_list *list_BLOCKED;
pthread_mutex_t mutex_blocked;

t_list *list_EXIT;
pthread_mutex_t mutex_exit;

t_pcb *pcb_RUNNING;
pthread_mutex_t mutex_running;

atomic_int pid_count;

sem_t sem_multiprogramming;
pthread_mutex_t mutex_multiprogramming;
sem_t sem_all_scheduler;
sem_t sem_ready_process;
sem_t sem_new_process;
sem_t sem_quantum;
sem_t sem_quantum_finished;
pthread_mutex_t mutex_multiprogramming;
pthread_mutex_t mutex_quantum_interrupted;
sem_t sem_unblock;
sem_t sem_cpu_dispatch;

int scheduler_paused = 0;
atomic_int current_multiprogramming_grade;
char* scheduler_algorithm;
char* puerto_server;
int* socket_server;

t_log *logger;
t_config *config;
int server_fd;

//t_list *client_sockets;
//pthread_mutex_t client_sockets_mutex;

pthread_t server_thread, console_thread, lt_sched_new_ready_thread, st_sched_ready_running_thread;

void destroy_all() {
    sem_destroy(&sem_multiprogramming);
    sem_destroy(&sem_all_scheduler);
    sem_destroy(&sem_ready_process);
    sem_destroy(&sem_quantum);
    sem_destroy(&sem_quantum_finished);
    sem_destroy(&sem_new_process);
    pthread_mutex_destroy(&mutex_multiprogramming);
    pthread_mutex_destroy(&mutex_new);
    pthread_mutex_destroy(&mutex_running);
    pthread_mutex_destroy(&mutex_ready);
    pthread_mutex_destroy(&mutex_exit);
    pthread_mutex_destroy(&mutex_blocked);
    pthread_mutex_destroy(&mutex_quantum_interrupted);
    pthread_mutex_destroy(&mutex_resources);
    state_list_destroy(list_NEW);
    state_list_destroy(list_READY);
    state_list_destroy(list_BLOCKED);
    state_list_destroy(list_EXIT);
    delete_pcb(pcb_RUNNING);
    destroy_resource_list();
    log_destroy(logger);
    config_destroy(config);
    destroy_interface_list(interface_list);
}

void initialize_lists() {
    list_NEW = list_create();
    list_READY = list_create();
    list_BLOCKED = list_create();
    list_EXIT = list_create();
}

void handle_client(void *arg) {
    int cliente_fd = *(int*) arg;
    free(arg);

    if(cliente_fd < 0){
        log_error(logger, "invalid client... closing connection");
        liberar_conexion(cliente_fd);
        exit(0);
    }
    log_info(logger, "New client connected, socket fd: %d", cliente_fd);

    char* name;
    char* mssg;

    while (1) {
        int cod_op = recibir_operacion(cliente_fd);
        t_list *lista = recibir_paquete(cliente_fd);
        switch (cod_op) {
            case IO:
                uint32_t status;
                t_interface* interface = list_to_interface(lista, cliente_fd);                
                name = get_interface_name(interface);

                if(find_interface_by_name(interface_list, name) != NULL) {
                    log_error(logger, "THE INTERFACE %s ALREADY EXISTS", name);
                    delete_interface(interface);
                    status = -1;
                } else {
                    char* type = type_from_list(lista);
                    log_info(logger, "NEW IO CONNECTED: NAME: %s, TYPE: %s", name, type);
                    add_interface_to_list(interface_list, interface);
                    status = 0;
                }

                // Print Message Log and Notify Interface
                mssg = mssg_log(status);
                log_info(logger, "STATUS %s: %s", name, mssg);
                send_confirmation(cliente_fd, &(status));
                break;
            case REPORT:
                t_report* report = list_to_report(lista);
                log_info(logger, "PID: %d", report->pid);
                mssg = mssg_from_report(report);
                log_info(logger, "OPERATION_RESULT: %s", mssg); // 0 - ERROR / 1 - OK
                if(!report->result){
                    exit_process_from_pid(report->pid, ERROR_INTERFACE);
                } else {
                    io_unblock(report->pid);
                }
                delete_report(report);
                break;
            case -1:
                log_info(logger, "Client disconnected. Finishing client connection...");
                delete_interface_from_list(interface_list, name);
                list_destroy_and_destroy_elements(lista, free);
                liberar_conexion(cliente_fd);
                return;
            default:
                log_warning(logger, "Unknown operation.");
                break;
        }
        list_destroy_and_destroy_elements(lista, free);
    }
}

void run_server(void *arg) {
    puerto_server = (char *) arg;

    server_fd = iniciar_servidor(puerto_server);
    log_info(logger, "Server ready to receive clients...");

    while(1) {
        int cliente_fd = esperar_cliente(server_fd);

        pthread_t client_thread;
        socket_server = malloc(sizeof(int));
        *socket_server = cliente_fd;

        if(pthread_create(&(client_thread), NULL, (void*) handle_client, (void*) socket_server) != 0) {
            log_error(logger, "Error creating thread for the client.");
            exit(0);
        }
        pthread_detach(client_thread);
/*
        pthread_mutex_lock(&client_sockets_mutex);
        list_add(client_sockets, socket_server);
        pthread_mutex_unlock(&client_sockets_mutex);*/
    }
}


void cleanup() {
    liberar_conexion(server_fd);
    printf("Socket %d closed\n", server_fd);
    /*pthread_mutex_lock(&client_sockets_mutex);
    void close_socket(void *socket) {
        int client_fd = *(int *)socket;
        liberar_conexion(client_fd);
        printf("Client Socket %d closed\n", client_fd);
        free(socket);
    }
    list_iterate(client_sockets, close_socket);
    list_destroy(client_sockets);
    pthread_mutex_unlock(&client_sockets_mutex);
    pthread_mutex_destroy(&client_sockets_mutex);*/
    destroy_all();
    printf("Clean up done.\n");
}

void handle_graceful_shutdown(int sig) {
    printf("Handling %d signal\n", sig);
    exit(0);
}


int main(int argc, char *argv[]) {
    // Register cleanup function to be called on exit
    atexit(cleanup);
    // Manage signals
    signal(SIGINT, handle_graceful_shutdown);
    signal(SIGTERM, handle_graceful_shutdown);

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

    pthread_mutex_init(&mutex_resources, NULL);
    initialize_resources();

    atomic_init(&pid_count, 0);
    int multiprogramming_grade = config_get_int_value(config, "GRADO_MULTIPROGRAMACION");
    atomic_init(&current_multiprogramming_grade, multiprogramming_grade);
    sem_init(&sem_multiprogramming, 0, multiprogramming_grade);
    sem_init(&sem_all_scheduler, 0, 1);
    sem_init(&sem_ready_process,0, 0);
    sem_init(&sem_quantum,0, 0);
    sem_init(&sem_quantum_finished, 0, 0);
    sem_init(&sem_new_process, 0, 0);
    sem_init(&sem_unblock,0, 1);
    sem_init(&sem_cpu_dispatch,0, 1);
    
    pthread_mutex_init(&mutex_multiprogramming, NULL);
    pthread_mutex_init(&mutex_new, NULL);
    pthread_mutex_init(&mutex_ready, NULL);
    pthread_mutex_init(&mutex_running, NULL);
    pthread_mutex_init(&mutex_blocked, NULL);
    pthread_mutex_init(&mutex_exit, NULL);
    pthread_mutex_init(&mutex_quantum_interrupted, NULL);

    scheduler_algorithm = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
    char *puerto = config_get_string_value(config, "PUERTO_ESCUCHA");

    /*client_sockets = list_create();
    pthread_mutex_init(&client_sockets_mutex, NULL);*/

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
    //destroy_all();
    return 0;
}