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
        sem_wait(&(use_time));
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
void std_in_manager(void* arg) 
{
    int mem_connection = atoi((char *) arg);
    while(1) 
    {
        sem_wait(&(use_time));
        sem_wait(&(sem_instruction));
        t_instruction* instruction = get_next_instruction(i_queue);
        
        generate_log_from_instruction(instruction);
        bool response;
        int result = 0;

        switch(instruction->code) 
        {
            case IO_STDIN_READ:
                uint32_t status = 0;
                send_write_request(instruction, mem_connection);
                receive_confirmation(mem_connection, &(status));
                if(status == 0) 
                {
                    log_error(logger, "AN ERROR HAS OCURRED");
                    send_report(instruction, false, k_socket);
                } else {
                    log_info(logger, "INSTRUCTION COMPLETE");
                    send_report(instruction, true, k_socket);
                }
                break;
            default:
                log_error(logger, "INVALID_INSTRUCTION");
                send_report(instruction, false, k_socket);
                break;
        }
        sem_post(&(use_time));
    }
}

void dialfs_manager(void* arg) 
{
    int mem_connection = atoi((char *) arg);
    while(1) 
    {
        sem_wait(&(use_time));
        sem_wait(&(sem_instruction));
        t_instruction* instruction = get_next_instruction(i_queue);
        
        generate_log_from_instruction(instruction);
        bool response;
        int result = 0;

        switch(instruction->code) 
        {
            case IO_FS_CREATE:
                wait_time_units(1, config);
                fs_create(instruction->f_name, instruction->pid);
                send_report(instruction, true, k_socket);
                break;
            case IO_FS_DELETE:
                wait_time_units(1, config);
                fs_delete(instruction->f_name, instruction->pid);
                send_report(instruction, true, k_socket);
                break;
            case IO_FS_READ:
                wait_time_units(1, config);
                result = fs_read(instruction->f_name, instruction->physical_address, instruction->size, instruction->f_pointer, instruction->pid, mem_connection);
                send_report(instruction, result == 0, k_socket);
                break;
            case IO_FS_TRUNCATE:
                wait_time_units(1, config);
                fs_truncate(instruction->f_name, instruction->size, instruction->pid);
                send_report(instruction, true, k_socket);
                break;
            case IO_FS_WRITE:
                wait_time_units(1, config);
                result = fs_write(instruction->f_name, instruction->physical_address, instruction->size, instruction->f_pointer, instruction->pid, mem_connection);
                send_report(instruction, result == 0, k_socket);
                break;
            default:
                log_error(logger, "INVALID_INSTRUCTION");
                send_report(instruction, false, k_socket);
                break;
        }
        sem_post(&(use_time));
    }
}

void std_out_manager(void* arg) 
{
    int mem_connection = atoi((char *) arg);
    while(1) 
    {
        sem_wait(&(use_time));
        sem_wait(&(sem_instruction));
        t_instruction* instruction = get_next_instruction(i_queue);
        
        generate_log_from_instruction(instruction);
        bool response;
        int result = 0;

        switch(instruction->code) 
        {
            case IO_STDOUT_WRITE:
                if (wait_time_units(1, config) == 0) 
                {
                    send_read_request(instruction, mem_connection);
                    char* word = receive_word(mem_connection);
                    if(strlen(word) == 0) 
                    {
                        log_error(logger, "ERROR READING FROM MEMORY");
                        response = false;
                    } else {
                        txt_write_in_stdout(word);
                        response = true;
                    }
                    send_report(instruction, response, k_socket);
                    free(word);
                } else {
                    log_error(logger, "ERROR HAS OCURRED");
                    send_report(instruction, response, k_socket);
                }
                break;
            default:
                log_error(logger, "INVALID_INSTRUCTION");
                send_report(instruction, false, k_socket);
                break;
        }
        sem_post(&(use_time));
    }
}

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

