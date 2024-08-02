#include "console.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <long_term_scheduler.h>
#include <commons/log.h>
#include <state_lists.h>
#include <errno.h>
#include <limits.h>

console_command get_command_type(const char *input) {
    if (strncmp(input, "EJECUTAR_SCRIPT", 15) == 0) return CMD_EJECUTAR_SCRIPT;
    if (strncmp(input, "INICIAR_PROCESO", 15) == 0) return CMD_INICIAR_PROCESO;
    if (strncmp(input, "FINALIZAR_PROCESO", 17) == 0) return CMD_FINALIZAR_PROCESO;
    if (strncmp(input, "DETENER_PLANIFICACION", 21) == 0) return CMD_DETENER_PLANIFICACION;
    if (strncmp(input, "INICIAR_PLANIFICACION", 21) == 0) return CMD_INICIAR_PLANIFICACION;
    if (strncmp(input, "MULTIPROGRAMACION", 17) == 0) return CMD_MULTIPROGRAMACION;
    if (strncmp(input, "PROCESO_ESTADO", 14) == 0) return CMD_PROCESO_ESTADO;
    if (strncmp(input, "EXIT", 4) == 0) return CMD_EXIT;
    if (strncmp(input, "DESBLOQUEAR PROCESO", 19) == 0) return CMD_TEST_UNBLOCK;
    if (strncmp(input, "LOG", 3) == 0) return CMD_TEST_LOG;
    if (strncmp(input, "SALIDA", 6) == 0) return CMD_TEST_EXIT_LOG;
    if (strncmp(input, "TIMEOUT", 7) == 0) return CMD_TEST_TIMEOUT;
    if (strncmp(input, "Q", 1) == 0) return CMD_TEST_PLANI;
    return CMD_UNKNOWN;
}

void handle_script_execution(const char *cmd_args, t_config* config) {
    FILE *file = fopen(cmd_args, "r");
    if (file == NULL) {
        log_error(logger, "Failed to open script file: %s", strerror(errno));
        return;
    }

    log_info(logger, "Executing script from path: %s", cmd_args);

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        char *newline = strchr(line, '\n');
        if (newline) {
            *newline = '\0';
        }

        // Get the command type and arguments
        console_command cmd = get_command_type(line);
        char *cmd_args = strchr(line, ' ');
        if (cmd_args) {
            *cmd_args++ = '\0';
        }

        // Execute the command
        if (execute_command(cmd, cmd_args, config)) {
            break;
        }
    }

    fclose(file);
}

void handle_start_process(const char *cmd_args, t_config* config) {
    char *path = strdup(cmd_args);
    start_process(path, config);
    free(path);
}

void handle_stop_process(const char *cmd_args) {
    char *endptr;
    errno = 0; // To distinguish success/failure after the call
    long val = strtol(cmd_args, &endptr, 10);

    // Check for various possible errors
    if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN)) || (errno != 0 && val == 0)) {
        log_error(logger, "Error parsing arg.");
    }

    if (endptr == cmd_args) {
        log_error(logger, "No digits were found.");
    }

    // If we got here, strtol() successfully parsed a number
    if (*endptr != '\0') { // If there are trailing characters left in the string
        log_error(logger, "Trailing characters after number: %s", endptr);
    }

    int pid = (int)val;
    exit_process_from_pid(pid, INTERRUPTED_BY_USER);
}

void handle_start_scheduler() {
    if(scheduler_paused) {
        scheduler_paused = 0;
        sem_post(&sem_all_scheduler);
        sem_post(&sem_cpu_dispatch);
        sem_post(&sem_unblock);
        log_info(logger, "Scheduler started.");
    } else {
        log_info(logger, "Scheduler is already running.");
    }
}

void handle_stop_scheduler() {
    if(!scheduler_paused){
        sem_wait(&sem_all_scheduler);
        sem_wait(&sem_cpu_dispatch);
        sem_wait(&sem_unblock);
        scheduler_paused = 1;
        log_info(logger, "Scheduler stopped.");
    } else {
        log_info(logger, "Scheduler is already stopped.");
    }
}

