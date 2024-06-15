
#ifndef PAGES_H
#define PAGES_H
#include <utils/client.h>
#include <commons/bitarray.h>


/*

Páginas: Se refieren a las divisiones lógicas de un proceso en la memoria virtual. 
Cada proceso se divide en páginas del mismo tamaño.
Frames: Se refieren a las divisiones físicas de la memoria principal (RAM).
Cada página de un proceso se carga en un frame de la memoria principal.

Each page in the main memory (physical memory) is mapped to a frame. 

Se crearan tres estructuras principales

1. memoria: es el espacio de usuario 

2. bitmap:  Para gestionar los frames ocupados.

3. Lista: cada proceso su propia lista

Las tres estructuras estaran protegidas con semaforas sem_wait y sem_post
Al finalizar un proceso, se actualizará el bitmap.

*/

char *mapabits;
t_bitarray* frames;
char* espacio_usuario;


// Estructura para representar la memoria en general
char* mainMemory;


// Estructura para representar una tabla de páginas


// Estructura para representar una página de memoria
typedef struct {
    int pagina_id;          // Identificador de la página 
    int numero_marco;       // Número de marco asignado a la página
} PaginaMemoria;

// Estructura para representar una tabla de páginas
typedef struct {
    PaginaMemoria* paginas; // Array de páginas
    int num_paginas;        // Número total de páginas
    pthread_mutex_t mutex_tabla;
} TablaPaginas;

#endif // PAGES_H