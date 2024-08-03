#include "mmu_tlb.h"
#include "time.h"

TLBEntry TLB[32];
int tlb_index = 0;
int tlb_size = 0; //para ir sabiendo cuantas entradas hay en la tlb
//int numero_pagina = 0;
int current_time = 0;

void contenido_tlb(){
    int i=0;
    for (i=0; i<cant_entradas_tlb();i++)
        log_debug(logger,"TLB i: %d - Pagina: %d - LTA: %d",i, TLB[i].pagina, TLB[i].last_time_access);
}

void actualizar_tlb_LRU(TLBEntry* tlbEntry){
    tlbEntry->last_time_access = buscar_mayor_TA_en_TLB() + 1000;
    log_debug(logger,"Actualizo entrada tlb con current time: %ld",tlbEntry->last_time_access);
}


uint32_t mmu(char* logicalAddress){
    int direccion_fisica;
    int direccion_logica = atoi(logicalAddress);
    TLBEntry* tlbEntry;

    int tam_pag = solicitar_tam_pag_a_mem();
    int marco = 0;

    int numero_pagina = floor(direccion_logica / tam_pag);
    int desplazamiento = direccion_logica % tam_pag;

    log_debug(logger, "En mmu....");
    log_debug(logger, "nroPagina: %d", numero_pagina);
    log_debug(logger, "desplazamiento: %d", desplazamiento);

    if(cant_entradas_tlb() != 0){
        if (!strcmp(algoritmo_tlb(),"FIFO")) {
            tlbEntry = buscar_en_TLB(pcb_en_ejecucion->pid,numero_pagina);

            if (tlbEntry == NULL){ //actualizo la TLB   
                log_info(logger, "PID: <%d> - TLB MISS - Pagina: <%d> ", pcb_en_ejecucion->pid, numero_pagina);
                marco = requestFrameToMem(numero_pagina);
                agregar_a_TLB(pcb_en_ejecucion->pid,numero_pagina,marco);
                // direccion_fisica = marco + desplazamiento;
                direccion_fisica = marco * tam_pag + desplazamiento;
            } else{
                log_info(logger, "PID: <%d> - TLB HIT - Pagina: <%d> ", pcb_en_ejecucion->pid, numero_pagina);
                //direccion_fisica = tlbEntry->marco + desplazamiento;
                direccion_fisica = tlbEntry->marco * tam_pag + desplazamiento;
                marco = tlbEntry->marco;
            }
        }   
        else{   //algoritmo LRU
            tlbEntry = buscar_en_TLB(pcb_en_ejecucion->pid,numero_pagina);

            if (tlbEntry == NULL){ //actualizo la TLB   
                log_info(logger, "PID: <%d> - TLB MISS - Pagina: <%d> ", pcb_en_ejecucion->pid, numero_pagina);
                marco = requestFrameToMem(numero_pagina);
                agregar_a_TLB_LRU(pcb_en_ejecucion->pid,numero_pagina,marco);
                direccion_fisica = marco * tam_pag + desplazamiento;
            }
            else{
                log_info(logger, "PID: <%d> - TLB HIT - Pagina: <%d> ", pcb_en_ejecucion->pid, numero_pagina);
                actualizar_tlb_LRU(tlbEntry);
                direccion_fisica = tlbEntry->marco * tam_pag + desplazamiento;
                marco = tlbEntry->marco;
            }
        }
        contenido_tlb();
    }
    else{
        log_info(logger,"TLB deshabilitada, buscando frame en memoria...");
        marco = requestFrameToMem(numero_pagina);
        direccion_fisica =  marco * tam_pag + desplazamiento;
    }

    log_debug(logger, "PID: <%d> - DL: <%d> - PAG:<%d> - OFFSET: <%d> ", pcb_en_ejecucion->pid, direccion_logica, numero_pagina, desplazamiento);
    log_debug(logger, "PID: <%d> - DF: <%d> - MARCO:<%d> - OFFSET: <%d> ", pcb_en_ejecucion->pid, direccion_fisica, marco, desplazamiento);
    return direccion_fisica;
}

TLBEntry* buscar_en_TLB(int pid, int pagina){
    int entradas = cant_entradas_tlb();
    for (int i = 0; i < entradas; i++){
        if (TLB[i].pid == pid && TLB[i].pagina == pagina){
            return &TLB[i];
        }  
    }
    return NULL; //si no encuentra en la tlb
}

int buscar_mayor_TA_en_TLB(){
    int mayor = TLB[0].last_time_access;

    for (int i = 1; i < cant_entradas_tlb(); i++) {
        if (TLB[i].last_time_access > mayor) {
            mayor = TLB[i].last_time_access;
        }
    }
    return mayor;
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
        TLB[tlb_index].last_time_access = 0;
        // Incrementar el índice para la próxima entrada
        tlb_index = (tlb_index + 1) % cant_entradas_tlb(); // Incremento circular del índice
    }
}

void agregar_a_TLB_LRU(int pid, int pagina, int marco) {
    current_time = buscar_mayor_TA_en_TLB() + 1000;
    //log_debug(logger,"CURRENT_TIME: %ld",current_time);
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
    int oldest = TLB[0].last_time_access;

    for (int i = 1; i < cant_entradas_tlb(); i++) {
        if (TLB[i].last_time_access < oldest) {
            oldest = TLB[i].last_time_access;
            lru_index = i;
        }
    }
    return lru_index;
}

int solicitar_tam_pag_a_mem(void){
    t_paquete* peticion = crear_paquete(TAM_PAG);
    agregar_a_paquete(peticion,"tam_pag",sizeof("tam_pag"));
    enviar_paquete(peticion, conexion_mem); //envio el paquete vacio solo con el opcode, aver si funciona
    
    eliminar_paquete(peticion);

    int op = recibir_operacion(conexion_mem);
    int tam_pag = recibir_tam_pag(conexion_mem); //mensaje desde memoria
    return tam_pag;
}


int recibir_tam_pag(int socket_cliente)
{
    int size;
    char* tam_pag = recibir_buffer(&size, socket_cliente);
    log_debug(logger, "Tam pag received.. %s", tam_pag);
    int tampagina = atoi(tam_pag);
    free(tam_pag);
    return tampagina;
}


