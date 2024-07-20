#include <stdlib.h>
#include <stdio.h>
#include <commons/txt.h>
#include <commons/log.h>
#include <commons/config.h>
#include <utils/client.h>
#include <utils/server.h>
#include <utils/cpu.h>
#include <utils/inout.h>
#include <pthread.h>
#include <semaphore.h>
#include <libgen.h>
#include <string.h>
#include <dialfs.h>
#include "generic_st.h"
#include "dialfs.h"

// GLOBAL VARIABLES

t_instruction_queue* i_queue;
sem_t sem_instruction;
sem_t use_time;
t_log* logger;
t_config* config;
char* io_name;
char* type;
int k_socket;

// FUNCTIONS

int create_connection(t_config* config, char* c_ip, char* c_puerto) 
{
    char* ip = config_get_string_value(config, c_ip);
    char* port = config_get_string_value(config, c_puerto);
    return crear_conexion(ip, port); 
}

void respond_to_requests() 
{
    while(1) 
    {
        int cod_op = recibir_operacion(k_socket);
        t_instruction* instruction = receive_instruction_IO(k_socket);

        if(is_valid_instruction(cod_op, config)) 
        {
            add_instruction_to_queue(i_queue, instruction);
            sem_post(&(sem_instruction));
        } else {
            send_report(instruction, false, k_socket);
            return;
        }
    }
}

void generic_interface_manager() 
{
    while(1) 
    {
        sem_wait(&(use_time)); // BLOQUEO PARA EJECUTAR
        sem_wait(&(sem_instruction)); // TOMO LA SIG. INSTRUCCION Y RESTO UNA
        t_instruction* instruction = get_next_instruction(i_queue);
        
        generate_log_from_instruction(instruction);
        bool response;

        switch(instruction->code) 
        {
            case IO_GEN_SLEEP:
                int result = wait_time_units(instruction->time, config);
                response = (result == 0) ? 1 : 0;
                log_info(logger, "INSTRUCTION_COMPLETE");
                send_report(instruction, response, k_socket);
                break;
            default:
                log_error(logger, "INVALID_INSTRUCTION");
                send_report(instruction, false, k_socket);
                break;
        }
        sem_post(&(use_time));
    }
}

void std_fs_manager(void* arg) 
{
    int connection = atoi((char *) arg);
    while(1) 
    {
        sem_wait(&(use_time)); // BLOQUEO PARA EJECUTAR
        sem_wait(&(sem_instruction)); // TOMO LA SIG. INSTRUCCION Y RESTO UNA
        t_instruction* instruction = get_next_instruction(i_queue);
        
        generate_log_from_instruction(instruction);
        bool response;

        switch(instruction->code) 
        {
            case IO_STDIN_READ:
                send_write_request(instruction, connection);
                break;
            case IO_STDOUT_WRITE:
                if (wait_time_units(1, config) == 0) 
                {
                    send_read_request(instruction, connection);
                    char* word = receive_word(connection);
                    log_info(logger, "%s", word);
                    free(word);
                } else {
                    log_error(logger, "ERROR HAS OCURRED");
                }
                break;
            case IO_FS_CREATE:
                char* c_path = path_from_config(config);
                fs_create(c_path, instruction->f_name, instruction->pid);
                break;
            case IO_FS_DELETE:
                char* d_path = path_from_config(config);
                fs_delete(d_path, instruction->f_name, instruction->pid);
                break;
            case IO_FS_READ:
                fs_read(config_get_string_value(config, "PATH_BASE_DIALFS"), "phys_addr", NULL, 0, instruction->pid);
                break;
            case IO_FS_TRUNCATE:
                fs_truncate(config_get_string_value(config, "PATH_BASE_DIALFS"), "", 0, instruction->pid);
                break;
            case IO_FS_WRITE:
                fs_write(config_get_string_value(config, "PATH_BASE_DIALFS"), "phys_addr", NULL, 0, instruction->pid);
                break;
            default:
                log_error(logger, "INVALID_INSTRUCTION");
                send_report(instruction, false, connection);
                break;
        }
        sem_post(&(use_time));
    }
}

