#include <pages.h>
#include <memoria.h>
pthread_mutex_t mutex_bitmap = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_usuario = PTHREAD_MUTEX_INITIALIZER;
 char* espacio_usuario_ptr;

void inicializarEspacioUsuario() {
    espacio_usuario = malloc(memory.memory_size);
    if (espacio_usuario == NULL) {
        perror("Memory allocation failed!\n");
        exit(EXIT_FAILURE);
    }
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
   // pthread_mutex_init(&memory.mutex_frames_ocupados, NULL); //Estaria bueno inicializarsu semaforo aca para c/u de las tres estrcururas
}

void calculos(void){
 int pageSize = memory.page_size; //TAMAÑO DE MARCO = TAMAÑO DE PAGINA
    int frameCount = memory.memory_size / pageSize; 
    printf("NUMERO DE MARCOS: %d\n",frameCount);
    log_info(logger, "Memory has %d frames of %d bytes", frameCount, pageSize);
}

int initPaging(void) {
    printf("TAMAÑO MEMORIA TOTAL: %d\n",memory.memory_size);
 /* ----------------First Structure ---------------- */
     inicializarEspacioUsuario();
 /* ----------------Second Structure ---------------- */ 
     inicializarBitmap();
 /* ----------------Third Structure ---------------- */
 // inicializarTabla(); falta implementar
  return 1;
    
}

   
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

int calcularMarcosNecesarios(int tamano_proceso, int tamano_marco) {
    return (tamano_proceso + tamano_marco - 1) / tamano_marco;
}


void escribirEnEspacioUsuario(const char* buffer, int tamano_proceso) {
    pthread_mutex_lock(&mutex_espacio_usuario);
    char* ptr_aux = espacio_usuario;
    memcpy(espacio_usuario, buffer, strlen(buffer) + 1); // +1 para incluir el carácter nulo de terminación
    espacio_usuario  = tamano_proceso;
    //*(ptr_aux+strlen(buffer));   LUEGO DE COPIAR EL HOLA EL ESPACIO DE USUARIO DEBERIA APUNTAR A LA PROXIMA POSICION LIBRE
    //memcpy(espacio_usuario_ptr, buffer, tamano_proceso);
    // Actualiza la posición del puntero del espacio de usuario
    // espacio_usuario_ptr += tamano_proceso; me parece que este ptr no es necesario
    pthread_mutex_unlock(&mutex_espacio_usuario);
        
  
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

void handle_paging(const char* buffer, uint32_t tamano_proceso, int pid) {
    buffer = "hola";//esto lo recibe de I/O
    // A) cuanto espacio necesita
    int tamano_marco=memory.page_size; //despues reviso bien esta cuenta
    calcularMarcosNecesarios(tamano_proceso,tamano_marco);
    //B) consultar bitmap, hay lugar ? falta implementar

    // en caso tenga espacio...Son tres pasos claves:

    //1.- crea la tabla por pid
    TablaPaginas tabla = crearTablaPaginas(pid, tamano_proceso, memory.page_size);
    printf("Tabla de Páginas del Proceso %d:\n", pid);

    //2.- escribe
    escribirEnEspacioUsuario(buffer, tamano_proceso);
    //escribirEnEspacioUsuario(buffer, strlen(buffer) + 1); Puede q vaya esta. El +1 puede asegurar que se actualice al final el ptr, hay q probarlo igual
   
     //Estaria para ponerlo en algun lado:
    //for (int i = 0; i < tabla.num_paginas; ++i) {
    //    printf("Página %d -> Marco %d\n", tabla.paginas[i].pagina_id, tabla.paginas[i].numero_marco);
    //}


    //3.- Actualiza
    int marcos_necesarios=3;
    actualizarBitmap(marcos_necesarios);
    //Liberar todo, especialmente los malloc :V
   // liberarTablaPaginas(tabla);
}


  
//HACER FUNCION QUE DADO UN PID SE ENCUENTRE LA TABLA DE PAGINA ASOCIADA falta implementar

