#include "instructions.h"

 char *listaComandos[] = {
    [_SET] = "SET",
    [_MOV_IN] = "MOV_IN",
    [_MOV_OUT] = "MOV_OUT",
    [_SUM] = "SUM",
    [_SUB] = "SUB",
    [_JNZ] = "JNZ",
    [_RESIZE] = "RESIZE",
    [_COPY_STRING] = "COPY_STRING",
    [_WAIT] = "WAIT",
    [_SIGNAL] = "SIGNAL",
    [_IO_GEN_SLEEP] = "IO_GEN_SLEEP",
    [_IO_STDIN_READ] = "IO_STDIN_READ",
    [_IO_STDOUT_WRITE] = "IO_STDOUT_WRITE",
    [_IO_FS_CREATE] = "IO_FS_CREATE",
    [_IO_FS_DELETE] = "IO_FS_DELETE",
    [_IO_FS_TRUNCATE] = "IO_FS_TRUNCATE",
    [_IO_FS_WRITE] = "IO_FS_WRITE",
    [_IO_FS_READ] = "IO_FS_READ",
    [_EXIT] = "EXIT"
};

int buscar(char *elemento, char **lista) //buscar un elemento en una lista
{
    int i = 0;
    // for (; strcmp(lista[i], elemento) && i < string_array_size(lista); i++);
    while (i <= string_array_size(lista))
    {
        if (i < string_array_size(lista))
            if (!strcmp(elemento, lista[i]))
                return i;
        //    debug ("%s", lista[i]);
        i++;
    }
    return (i > string_array_size(lista)) ? -1 : i;
}

int init_reg_proceso_actual(void){
    reg_proceso_actual = (t_reg_cpu *)malloc(sizeof(t_reg_cpu));
    if (reg_proceso_actual == NULL) {
        fprintf(stderr, "Error al asignar memoria\n");
        return -1;
    }

    reg_proceso_actual->AX = 0;
    reg_proceso_actual->BX = 0;
    reg_proceso_actual->CX = 0;
    reg_proceso_actual->DI = 0;
    reg_proceso_actual->EAX = 0;
    reg_proceso_actual->EBX = 0;
    reg_proceso_actual->ECX = 0;
    reg_proceso_actual->EDX = 0;
    reg_proceso_actual->PC = 0;
    reg_proceso_actual->SI = 0;

    return 1;
}

int valueOfReg (char* reg){
    if (strcmp(reg, "AX") == 0)
        return reg_proceso_actual->AX;
    if (strcmp(reg, "BX") == 0)
        return reg_proceso_actual->BX;
    if (strcmp(reg, "CX") == 0)
        return reg_proceso_actual->CX;
    if (strcmp(reg, "EAX") == 0)
        return reg_proceso_actual->EAX;
    if (strcmp(reg, "EBX") == 0)
        return reg_proceso_actual->EBX;
    if (strcmp(reg, "ECX") == 0)
        return reg_proceso_actual->ECX;
    if (strcmp(reg, "EDX") == 0)
        return reg_proceso_actual->EDX;

    return 0;
}

void recibir_instruccion(int socket_cliente)
{
    int size;
    instruccion_actual = recibir_buffer(&size, socket_cliente);
    log_info(logger, "Before fetching instruction.. %s", instruccion_actual);
}


void fetch(t_pcb *pcb)
{
    t_buffer *buffer = malloc(sizeof(t_buffer));
    serialize_pcb(pcb, buffer);

    t_paquete *paquete = crear_paquete(PC);
    agregar_a_paquete(paquete, buffer->stream, buffer->size);

    enviar_paquete(paquete, conexion_mem);
    eliminar_paquete(paquete);

    recibir_operacion(conexion_mem);   // sera MENSAJE DESDE MEMORIA (LA INSTRUCCION A EJECUTAR) //PROBAR SI FUNCIONA SIN ESTA LINEA YA QUE LA INSTRUCCION VIENE EN UN MENSAJE
    recibir_instruccion(conexion_mem); // LA INSTRUCCION

    //“PID: <PID> - FETCH - Program Counter: <PROGRAM_COUNTER>”
    log_info(logger, "PID: <%d> - <%s> - Program Counter: <%d>", pcb->pid, "FETCH", pcb->pc);
}

void decode(t_pcb *pcb)
{
    instr_decode = string_n_split(instruccion_actual, 4, " ");
    cant_parametros = string_array_size(instr_decode) - 1;
    instruccion_decodificada = buscar(instr_decode[0], listaComandos);
    log_info(logger, "Decode instruction fetched.. %s", listaComandos[instruccion_decodificada]);
    /*
    instr_decode = strtok(instruccion_actual, " ");
    instruccion_decodificada = buscar(instr_decode,listaComandos);
    log_info(logger, "Decode instruction fetched.. %s", instruccion_actual);
    */
}

