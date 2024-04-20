
#include "entradasalida.h"
#include <stdio.h>
#include <stdlib.h>
#include "client.h"
#include "server.h"

// ----- Definiciones -----

t_interfaz* crear_interfaz(char* nombre, t_config* config) 
{
    t_interfaz* nueva_int = malloc(sizeof(t_interfaz));
    nueva_int->nombre = nombre;
    nueva_int->config = config;

    return nueva_int;
}

void iterator(char* value) 
{
    log_info(logger, "%s", value);
}