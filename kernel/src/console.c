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
    return CMD_UNKNOWN;
}

void handle_script_execution(const char *cmd_args) {
    log_info(logger, "Executing script from path: %s\n", cmd_args);
    // Add the actual script execution logic here
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
    exit_process(pid, INTERRUPTED_BY_USER);
}

void start_scheduler() {
    if(scheduler_paused) {
        scheduler_paused = 0;
        sem_post(&sem_all_scheduler);
        log_info(logger, "Scheduler started.");
    } else {
        log_info(logger, "Scheduler is already running.");
    }
}

void stop_scheduler() {
    if(!scheduler_paused){
        sem_wait(&sem_all_scheduler);
        scheduler_paused = 1;
        log_info(logger, "Scheduler stopped.");
    } else {
        log_info(logger, "Scheduler is already stopped.");
    }
}

void multiprogramming_grade(const char *cmd_args) {
    log_info(logger, "New multiprogramming grade: %s\n", cmd_args);
    // You need to count running_pcb + length Ready + actual sem_val
    // use mutex.
    // sem_post the difference.
}

void handle_process_state() {
    log_info(logger, "Listing processes by state.\n");
    // Add logic to display process states here
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
                *cmd_args++ = '\0'; // Split the command from the arguments
            }

            switch (cmd) {
                case CMD_EJECUTAR_SCRIPT:
                    handle_script_execution(cmd_args);
                    break;
                case CMD_INICIAR_PROCESO:
                    handle_start_process(cmd_args, config);
                    break;
                case CMD_FINALIZAR_PROCESO:
                    handle_stop_process(cmd_args);
                    break;
                case CMD_DETENER_PLANIFICACION:
                    stop_scheduler();
                    break;
                case CMD_INICIAR_PLANIFICACION:
                    start_scheduler();
                    break;
                case CMD_MULTIPROGRAMACION:
                    multiprogramming_grade(cmd_args);
                    break;
                case CMD_PROCESO_ESTADO:
                    handle_process_state();
                    break;
                case CMD_EXIT:
                    free(line);
                    return NULL;
                default:
                    log_error(logger, "Unknown command or invalid syntax: %s", line);
                    break;
            }
        }
        free(line);
    }
}