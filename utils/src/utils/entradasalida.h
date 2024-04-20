
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