void execute(t_pcb *pcb)
{
    pcb_en_ejecucion = pcb;
    switch(cant_parametros){
        case 0:
            log_info(logger, "PID: <%d> - Ejecutando: <%s> ", pcb->pid, listaComandos[instruccion_decodificada]);
        break;
        case 1:
            log_info(logger, "PID: <%d> - Ejecutando: <%s> - <%s>", pcb->pid, listaComandos[instruccion_decodificada], instr_decode[1]);
        break;
        case 2:
            log_info(logger, "PID: <%d> - Ejecutando: <%s> - <%s>, <%s>", pcb->pid, listaComandos[instruccion_decodificada], instr_decode[1], instr_decode[2]);
        break;
        case 3:
            log_info(logger, "PID: <%d> - Ejecutando: <%s> - <%s>, <%s>, <%s>", pcb->pid, listaComandos[instruccion_decodificada], instr_decode[1], instr_decode[2], instr_decode[3]);
        break;
    } 

    switch (instruccion_decodificada)
    {
        case _SET:
            set(instr_decode[1],instr_decode[2]);
        break;
        case _MOV_IN:
            mov_in(instr_decode[1],instr_decode[2]);
        break;
        case _MOV_OUT:
            mov_out(instr_decode[1],instr_decode[2]);
        break;
        case _SUM:
            sum(instr_decode[1],instr_decode[2]);
        break;
        case _SUB:
            sub(instr_decode[1],instr_decode[2]);
        break;
        case _JNZ:
            jnz(instr_decode[1],instr_decode[2]);
        break;
        case _COPY_STRING:
            copy_string(instr_decode[1]);
        break;
        case _WAIT:
            wait(instr_decode[1]);
        break;
        case _RESIZE:
            resize(instr_decode[1]);
        break;   
        case _SIGNAL:
            inst_signal(instr_decode[1]);
        break;
        case _IO_GEN_SLEEP:
            io_gen_sleep(instr_decode[1],instr_decode[2]);
        break;
        case _IO_STDIN_READ:
            io_stdin_read(instr_decode[1],instr_decode[2],instr_decode[3]);
        break;
        case _IO_STDOUT_WRITE:
            io_stdin_write(instr_decode[1],instr_decode[2],instr_decode[3]);
        break;
        case _IO_FS_CREATE:
            io_fs_create(instr_decode[1],instr_decode[2]);
        break;
        case _IO_FS_DELETE:
            io_fs_delete(instr_decode[1],instr_decode[2]);
        break;
        case _IO_FS_TRUNCATE:
            io_fs_truncate(instr_decode[1],instr_decode[2],instr_decode[3]);
        break;
        case _IO_FS_WRITE:
            io_fs_write(instr_decode[1],instr_decode[2],instr_decode[3],instr_decode[4],instr_decode[5]); 
        break;
        case _IO_FS_READ:
            io_fs_read(instr_decode[1],instr_decode[2],instr_decode[3],instr_decode[4],instr_decode[5]);
        break;
        case _EXIT:
            exxxit(pcb_en_ejecucion);
        break;
        default:
        break;
    }
}

void check_interrupt (void){


    //t_paquete* interrupt = crear_paquete(QUANTUM_FINISHED); //POR EJEMPLO Q FINISHED
    t_buffer* buffer = malloc(sizeof(t_buffer));
    serialize_pcb(pcb_en_ejecucion, buffer);



}



//INSTRUCTIONS:

void set(char* registro, char* valor){

    if (strcmp(registro, "AX") == 0)
        reg_proceso_actual->AX = (uint8_t)atoi(valor);
    if (strcmp(registro, "BX") == 0) 
        reg_proceso_actual->BX = atoi(valor);
    if (strcmp(registro, "CX") == 0) 
        reg_proceso_actual->CX = atoi(valor);
    if (strcmp(registro, "EAX") == 0)
        reg_proceso_actual->EAX = atoi(valor);
    if (strcmp(registro, "EBX") == 0)
        reg_proceso_actual->EBX = atoi(valor);
    if (strcmp(registro, "ECX") == 0)
        reg_proceso_actual->ECX = atoi(valor);
    if (strcmp(registro, "EDX") == 0)
        reg_proceso_actual->EDX = atoi(valor);
    if (strcmp(registro, "SI") == 0)
        reg_proceso_actual->SI = atoi(valor);
    if (strcmp(registro, "DI") == 0)
        reg_proceso_actual->DI = atoi(valor);

    log_info(logger, "%s in actual process: %d", registro, reg_proceso_actual->AX); //funciona
}

