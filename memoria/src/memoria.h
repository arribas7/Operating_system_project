#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <utils/client.h>
#include <commons/log.h>
#include <commons/config.h>
#include <utils/server.h>
#include <utils/kernel.h>
#include <pthread.h>
#include <stdint.h>
#include <pthread.h>
#include <stdio.h>
#include <utils/cpu.h>
#include <files.h>

typedef struct

{    //char *espacio_usuario; lo declaré en pages.h
     int memory_size; 		//  bytes 
     int page_size; 		//  bytes 
     char* port;
     char* ip;
     int respond_time;
     t_list *tablas_paginas;  // Lista de tablas de páginas
     bool *frames_ocupados; 
     pthread_mutex_t mutex_memoria;
} t_memory;
        
extern t_memory memory;



int iniciarMemoria(void);
int correr_servidor(void *arg);
void clean(t_config *config);
//void liberarMemoria();