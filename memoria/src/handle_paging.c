#include "handle_paging.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

Memory memory;
void* espacio_usuario;
char* espacio_usuario_ptr;
pthread_mutex_t mutex_espacio_usuario = PTHREAD_MUTEX_INITIALIZER;

int calcularMarcosNecesarios(int tamano_proceso, int tamano_marco) {
    return (tamano_proceso + tamano_marco - 1) / tamano_marco;
}

int asignarMarcoLibre(void) {
    int frameCount = memory.memory_size / memory.page_size;
    for (int i = 0; i < frameCount; ++i) {
        if (!memory.frames_ocupados[i]) {
            memory.frames_ocupados[i] = true;
            return i;
        }
    }
    return -1;
}

void actualizarBitmap(int marcos_necesarios) {
    pthread_mutex_lock(&memory.mutex_frames_ocupados);
    for (int i = 0; i < marcos_necesarios; ++i) {
        int marco = asignarMarcoLibre();
        if (marco == -1) {
            perror("No free frames available");
            pthread_mutex_unlock(&memory.mutex_frames_ocupados);
            exit(EXIT_FAILURE);
        }
        memory.frames_ocupados[marco] = true;
    }
    pthread_mutex_unlock(&memory.mutex_frames_ocupados);
}

void escribirEnEspacioUsuario(const char* buffer, int tamano_proceso) {
    pthread_mutex_lock(&mutex_espacio_usuario);
    // Escribe el buffer en el espacio de usuario
    memcpy(espacio_usuario_ptr, buffer, tamano_proceso);
    // Actualiza la posición del puntero del espacio de usuario
    espacio_usuario_ptr += tamano_proceso;
    pthread_mutex_unlock(&mutex_espacio_usuario);
    
}

TablaPaginas crearTablaPaginas(int pid, int tamano_proceso, int tamano_marco) {
    int num_marcos = calcularMarcosNecesarios(tamano_proceso, tamano_marco);

    TablaPaginas tabla;
    tabla.pid_tabla = pid;
    tabla.num_paginas = num_marcos;
    tabla.paginas = (PaginaMemoria*)malloc(num_marcos * sizeof(PaginaMemoria));
    pthread_mutex_init(&tabla.mutex_tabla, NULL);

    if (tabla.paginas == NULL) {
        perror("Failed to allocate memory for page table");
        exit(EXIT_FAILURE);
    }

    pthread_mutex_lock(&memory.mutex_frames_ocupados);
    for (int i = 0; i < num_marcos; ++i) {
        int marco = asignarMarcoLibre();
        if (marco == -1) {
            perror("No free frames available");
            pthread_mutex_unlock(&memory.mutex_frames_ocupados);
            exit(EXIT_FAILURE);
        }
        tabla.paginas[i].pagina_id = i;
        tabla.paginas[i].numero_marco = marco;
    }
    pthread_mutex_unlock(&memory.mutex_frames_ocupados);

    return tabla;
}

void liberarTablaPaginas(TablaPaginas tabla) {
    pthread_mutex_destroy(&tabla.mutex_tabla);
    free(tabla.paginas);
}

void inicializarEspacioUsuario() {
    espacio_usuario = malloc(memory.memory_size);
    if (espacio_usuario == NULL) {
        perror("Memory allocation failed!\n");
        exit(EXIT_FAILURE);
    }
    espacio_usuario_ptr = espacio_usuario;
}

void inicializarBitmap() {
    int frameCount = memory.memory_size / memory.page_size;
    memory.frames_ocupados = calloc(frameCount, sizeof(bool));
    if (memory.frames_ocupados == NULL) {
        perror("Failed to allocate memory for frame occupancy");
        free(espacio_usuario);
        exit(EXIT_FAILURE);
    }
    memset(memory.frames_ocupados, 0, frameCount * sizeof(bool));
    pthread_mutex_init(&memory.mutex_frames_ocupados, NULL);
}

void logMemoryInfo() {
    int pageSize = memory.page_size;
    int frameCount = memory.memory_size / pageSize;
    printf("NUMERO DE MARCOS: %d\n", frameCount);
    // Supongo que log_info es una función que tienes implementada
    // log_info(logger, "Memory has %d frames of %d bytes", frameCount, pageSize);
}

int initPaging(void) {
    printf("TAMAÑO MEMORIA TOTAL: %d\n", memory.memory_size);
    
    inicializarEspacioUsuario();
     
    inicializarBitmap();
    logMemoryInfo();

    return 1;
}

void handle_paging(const char* buffer, uint32_t tamano_proceso, int pid) {
    
  
     char* buffer = "hola";
     //falta:

    //determinar cuanto espacio necesita
    //consultar bitmap
    // en caso tenga espacio...

    

    //1.- crea la tabla por pid

    TablaPaginas tabla = crearTablaPaginas(pid, tamano_proceso, memory.page_size);
    //2.- escribe
    escribirEnEspacioUsuario(buffer, tamano_proceso);
    //escribirEnEspacioUsuario(buffer, strlen(buffer) + 1); +1 para que asegurar que se actualice al final el ptr, hay q probarlo igual
    printf("Tabla de Páginas del Proceso %d:\n", pid);
    for (int i = 0; i < tabla.num_paginas; ++i) {
        printf("Página %d -> Marco %d\n", tabla.paginas[i].pagina_id, tabla.paginas[i].numero_marco);
    }
    //3.- Actualiza
    actualizarBitmap(marcos_necesarios);

    liberarTablaPaginas(tabla);
}
