
#ifndef PAGES_H
#define PAGES_H
#include <utils/client.h>
#include <stdbool.h>
//#include <commons/bitarray.h>
 #include "semaphore.h"
#include <memoria.h>
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


//extern void* espacio_usuario;

// Estructura para representar una página de memoria
typedef struct {
    int pagina_id;          // Identificador de la página 
    int numero_marco;       // Número de marco asignado a la página
} PaginaMemoria;

// Estructura para representar una tabla de páginas
typedef struct {
    PaginaMemoria* paginas; // Array de páginas
    int num_paginas;        // Número total de páginas
    int pid_tabla;
    pthread_mutex_t mutex_tabla;
} TablaPaginas;



int initPaging(void);
void handle_paging(const char* buffer, uint32_t tamano_proceso, int pid);
TablaPaginas* crearTablaPaginas(int pid, int tamano_proceso, int tamano_marco);
TablaPaginas* tablaDePaginasAsociada (int pid);
void enviar_marco(int pagina, int pid, int cliente_fd);
void liberarTablaPaginas(TablaPaginas tabla);
bool hayEspacioEnBitmap(int marcos_necesarios);
void copy_string(int direc_fis_1, int direc_fis_2, int tamanio, int pid);
char* obtenerDireccionFisicafull(int direccion_fisica, TablaPaginas* tablaAsociada);
int calcularDesplazamiento(int direccion_fisica);
char* obtenerDireccionFisica(int marco, int desplazamiento);
void escribir_en_direcc_fisica(int pid,int df,int val);
char* obtener_valor(int pid,int df);

void escribirEnEspacioUsuario2(int direccion_fisica, char* datos, int tamano, int pid);
int resize_process(int pid, int nuevo_tamano);
void liberarMarcoFisico(int marco);
char* leerDesdeEspacioUsuario(int direccion_fisica, int tamano, int pid);

// Escritura y lectura contigua

uint32_t escribirEnDireccionFisica(uint32_t dirFisica, char* txt, uint32_t size, uint32_t pid);
char* leerDeDireccionFisica(uint32_t dirFisica, uint32_t size, uint32_t pid);

uint32_t escribirEnDireccionFisica2(uint32_t dirFisica, char* txt, uint32_t size, uint32_t pid) ;
int leerDeDireccionFisica3(uint32_t dirFisica, uint32_t size, char* buffer, uint32_t pid);
void finish_process(int pid);


#endif // PAGES_H