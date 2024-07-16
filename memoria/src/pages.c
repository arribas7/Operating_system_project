#include <pages.h>
#include <memoria.h>

char* espacio_usuario;
t_dictionary* listaTablasDePaginas;

/********************************init Paging*************************************/

void logMemoryInfo() {
    int pageSize = memory.page_size;
    int frameCount = memory.memory_size / pageSize;
    log_info(logger, "TAMAÑO MEMORIA TOTAL: %d bytes - Memory has %d frames of %d bytes", memory.memory_size, frameCount, pageSize);
}

void verifyPagesize() {
   int pageSize = memory.page_size;
   int totalMemorySize = memory.memory_size;
    if (totalMemorySize % pageSize == 0) {
      // printf("Multiplicity pageSize ok\n"); //si está todo bien que no reporte nada.
   } else {
       printf("Multiplicity pageSize failed.\n");
       exit(EXIT_FAILURE); // Terminar el programa si falla la verificación
   }
}


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
    verifyPagesize();
    logMemoryInfo();
/* ----------------First Structure ---------------- */
     inicializarEspacioUsuario();
 /* ----------------Second Structure ---------------- */ 
     inicializarBitmap();
 /* ----------------Third Structure ---------------- */
 // inicializarTabla(); no creo q sea necesario, reviso
    inicializarListaDeTablasDePaginas();
  return 1;
    
}


/********************************Handle Paging*************************************/
int calcularMarcosNecesarios(int tamano_proceso, int tamano_marco) {
     //calculamos la cantidad de marcos para el proceso redondeando siempre para arriba
    int num_marcos = ceil((double)tamano_proceso / tamano_marco); 
    //printf("Número de marcos necesarios: %d\n", num_marcos); just for check. devuelve bien!
    // Suponiendo que tamano_proceso = 41 bytes y tamano_marco = 32 bytes,
    // Se necesitan ceil(41 / 32) = 2 marcos. redondea para arriba!
    return num_marcos;
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
    // Avanzar el puntero del espacio de usuario
    espacio_usuario = (char*)espacio_usuario + tamano_proceso;  //ESTO DEBERIA SER CIRCULAR, EL ESPACIO DE USUARIO ES LIMITADO, POSIBLE SOLUCION: % memory.memory_size
    pthread_mutex_unlock(&memory.mutex_espacio_usuario); 
    // Actualizar el bitmap con los marcos necesarios    
    int marcos_necesarios = calcularMarcosNecesarios(tamano_proceso, memory.page_size);
    actualizarBitmap(marcos_necesarios);
}   
/*
void escribirEnEspacioUsuarioIndex(const char* buffer, int tamano_proceso) {
    pthread_mutex_lock(&memory.mutex_espacio_usuario);
    memcpy(espacio_usuario, buffer, strlen(buffer) + 1); // +1 para incluir el carácter nulo de terminación
    espacio_usuario = (char*)espacio_usuario + tamano_proceso;  //ESTO DEBERIA SER CIRCULAR, EL ESPACIO DE USUARIO ES LIMITADO, POSIBLE SOLUCION: % memory.memory_size
    pthread_mutex_unlock(&memory.mutex_espacio_usuario);     
     int marcos_necesarios = calcularMarcosNecesarios(tamano_proceso, memory.page_size);
    actualizarBitmap(marcos_necesarios);
}  
*/ 
// Función para crear la tabla de páginas de un proceso (y asignar marcos a cada página)
TablaPaginas crearTablaPaginas(int pid, int tamano_proceso, int tamano_marco) {
    TablaPaginas tabla;
    int num_marcos = calcularMarcosNecesarios(tamano_proceso, tamano_marco);
    log_info(logger, "PID: %d - Tamaño del Proceso: %d - Número de marcos necesarios: %d marcos",pid,tamano_proceso, num_marcos);
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
    log_info(logger, "****Se registra la creación de la tabla de páginas***");
    log_info(logger, "PID: %d - Tabla de páginas creada - Tamaño: %d páginas", pid, num_marcos);
    log_info(logger, "PID: %d - Tamaño: %d", pid, num_marcos);

    return tabla;
}

