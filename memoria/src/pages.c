#include <pages.h>


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

//busca el siguiente frame libre de memoria
int buscar_proximo_frame_libre (bool* frames){
    int i=0;
    for (i=0; frames[i] != false ; i++);
    return i;
}

//actualiza el bitmap y devuelve un array con los numeros de marcos asignados
int* actualizarBitmap(int marcos_necesarios) {
    pthread_mutex_lock(&memory.mutex_frames_ocupados);
    int* marcos_asignados = malloc(sizeof(int) * marcos_necesarios);
    for (int i = 0; i < marcos_necesarios; ++i) {
        marcos_asignados[i] = asignarMarcoLibre();
        if (marcos_asignados[i] == -1) {
            perror("No free frames available");
            pthread_mutex_unlock(&memory.mutex_frames_ocupados);
            exit(EXIT_FAILURE);
        }
        /*
        int libre = buscar_proximo_frame_libre(memory.frames_ocupados);
        memory.frames_ocupados[libre] = true; 
        */
    }
    pthread_mutex_unlock(&memory.mutex_frames_ocupados);
    return marcos_asignados;
}

void escribirEnEspacioUsuario(const char* buffer, int tamano_proceso) {
    pthread_mutex_lock(&memory.mutex_espacio_usuario);
    memcpy(espacio_usuario, buffer, strlen(buffer) + 1); // +1 para incluir el carácter nulo de terminación
    // Avanzar el puntero del espacio de usuario
    //espacio_usuario = (char*)espacio_usuario + tamano_proceso;  //ESTO DEBERIA SER CIRCULAR, EL ESPACIO DE USUARIO ES LIMITADO, POSIBLE SOLUCION: % memory.memory_size
    espacio_usuario += tamano_proceso % memory.memory_size;
    pthread_mutex_unlock(&memory.mutex_espacio_usuario); 
    // Actualizar el bitmap con los marcos necesarios    
    int marcos_necesarios = calcularMarcosNecesarios(tamano_proceso, memory.page_size);
    actualizarBitmap(marcos_necesarios);
}  

// Función para crear la tabla de páginas de un proceso (y asignar marcos a cada página)
TablaPaginas* crearTablaPaginas(int pid, int tamano_proceso, int tamano_marco) {
    TablaPaginas* tabla = (TablaPaginas*)malloc(sizeof(TablaPaginas));  // Cambiar a puntero y usar malloc
    tabla->paginas = NULL;
    /*
    if (tabla == NULL) {
        perror("Failed to allocate memory for page table structure");
        exit(EXIT_FAILURE);
    }
    */

    int num_marcos = calcularMarcosNecesarios(tamano_proceso, tamano_marco);
    log_info(logger, "PID: %d - Tamaño del Proceso: %d - Número de marcos necesarios: %d marcos",pid,tamano_proceso, num_marcos);
    tabla->pid_tabla = pid;
    tabla->num_paginas = num_marcos;
    tabla->paginas = NULL;
    //tabla.paginas = (PaginaMemoria*)malloc(num_marcos * sizeof(PaginaMemoria));
    pthread_mutex_init(&tabla->mutex_tabla,NULL); //puede no ser necesario semaforo porque cada tabla con su pid.

    /*
    if (tabla->paginas == NULL) {
        perror("Failed to allocate memory for page table");
        free(tabla);  // Liberar memoria asignada a tabla antes de salir
        exit(EXIT_FAILURE);
    }
    */

    //aca no entra nunca:
    for (int i = 0; i < num_marcos; ++i) {
        int marco = asignarMarcoLibre();
        if (marco == -1) {
            perror("No free frames available");
            free(tabla->paginas);
            pthread_mutex_destroy(&tabla->mutex_tabla);
            free(tabla);  // Liberar memoria asignada a tabla antes de salir
            exit(EXIT_FAILURE);
        }
        tabla->paginas[i].pagina_id = i;
        tabla->paginas[i].numero_marco = marco;
    }

    // Agregar la tabla al diccionario de tablas de páginas
    char* s_pid_tabla = string_itoa(tabla->pid_tabla);
    dictionary_put(listaTablasDePaginas, s_pid_tabla , tabla);
    log_info(logger, "****Se registra la creación de la tabla de páginas***");
    log_info(logger, "PID: %d - Tabla de páginas creada - Tamaño: %d páginas", pid, num_marcos);
    //log_info(logger, "PID: %d - Tamaño: %d", pid, num_marcos);
    free(s_pid_tabla);
    return tabla;
}

