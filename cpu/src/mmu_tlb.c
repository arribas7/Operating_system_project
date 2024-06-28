#include "mmu_tlb.h"

TLBEntry TLB[32];
int tlb_index = 0;
int tlb_size = 0; //para ir sabiendo cuantas entradas hay en la tlb
//int numero_pagina = 0;
int current_time = 0;

uint32_t mmu(char* logicalAddress){
    int direccion_fisica;
    int direccion_logica = atoi(logicalAddress);
    TLBEntry* tlbEntry;

    int tam_pag = solicitar_tam_pag_a_mem();
    int marco;

    double numero_pagina = floor(direccion_logica / tam_pag);
    int desplazamiento = direccion_logica - (numero_pagina * tam_pag);

    log_debug(logger, "En mmu....");
    log_debug(logger, "nroPagina: %d", numero_pagina);
    log_debug(logger, "desplazamiento: %d", desplazamiento);

    if(cant_entradas_tlb() != 0){
        if (algoritmo_tlb() == "FIFO") {
            tlbEntry = buscar_en_TLB(pcb_en_ejecucion->pid,numero_pagina);

            if (tlbEntry == NULL){ //actualizo la TLB   
                log_info(logger, "PID: <%d> - TLB MISS - Pagina: <%d> ", pcb_en_ejecucion->pid, numero_pagina);
                marco = requestFrameToMem(numero_pagina);
                agregar_a_TLB(pcb_en_ejecucion->pid,numero_pagina,marco);
            }
            else{
                log_info(logger, "PID: <%d> - TLB HIT - Pagina: <%d> ", pcb_en_ejecucion->pid, numero_pagina);
                direccion_fisica = tlbEntry->marco + desplazamiento;
            }
        }   
        else{   //algoritmo LRU
            tlbEntry = buscar_en_TLB(pcb_en_ejecucion->pid,numero_pagina);

            if (tlbEntry == NULL){ //actualizo la TLB   
                log_info(logger, "PID: <%d> - TLB MISS - Pagina: <%d> ", pcb_en_ejecucion->pid, numero_pagina);
                marco = requestFrameToMem(numero_pagina);
                agregar_a_TLB_LRU(pcb_en_ejecucion->pid,numero_pagina,marco);
            }
            else{
                log_info(logger, "PID: <%d> - TLB HIT - Pagina: <%d> ", pcb_en_ejecucion->pid, numero_pagina);
                direccion_fisica = tlbEntry->marco + desplazamiento;
            }
        }
    }
    else{
        log_info(logger,"TLB deshabilitada, buscando frame en memoria...");
        direccion_fisica = requestFrameToMem(numero_pagina) + desplazamiento;
    }
    
    return direccion_fisica;
}

TLBEntry* buscar_en_TLB(int pid, int pagina){
    for (int i=0; i < cant_entradas_tlb() ; i++){
        if (TLB[i].pid == pid && TLB[i].pagina == pagina){
            return &TLB[i];
        }  
    }
    return NULL; //si no encuentra en la tlb
}

void agregar_a_TLB(int pid, int pagina, int marco) {
    // Verificar si la TLB está llena

    if (tlb_size < cant_entradas_tlb()) {
        // Si hay espacio disponible, agregar la entrada al final de la TLB
        TLB[tlb_size].pid = pid;
        TLB[tlb_size].pagina = pagina;
        TLB[tlb_size].marco = marco;
        TLB[tlb_size].last_time_access = 0;
        tlb_size++;        // Incrementar el tamaño de la TLB
    } else {
        // Se sobrescribirá la entrada más antigua (la que está en el índice tlb_index)
        TLB[tlb_index].pid = pid;
        TLB[tlb_index].pagina = pagina;
        TLB[tlb_index].marco = marco;
        TLB[tlb_size].last_time_access = 0;
        // Incrementar el índice para la próxima entrada
        tlb_index = (tlb_index + 1) % cant_entradas_tlb(); // Incremento circular del índice
    }
}

void agregar_a_TLB_LRU(int pid, int pagina, int marco) {
    // Verificar si la TLB está llena
    if (tlb_size < cant_entradas_tlb()) {
        // Si hay espacio disponible, agregar la entrada al final de la TLB
        TLB[tlb_size].pid = pid;
        TLB[tlb_size].pagina = pagina;
        TLB[tlb_size].marco = marco;
        TLB[tlb_size].last_time_access = current_time; //registro el tiempo de acceso
        tlb_size++;        // Incrementar el tamaño de la TLB
    } else {
        int lru_index = find_LRU_index();
        TLB[lru_index].pid = pid;
        TLB[lru_index].pagina = pagina;
        TLB[lru_index].marco = marco;
        TLB[lru_index].last_time_access = current_time;
    }
}

int find_LRU_index() {
    int lru_index = 0;
    int lru_time = 999999999;

    for (int i = 0; i < cant_entradas_tlb(); i++) {
        if (TLB[i].last_time_access < lru_time) {
            lru_time = TLB[i].last_time_access;
            lru_index = i;
        }
    }
    return lru_index;
}

int solicitar_tam_pag_a_mem(void){
    t_paquete* peticion = crear_paquete(TAM_PAG);
    enviar_paquete(peticion, conexion_mem); //envio el paquete vacio solo con el opcode, aver si funciona

    return recibir_tam_pag(conexion_mem); //mensaje desde memoria
}


int recibir_tam_pag(int socket_cliente)
{
    int size;
    int* tam_pag = recibir_buffer(&size, socket_cliente);
    log_debug(logger, "Tam pag received.. %d", *tam_pag);

    return *tam_pag;
}
#include "mmu_tlb.h"

