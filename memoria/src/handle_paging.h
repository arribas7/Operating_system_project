#ifndef HANDLE_PAGING_H
#define HANDLE_PAGING_H

#include <stdbool.h>
#include <pthread.h>

typedef struct {
    int memory_size;
    int page_size;
    bool* frames_ocupados;
    pthread_mutex_t mutex_frames_ocupados;
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

#endif // HANDLE_PAGING_H