void mov_in(char* registro, char* logicalAddress){
    //int valor = 0; //quitar luego de hacer el siguiente TO DO
    int fisicalAddr = mmu(logicalAddress);
    
    //request to mem
    //int valor = requestRegToMem(fisicalAddr); //TO DO
    int valor = requestRegToMem(fisicalAddr);

    if (strcmp(registro, "AX") == 0)
        reg_proceso_actual->AX = valor;
    if (strcmp(registro, "BX") == 0) 
        reg_proceso_actual->BX = valor;
    if (strcmp(registro, "CX") == 0) 
        reg_proceso_actual->CX = valor;
    if (strcmp(registro, "EAX") == 0)
        reg_proceso_actual->EAX = valor;
    if (strcmp(registro, "EBX") == 0)
        reg_proceso_actual->EBX = valor;
    if (strcmp(registro, "ECX") == 0)
        reg_proceso_actual->ECX = valor;
    if (strcmp(registro, "EDX") == 0)
        reg_proceso_actual->EDX = valor;
}

void mov_out(char* logicalAddr, char* reg){

    int valor;
    int fisicalAddress = mmu(logicalAddr);

    if (strcmp(reg, "AX") == 0)
        valor = reg_proceso_actual->AX;
    if (strcmp(reg, "BX") == 0) 
        valor = reg_proceso_actual->BX;
    if (strcmp(reg, "CX") == 0) 
        valor = reg_proceso_actual->CX;
    if (strcmp(reg, "EAX") == 0)
        valor = reg_proceso_actual->EAX;
    if (strcmp(reg, "EBX") == 0)
        valor = reg_proceso_actual->EBX;
    if (strcmp(reg, "ECX") == 0)
        valor = reg_proceso_actual->ECX;
    if (strcmp(reg, "EDX") == 0)
        valor = reg_proceso_actual->EDX;

    putRegValueToMem(fisicalAddress,valor);
}

void io_gen_sleep(char* interfaz, char* job_unit){

    t_paquete* peticion = crear_paquete(IO_GEN_SLEEP); //this opcode receive in KERNEL
    int tamInterfaz = string_length(interfaz);
    int tamJobUnit = string_length(job_unit);
    t_instruction* IO = new_instruction_IO(pcb_en_ejecucion->pid,interfaz,job_unit);

    t_buffer *buffer = malloc(sizeof(t_buffer));
    serializar_instruccion_IO(IO,buffer);

    agregar_a_paquete(peticion, buffer->stream, buffer->size);

    enviar_paquete(peticion, cliente_fd);
    eliminar_paquete(peticion);

    //log_info(logger, "PID: <%d> - Accion: <%s> - IO: <%s>", pcb_en_ejecucion->pid, "IO_GEN_SLEEP", interfaz);
    /*
    recibir_operacion(cliente_fd);
    recibir_mensaje(cliente_fd); //receive ack from kermel
    */
}

void sum(char* destReg, char* origReg){                 

    int valor1,valor2,suma;

    valor1 = valueOfReg(destReg);
    valor2 = valueOfReg(origReg);

    suma = valor1 + valor2;

    set(destReg,string_itoa(suma));  
}

void sub(char* destReg, char* origReg){

    int valor1,valor2,resta;

    valor1 = valueOfReg(destReg);
    valor2 = valueOfReg(origReg);

    resta = valor1 - valor2;

    set(destReg,string_itoa(resta));   
}

void jnz(char* reg, char* inst){
    //log_info(logger, "PC in actual process before: %d", reg_proceso_actual->PC); //funciona

    if(valueOfReg(reg))
        reg_proceso_actual->PC = atoi(inst);

    //log_info(logger, "PC in actual process after: %d", reg_proceso_actual->PC); //funciona probe init reg proceso actual con AX = 1
}
//COPY_STRING:

void copy_string (char* tamanio){
    t_paquete* copy_string_paq = crear_paquete(COPY_STRING);
    t_buffer* buffer = malloc(sizeof(t_buffer));

    t_copy_string* copy_string = new_copy_string(atoi(tamanio));

    serializar_copy_string(copy_string,buffer);

    agregar_a_paquete(copy_string_paq,buffer->stream,buffer->size);
    enviar_paquete(copy_string_paq,conexion_mem);

    eliminar_paquete(copy_string_paq);
}

