#ifndef CONSOLE_H
#define CONSOLE_H
#include<commons/log.h>
#include <state_lists.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <pthread.h>

extern t_log* logger;

extern t_list *list_NEW;
extern pthread_mutex_t mutex_new;

extern sem_t sem_all_scheduler;
extern pthread_mutex_t mutex_multiprogramming;
extern int scheduler_paused;
extern atomic_int current_multiprogramming_grade;
extern sem_t sem_cpu_dispatch;
extern sem_t sem_unblock;

typedef enum {
    CMD_EJECUTAR_SCRIPT,
    CMD_INICIAR_PROCESO,
    CMD_FINALIZAR_PROCESO,
    CMD_DETENER_PLANIFICACION,
    CMD_INICIAR_PLANIFICACION,
    CMD_MULTIPROGRAMACION,
    CMD_PROCESO_ESTADO,
    CMD_EXIT,
    CMD_UNKNOWN,
    CMD_TEST_UNBLOCK,
    CMD_TEST_LOG,
    CMD_TEST_TIMEOUT,
    CMD_TEST_PLANI,
    CMD_TEST_EXIT_LOG
} console_command;

void *interactive_console(void *arg);

#endif