void handle_multiprogramming_grade(const char *cmd_args) {
    char *endptr;
    errno = 0;
    long new_grade = strtol(cmd_args, &endptr, 10);

    // Check for various possible errors
    if ((errno == ERANGE && (new_grade == LONG_MAX || new_grade == LONG_MIN)) || (errno != 0 && new_grade == 0)) {
        log_error(logger, "Error parsing new multiprogramming grade.");
        return;
    }

    if (endptr == cmd_args) {
        log_error(logger, "No digits were found.");
        return;
    }

    if (*endptr != '\0') {
        log_error(logger, "Trailing characters after number: %s", endptr);
        return;
    }

    pthread_mutex_lock(&mutex_multiprogramming); // Lock the mutex
    int current_grade = atomic_load(&current_multiprogramming_grade);
    int difference = (int)new_grade - current_grade;

    log_info(logger, "Changing multiprogramming grade from %d to %ld\n", current_grade, new_grade);

    if (difference > 0) {
        // Increase the semaphore value
        for (int i = 0; i < difference; i++) {
            sem_post(&sem_multiprogramming);
        }
    } else if (difference < 0) {
        // Decrease the semaphore value
        for (int i = 0; i < -difference; i++) {
            sem_wait(&sem_multiprogramming);
        }
    }

    atomic_store(&current_multiprogramming_grade, new_grade);
    log_info(logger, "New multiprogramming grade set to %ld\n", new_grade);

    pthread_mutex_unlock(&mutex_multiprogramming); // Unlock the mutex
}

void handle_process_state() {
    log_info(logger, "RUNNING state process:");
    
    pthread_mutex_lock(&mutex_running);
    if(pcb_RUNNING != NULL) {
        log_info(logger, "---pid: %d", pcb_RUNNING->pid);
    } else {
        log_info(logger, "No process running.");
    }
    pthread_mutex_unlock(&mutex_running);

    log_info(logger, "NEW state processes:");
    log_list_contents(logger, list_NEW, mutex_new);

    log_info(logger, "READY state processes:");
    log_list_contents(logger, list_READY, mutex_ready);

    log_info(logger, "BLOCKED state processes:");
    log_list_contents(logger, list_BLOCKED, mutex_blocked);
}

int execute_command(console_command cmd, const char *cmd_args, t_config* config) {
    switch (cmd) {
        case CMD_EJECUTAR_SCRIPT:
            handle_script_execution(cmd_args, config);
            break;
        case CMD_INICIAR_PROCESO:
            handle_start_process(cmd_args, config);
            break;
        case CMD_FINALIZAR_PROCESO:
            handle_stop_process(cmd_args);
            break;
        case CMD_DETENER_PLANIFICACION:
            handle_stop_scheduler();
            break;
        case CMD_INICIAR_PLANIFICACION:
            handle_start_scheduler();
            break;
        case CMD_MULTIPROGRAMACION:
            handle_multiprogramming_grade(cmd_args);
            break;
        case CMD_PROCESO_ESTADO:
            handle_process_state();
            break;
        case CMD_TEST_EXIT_LOG:
            log_list_contents(logger, list_EXIT, mutex_exit);;
            break;
        case CMD_TEST_LOG:
            handle_process_state();
        break;
        case CMD_TEST_TIMEOUT:
            log_list_contents(logger, list_BLOCKED, mutex_blocked);
            log_list_contents(logger, list_READY, mutex_ready);
        break;
        case CMD_TEST_UNBLOCK:
            move_pcb(list_get_first(list_BLOCKED), BLOCKED, READY, list_READY, &mutex_blocked);
            break;
        case CMD_EXIT:
            log_info(logger, "Exiting console.");
            exit(0);
            break;
        case CMD_TEST_PLANI:
            execute_command(CMD_FINALIZAR_PROCESO, "4", config);
            break;
        default:
            log_error(logger, "Unknown command or invalid syntax: %s", cmd_args);
            break;
    }
    return 0;
}

void *interactive_console(void *arg) {
    t_config *config = (t_config *) arg;

    char *line;
    console_command cmd;
    char *cmd_args;

    while (1) {
        line = readline("> ");
        if (line && *line) {
            add_history(line);

            cmd = get_command_type(line);
            cmd_args = strchr(line, ' ');
            if (cmd_args) {
                *cmd_args++ = '\0';
            }

            if (execute_command(cmd, cmd_args, config)) {
                free(line);
                break; // Exit interactive console if CMD_EXIT is encountered
            }
        }
        free(line);
    }
}