void serializar_copy_string(t_copy_string* copy_string, t_buffer* buffer){
    buffer->offset = 0;
    size_t size = sizeof(u_int32_t) + sizeof(int) * 3;
    buffer->size = size;
    buffer->stream = malloc(size);

    //serializo:
    memcpy(buffer->stream + buffer->offset, &(copy_string->pid), sizeof(u_int32_t));
    buffer->offset += sizeof(u_int32_t);

    memcpy(buffer->stream + buffer->offset, &(copy_string->tamaño), sizeof(int));
    buffer->offset += sizeof(int);

    memcpy(buffer->stream + buffer->offset, &(copy_string->fisical_si), sizeof(int));
    buffer->offset += sizeof(int);

    memcpy(buffer->stream + buffer->offset, &(copy_string->fisical_di), sizeof(int));
    buffer->offset += sizeof(int);
}

t_copy_string* deserializar_copy_string(void* stream){
    t_copy_string* copy_string = malloc(sizeof(t_copy_string));
    int offset = 0;

    memcpy(&(copy_string->pid), stream + offset, sizeof(u_int32_t));
    offset += sizeof(u_int32_t);

    memcpy(&(copy_string->tamaño), stream + offset, sizeof(int));
    offset += sizeof(int);

    memcpy(&(copy_string->fisical_si), stream + offset, sizeof(int));
    offset += sizeof(int);

    memcpy(&(copy_string->fisical_di), stream + offset, sizeof(int));
    offset += sizeof(int);

    return copy_string;
}

t_copy_string* new_copy_string(int tamanio){
    t_copy_string* copy_string = malloc(sizeof(t_copy_string));
    if (copy_string == NULL) {
        return NULL; 
    }

    copy_string->pid = pcb_en_ejecucion->pid;
    copy_string->tamaño = tamanio;
    copy_string->fisical_di = mmu(string_itoa(pcb_en_ejecucion->reg->AX)); //agregar registro DI
    copy_string->fisical_si = mmu(string_itoa(pcb_en_ejecucion->reg->BX)); //AGREGAR REGISTRO SI AL PCB

    return copy_string;
}

//, IO_STDIN_READ:,

void io_stdin_read(char* interfaz, char* logicalAdress, int tamanio){
    t_paquete* io_stdin_read_paq = crear_paquete(IO_STDIN_READ);
    t_buffer* buffer = malloc(sizeof(t_buffer));

    t_io_stdin* io_stdin_read = new_io_stdin(interfaz, tamanio, atoi(logicalAdress));

    serializar_io_stdin(io_stdin_read,buffer);

    agregar_a_paquete(io_stdin_read_paq,buffer->stream,buffer->size);
    enviar_paquete(io_stdin_read_paq,cliente_fd);

    eliminar_paquete(io_stdin_read_paq);
}

// IO_STDOUT_WRITE:

void io_stdin_write(char* interfaz, char* logicalAdress, int tamanio){
    t_paquete* io_stdin_write_paq = crear_paquete(IO_STDOUT_WRITE);
    t_buffer* buffer = malloc(sizeof(t_buffer));

    t_io_stdin* io_stdin_write = new_io_stdin(interfaz, tamanio, atoi(logicalAdress));

    serializar_io_stdin(io_stdin_write,buffer);

    agregar_a_paquete(io_stdin_write_paq,buffer->stream,buffer->size);
    enviar_paquete(io_stdin_write_paq,cliente_fd);

    eliminar_paquete(io_stdin_write_paq);
}
/*
typedef struct{
    u_int32_t pid;
    char* interfaz;
    int tamanio;
    int fisical_dir;
} t_io_stdin;
*/

void serializar_io_stdin(t_io_stdin* io_stdin, t_buffer* buffer){
    buffer->offset = 0;
    size_t size = sizeof(u_int32_t) + sizeof(int) * 2;
    if(io_stdin->interfaz != NULL){
        size+= sizeof(u_int32_t); //largo interfaz
        size+= string_length(io_stdin->interfaz_length) + 1;
    }

    buffer->size = size;
    buffer->stream = malloc(size);

    //serializo:
    memcpy(buffer->stream + buffer->offset, &(io_stdin->pid), sizeof(u_int32_t));
    buffer->offset += sizeof(u_int32_t);

    memcpy(buffer->stream + buffer->offset, &(io_stdin->tamanio), sizeof(int));
    buffer->offset += sizeof(int);

    memcpy(buffer->stream + buffer->offset, &(io_stdin->fisical_dir), sizeof(int));
    buffer->offset += sizeof(int);

    u_int32_t interfaz_length = strlen(io_stdin->interfaz) + 1;
    memcpy(buffer->stream + buffer->offset, &(interfaz_length), sizeof(u_int32_t));
    buffer->offset += sizeof(u_int32_t);

    memcpy(buffer->stream + buffer->offset, io_stdin->interfaz, interfaz_length);
    buffer->offset += interfaz_length;
}