void test_dialfs() {
    // Test file creation
    /*
    fs_create("salida.txt", 1);
    fs_create("cronologico.txt", 2);
    debug_print_bitmap();
    fs_truncate("salida.txt", 80, 1);
    fs_truncate("cronologico.txt", 80, 2);
    debug_print_bitmap();

    fs_write("salida.txt", 0, 69, 0, 1, -1); // Fallout 1 Fallout 2 Fallout 3 Fallout: New Vegas Fallout 4 Fallout 76
    fs_read("salida.txt", 0, 69, 0, 1, -1); // READ y escribir en memoria -> Fallout 1 Fallout 2 Fallout 3 Fallout: New Vegas Fallout 4 Fallout 76

    //fs_create("f2.txt", 2);
    //fs_truncate("f1.txt", 16 * 1, 1);
    //debug_print_bitmap();
    //fs_truncate("f2.txt", 32, 2); // acá lo hace mal
    //compact_dialfs();
    //debug_print_bitmap();

    // Test file writing
    //fs_write(100, 16, 0, 1);  // Write 16 bytes starting at block 0 for file with PID 1
    //fs_write(116, 16, 32, 2); // Write 16 bytes starting at block 16 for file with PID 2

    // Test file reading
    //fs_read(100, 16, 0, 1);
    //fs_read(116, 16, 16, 2);

    // Test file deletion
    fs_delete("salida.txt", 1);
    fs_delete("cronologico.txt", 2);

    // Test compactation
    //compact_dialfs(1);
    */
}

int main(int argc, char* argv[]) {
    
    // INITIALIZE VARIABLES
    config = config_create(argv[1]);
    io_name = extract_name_from_path(argv[1]);

    // CREATE THE LOG FILE NAME

    size_t log_name_length = strlen(io_name) + strlen(".log") + 1;
    char* log_name = malloc(log_name_length);
    snprintf(log_name, log_name_length, "%s.log", io_name);

    // CREATE THE LOGGER
    logger = log_create(log_name, io_name, true, LOG_LEVEL_DEBUG);
    type = type_from_config(config);
    t_info* info = create_info(io_name, type);

    free(log_name);

    // CREATE KERNEL CONNECTION
    
    k_socket = create_connection(config, "IP_KERNEL", "PUERTO_KERNEL");
    log_info(logger, "La conexion es: %d", k_socket);

    // CONNECTION TO STRING (TO THREADS)

    char* s_k_socket = int_to_string(k_socket);

    // SEND INFO TO KERNEL FROM CONNECTION

    send_info(info, k_socket);

    uint32_t error_exists = 0;
    receive_confirmation(k_socket, &(error_exists));

    if(error_exists == 0) 
    {
        log_info(logger, "CONNECTION SUCESSFULL. I/O INFO - NAME: %s - TYPE: %s", io_name, type);
        i_queue = create_instruction_queue();
        sem_init(&(sem_instruction), 0, 0);
        sem_init(&(use_time), 0, 1);
    } else {
        log_info(logger, "ERROR. FINISHING I/O INSTANCE...");
        return -1;
    }
    
    // CREATE THREADS

    pthread_t connection_thread, manager_thread;

    if(pthread_create(&(connection_thread), NULL, (void *) respond_to_requests, s_k_socket) != 0) 
    {
        log_info(logger, "Error creating connection thread");
        return -1;
    }

    if(strcmp(type, "GENERICA") == 0) {
        if(pthread_create(&(manager_thread), NULL, (void *) generic_interface_manager, NULL) != 0) 
        {
            log_error(logger, "Error creating generic manager thread");
            return -1;
        }    
    } else if(strcmp(type, "STDIN") == 0 || strcmp(type, "STDOUT") == 0 || strcmp(type, "DIALFS") == 0) {
        int m_socket = create_connection(config, "IP_MEMORIA", "PUERTO_MEMORIA");
        char* s_m_socket = int_to_string(m_socket);

        if(strcmp(type, "STDIN") == 0){
                if(pthread_create(&(manager_thread), NULL, (void *) std_in_manager, (void *) s_m_socket) != 0) 
            {
                log_error(logger, "Error creating STDIN manager thread");
                return -1;
            }
        } else if(strcmp(type, "STDOUT") == 0){
                if(pthread_create(&(manager_thread), NULL, (void *) std_out_manager, (void *) s_m_socket) != 0) 
            {
                log_error(logger, "Error creating STDOUT manager thread");
                return -1;
            }
        } else if(strcmp(type, "DIALFS") == 0){
            int block_size = config_get_int_value(config, "BLOCK_SIZE");
            int block_count = config_get_int_value(config, "BLOCK_COUNT");
            const char *path_base_dialfs = config_get_string_value(config, "PATH_BASE_DIALFS");
            initialize_dialfs(path_base_dialfs, block_size, block_count);
            
            if(pthread_create(&(manager_thread), NULL, (void *) dialfs_manager, (void *) s_m_socket) != 0) 
            {
                log_error(logger, "Error creating DIALFS manager thread");
                return -1;
            }
        }
    } else {
        log_error(logger, "Error creating manager thread");
        return -1;
    }

    pthread_join(connection_thread, NULL);
    pthread_join(manager_thread, NULL);
    free(s_k_socket);
    free(io_name);
    liberar_conexion(k_socket);

    return 0;
}