void liberarTablaPaginas(TablaPaginas tabla) {
    pthread_mutex_destroy(&tabla.mutex_tabla);
    free(tabla.paginas);
}
//ESTA FUNCION ES SOLO PARA LAS PRUEBAS:
void mostrarContenidoMemoria(int tamano_proceso) {
    pthread_mutex_lock(&memory.mutex_espacio_usuario);
    printf("Contenido de la memoria:\n");
  
    for (int i = -tamano_proceso; i < 0 + 1; ++i) {
        printf("0x%p: %c\n", (void*)(espacio_usuario + i), *(char*)(espacio_usuario + i));
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
     TablaPaginas* tabla = crearTablaPaginas(pid, tamano_proceso, memory.page_size);

    // TablaPaginas tabla = crearTablaPaginas(pid, tamano_proceso, memory.page_size);
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
 /*
TablaPaginas* tablaDePaginasAsociada (int pid){
   return dictionary_get(listaTablasDePaginas,string_itoa(pid));
}
*/
TablaPaginas* tablaDePaginasAsociada(int pid) {
    char* pid_str = string_itoa(pid);
    if (pid_str == NULL) {
        log_error(logger, "Error al convertir el PID %d a cadena", pid);
        return NULL;
    }

    TablaPaginas* tabla = dictionary_get(listaTablasDePaginas, pid_str);
    if (tabla == NULL) {
        log_error(logger, "No se encontró la tabla de páginas para el PID %d (string PID: %s)", pid, pid_str);
    } else {
        log_info(logger, "Tabla de páginas encontrada para PID %d (string PID: %s)", pid, pid_str);
        log_info(logger, "Contenido de la tabla de páginas para PID %d: num_paginas: %d", pid, tabla->num_paginas);
        for (int i = 0; i < tabla->num_paginas; ++i) {
        log_info(logger, "Página %d -> Marco %d", tabla->paginas[i].pagina_id, tabla->paginas[i].numero_marco);
        }
    }
     
    free(pid_str); // Liberar la memoria asignada por string_itoa

    return tabla;
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
void copy_string(int source_df, int dest_df, int tamanio, int pid) {
    char* leido = malloc(sizeof(char)*tamanio);
    leerDeDireccionFisica3(source_df,tamanio,leido,pid);
    escribirEnDireccionFisica2(dest_df,leido,tamanio,pid);
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
    
        char* s_pid = string_itoa(pid); 
        dictionary_remove_and_destroy(listaTablasDePaginas, s_pid , (void(*)(void*)) destroy_page_table);
        free(s_pid);
    } 
    else 
        log_error(logger, "No se encontró la tabla de páginas para el PID %d", pid);
 }

void finish_process(int pid) {
    log_info(logger, "Inicio de finish_process para PID: %d", pid);
    
    TablaPaginas* tabla = tablaDePaginasAsociada(pid);
    log_info(logger, "Tabla de páginas no encontrada para PID: %d", pid);
    if (!tabla) {
        perror("Tabla de páginas not found for PID ");
        return;
    }
    int num_marcos = tabla->num_paginas;
    log_info(logger, "Destrucción de Tabla de Páginas - PID: %d - Cantidad de Páginas: %d (Tamaño de la Tabla)", pid, num_marcos);
    free_frame(tabla);
    log_info(logger, "Liberando tabla para PID: %d - Cantidad de Páginas: %d (Tamaño de la Tabla)", pid, num_marcos);
    free_TablaDePaginas(pid);
    log_info(logger, "Finalización de finish_process para PID: %d - Cantidad de Páginas: %d (Tamaño de la Tabla)", pid, num_marcos);
    
}
    


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

int ampliar_tamanio_proceso(TablaPaginas* tabla, int pid, int marcos_actuales, int marcos_necesarios_nuevos) {
    int marcos_a_asignar = marcos_necesarios_nuevos - marcos_actuales;

    if (!hayEspacioEnBitmap(marcos_a_asignar)) {
        log_error(logger, "Out Of Memory - No hay suficiente espacio en la memoria para ampliar el proceso PID %d", pid);
        return 0;
    }

    tabla->paginas = realloc(tabla->paginas, marcos_necesarios_nuevos * sizeof(PaginaMemoria));
    if (tabla->paginas == NULL) {
        log_error(logger, "Falló la reubicación de memoria para la tabla de páginas del proceso PID %d", pid);
        return 0;
    }

    int* marcos_nuevos = actualizarBitmap(marcos_a_asignar);
    int j = 0;


    for (int i = marcos_actuales, j=0; i < marcos_necesarios_nuevos; ++i , j++) {
        tabla->paginas[i].pagina_id = i;
        tabla->paginas[i].numero_marco = marcos_nuevos[j];
    }

    tabla->num_paginas = marcos_necesarios_nuevos;

    log_info(logger, "PID: %d - Tamaño actual: %d bytes - Tamaño a Ampliar: %d bytes", pid, marcos_necesarios_nuevos * memory.page_size, marcos_necesarios_nuevos * memory.page_size);
    log_info(logger, "Proceso PID %d ampliado a %d bytes (%d páginas)", pid, marcos_necesarios_nuevos * memory.page_size, marcos_necesarios_nuevos);
    free(marcos_nuevos);

    return 1;
}

int reducir_tamanio_proceso(TablaPaginas* tabla, int pid, int marcos_actuales, int marcos_necesarios_nuevos) {

    for (int i = marcos_actuales - 1; i >= marcos_necesarios_nuevos; --i) {
        int marco = tabla->paginas[i].numero_marco;
        liberarMarcoFisico(marco);
    }

    tabla->paginas = realloc(tabla->paginas, marcos_necesarios_nuevos * sizeof(PaginaMemoria));
    if (tabla->paginas == NULL && marcos_necesarios_nuevos > 0) {
        log_error(logger, "Falló la reubicación de memoria para la tabla de páginas del proceso PID %d", pid);
        return 0;
    }

    tabla->num_paginas = marcos_necesarios_nuevos;
    log_info(logger, "PID: %d - Tamaño actual: %d bytes - Tamaño a Reducir: %d bytes", pid, marcos_necesarios_nuevos * memory.page_size, marcos_necesarios_nuevos * memory.page_size);
    log_info(logger, "Proceso PID %d reducido a %d bytes (%d páginas)", pid, marcos_necesarios_nuevos * memory.page_size, marcos_necesarios_nuevos);

    return 1;
}

int resize_process(int pid, int nuevo_tamano) {
    // Obtener la tabla de páginas asociada al PID
    TablaPaginas* tabla = tablaDePaginasAsociada(pid);
    
    if (!tabla) {
        log_error(logger, "No se encontró la tabla de páginas para el PID %d", pid);
        return 0;
    }

    pthread_mutex_lock(&tabla->mutex_tabla);

    int tamano_actual = tabla->num_paginas * memory.page_size;
    int marcos_necesarios_nuevos = calcularMarcosNecesarios(nuevo_tamano, memory.page_size);
    int marcos_actuales = tabla->num_paginas;

    int resultado = 0;

    if (nuevo_tamano > tamano_actual)
        resultado = ampliar_tamanio_proceso(tabla, pid, marcos_actuales, marcos_necesarios_nuevos);
    if (nuevo_tamano < tamano_actual)
        resultado = reducir_tamanio_proceso(tabla, pid, marcos_actuales, marcos_necesarios_nuevos);

    pthread_mutex_unlock(&tabla->mutex_tabla);

    return resultado;
}

void liberarMarcoFisico(int marco) {
    // Marcar el marco como libre en el bitmap
    memory.frames_ocupados[marco] = false;

    // Opcional: Limpiar el contenido del marco en la memoria física (espacio_usuario)
    memset(espacio_usuario + (marco * memory.page_size), 0, memory.page_size);
}

char* leerDesdeEspacioUsuario(int direccion_fisica, int tamano, int pid) {
    // Obtener la tabla de páginas del proceso
    TablaPaginas* tabla = tablaDePaginasAsociada(pid);
    if (!tabla) {
        perror("Tabla de páginas no encontrada para el PID proporcionado");
        return NULL;
    }

    char* buffer = malloc(tamano + 1);  // +1 para el carácter nulo al final
    if (!buffer) {
        perror("Failed to allocate buffer for reading");
        return NULL;
    }

    int bytesLeidos = 0;
    while (bytesLeidos < tamano) {
        int numPagina = calcularNumeroPagina(direccion_fisica); //TO DO 
        int desplazamiento = calcularDesplazamiento(direccion_fisica);

        char* dirFisica = obtenerDireccionFisica(tabla->paginas[numPagina].numero_marco, desplazamiento);
        int bytesRestantesEnPagina = memory.page_size - desplazamiento;
        int bytesALeer = tamano - bytesLeidos < bytesRestantesEnPagina ? tamano - bytesLeidos : bytesRestantesEnPagina;

        memcpy(buffer + bytesLeidos, dirFisica, bytesALeer);

        bytesLeidos += bytesALeer;
        direccion_fisica += bytesALeer;
    }

    buffer[tamano] = '\0';  // Agregar el carácter nulo al final del buffer

    return buffer;
}



void escribirEnEspacioUsuario2(int direccion_fisica, char* datos, int tamanio, int pid) {
    // Obtener la tabla de páginas del proceso
    TablaPaginas* tabla = tablaDePaginasAsociada(pid);
    if (!tabla) {
        perror("Tabla de páginas no encontrada para el PID proporcionado");
        return;
    }

    int bytesEscritos = 0;
    while (bytesEscritos < tamanio) {
        int numPagina = calcularNumeroPagina(direccion_fisica); 
        int desplazamiento = calcularDesplazamiento(direccion_fisica);

        char* dirFisica = obtenerDireccionFisica(tabla->paginas[numPagina].numero_marco, desplazamiento);
        int bytesRestantesEnPagina = memory.page_size - desplazamiento;
        int bytesAEscribir = tamanio - bytesEscritos < bytesRestantesEnPagina ? tamanio - bytesEscritos : bytesRestantesEnPagina;

        pthread_mutex_lock(&memory.mutex_espacio_usuario);
        memcpy(dirFisica, datos + bytesEscritos, bytesAEscribir);
        pthread_mutex_unlock(&memory.mutex_espacio_usuario);

        bytesEscritos += bytesAEscribir;
        direccion_fisica += bytesAEscribir;
    }

    log_info(logger, "PID: %d - Acción: ESCRIBIR - Dirección física: %p - Tamaño: %d", pid, (void*)direccion_fisica, tamanio);
}

// Escritura y Lectura contigua

uint32_t escribirEnDireccionFisica(uint32_t dirFisica, char* txt, uint32_t size, uint32_t pid) 
{
    uint32_t operation_status = 0;
    pthread_mutex_lock(&(memory.mutex_espacio_usuario));
    char* ptr = &(espacio_usuario[dirFisica]);
    if((dirFisica + size) < memory.memory_size) 
    {
        for(int i = 0; i < size; i++) 
        {
            *(ptr + i) = txt[i];
        }
        operation_status = 1;
    } else {
        log_error(logger, "ERROR WRITING IN MEMORY");
    }
    pthread_mutex_unlock(&(memory.mutex_espacio_usuario));

    return operation_status;
}

char* leerDeDireccionFisica(uint32_t dirFisica, uint32_t size, uint32_t pid) 
{
    char* palabraLeida = malloc(size);
    pthread_mutex_lock(&(memory.mutex_espacio_usuario));
    if((dirFisica + size) < memory.memory_size) {
        for(int i = 0; i < size; i++) 
        {
            *(palabraLeida + i) = espacio_usuario[i];
        }
    } else {
        log_error(logger, "ERROR WRITING IN MEMORY");
    }
    pthread_mutex_unlock(&(memory.mutex_espacio_usuario));

    if(strlen(palabraLeida) != size) 
    {
        free(palabraLeida);
        return "";
    } else {
        return palabraLeida;
    }
}


//lectura y escritura no contigua:

uint32_t escribirEnDireccionFisica2(uint32_t dirFisica, char* txt, uint32_t size, uint32_t pid) 
{
    // Obtener la tabla de páginas del proceso
    TablaPaginas* tabla = tablaDePaginasAsociada(pid);
    if (!tabla) {
        log_error(logger, "Tabla de páginas no encontrada para el PID %d", pid);
        return -1;
    }

    uint32_t page_size = memory.page_size;
    int marco_inicial = dirFisica / page_size;
    uint32_t offset_dentro_marco = dirFisica % page_size; // Desplazamiento dentro del primer marco

    // Ordenar los marcos de menor a mayor
    int num_paginas = tabla->num_paginas;
    int* marcos = malloc(num_paginas * sizeof(int));
    int count_marcos = 0;
    for (int i = 0; i < num_paginas; i++) {
        if (count_marcos > 0 || tabla->paginas[i].numero_marco == marco_inicial) {
            marcos[count_marcos] = tabla->paginas[i].numero_marco;
            log_info(logger,"Acceso a tabla de paginas - PID: %d - Pagina: %d - Marco: %d",pid,tabla->paginas[i].pagina_id,tabla->paginas[i].numero_marco);
            count_marcos++;
        }
    }

    uint32_t bytes_escritos = 0;
    pthread_mutex_lock(&(memory.mutex_espacio_usuario));

    for (int i = 0; bytes_escritos < size && i < count_marcos; i++) {
        int marco = marcos[i];
        char* dir_fisica_real = espacio_usuario + (marco * page_size) + offset_dentro_marco;

        uint32_t bytes_a_escribir = page_size - offset_dentro_marco;
        if (bytes_a_escribir > (size - bytes_escritos)) {
            bytes_a_escribir = size - bytes_escritos;
        }

        memcpy(dir_fisica_real, txt + bytes_escritos, bytes_a_escribir);
        bytes_escritos += bytes_a_escribir;

        // Mover al siguiente marco
        offset_dentro_marco = 0; // Después del primer marco, siempre escribimos desde el inicio del siguiente marco
    }

    pthread_mutex_unlock(&(memory.mutex_espacio_usuario));
    free(marcos);

    return bytes_escritos;
}

int leerDeDireccionFisica3(uint32_t dirFisica, uint32_t size, char* buffer, uint32_t pid) 
{
    // Obtener la tabla de páginas del proceso
    TablaPaginas* tabla = tablaDePaginasAsociada(pid);
    if (!tabla) {
        log_error(logger, "Tabla de páginas no encontrada para el PID %d", pid);
        return -1;
    }

    uint32_t page_size = memory.page_size;
    int marco_inicial = dirFisica / page_size;
    uint32_t offset_dentro_marco = dirFisica % page_size; // Desplazamiento dentro del primer marco

    // Ordenar los marcos de menor a mayor
    int num_paginas = tabla->num_paginas;
    int* marcos = malloc(num_paginas * sizeof(int));
    int count_marcos = 0;
    for (int i = 0; i < num_paginas; i++) {
        if (count_marcos > 0 || tabla->paginas[i].numero_marco == marco_inicial) {
            marcos[count_marcos] = tabla->paginas[i].numero_marco;
            log_info(logger,"Acceso a tabla de paginas - PID: %d - Pagina: %d - Marco: %d",pid,tabla->paginas[i].pagina_id,tabla->paginas[i].numero_marco);
            count_marcos++;
        }
    }

    uint32_t bytes_leidos = 0;
    pthread_mutex_lock(&(memory.mutex_espacio_usuario));

    log_info(logger,"PID: %d - Accion: %s - Direccion fisica: %d - Tamaño: %d",pid,"LEER",dirFisica,size);
    
    for (int i = 0; bytes_leidos < size && i < count_marcos; i++) {
        int marco = marcos[i];
        char* dir_fisica_real = espacio_usuario + (marco * page_size) + offset_dentro_marco;

        uint32_t bytes_a_leer = page_size - offset_dentro_marco;
        if (bytes_a_leer > (size - bytes_leidos)) {
            bytes_a_leer = size - bytes_leidos;
        }

        memcpy(buffer + bytes_leidos, dir_fisica_real, bytes_a_leer);
        bytes_leidos += bytes_a_leer;

        // Mover al siguiente marco
        offset_dentro_marco = 0; // Después del primer marco, siempre leemos desde el inicio del siguiente marco
    }

    pthread_mutex_unlock(&(memory.mutex_espacio_usuario));
    free(marcos);

    return bytes_leidos;
}




int leerDeDireccionFisica3Old(uint32_t dirFisica, uint32_t size, char* buffer, uint32_t pid) 
{
    pthread_mutex_lock(&(memory.mutex_espacio_usuario));

    memcpy(buffer, espacio_usuario + dirFisica, size);

    pthread_mutex_unlock(&(memory.mutex_espacio_usuario));

    return 1;


    /*uint32_t operation_status = 0;
    uint32_t page_size = memory.page_size;

    /* Posible mejora de espacio de usuario
    // Obtener el tamaño total del espacio de usuario
    uint32_t espacio_usuario_size = obtener_tam_espacio_usuario();

    // Validar que la dirección física y el tamaño solicitado no excedan los límites
    if (dirFisica + size > espacio_usuario_size) {
        log_error(logger, "La dirección física y el tamaño solicitado exceden los límites de la memoria para el PID %d", pid);
        return operation_status;
    }

    pthread_mutex_lock(&(memory.mutex_espacio_usuario));

    uint32_t current_offset = 0;
    while (current_offset < size) {
        // Calcular la página actual y el desplazamiento dentro de esa página
        uint32_t page_offset = (dirFisica + current_offset) % page_size;

        // Calcular la cantidad de datos que se pueden leer del marco actual
        uint32_t to_read = page_size - page_offset;
        if (to_read > (size - current_offset)) {
            to_read = size - current_offset;
        }

        // Realizar la lectura del espacio de usuario
        // memcpy(buffer + current_offset, espacio_usuario + (marco * page_size) + page_offset, to_read); -> con marco asociado
         memcpy(buffer + current_offset, espacio_usuario + dirFisica + current_offset, to_read);
        log_debug(logger, "Actual buffer read: %s", buffer + current_offset);
        current_offset += to_read;
    }

    pthread_mutex_unlock(&(memory.mutex_espacio_usuario));

    if (current_offset == size) {
        operation_status = 1;
    } else {
        log_error(logger, "Error al leer desde la memoria física para el PID %d", pid);
    }

    return operation_status;*/
}