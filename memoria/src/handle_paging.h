#ifndef HANDLE_PAGING_H
#define HANDLE_PAGING_H

#include <stdbool.h>
#include <pthread.h>
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


/*
typedef struct {
    int memory_size;
    int page_size;
    bool* frames_ocupados;
    //pthread_mutex_t mutex_frames_ocupados;
} Memory;

typedef struct {
    int pagina_id;
    int numero_marco;
} PaginaMemoria; 

typedef struct {
    int pid_tabla;
    int num_paginas;
    PaginaMemoria* paginas;
    pthread_mutex_t mutex_tabla;
} TablaPaginas;

extern Memory memory;
extern void* espacio_usuario;
extern char* espacio_usuario_ptr;
extern pthread_mutex_t mutex_espacio_usuario;

int initPaging(void);
void handle_paging(const char* buffer, uint32_t tamano_proceso, int pid);



char *mapabits;
t_bitarray* frames;
//char* espacio_usuario;


// Estructura para representar la memoria en general
char* mainMemory;

*/

#endif // HANDLE_PAGING_H
