#include "console.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <process.h>
#include <commons/log.h>

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

void handle_script_execution(const char *args) {
    log_info(logger, "Executing script from path: %s\n", args);
    // Add the actual script execution logic here
}

void handle_start_process(const char *args) {
    log_info(logger, "Se crea el proceso <%s> en NEW: %s\n", args);
    start_process(args);
}

void handle_stop_process(const char *args) {
    log_info(logger, "Stopping process with PID: %s\n", args);
    // Add the process stopping logic here
}

void start_scheduler() {
    log_info(logger, "Starting scheduler.\n");
    // Add the scheduler start logic here
}

void stop_scheduler() {
    log_info(logger, "Stopping scheduler.\n");
    // Add the scheduler stop logic here
}

void multiprogramming_grade(const char *args) {
    log_info(logger, "New multiprogramming grade: %s\n", args);
    // Add the logic to adjust multiprogramming grade here
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
                    process_args *proc_args = malloc(sizeof(process_args));
                    proc_args->pid = strdup(cmd_args);
                    proc_args->config = config;
                    handle_start_process(proc_args);
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
                    printf("Unknown command or invalid syntax: %s\n", line);
                    break;
            }
        }
        free(line);
    }
}