void liberarTablaPaginas(TablaPaginas tabla) {
//void liberarTablaPaginas(TablaPaginas tabla) {
    pthread_mutex_destroy(&tabla.mutex_tabla);
    free(tabla.paginas);
}

//ESTA FUNCION ES SOLO PARA LAS PRUEBAS:
void mostrarContenidoMemoria(int tamano_proceso) {
    pthread_mutex_lock(&memory.mutex_espacio_usuario);
    printf("Contenido de la memoria:\n");
  
    for (int i = -tamano_proceso; i < 0 + 1; ++i) {
        printf("%p: %c\n", (void*)(espacio_usuario + i), *(char*)(espacio_usuario + i));
    }
    pthread_mutex_unlock(&memory.mutex_espacio_usuario);
}


void handle_paging(const char* buffer, uint32_t tamano_proceso, int pid) {
    int tamano_marco=memory.page_size;
    int marcos_necesarios = calcularMarcosNecesarios(tamano_proceso,tamano_marco);
     if (!hayEspacioEnBitmap(marcos_necesarios)) {
        perror("No hay suficiente espacio en la memoria");
        exit(EXIT_FAILURE);
    }
     TablaPaginas tabla = crearTablaPaginas(pid, tamano_proceso, memory.page_size);
     log_info(logger, "PID: %d - Acción: ESCRIBIR - Direccion física: %p - Tamaño: %d", pid, (void*)espacio_usuario, tamano_proceso);

    escribirEnEspacioUsuario(buffer, tamano_proceso);
    actualizarBitmap(marcos_necesarios);
    log_info(logger, "PID: %d - Proceso de escritura completado - Tamaño total escrito: %d bytes", pid, tamano_proceso);
    mostrarContenidoMemoria(tamano_proceso);
    char* nueva_direccion = (char*)espacio_usuario;
    log_info(logger, "Nueva posición de la dirección física: %p", (void*)nueva_direccion);
    //liberarTablaPaginas(tabla);
}

/********************************Sending Frame*************************************/
  
//Dado un numero de pagina y una tabla de paginas asociada, retorna el marco asociado
int marcoAsociado(int numero_pagina, TablaPaginas* tablaAsociada){
   int i=0;
   int marco = -1; //en caso no find nunca se modificara y devolvera este
   for (i=0; i<tablaAsociada->num_paginas ; i++){
       if(numero_pagina == tablaAsociada->paginas[i].pagina_id){
           marco = tablaAsociada->paginas[i].numero_marco;
            // Access to the page table
           log_info(logger, "Acceso a Tabla de Páginas - PID: %d - Página: %d - Marco: %d", tablaAsociada->pid_tabla, numero_pagina, marco);
           log_info(logger, "PID: %d - Pagina: %d - Marco: %d", tablaAsociada->pid_tabla, numero_pagina, marco);
           break;
       }
   }
  return marco;
 }

void enviar_marco(int pagina, int pid, int cliente_fd){
    TablaPaginas* tablaAsociada = tablaDePaginasAsociada(pid);
    int marco = marcoAsociado(pagina,tablaAsociada);
    enviar_mensaje(string_itoa(marco),cliente_fd);
}



/********************************COPY STRING**************************************/
 //Dado un pid se encuentra la tabla de paginas asociada
TablaPaginas* tablaDePaginasAsociada (int pid){
   return dictionary_get(listaTablasDePaginas,string_itoa(pid));
}


int calcularNumeroPagina(int direccion_fisica) {
   return direccion_fisica / memory.page_size;
}

int calcularDesplazamiento(int direccion_fisica) {
   return direccion_fisica % memory.page_size;
   //Suponiendo que el tamaño de la página es 4096 bytes (4 KB). La dirección física es 8195.
   //desplazamiento = 8195 % 4096 = 3;  Es el resto!
}


char* obtenerDireccionFisica(int marco, int desplazamiento) {
   return (char*)espacio_usuario + (marco * memory.page_size) + desplazamiento;
   //calcula la dirección física basada en el marco y el desplazamiento dentro de ese marco, 
   //Suponiendo que el espacio_usuario apunta a la dirección 0x1000(dirección base del espacio de usuario)
   // marco = 2 y el desplazamiento = 5 
   //Se sabe que TAM_PAGINA=32
   //direccionFisica = 0x1000 + (2 * 32) + 5  = 0x1045
}