t_io_stdin* deserialize_io_stdin(void* stream){
    t_io_stdin* io_stdin = malloc(sizeof(t_io_stdin));
    int offset = 0;

    memcpy(&(io_stdin->pid), stream + offset, sizeof(u_int32_t));
    offset += sizeof(u_int32_t);

    memcpy(&(io_stdin->tamanio), stream + offset, sizeof(int));
    offset += sizeof(int);

    memcpy(&(io_stdin->fisical_dir), stream + offset, sizeof(int));
    offset += sizeof(int);

    u_int32_t interfaz_length;
    memcpy(&interfaz_length, stream + offset, sizeof(u_int32_t));
    offset += sizeof(u_int32_t);

    io_stdin->interfaz_length = interfaz_length;

    io_stdin->interfaz = malloc(interfaz_length);
    memcpy(&(io_stdin->interfaz), stream + offset, interfaz_length);
    offset += interfaz_length;

    return io_stdin;
}

t_io_stdin* new_io_stdin(char* interfaz, int tamanio, int logical_address){
    t_io_stdin* io_stdin = malloc(sizeof(t_io_stdin));
    if (io_stdin == NULL) {
        return NULL; 
    }

    io_stdin->pid = pcb_en_ejecucion->pid;
    io_stdin->tamanio = tamanio;
    io_stdin->interfaz = strdup(interfaz);
    io_stdin->fisical_dir = mmu(string_itoa(logical_address));
    
    return io_stdin;
}


//check_interrupt:


void serializar_interrupcion(t_interrupt* interrupt, t_buffer* buffer){
    buffer->offset = 0;
    size_t size = sizeof(u_int32_t);
    if(interrupt->motivo != NULL){
        size+= sizeof(u_int32_t); //largo motivo
        size+= string_length(interrupt->motivo) + 1;
    }

    buffer->size = size;
    buffer->stream = malloc(size);

    //serializo:
    memcpy(buffer->stream + buffer->offset, &(interrupt->pid), sizeof(u_int32_t));
    buffer->offset += sizeof(u_int32_t);

    u_int32_t motivo_length = strlen(interrupt->motivo) + 1;
    memcpy(buffer->stream + buffer->offset, &(motivo_length), sizeof(u_int32_t));
    buffer->offset += sizeof(u_int32_t);

    memcpy(buffer->stream + buffer->offset, interrupt->motivo, motivo_length);
    buffer->offset += motivo_length;
}

t_interrupt* deserializar_interrupcion(void* stream){
    t_interrupt* interrupt = malloc(sizeof(t_interrupt));
    int offset = 0;


    memcpy(&(interrupt->pid), stream + offset, sizeof(u_int32_t));
    offset += sizeof(u_int32_t);

    u_int32_t motivo_length;
    memcpy(&motivo_length, stream + offset, sizeof(u_int32_t));
    offset += sizeof(u_int32_t);

    interrupt->motivo_length = motivo_length;

    interrupt->motivo = malloc(motivo_length);
    memcpy(&(interrupt->motivo), stream + offset, motivo_length);
    offset += motivo_length;

    return interrupt;
}

t_interrupt* new_interupt(u_int32_t pid, char* motivo){
    t_interrupt* interrupt = malloc(sizeof(t_interrupt));
    if (interrupt == NULL) {
        return NULL; 
    }

    interrupt->pid = pid;
    interrupt->motivo_length = string_length(motivo);
    interrupt->motivo = strdup(motivo);
    
    return interrupt;
}

//IO_FS_CREATE e IO_FS_DELETE
void io_fs_create(char* interfaz, char* nombre_archivo){
    t_paquete* io_fs_create = crear_paquete(IO_FS_CREATE);
    t_buffer* buffer = malloc(sizeof(t_buffer));

    t_interfaz* _interfaz = new_interfaz(interfaz,nombre_archivo,0,0,0);
    serializar_interfaz(_interfaz,buffer);

    agregar_a_paquete(io_fs_create,buffer->stream,buffer->size);
    enviar_paquete(io_fs_create,cliente_fd);
    eliminar_paquete(io_fs_create);
}

