#ifndef MMU_TLB_H
#define MMU_TLB_H

#include <stdlib.h>
#include <stdio.h>
#include "utils/cpu.h"
#include "utils/kernel.h"
#include <utils/server.h>
#include "math.h"
#include "connections.h"

extern t_log *logger;
extern t_log *loggerError;
extern t_config *config;

extern t_reg_cpu* reg_proceso_actual;
extern t_pcb* pcb_en_ejecucion;

extern int conexion_mem;

#define cant_entradas_tlb() config_get_int_value("cpu.config","CANTIDAD_ENTRADAS_TLB")

typedef struct tlb{
    int pid;
    int pagina;
    int marco;
    // Otros campos que puedan ser necesarios
} TLBEntry;

extern TLBEntry TLB[32];
extern int tlb_size; //para ir sabiendo cuantas entradas hay en la tlb
extern int tlb_index; //indice para el algoritmo FIFO

extern int numero_pagina;

TLBEntry* buscar_en_TLB(int pid, int pagina);
uint32_t mmu(char* logicalAddress);
void agregar_a_TLB(int pid, int pagina, int marco);
int solicitar_tam_pag_a_mem(void);
int recibir_tam_pag(int socket_cliente);


#endif