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
{
     int memory_size; 		//  bytes 
     int page_size; 		//  bytes 
     char* port;
     char* ip;
     int respond_time;
} t_memory;
        
extern t_memory value_memory;


// Estructura para representar una página de memoria
typedef struct {
    uint32_t pagina_id; 
    void* direccion; 
    uint32_t size; 
 } 
	PaginaMemoria;

// Estructura para representar una tabla de páginas
typedef struct {
    uint32_t tabla_id; 
    PaginaMemoria* paginas; 
    uint32_t num_paginas; 
} TablaPaginas;

// Estructura para representar la memoria en general
typedef struct {
    uint8_t *espacio_usuario; // Espacio de usuario de la memoria
	//void* espacio_usuario; //en el caso de que reciba cualquier tipo de datos va este
    uint32_t tamano_memoria; 
    uint32_t tamano_pagina; 
    TablaPaginas tabla_paginas; 
   void *config;
} MemoriaPrincipal;



int iniciarMemoria(void);
int correr_servidor(void *arg);
void clean(t_config *config);
//void liberarMemoria();




