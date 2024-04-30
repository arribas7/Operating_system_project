#ifndef CONSOLE_H
#define CONSOLE_H

typedef enum {
    CMD_EJECUTAR_SCRIPT,
    CMD_INICIAR_PROCESO,
    CMD_FINALIZAR_PROCESO,
    CMD_DETENER_PLANIFICACION,
    CMD_INICIAR_PLANIFICACION,
    CMD_MULTIPROGRAMACION,
    CMD_PROCESO_ESTADO,
    CMD_EXIT,
    CMD_UNKNOWN
} console_command;

void *interactive_console(void *arg);

#endif