void io_fs_delete(char* interfaz, char* nombre_archivo){
    t_paquete* io_fs_delete = crear_paquete(IO_FS_DELETE);
    t_buffer* buffer = malloc(sizeof(t_buffer));    

    t_interfaz* _interfaz = new_interfaz(interfaz,nombre_archivo,0,0,0);
    serializar_interfaz(_interfaz,buffer);

    agregar_a_paquete(io_fs_delete,buffer->stream,buffer->size);
    enviar_paquete(io_fs_delete,cliente_fd);
    eliminar_paquete(io_fs_delete);
}

//IO_FS:

void serializar_interfaz(t_interfaz* interfaz, t_buffer* buffer){
    buffer->offset = 0;
    size_t size;
    if(interfaz->interfaz != NULL){
        size+= sizeof(u_int32_t);
        size+= string_length(interfaz->interfaz) + 1;
    }

    if(interfaz->nombre_archivo != NULL){
        size+= sizeof(u_int32_t);
        size+= string_length(interfaz->nombre_archivo) + 1;
    }

    size+= sizeof(u_int32_t) * 3;

    buffer->size = size;
    buffer->stream = malloc(size);

    //serializo:
    u_int32_t interfaz_length = strlen(interfaz->interfaz) + 1;
    memcpy(buffer->stream + buffer->offset, &(interfaz_length), sizeof(u_int32_t));
    buffer->offset += sizeof(u_int32_t);

    memcpy(buffer->stream + buffer->offset, interfaz->interfaz, interfaz_length);
    buffer->offset += interfaz_length;

    u_int32_t nombre_archivo_length = strlen(interfaz->nombre_archivo) + 1;
    memcpy(buffer->stream + buffer->offset, &(nombre_archivo_length), sizeof(u_int32_t));
    buffer->offset += sizeof(u_int32_t);

    memcpy(buffer->stream + buffer->offset, interfaz->nombre_archivo, nombre_archivo_length);
    buffer->offset += nombre_archivo_length;

    memcpy(buffer->stream + buffer->offset, &(interfaz->direccion_fisica), sizeof(u_int32_t));
    buffer->offset += sizeof(u_int32_t);

    memcpy(buffer->stream + buffer->offset, &(interfaz->tamanio_bytes), sizeof(u_int32_t));
    buffer->offset += sizeof(u_int32_t);

    memcpy(buffer->stream + buffer->offset, &(interfaz->puntero_archivo), sizeof(u_int32_t));
    buffer->offset += sizeof(u_int32_t);
}

t_interfaz* deserializar_interfaz(void* stream){
    t_interfaz* interfaz = malloc(sizeof(t_interfaz));
    int offset = 0;

    u_int32_t interfaz_length;
    memcpy(&interfaz_length, stream + offset, sizeof(u_int32_t));
    offset += sizeof(u_int32_t);

    interfaz->interfaz_length = interfaz_length;

    interfaz->interfaz = malloc(interfaz_length);
    memcpy(&(interfaz->interfaz), stream + offset, interfaz_length);
    offset += interfaz_length;

    u_int32_t nombre_archivo_length;
    memcpy(&nombre_archivo_length, stream + offset, sizeof(u_int32_t));
    offset += sizeof(u_int32_t);

    interfaz->nombre_archivo_length = nombre_archivo_length;

    interfaz->nombre_archivo = malloc(nombre_archivo_length);
    memcpy(&(interfaz->nombre_archivo), stream + offset, nombre_archivo_length);
    offset += nombre_archivo_length;

    memcpy(&(interfaz->direccion_fisica), stream + offset, sizeof(u_int32_t));
    offset += sizeof(u_int32_t);

    memcpy(&(interfaz->tamanio_bytes), stream + offset, sizeof(u_int32_t));
    offset += sizeof(u_int32_t);

    memcpy(&(interfaz->puntero_archivo), stream + offset, sizeof(u_int32_t));
    offset += sizeof(u_int32_t);

    return interfaz;
}

//en caso de no necesitar algun argumento poner 0 o NULL
t_interfaz* new_interfaz(char* interfazs, char* nombre_archivo, u_int32_t direccion_fisica, u_int32_t tamanio_bytes, u_int32_t puntero_archivo){
    t_interfaz* interfaz = malloc(sizeof(t_interfaz));
    if (interfaz == NULL) {
        return NULL; 
    }

    interfaz->interfaz_length = string_length(interfazs);
    interfaz->interfaz = strdup(interfazs);
    interfaz->nombre_archivo_length = string_length(nombre_archivo);
    interfaz->nombre_archivo = strdup(nombre_archivo);
    interfaz->direccion_fisica = direccion_fisica;
    interfaz->tamanio_bytes = tamanio_bytes;
    interfaz->puntero_archivo = puntero_archivo;
    
    return interfaz;
}


