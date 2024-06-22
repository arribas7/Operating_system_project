#include <pages.h>
#include <memoria.h>


int initPaging(void) {

     /* ----------------First Structure ---------------- */
    
 
    printf("TAMAÑO MEMORIA TOTAL: %d\n",memory.memory_size);

       espacio_usuario = malloc(memory.memory_size);
    if (espacio_usuario == NULL) {
        perror("Memory allocation failed!\n");
        return 0;
    }
    //Ejemplo escribimos hola en el espacio de usuario, esto va a venir de CPU 
    char* buffer = "hola";
    memcpy(espacio_usuario, buffer, strlen(buffer) + 1); // +1 para incluir el carácter nulo de terminación


    int pageSize = memory.page_size;
    int frameCount = memory.memory_size / pageSize; 
     printf("TAMAÑO MARCO: %d\n",frameCount);
    //Each bit in the bit array represents one frame (0 for free, 1 for occupied).

    log_info(logger, "Memory has %d frames of %d bytes", frameCount, pageSize);





       /* ----------------Second Structure ---------------- */


      // Inicializar el arreglo para llevar registro de los frames ocupados
       memory.frames_ocupados = calloc(frameCount, sizeof(bool));
       if (memory.frames_ocupados == NULL) {
       perror("Failed to allocate memory for frame occupancy");
       free(espacio_usuario);
       exit(EXIT_FAILURE);
     }

      // establecer todos los elementos del arreglo frames_ocupados a cero
       memset(memory.frames_ocupados, 0, frameCount * sizeof(bool));
   
      //memset(mapabits, 0, frameCount / 8); 
    
    /*
    Memory Frame Allocation After memset
    -----------------------------------------------------------
    | Byte 0          | Byte 1          | Byte 2          | ...
    -----------------------------------------------------------
    | 0 0 0 0 0 0 0 0 | 0 0 0 0 0 0 0 0 | 0 0 0 0 0 0 0 0 | ...
    -----------------------------------------------------------
    | 0 1 2 3 4 5 6 7 | 8 9 10 11 12 13 14 15 | 16 17 18 19 20 21 22 23 | ...
    (Bits representing frames; all bits set to 0)
    */
   
   


    /* ----------------Third Structure ---------------- */
   
    // Función para crear la tabla de páginas y asignar marcos a cada página
    TablaPaginas crearTablaPaginas(int tamano_proceso, int tamano_marco) {
    // Calculamos el número de marcos necesarios para el proceso
    int num_marcos = calcularMarcosNecesarios(tamano_proceso, tamano_marco);

    // Creamos la tabla de páginas
    TablaPaginas tabla;
    tabla.num_paginas = num_marcos;
    tabla.paginas = (PaginaMemoria*)malloc(num_marcos * sizeof(PaginaMemoria)); 
    if (tabla.paginas == NULL) {
        perror("Failed to allocate memory for page table");
        exit(EXIT_FAILURE);
    }

    // Asignamos marcos a cada página
    for (int i = 0; i < num_marcos; ++i) {
        tabla.paginas[i].pagina_id = i;
        tabla.paginas[i].numero_marco = i; // Simplemente asignamos el número de marco consecutivo
    }

    return tabla;
}

// Función para liberar la memoria ocupada por la tabla de páginas
void liberarTablaPaginas(TablaPaginas tabla) {
    free(tabla.paginas);
}

    
    return 1;
}