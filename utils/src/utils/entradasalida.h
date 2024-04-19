
#ifndef UTILS_ENTRADASALIDA_H_
#define UTILS_ENTRADASALIDA_H_

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <commons/log.h>
#include "client.h"

// ----- Instrucciones -----

typedef enum 
{
    IO_GEN_SLEEP, // Interfaz generica
    IO_STDIN_READ, // Interfaz STDIN
    IO_STDOUT_WRITE, // Interfaz STDOUT
    IO_FS_CREATE, // Interfaz dialFS (todas las siguientes)
    IO_FS_DELETE,
    IO_FS_TRUNCATE,
    IO_FS_READ,
    IO_FS_WRITE
} instruction_code;

// ----- Codigos de operacion -----

/* NECESARIO: Incluir estos en un archivo comun con los demas codigos de operacion
typedef enum 
{
    CREATE,
    DELETE,
    TRUNCATE,
    READ,
    WRITE
} op_code;
*/

// ----- Estructuras -----

typedef struct 
{
    char* nombre;
    t_config* config;
} t_interfaz;

// ----- Declaraciones -----

t_interfaz* crear_interfaz(char* nombre, t_config* config);
void iterator(char* value);

#endif