//IO_FS_TRUNCATE:

void io_fs_truncate(char* interfaz, char* nombre_archivo, char* registro_tamanio){
    t_paquete* io_fs_truncate = crear_paquete(IO_FS_TRUNCATE);
    t_buffer* buffer = malloc(sizeof(t_buffer));    

    t_interfaz* _interfaz = new_interfaz(interfaz,nombre_archivo,0,/*resolver que aqui se ponga el valor del registro_tamanio*/0,0);
    serializar_interfaz(_interfaz,buffer);

    agregar_a_paquete(io_fs_delete,buffer->stream,buffer->size);

    enviar_paquete(io_fs_delete,cliente_fd);
    eliminar_paquete(io_fs_delete);
}

void io_fs_write(char* interfaz, char* nombre_archivo, char* registro_direccion, char* registro_tamanio, char* registro_puntero_archivo){
    t_paquete* io_fs_write = crear_paquete(IO_FS_WRITE);
    t_buffer* buffer = malloc(sizeof(t_buffer));    

    t_interfaz* _interfaz = new_interfaz(interfaz,nombre_archivo,mmu(obtener_valor_registro(registro_direccion)),obtener_valor_registro(registro_tamanio),obtener_valor_registro(registro_puntero_archivo));
    serializar_interfaz(_interfaz,buffer);

    agregar_a_paquete(io_fs_write,buffer->stream,buffer->size);

    enviar_paquete(io_fs_write,cliente_fd);
    eliminar_paquete(io_fs_write);
}


void io_fs_read(char* interfaz, char* nombre_archivo, char* registro_direccion, char* registro_tamanio, char* registro_puntero_archivo){
    t_paquete* io_fs_read = crear_paquete(IO_FS_READ);
    t_buffer* buffer = malloc(sizeof(t_buffer));    

    t_interfaz* _interfaz = new_interfaz(interfaz,nombre_archivo,mmu(obtener_valor_registro(registro_direccion)),obtener_valor_registro(registro_tamanio),obtener_valor_registro(registro_puntero_archivo));
    serializar_interfaz(_interfaz,buffer);

    agregar_a_paquete(io_fs_read,buffer->stream,buffer->size);
    enviar_paquete(io_fs_read,cliente_fd);
    eliminar_paquete(io_fs_read);
}


void exxxit(t_pcb* pcb_en_ejecucion){
    t_paquete* contexto_actualizado = crear_paquete(EXIT_);
    t_buffer* buffer = malloc(sizeof(t_buffer));  

    serialize_pcb(pcb_en_ejecucion, buffer);
    agregar_a_paquete(contexto_actualizado,buffer->stream,buffer->size);
    enviar_paquete(contexto_actualizado,cliente_fd);
    eliminar_paquete(contexto_actualizado);
}

//del pcb en ejecucion
int obtener_valor_registro (char* registro){

    int valor = 0;
    //maybe make sense sem for pcb en ejecucion
    if(!strcmp(registro,"PC"))
        return pcb_en_ejecucion->pc;
    if(!strcmp(registro,"AX"))
        return pcb_en_ejecucion->reg->AX;
    if(!strcmp(registro,"BX"))
        return pcb_en_ejecucion->reg->BX;
    if(!strcmp(registro,"CX"))
        return pcb_en_ejecucion->reg->CX;
    if(!strcmp(registro,"DX"))
        return pcb_en_ejecucion->reg->DX;
    if(!strcmp(registro,"EAX"))
        return pcb_en_ejecucion->reg->EAX;
    if(!strcmp(registro,"EBX"))
        return pcb_en_ejecucion->reg->EBX;
    if(!strcmp(registro,"ECX"))
        return pcb_en_ejecucion->reg->ECX;
    if(!strcmp(registro,"EDX"))
        //return pcb_en_ejecucion->reg->EDX;
    if(!strcmp(registro,"SI"))
        //return pcb_en_ejecucion->reg->SI;
    if(!strcmp(registro,"DI"))
        //return pcb_en_ejecucion->reg->DI;

    return valor;
}

//RESIZE, WAIT Y SIGNAL:

t_resize* new_resize(u_int32_t tamanio){
    t_resize* resize = malloc(sizeof(t_resize));

    resize->pid = pcb_en_ejecucion->pid;
    resize->tamanio = tamanio;

    return resize;
} 