// Obtener la dirección física completa
char* obtenerDireccionFisicafull(int direccion_fisica, TablaPaginas* tablaAsociada) {
    int marco = marcoAsociado(direccion_fisica, tablaAsociada);
    if (marco == -1) return NULL;
    int desplazamiento = calcularDesplazamiento(direccion_fisica);
    return obtenerDireccionFisica(marco, desplazamiento);
}

void copy_string(int direc_fis_1, int pid, int direc_fis_2, int cliente_fd, t_config *config) {
   TablaPaginas* tablaAsociada = tablaDePaginasAsociada(pid);
   if (!tablaAsociada) {
       perror("Tabla de páginas no encontrada para el PID proporcionado");
       return;
   }
   char *dir_fisica_1 = obtenerDireccionFisicafull(direc_fis_1, tablaAsociada);
   char *dir_fisica_2 = obtenerDireccionFisicafull(direc_fis_2, tablaAsociada);
  
   if (!dir_fisica_1 || !dir_fisica_2) {
       perror("No se pudo obtener la dirección física");
       return;
   }

   pthread_mutex_lock(&memory.mutex_espacio_usuario);
   strcpy(dir_fisica_2, dir_fisica_1);
   pthread_mutex_unlock(&memory.mutex_espacio_usuario);
}
/********************************FINISH PROCESS**************************************/

//Solo actualiza bitmap (no se sobre-escribe nada en el espacio de usuario)
void free_frame(TablaPaginas* tabla) {
    pthread_mutex_lock(&memory.mutex_frames_ocupados);
    for (int i = 0; i < tabla->num_paginas; ++i) {
        int marco = tabla->paginas[i].numero_marco;
        memory.frames_ocupados[marco] = false;
    }
    pthread_mutex_unlock(&memory.mutex_frames_ocupados);
}

void free_TablaDePaginas(int pid) {
     TablaPaginas* tabla = tablaDePaginasAsociada(pid);
    if (tabla != NULL) {
        int num_marcos = tabla->num_paginas; // Obtener el número de marcos
        // Registrar el evento de destrucción de la tabla de páginas
        log_info(logger, "Destrucción de Tabla de Páginas - PID: %d - Cantidad de Páginas: %d (Tamaño de la Tabla)", pid, num_marcos);
        log_info(logger, "PID: %d - Tamaño: %d", pid, num_marcos);

    dictionary_remove_and_destroy(listaTablasDePaginas, string_itoa(pid), (void(*)(void*)) liberarTablaPaginas);
      } else {
        log_error(logger, "No se encontró la tabla de páginas para el PID %d", pid);
     }
 }

void finish_process(int pid) {
    TablaPaginas* tabla = tablaDePaginasAsociada(pid);
    if (!tabla) {
        perror("Tabla de páginas not found for PID ");
        return;
    }

    free_frame(tabla);
    free_TablaDePaginas(pid);
}
/**********************************************************************/
 //Estaria para ponerlo en algun lado:
    //for (int i = 0; i < tabla.num_paginas; ++i) {
    //    printf("Página %d -> Marco %d\n", tabla.paginas[i].pagina_id, tabla.paginas[i].numero_marco);
    //}

void escribir_en_direcc_fisica(int pid,int df,int val){
    TablaPaginas* tablaAsociada = tablaDePaginasAsociada(pid);
    char *dir_fisica = obtenerDireccionFisicafull(df, tablaAsociada);
  
   if (!dir_fisica) {
       perror("No se pudo obtener la dirección física");
       return;
   }

   pthread_mutex_lock(&memory.mutex_espacio_usuario);
   strcpy(dir_fisica, string_itoa(val));
   pthread_mutex_unlock(&memory.mutex_espacio_usuario);
}

char* obtener_valor(int pid,int df){
    TablaPaginas* tablaAsociada = tablaDePaginasAsociada(pid);
    return obtenerDireccionFisicafull(df, tablaAsociada);
}