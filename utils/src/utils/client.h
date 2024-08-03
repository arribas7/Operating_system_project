#ifndef UTILS_CLIENT_H_
#define UTILS_CLIENT_H_

#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>
#include<commons/string.h>
#include<commons/log.h>
#include <commons/config.h>

typedef enum {
    // generic
    OK,
    NOT_FOUND,
    NOT_SUPPORTED,
    GENERAL_ERROR,
    MENSAJE,
    PAQUETE,

    // Kernel -> MEMORY
    CREATE_PROCESS,
    FINISH_PROCESS,
    OUT_OF_MEMORY,

    // Kernel -> CPU
    DISPATCH,
    INTERRUPT_BY_USER,
    INTERRUPT_TIMEOUT, // Quantum timeout

    // CPU -> MEMORY
    PC,
    WRITE,
    TLB_MISS,
    INSTRUCTION,
    TAM_PAG,
    REG_REQUEST,
    RESIZE,
    COPY_STRING,
    PAGE_REQUEST,

    // CPU -> KERNEL -> IO -> MEMORY
    IO_GEN_SLEEP,
    IO_STDIN_READ,
    IO_STDOUT_WRITE,
    IO_FS_CREATE,
    IO_FS_DELETE,
    IO_FS_TRUNCATE,
    IO_FS_WRITE,
    IO_FS_READ,
    RELEASE,
    WAIT,
    SIGNAL,

    // IO -> KERNEL
    // KERNEL -> IO
    // IO -> MEMORY
    IO,
    REPORT,
    W_REQ,
    R_REQ,
} op_code;

typedef struct {
    u_int32_t size;
    u_int32_t offset;
    void *stream;
} t_buffer;

typedef struct {
    op_code codigo_operacion;
    t_buffer *buffer;
} t_paquete;

/**
* @fn    crear_conexion
* @brief Crea una conexión hacia la ip y puerto indicados
*/
int crear_conexion(char *ip, char *puerto);

t_paquete *crear_paquete(op_code op);

void agregar_a_paquete(t_paquete *paquete, void *valor, int tamanio);

void enviar_paquete(t_paquete *paquete, int socket_cliente);

void liberar_conexion(int socket_cliente);

void eliminar_paquete(t_paquete *paquete);

int conexion_by_config(t_config *config, char *ip_config, char *puerto_config);

void enviar_mensaje(char *mensaje, int socket_cliente);

void *serializar_paquete(t_paquete *paquete, int bytes);

void crear_buffer(t_paquete *paquete);

void enviar_respuesta(int socket_cliente, op_code code);

#endif