void resize(char* tamanio){
    //solicitar a mem ajustar el tamaño del proceso a tamanio, deberia ser un opCode que reciba mem

    //armar paquete con opcode RESIZE
    t_paquete* resizep = crear_paquete(RESIZE);
    t_buffer* buffer = malloc(sizeof(t_buffer));
    t_resize* resize = new_resize(atoi(tamanio));

    serializar_resize(resize,buffer);
    agregar_a_paquete(resizep,buffer->stream,buffer->size);
    enviar_paquete(resizep,conexion_mem);

    //if out of memory como respuesta
    char* ack = strdup(recibir_ack_resize(conexion_mem));

    if(!strcmp(ack,"Out of memory")){
        //devolver contexto ej a kernel informando esto.
        t_paquete* out_of_memory = crear_paquete(OUT_OF_MEMORY);
        serialize_pcb(pcb_en_ejecucion,buffer);
        agregar_a_paquete(out_of_memory,buffer->stream,buffer->size);
        enviar_paquete(out_of_memory,cliente_fd);
        eliminar_paquete(out_of_memory);
    }
    else
        log_info(logger, "PID: <%d> - Accion: <%s> - New Size: <%d>", pcb_en_ejecucion->pid, "RESIZE", atoi(tamanio));   


    eliminar_paquete(resize);
}

void serializar_resize(t_resize* resize, t_buffer* buffer){
    buffer->offset = 0;
    size_t size = sizeof(u_int32_t) * 2;

    buffer->size = size;
    buffer->stream = malloc(size);

    //serializo:
    memcpy(buffer->stream + buffer->offset, &(resize->pid), sizeof(u_int32_t));
    buffer->offset += sizeof(u_int32_t);

    memcpy(buffer->stream + buffer->offset, &(resize->tamanio), sizeof(u_int32_t));
    buffer->offset += sizeof(u_int32_t);
}

t_resize* deserializar_resize(void* stream){
    t_resize* resize = malloc(sizeof(t_resize));
    int offset = 0;

    memcpy(&(resize->pid), stream + offset, sizeof(u_int32_t));
    offset += sizeof(u_int32_t);

    memcpy(&(resize->tamanio), stream + offset, sizeof(u_int32_t));
    offset += sizeof(u_int32_t);

    return resize;
}

char* recibir_ack_resize(int conexion_mem){
    int size;
    char* ack;
    ack = recibir_buffer(&size, conexion_mem);
    log_info(logger, "ACK RESIZE received... %s", ack);
    return ack;
}

void wait (char* recurso){
    t_ws* ws = new_ws(recurso);
    t_buffer* buffer = malloc(sizeof(t_buffer));
    t_paquete* wsp = crear_paquete(WAIT);

    serializar_wait_o_signal(ws,buffer);
    agregar_a_paquete(wsp,buffer->stream,buffer->size);
    enviar_paquete(wsp,cliente_fd);

    eliminar_paquete(wsp);
}

void inst_signal (char* recurso){
    t_ws* ws = new_ws(recurso);
    t_buffer* buffer = malloc(sizeof(t_buffer));
    t_paquete* wsp = crear_paquete(SIGNAL);

    serializar_wait_o_signal(ws,buffer);
    agregar_a_paquete(wsp,buffer->stream,buffer->size);
    enviar_paquete(wsp,cliente_fd);

    eliminar_paquete(wsp);
}


t_ws* new_ws(char* recurso){
    t_ws* ws = malloc(sizeof(t_ws));

    ws->recurso_length = string_length(recurso);
    ws->recurso = strdup(recurso);

    return ws;
}

void serializar_wait_o_signal(t_ws* ws, t_buffer* buffer){
    buffer->offset = 0;
    size_t size;
    if(ws->recurso != NULL){
        size+= sizeof(u_int32_t);
        size+= string_length(ws->recurso) + 1;
    }

    buffer->size = size;
    buffer->stream = malloc(size);

    //serializo:
    u_int32_t recurso_length = strlen(ws->recurso) + 1;
    memcpy(buffer->stream + buffer->offset, &(recurso_length), sizeof(u_int32_t));
    buffer->offset += sizeof(u_int32_t);

    memcpy(buffer->stream + buffer->offset, ws->recurso, recurso_length);
    buffer->offset += recurso_length;
}

t_ws* deserializar_wait_o_signal(void* stream){
    t_ws* ws = malloc(sizeof(t_ws));
    int offset = 0;

    u_int32_t recurso_length;
    memcpy(&recurso_length, stream + offset, sizeof(u_int32_t));
    offset += sizeof(u_int32_t);

    ws->recurso_length = recurso_length;

    ws->recurso = malloc(recurso_length);
    memcpy(&(ws->recurso), stream + offset, recurso_length); //esta bien el primer argumento o es sin el &
    offset += recurso_length;

    return ws;
}

