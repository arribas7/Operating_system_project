#include <pages.h>
#include <memoria.h>


void inicializarEspacioUsuario() {
    espacio_usuario = malloc(memory.memory_size);
    if (espacio_usuario == NULL) {
        perror("Memory allocation failed!\n");
        exit(EXIT_FAILURE);
    }
    pthread_mutex_init(&memory.mutex_espacio_usuario, NULL); 
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

void inicializarListaDeTablasDePaginas(){
    listaTablasDePaginas = dictionary_create();
}

int initPaging(void) {
    printf("TAMAÑO MEMORIA TOTAL: %d\n",memory.memory_size);
 /* ----------------First Structure ---------------- */
     inicializarEspacioUsuario();
 /* ----------------Second Structure ---------------- */ 
     inicializarBitmap();
 /* ----------------Third Structure ---------------- */
 // inicializarTabla(); no creo q sea necesario, reviso
    inicializarListaDeTablasDePaginas();
  return 1;
    
}
/*
 void calculos(void){
 int pageSize = memory.page_size; //TAMAÑO DE MARCO = TAMAÑO DE PAGINA
    int frameCount = memory.memory_size / pageSize; 
    printf("NUMERO DE MARCOS: %d\n",frameCount);
    log_info(logger, "Memory has %d frames of %d bytes", frameCount, pageSize);
}

void logMemoryInfo() {
    int pageSize = memory.page_size;
    int frameCount = memory.memory_size / pageSize;
    printf("NUMERO DE MARCOS: %d\n", frameCount);
    log_info(logger, "Memory has %d frames of %d bytes", frameCount, pageSize);
}
*/
int calcularMarcosNecesarios(int tamano_proceso, int tamano_marco) {
    return( ceil(tamano_proceso / tamano_marco)); //calculamos la cantidad de marcos para el proceso redondeando siempre para arriba
   // return (tamano_proceso + tamano_marco - 1) / tamano_marco;
}

bool hayEspacioEnBitmap(int marcos_necesarios) {
    int frameCount = memory.memory_size / memory.page_size;
    int marcos_libres = 0;

    pthread_mutex_lock(&memory.mutex_frames_ocupados);
    for (int i = 0; i < frameCount; ++i) {
        if (!memory.frames_ocupados[i]) {
            marcos_libres++;
        }
        if (marcos_libres >= marcos_necesarios) {
            pthread_mutex_unlock(&memory.mutex_frames_ocupados);
            return true;
        }
    }
    pthread_mutex_unlock(&memory.mutex_frames_ocupados);
    return false;
}

// Asigna un marco libre y devuelve su índice, o -1 si no hay marcos libres. SIrve  para actualozar bitmap:
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
    pthread_mutex_lock(&memory.mutex_espacio_usuario);
    memcpy(espacio_usuario, buffer, strlen(buffer) + 1); // +1 para incluir el carácter nulo de terminación
    espacio_usuario = (char*)espacio_usuario + tamano_proceso;  //ESTO DEBERIA SER CIRCULAR, EL ESPACIO DE USUARIO ES LIMITADO, POSIBLE SOLUCION: % memory.memory_size
    pthread_mutex_unlock(&memory.mutex_espacio_usuario);     
     int marcos_necesarios = calcularMarcosNecesarios(tamano_proceso, memory.page_size);
    actualizarBitmap(marcos_necesarios);
}   


// Función para crear la tabla de páginas de un proceso (y asignar marcos a cada página)
TablaPaginas crearTablaPaginas(int pid, int tamano_proceso, int tamano_marco) {
    TablaPaginas tabla;
    int num_marcos = calcularMarcosNecesarios(tamano_proceso, tamano_marco);
    tabla.pid_tabla = pid;
    tabla.num_paginas = num_marcos;
    tabla.paginas = (PaginaMemoria*)malloc(num_marcos * sizeof(PaginaMemoria));
    pthread_mutex_init(&tabla.mutex_tabla,NULL); //puede no ser necesario semaforo porque cada tabla con su pid.

    if (tabla.paginas == NULL) {
        perror("Failed to allocate memory for page table");
        exit(EXIT_FAILURE);
    }
       for (int i = 0; i < num_marcos; ++i) {
        int marco = asignarMarcoLibre();
        if (marco == -1) {
            perror("No free frames available");
            free(tabla.paginas);
            pthread_mutex_destroy(&tabla.mutex_tabla);
            exit(EXIT_FAILURE);
        }
        tabla.paginas[i].pagina_id = i;
        tabla.paginas[i].numero_marco = marco;
    }
 
    //agrego la tabla al diccionario de tablas de paginas
    dictionary_put(listaTablasDePaginas,string_itoa(tabla.pid_tabla),&tabla);

    return tabla;
}

void liberarTablaPaginas(TablaPaginas tabla) {
    pthread_mutex_destroy(&tabla.mutex_tabla);
    free(tabla.paginas);
}

void handle_paging(const char* buffer, uint32_t tamano_proceso, int pid) {
    int tamano_marco=memory.page_size;
    int marcos_necesarios = calcularMarcosNecesarios(tamano_proceso,tamano_marco);
     if (!hayEspacioEnBitmap(marcos_necesarios)) {
        perror("No hay suficiente espacio en la memoria");
        exit(EXIT_FAILURE);
    }
     TablaPaginas tabla = crearTablaPaginas(pid, tamano_proceso, memory.page_size);
    printf("Tabla de Páginas del Proceso %d:\n", pid);
    escribirEnEspacioUsuario(buffer, tamano_proceso);
    actualizarBitmap(marcos_necesarios);
    liberarTablaPaginas(tabla);
}

  
//Dado un pid se encuentra la tabla de paginas asociada
TablaPaginas* tablaDePaginasAsociada (int pid){
    return dictionary_get(listaTablasDePaginas,string_itoa(pid));
}

//Dado un numero de pagina y una tabla de paginas asociada, retorna el marco asociado
int marcoAsociado(int numero_pagina, TablaPaginas* tablaAsociada){
    int i=0;
    int marco = -1; //en caso no find nunca se modificara y devolvera este
    for (i=0; i<tablaAsociada->num_paginas ; i++){
        if(numero_pagina == tablaAsociada->paginas[i].pagina_id){
            marco = tablaAsociada->paginas[i].numero_marco;
            break;
        }
    }

    return marco;
}


 //Estaria para ponerlo en algun lado:
    //for (int i = 0; i < tabla.num_paginas; ++i) {
    //    printf("Página %d -> Marco %d\n", tabla.paginas[i].pagina_id, tabla.paginas[i].numero_marco);
    //}

  