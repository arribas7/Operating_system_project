#include "mmu_tlb.h"

TLBEntry TLB[32];
int tlb_size = 0; //para ir sabiendo cuantas entradas hay en la tlb
int tlb_index = 0; 
int numero_pagina = 0;

uint32_t mmu(char* logicalAddress){
    int direccion_fisica;
    int direccion_logica = atoi(logicalAddress);
    TLBEntry* tlbEntry;

    //int tam_pag = solicitar_tam_pag();//a memoria (32)  TO DO
    int tam_pag = solicitar_tam_pag_a_mem();

    numero_pagina = floor(direccion_logica / tam_pag);
    int desplazamiento = direccion_logica - (numero_pagina * tam_pag);

    log_debug(logger, "nroPagina: %d", numero_pagina);
    log_debug(logger, "desplazamiento: %d", desplazamiento);


    tlbEntry = buscar_en_TLB(pcb_en_ejecucion->pid,numero_pagina);

    if (tlbEntry == NULL){ //actualizo la TLB
        log_info(logger, "PID: <%d> - TLB MISS - Pagina: <%d> ", pcb_en_ejecucion->pid, numero_pagina);
        direccion_fisica = requestFrameToMem(numero_pagina);
        agregar_a_TLB(pcb_en_ejecucion->pid,numero_pagina,direccion_fisica);
    }
    else{
        log_info(logger, "PID: <%d> - TLB HIT - Pagina: <%d> ", pcb_en_ejecucion->pid, numero_pagina);
        direccion_fisica = tlbEntry->marco;
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
        tlb_size++;        // Incrementar el tamaño de la TLB
    } else {
        // Si la TLB está llena, se necesita aplicar un algoritmo de reemplazo
        // En este caso, utilizaremos FIFO (Primero en entrar, primero en salir)
        // Se sobrescribirá la entrada más antigua (la que está en el índice tlb_index)
        TLB[tlb_index].pid = pid;
        TLB[tlb_index].pagina = pagina;
        TLB[tlb_index].marco = marco;
        // Incrementar el índice para la próxima entrada
        tlb_index = (tlb_index + 1) % cant_entradas_tlb(); // Incremento circular del índice
    }
}


int solicitar_tam_pag_a_mem(void){
    t_paquete* peticion = crear_paquete(TAM_PAG);
    enviar_paquete(peticion, conexion_mem); //envio el paquete vacio solo con el opcode, aver si funciona

    return recibir_tam_pag(conexion_mem);
}


int recibir_tam_pag(int socket_cliente)
{
    int size;
    int* tam_pag = recibir_buffer(&size, socket_cliente);
    log_debug(logger, "Tam pag received.. %d", *tam_pag);

    return *tam_pag;
}