/*

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
        
        generate_log_from_instruction(instruction);
        bool response;

        switch(instruction->code) 
        {
            case IO_GEN_SLEEP:
                int result = wait_time_units(instruction->time, config);
                response = (result == 0) ? 1 : 0;
                log_info(logger, "INSTRUCTION_COMPLETE");
                send_report(instruction, response, connection);
                break;
            case IO_STDIN_READ:
                int in_socket = create_connection(config, "IP_MEMORIA", "PUERTO_MEMORIA");
                send_write_request(instruction, in_socket);
                break;
            case IO_STDOUT_WRITE:
                int out_socket = create_connection(config, "IP_MEMORIA", "PUERTO_MEMORIA");
                if (wait_time_units(1, config) == 0) 
                {
                    send_read_request(instruction, out_socket);
                    char* word = receive_word(out_socket);
                    log_info(logger, "%s", word);
                    free(word);
                } else {
                    log_error(logger, "ERROR HAS OCURRED");
                }
                break;
            case IO_FS_CREATE:
                char* c_path = path_from_config(config);
                fs_create(c_path, instruction->f_name, instruction->pid);
                break;
            case IO_FS_DELETE:
                char* d_path = path_from_config(config);
                fs_delete(d_path, instruction->f_name, instruction->pid);
                break;
            case IO_FS_READ:
                fs_read(config_get_string_value(config, "PATH_BASE_DIALFS"), "phys_addr", NULL, 0, instruction->pid);
                break;
            case IO_FS_TRUNCATE:
                fs_truncate(config_get_string_value(config, "PATH_BASE_DIALFS"), "", 0, instruction->pid);
                break;
            case IO_FS_WRITE:
                fs_write(config_get_string_value(config, "PATH_BASE_DIALFS"), "phys_addr", NULL, 0, instruction->pid);
                break;
            default:
                log_error(logger, "INVALID_INSTRUCTION");
                send_report(instruction, false, connection);
                break;
        }
        sem_post(&(use_time));
    }
}

*/

// EXTRACT THE NAME FROM THE CONFIG FILE PATH
char* extract_name_from_path(const char* path) 
{
    char* path_dup = strdup(path);
    char* base = basename(path_dup);

    char* dot = strrchr(base, '.');
    if (dot != NULL) 
    {
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

    // CREATE THE LOG FILE NAME
    /*char* log_name = create_log_file_name(io_name);*/

    size_t log_name_length = strlen(io_name) + strlen(".log") + 1;
    char* log_name = malloc(log_name_length);
    snprintf(log_name, log_name_length, "%s.log", io_name);

    // CREATE THE LOGGER
    logger = log_create(log_name, io_name, true, LOG_LEVEL_DEBUG);
    type = type_from_config(config);
    t_info* info = create_info(io_name, type);

    free(log_name);

    // INITIALIZE DIALFS IF NEEDED
    if(strcmp(type, "DIALFS") == 0) 
    {
        int block_size = config_get_int_value(config, "BLOCK_SIZE");
        int block_count = config_get_int_value(config, "BLOCK_COUNT");
        const char *path_base_dialfs = config_get_string_value(config, "PATH_BASE_DIALFS");
        initialize_dialfs(path_base_dialfs, block_size, block_count);
    }
    
    // CREATE KERNEL CONNECTION
    
    k_socket = create_connection(config, "IP_KERNEL", "PUERTO_KERNEL");
    log_info(logger, "La conexion es: %d", k_socket);

    // CONNECTION TO STRING (TO THREADS)

    char* s_k_socket = int_to_string(k_socket);

    // SEND INFO TO KERNEL FROM CONNECTION

    send_info(info, k_socket);

    uint32_t error_exists = 0;
    receive_confirmation(k_socket, error_exists);

    if(error_exists == 0) 
    {
        log_info(logger, "CONNECTION SUCESSFULL. I/O INFO - NAME: %s - TYPE: %s", io_name, type);
        i_queue = create_instruction_queue();
        sem_init(&(sem_instruction), 0, 0);
        sem_init(&(use_time), 0, 1);
    } else {
        log_info(logger, "ERROR. FINISHING ...");
        return -1;
    }
    
    // CREATE THREADS

    pthread_t connection_thread, manager_thread;

    if(pthread_create(&(connection_thread), NULL, (void *) respond_to_requests, s_k_socket) != 0) 
    {
        log_info(logger, "Error creating connection thread");
        return -1;
    }

    if(strcmp(type, "GENERICA") == 0) 
    {
        if(pthread_create(&(manager_thread), NULL, (void *) generic_interface_manager, NULL) != 0) 
        {
            log_error(logger, "Error creating generic manager thread");
            return -1;
        }    
    } else {
        if(strcmp(type, "STDIN") == 0 || strcmp(type, "STDOUT") == 0 || strcmp(type, "DIALFS") == 0) 
        {
            int m_socket = create_connection(config, "IP_MEMORIA", "PUERTO_MEMORIA");
            char* s_m_socket = int_to_string(m_socket);
            if(pthread_create(&(manager_thread), NULL, (void *) std_fs_manager, (void *) s_m_socket) != 0) 
            {
                log_error(logger, "Error creating STD - FS manager thread");
                return -1;
            }
            free(s_m_socket);
        } else {
            log_error(logger, "Error creating manager thread");
            return -1;
        }
    }

    pthread_join(connection_thread, NULL);
    pthread_join(manager_thread, NULL);
    free(s_k_socket);
    free(io_name);
    liberar_conexion(k_socket);

    return 0;
}