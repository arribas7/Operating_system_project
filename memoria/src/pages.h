
#ifndef PAGES_H
#define PAGES_H
#include <utils/client.h>
#include <commons/bitarray.h>
char *mapabits;
t_bitarray* frames;
char* mainMemory;
char* mainMemory;

typedef struct {
    int idPage;
    int sizeInstruction;
    t_list* pages;
    uint32_t tabla_id; 
   //PaginaMemoria* paginas; 
    uint32_t num_paginas; 
} pageTable;

#endif // PAGES_H