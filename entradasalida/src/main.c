#include <stdlib.h>
#include <stdio.h>
#include <libgen.h>
#include <commons/txt.h>
#include <commons/log.h>
#include <commons/config.h>
#include <utils/client.h>
#include <utils/server.h>
#include <utils/cpu.h>
#include <utils/inout.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <dialfs.h>
#include "generic_st.h"

// GLOBAL VARIABLES

t_instruction_queue* i_queue;
sem_t sem_instruction;
sem_t use_time;
t_log* logger;
t_config* config;
char* io_name;
char* type;



int create_connection(t_config* config, char* c_ip, char* c_puerto) 
{
    char* ip = config_get_string_value(config, c_ip);
    char* port = config_get_string_value(config, c_puerto);
    return crear_conexion(ip, port); 
}

void respond_to_requests(void* arg) 
{
    int connection = atoi((char *) arg);
    while(1) 
    {
        int cod_op = recibir_operacion(connection);
        t_instruction* instruction = receive_instruction_IO(connection);

        // PRUEBA - LLEGADA INSTRUCTION
        /*
        log_info(logger, "NOMBRE: %s", instruction->name);
        log_info(logger, "PID: %d", instruction->pid);
        log_info(logger, "PATH: %s", instruction->path);
        */

        if(is_valid_instruction(cod_op, config)) 
        {
            add_instruction_to_queue(i_queue, instruction);
            sem_post(&(sem_instruction));
        } else {
            send_report(instruction, false, connection);
            return;
        }
    }
}

void execute_instruction(void* arg) 
{
    int connection = atoi((char *) arg);
    while(1) 
    {
        // Bloquea el hilo para ejecutar una instrucción
        sem_wait(&(use_time));
        // Toma la siguiente instruccion y resta una
        sem_wait(&(sem_instruction));
        t_instruction* instruction = get_next_instruction(i_queue);
        
        char* code = type_from_code(instruction->code);
        log_info(logger, "PID: %i - Operación a realizar: %s", instruction->pid, code);
        uint32_t response;

        switch(instruction->code) 
        {
            case IO_GEN_SLEEP:
                int result = interface_wait(instruction, config);
                response = (result == 0) ? 1 : 0;
                log_info(logger, "INSTRUCTION_COMPLETE");
                send_report(instruction, (bool) response, connection);
                break;
            case IO_STDIN_READ:
                int stdin_ms = create_connection(config, "IP_MEMORIA", "PUERTO_MEMORIA");
                char* w_word = write_console();
                write_in_memory(instruction, w_word, stdin_ms);
                break;
            case IO_STDOUT_WRITE:
                int stdout_ms = create_connection(config, "IP_MEMORIA", "PUERTO_MEMORIA");
                char* r_word = read_from_memory(instruction, stdout_ms);
                log_info(logger, "PALABRA LEIDA: %s", r_word);
                break;
            case IO_FS_CREATE:
                io_fs_create(config_get_string_value(config, "PATH_BASE_DIALFS"), instruction->path, instruction->pid, logger);
                break;
            case IO_FS_DELETE:
                io_fs_delete(config_get_string_value(config, "PATH_BASE_DIALFS"), instruction->path, instruction->pid, logger);
                break;
            case IO_FS_READ:
                io_fs_read(config_get_string_value(config, "PATH_BASE_DIALFS"), "phys_addr", NULL, 0, instruction->pid, logger);
                // TODO: add when instruction changes 
                // io_fs_read(config_get_string_value(config, "PATH_BASE_DIALFS"), instruction->phys_addr, instruction->buffer, instruction->size, instruction->pid, logger);
                break;
            case IO_FS_TRUNCATE:
                io_fs_truncate(config_get_string_value(config, "PATH_BASE_DIALFS"), instruction->path, 0, instruction->pid, logger);
                // TODO: add when instruction changes 
                // io_fs_truncate(config_get_string_value(config, "PATH_BASE_DIALFS"), instruction->path, instruction->size, instruction->pid, logger);
                break;
            case IO_FS_WRITE:
                io_fs_write(config_get_string_value(config, "PATH_BASE_DIALFS"), "phys_addr", NULL, 0, instruction->pid, logger);
                // TODO: add when instruction changes 
                // io_fs_write(config_get_string_value(config, "PATH_BASE_DIALFS"), instruction->phys_addr, instruction->buffer, instruction->size, instruction->pid, logger);
                break;
            default:
                log_error(logger, "INVALID_INSTRUCTION");
                send_report(instruction, false, connection);
                break;
        }
        sem_post(&(use_time));
    }
}

// Function to extract the name from the config file path
char* extract_name_from_path(const char* path) {
    char* path_dup = strdup(path);
    char* base = basename(path_dup);
    
    char* dot = strrchr(base, '.');
    if (dot != NULL) {
        *dot = '\0';
    }
    
    char* name = strdup(base);
    free(path_dup);
    return name;
}

int main(int argc, char* argv[]) {
    // INITIALIZE VARIABLES    
    config = config_create(argv[1]);
    io_name = extract_name_from_path(argv[1]);

    // Create the log file name
    size_t log_name_length = strlen(io_name) + strlen(".log") + 1;
    char* log_name = malloc(log_name_length);
    snprintf(log_name, log_name_length, "%s.log", io_name);
    
    // Create the logger
    logger = log_create(log_name, io_name, true, LOG_LEVEL_INFO);

    type = type_from_config(config);
    t_info* info = create_info(io_name, type);
    
    free(log_name);

    // Initialize DialFS if needed
    if (strcmp(type, "dialfs") == 0) {
        int block_size = config_get_int_value(config, "BLOCK_SIZE");
        int block_count = config_get_int_value(config, "BLOCK_COUNT");
        const char *path_base_dialfs = config_get_string_value(config, "PATH_BASE_DIALFS");
        initialize_dialfs(path_base_dialfs, block_size, block_count);
    }
    
    // CREATE KERNEL CONNECTION
    
    int kernel_socket = create_connection(config, "IP_KERNEL", "PUERTO_KERNEL");
    log_info(logger, "La conexion es: %d", kernel_socket);

    // CONNECTION TO STRING (TO THREADS)

    char* str_conn = int_to_string(kernel_socket);

    // SEND INFO TO KERNEL FROM CONNECTION

    t_paquete* package = info_to_package(info);
    enviar_paquete(package, kernel_socket);
    eliminar_paquete(package);
    delete_info(info);

    uint32_t status = 0;
    receive_confirmation(kernel_socket, status);

    if (status) 
    {
        log_info(logger, "ERROR. FINISHING ...");
        return -1;
    } else {
        log_info(logger, "CONNECTION SUCESSFULL");
        i_queue = create_instruction_queue();
        sem_init(&(sem_instruction), 0, 0);
        sem_init(&(use_time), 0, 1);
    }
    
    // CREATE THREADS

    pthread_t connection_thread, instruction_manager_thread;

    if(pthread_create(&(connection_thread), NULL, (void *) respond_to_requests, str_conn) != 0) 
    {
        log_info(logger, "Error creating connection thread");
        return -1;
    }

    if(pthread_create(&(instruction_manager_thread), NULL, (void *) execute_instruction, str_conn) != 0) 
    {
        log_info(logger, "Error creating instruction manager thread");
        return -1;
    }

    pthread_join(connection_thread, NULL);
    pthread_join(instruction_manager_thread, NULL);
    free(str_conn);
    free(io_name);
    liberar_conexion(kernel_socket);

    return 0;
}