TLBEntry TLB[32];
int tlb_index = 0;
int tlb_size = 0; //para ir sabiendo cuantas entradas hay en la tlb
//int numero_pagina = 0;
int current_time = 0;

uint32_t mmu(char* logicalAddress){
    int direccion_fisica;
    int direccion_logica = atoi(logicalAddress);
    TLBEntry* tlbEntry;

    int tam_pag = solicitar_tam_pag_a_mem();
    int marco;

    double numero_pagina = floor(direccion_logica / tam_pag);
    int desplazamiento = direccion_logica - (numero_pagina * tam_pag);

    log_debug(logger, "En mmu....");
    log_debug(logger, "nroPagina: %d", numero_pagina);
    log_debug(logger, "desplazamiento: %d", desplazamiento);

    if(cant_entradas_tlb() != 0){
        if (algoritmo_tlb() == "FIFO") {
            tlbEntry = buscar_en_TLB(pcb_en_ejecucion->pid,numero_pagina);

            if (tlbEntry == NULL){ //actualizo la TLB   
                log_info(logger, "PID: <%d> - TLB MISS - Pagina: <%d> ", pcb_en_ejecucion->pid, numero_pagina);
                marco = requestFrameToMem(numero_pagina);
                agregar_a_TLB(pcb_en_ejecucion->pid,numero_pagina,marco);
            }
            else{
                log_info(logger, "PID: <%d> - TLB HIT - Pagina: <%d> ", pcb_en_ejecucion->pid, numero_pagina);
                direccion_fisica = tlbEntry->marco * tam_pag + desplazamiento;
            }
        }   
        else{   //algoritmo LRU
            tlbEntry = buscar_en_TLB(pcb_en_ejecucion->pid,numero_pagina);

            if (tlbEntry == NULL){ //actualizo la TLB   
                log_info(logger, "PID: <%d> - TLB MISS - Pagina: <%d> ", pcb_en_ejecucion->pid, numero_pagina);
                marco = requestFrameToMem(numero_pagina);
                agregar_a_TLB_LRU(pcb_en_ejecucion->pid,numero_pagina,marco);
            }
            else{
                log_info(logger, "PID: <%d> - TLB HIT - Pagina: <%d> ", pcb_en_ejecucion->pid, numero_pagina);
                direccion_fisica = tlbEntry->marco * tam_pag + desplazamiento;
            }
        }
    }
    else{
        log_info(logger,"TLB deshabilitada, buscando frame en memoria...");
        direccion_fisica = requestFrameToMem(numero_pagina) + desplazamiento;
    }
    
    return direccion_fisica;
}

TLBEntry* buscar_en_TLB(int pid, int pagina){
    for (int i=0; i < cant_entradas_tlb() ; i++){
        if (TLB[i].pid == pid && TLB[i].pagina == pagina){
            return &TLB[i];
        }  
    }
    return NULL; //si no encuentra en la tlb
}

void agregar_a_TLB(int pid, int pagina, int marco) {
    // Verificar si la TLB está llena

    if (tlb_size < cant_entradas_tlb()) {
        // Si hay espacio disponible, agregar la entrada al final de la TLB
        TLB[tlb_size].pid = pid;
        TLB[tlb_size].pagina = pagina;
        TLB[tlb_size].marco = marco;
        TLB[tlb_size].last_time_access = 0;
        tlb_size++;        // Incrementar el tamaño de la TLB
    } else {
        // Se sobrescribirá la entrada más antigua (la que está en el índice tlb_index)
        TLB[tlb_index].pid = pid;
        TLB[tlb_index].pagina = pagina;
        TLB[tlb_index].marco = marco;
        TLB[tlb_size].last_time_access = 0;
        // Incrementar el índice para la próxima entrada
        tlb_index = (tlb_index + 1) % cant_entradas_tlb(); // Incremento circular del índice
    }
}

void agregar_a_TLB_LRU(int pid, int pagina, int marco) {
    // Verificar si la TLB está llena
    if (tlb_size < cant_entradas_tlb()) {
        // Si hay espacio disponible, agregar la entrada al final de la TLB
        TLB[tlb_size].pid = pid;
        TLB[tlb_size].pagina = pagina;
        TLB[tlb_size].marco = marco;
        TLB[tlb_size].last_time_access = current_time; //registro el tiempo de acceso
        tlb_size++;        // Incrementar el tamaño de la TLB
    } else {
        int lru_index = find_LRU_index();
        TLB[lru_index].pid = pid;
        TLB[lru_index].pagina = pagina;
        TLB[lru_index].marco = marco;
        TLB[lru_index].last_time_access = current_time;
    }
}

int find_LRU_index() {
    int lru_index = 0;
    int lru_time = 999999999;

    for (int i = 0; i < cant_entradas_tlb(); i++) {
        if (TLB[i].last_time_access < lru_time) {
            lru_time = TLB[i].last_time_access;
            lru_index = i;
        }
    }
    return lru_index;
}

int solicitar_tam_pag_a_mem(void){
    t_paquete* peticion = crear_paquete(TAM_PAG);
    enviar_paquete(peticion, conexion_mem); //envio el paquete vacio solo con el opcode, aver si funciona

    return recibir_tam_pag(conexion_mem); //mensaje desde memoria
}


int recibir_tam_pag(int socket_cliente)
{
    int size;
    int* tam_pag = recibir_buffer(&size, socket_cliente);
    log_debug(logger, "Tam pag received.. %d", *tam_pag);

    return *tam_pag;
}
