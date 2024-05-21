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
extern t_list *list_NEW;
extern t_list *list_READY;
extern t_list *list_RUNNING;
extern t_list *list_BLOCKED;
extern t_list *list_EXIT;

t_log *logger;
t_config *config;

void clean(t_config *config);

int run_server();

void iterator(char *value);

void* dispatch(void* arg);

void *create_process(void *arg);

void *interactive_console(void *arg);

int main(int argc, char *argv[]) {
    /* ---------------- Initial Setup ---------------- */
    t_config *config;
    logger = log_create("kernel.log", "kernel", true, LOG_LEVEL_INFO);
    if (logger == NULL) {
        return -1;
    }

    /* ---------------- Lists Testing (Delete Later) ---------------- */
    initialize_lists();

    t_pcb *testpcb = new_pcb(1);
    t_pcb *testpcb2 = new_pcb(2);
    t_pcb *testpcb3 = new_pcb(3);

    list_push(list_NEW, &testpcb);
    list_push(list_NEW, &testpcb2);
    list_push(list_NEW, &testpcb3);

    t_pcb *testpcb4 = list_pop(list_NEW);

    log_list_contents(logger, list_NEW);
    log_info(logger, "Popped element pid: %u", testpcb4->pid);

    bool has_pid_1 = list_has_pid(list_NEW, 1);
    //0 For False, 1 For True, apparently. Trust me I've looked it up.
    log_info(logger, "The list new having a pid==1 is %d. (true being 1 and false being 0)", has_pid_1);

    int index_address_for_1 = list_pid_element_index(list_NEW, 1);
    log_info(logger, "The index of pid==1 is %d", index_address_for_1);
    /* ---------------- End of Lists Testing (Delete Later) ---------------- */
    

    config = config_create("kernel.config");
    if (config == NULL) {
        return -1;
    }

    pthread_t server_thread, console_thread;

    char *puerto = config_get_string_value(config, "PUERTO_ESCUCHA");
    // TODO: Use a new log with other name for each thread?
    if (pthread_create(&server_thread, NULL, (void*) run_server, puerto) != 0) {
        log_error(logger, "Error creating server thread");
        return -1;
    }

    if (pthread_create(&console_thread, NULL, (void*) interactive_console, config) != 0) {
        log_error(logger, "Error creating console thread");
        return -1;
    }

    pthread_join(server_thread, NULL);
    pthread_join(console_thread, NULL);

    clean(config);
    return 0;
}

void clean(t_config *config) {
    log_destroy(logger);
    config_destroy(config);
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

    t_pcb *pcb = new_pcb(22);
    t_buffer *buffer = malloc(sizeof(t_buffer));
    serialize_pcb(pcb, buffer);

    t_paquete *paquete = crear_paquete(DISPATCH);
    agregar_a_paquete(paquete, buffer->stream, buffer->size);


    pcb = new_pcb(9);
    buffer = malloc(sizeof(t_buffer));
    serialize_pcb(pcb, buffer);
    agregar_a_paquete(paquete, buffer->stream, buffer->size);

    enviar_paquete(paquete, conexion_cpu);
    eliminar_paquete(paquete);
    delete_pcb(pcb);

    response_code code = esperar_respuesta(conexion_cpu);
    log_info(logger, "Response code: %d", code);

    liberar_conexion(conexion_cpu);
    log_debug(logger, "CPU connection released");
    return NULL;
}