#include <pages.h>
#include <memoria.h>
pthread_mutex_t mutex_bitmap = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_usuario = PTHREAD_MUTEX_INITIALIZER;


int initPaging(void) {

    /* ----------------First Structure ---------------- */
 
    printf("TAMAÑO MEMORIA TOTAL: %d\n",memory.memory_size);

    espacio_usuario = malloc(memory.memory_size);
    if (espacio_usuario == NULL) {
        perror("Memory allocation failed!\n");
        return 0;
    }
    char* ptr_aux = espacio_usuario;
    //Ejemplo escribimos hola en el espacio de usuario, esto va a venir de IO 
    char* buffer = "hola";
    memcpy(espacio_usuario, buffer, strlen(buffer) + 1); // +1 para incluir el carácter nulo de terminación
    //*(ptr_aux+strlen(buffer));   LUEGO DE COPIAR EL HOLA EL ESPACIO DE USUARIO DEBERIA APUNTAR A LA PROXIMA POSICION LIBRE

    int pageSize = memory.page_size; //TAMAÑO DE MARCO = TAMAÑO DE PAGINA
    int frameCount = memory.memory_size / pageSize; 
    printf("NUMERO DE MARCOS: %d\n",frameCount);
    //Each bit in the bit array represents one frame (0 for free, 1 for occupied).

    log_info(logger, "Memory has %d frames of %d bytes", frameCount, pageSize);

       /* ----------------Second Structure ---------------- */


    // Inicializar el arreglo para llevar registro de los frames ocupados
    memory.frames_ocupados = calloc(frameCount, sizeof(bool)); //ARMAR UNA FUNCION QUE A MEDIDA QUE OCUPO N BYTES EN ESPACIO DE USUARIO => ACTUALIZO N BYTES OCUPADOS EN BITMAP
    if (memory.frames_ocupados == NULL) {
       perror("Failed to allocate memory for frame occupancy");
       free(espacio_usuario);
       exit(EXIT_FAILURE);
    }
    // establecer todos los elementos del arreglo frames_ocupados a cero
    memset(memory.frames_ocupados, 0, frameCount * sizeof(bool)); //inicializa el bitmap en 0
    
    return 1;
}

/* ----------------Third Structure ---------------- */
   
// Función para crear la tabla de páginas de un proceso (y asignar marcos a cada página)
TablaPaginas crearTablaPaginas(int pid, int tamano_proceso, int tamano_marco) {
    // Calculamos el número de marcos necesarios para el proceso
    int num_marcos = ceil(tamano_proceso / tamano_marco); //calculamos la cantidad de marcos para el proceso redondeando siempre para arriba

    // Creamos la tabla de páginas
    TablaPaginas tabla;
    tabla.pid_tabla = pid;
    tabla.num_paginas = num_marcos;
    tabla.paginas = (PaginaMemoria*)malloc(num_marcos * sizeof(PaginaMemoria));
    pthread_mutex_init(&tabla.mutex_tabla,NULL);

    if (tabla.paginas == NULL) {
        perror("Failed to allocate memory for page table");
        exit(EXIT_FAILURE);
    }

    //poner lo siguiente en una funcion de asignacion de marcos:
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

//HACER FUNCION PARA ESCRIBIR EN ESPACIO DE USUARIO, Y ACTUALIZAR A LA VEZ EL BITMAP SINCRONIZADAMENTE (dados una cantidad de bytes y algo para escribir)
// Asigna un marco libre y devuelve su índice, o -1 si no hay marcos libres
int asignarMarcoLibre(void) {
    int frameCount = memory.memory_size / memory.page_size;
    for (int i = 0; i < frameCount; ++i) {
        if (!memory.frames_ocupados[i]) {
            memory.frames_ocupados[i] = true;
            return i;
        }
    }
    return -1; // No hay marcos libres
}

    // Crear un proceso con un tamaño específico
   void handle_paging(const char *buffer, uint32_t tamano_proceso, int pid){
     int tamano_proceso = 256; // Por ejemplo, 256 bytes
     TablaPaginas tabla = crearTablaPaginas(tamano_proceso); //cada proceso tiene su propia tabla de pagina
     PageCount=num_marcos //calcular paginas necesarias, usar ceil
     guardamensaje();//Guarda el buffer en el espacio de usuario espacio_usuario
     actualizar_bitmap() //actualizar el bitmap , asi queda asociado al espacio de usuario
   
  }
//HACER FUNCION QUE DADO UN PID SE ENCUENTRE LA TABLA DE PAGINA ASOCIADA 