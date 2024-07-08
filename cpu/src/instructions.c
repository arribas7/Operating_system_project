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
    free(buffer->stream);
    free(buffer);

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

t_paquete *execute(t_pcb *pcb)
{
    t_paquete *response = NULL;
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
            response = wait(instr_decode[1]);
        break;
        case _RESIZE:
            response = resize(instr_decode[1]);
        break;   
        case _SIGNAL:
            response = inst_signal(instr_decode[1]);
        break;
        case _IO_GEN_SLEEP:
            response = io_gen_sleep(instr_decode[1],instr_decode[2]);
        break;
        case _IO_STDIN_READ:
            response = io_stdin_read(instr_decode[1],instr_decode[2],instr_decode[3]);
        break;
        case _IO_STDOUT_WRITE:
            response = io_stdin_write(instr_decode[1],instr_decode[2],instr_decode[3]);
        break;
        case _IO_FS_CREATE:
            response = io_fs_create(instr_decode[1],instr_decode[2]);
        break;
        case _IO_FS_DELETE:
            response = io_fs_delete(instr_decode[1],instr_decode[2]);
        break;
        case _IO_FS_TRUNCATE:
            response = io_fs_truncate(instr_decode[1],instr_decode[2],instr_decode[3]);
        break;
        case _IO_FS_WRITE:
            response = io_fs_write(instr_decode[1],instr_decode[2],instr_decode[3],instr_decode[4],instr_decode[5]); 
        break;
        case _IO_FS_READ:
            response = io_fs_read(instr_decode[1],instr_decode[2],instr_decode[3],instr_decode[4],instr_decode[5]);
        break;
        case _EXIT:
            response = release();
        break;
        default:
        break;
    }
    return response;
}

op_code check_interrupt(void){
    if(interrupted_reason > 0){
        op_code actual_interrupted_reason = interrupted_reason;
        interrupted_reason = 0;
        return actual_interrupted_reason;
    }
    return 0;
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

t_paquete *io_gen_sleep(char* interfaz, char* job_unit){

    t_paquete* peticion = crear_paquete(IO_GEN_SLEEP); //this opcode receive in KERNEL
    int tamInterfaz = string_length(interfaz);
    uint32_t int_job_unit = (uint32_t) atoi(job_unit);
    t_instruction* IO = create_instruction_IO(pcb_en_ejecucion->pid, IO_GEN_SLEEP, interfaz, int_job_unit, "test-path");

    t_buffer *buffer = malloc(sizeof(t_buffer));
    serialize_instruccion_IO(IO, buffer);
    agregar_a_paquete(peticion, buffer->stream, buffer->size);
    free(buffer);

    return peticion;

    //log_info(logger, "PID: <%d> - Accion: <%s> - IO: <%s>", pcb_en_ejecucion->pid, "IO_GEN_SLEEP", interfaz);
    /*
    recibir_operacion(kernel_dispatch_socket);
    recibir_mensaje(kernel_dispatch_socket); //receive ack from kermel
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

t_paquete *io_stdin_read(char* interfaz, char* logicalAdress, int tamanio){
    t_paquete* io_stdin_read_paq = crear_paquete(IO_STDIN_READ);
    /*t_buffer* buffer = malloc(sizeof(t_buffer));

    t_io_stdin* io_stdin_read = new_io_stdin(pcb_en_ejecucion->pid, interfaz, tamanio, atoi(logicalAdress), mmu(logicalAdress));
    serializar_io_stdin(io_stdin_read,buffer);
    agregar_a_paquete(io_stdin_read_paq,buffer->stream,buffer->size);
    free(io_stdin_read);*/

    return io_stdin_read_paq;
}

// IO_STDOUT_WRITE:

t_paquete *io_stdin_write(char* interfaz, char* logicalAdress, int tamanio){
    t_paquete* io_stdin_write_paq = crear_paquete(IO_STDOUT_WRITE);
    /*t_buffer* buffer = malloc(sizeof(t_buffer));

    t_io_stdin* io_stdin_write = new_io_stdin(pcb_en_ejecucion->pid, interfaz, tamanio, atoi(logicalAdress), mmu(logicalAdress));
    serializar_io_stdin(io_stdin_write,buffer);
    agregar_a_paquete(io_stdin_write_paq,buffer->stream,buffer->size);
    free(io_stdin_write);*/

    return io_stdin_write_paq;
}

//IO_FS_CREATE e IO_FS_DELETE
t_paquete *io_fs_create(char* interfaz, char* nombre_archivo){
    t_paquete* io_fs_create = crear_paquete(IO_FS_CREATE);
    t_buffer* buffer = malloc(sizeof(t_buffer));

    t_interfaz* _interfaz = new_interfaz(interfaz,nombre_archivo,0,0,0);
    serializar_interfaz(_interfaz,buffer);

    agregar_a_paquete(io_fs_create,buffer->stream,buffer->size);
    free(_interfaz);
    return io_fs_create;
}

t_paquete *io_fs_delete(char* interfaz, char* nombre_archivo){
    t_paquete* io_fs_delete = crear_paquete(IO_FS_DELETE);
    t_buffer* buffer = malloc(sizeof(t_buffer));    

    t_interfaz* _interfaz = new_interfaz(interfaz,nombre_archivo,0,0,0);
    serializar_interfaz(_interfaz,buffer);

    agregar_a_paquete(io_fs_delete,buffer->stream,buffer->size);
    free(_interfaz);
    return io_fs_delete;
}

//IO_FS_TRUNCATE:
t_paquete *io_fs_truncate(char* interfaz, char* nombre_archivo, char* registro_tamanio){
    t_paquete* io_fs_truncate = crear_paquete(IO_FS_TRUNCATE);
    t_buffer* buffer = malloc(sizeof(t_buffer));    

    t_interfaz* _interfaz = new_interfaz(interfaz,nombre_archivo,0,/*resolver que aqui se ponga el valor del registro_tamanio*/0,0);
    serializar_interfaz(_interfaz,buffer);

    agregar_a_paquete(io_fs_truncate,buffer->stream,buffer->size);
    free(_interfaz);
    return io_fs_truncate;
}

t_paquete *io_fs_write(char* interfaz, char* nombre_archivo, char* registro_direccion, char* registro_tamanio, char* registro_puntero_archivo){
    t_paquete* io_fs_write = crear_paquete(IO_FS_WRITE);
    t_buffer* buffer = malloc(sizeof(t_buffer));    

    t_interfaz* _interfaz = new_interfaz(interfaz,nombre_archivo,mmu(obtener_valor_registro(registro_direccion)),obtener_valor_registro(registro_tamanio),obtener_valor_registro(registro_puntero_archivo));
    serializar_interfaz(_interfaz,buffer);
    agregar_a_paquete(io_fs_write,buffer->stream,buffer->size);
    free(_interfaz);
    return io_fs_write;
}


t_paquete *io_fs_read(char* interfaz, char* nombre_archivo, char* registro_direccion, char* registro_tamanio, char* registro_puntero_archivo){
    t_paquete* io_fs_read = crear_paquete(IO_FS_READ);
    t_buffer* buffer = malloc(sizeof(t_buffer));    

    t_interfaz* _interfaz = new_interfaz(interfaz,nombre_archivo,mmu(obtener_valor_registro(registro_direccion)),obtener_valor_registro(registro_tamanio),obtener_valor_registro(registro_puntero_archivo));
    serializar_interfaz(_interfaz,buffer);

    agregar_a_paquete(io_fs_read,buffer->stream,buffer->size);
    free(_interfaz);
    return io_fs_read;
}

t_paquete *release(){
    return crear_paquete(RELEASE);
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

t_paquete *resize(char* tamanio){
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
    eliminar_paquete(resize);

    if(!strcmp(ack,"Out of memory")){
        //devolver contexto ej a kernel informando esto.
        return crear_paquete(OUT_OF_MEMORY);
    } else {
        log_info(logger, "PID: <%d> - Accion: <%s> - New Size: <%d>", pcb_en_ejecucion->pid, "RESIZE", atoi(tamanio));   
    }

    return NULL;
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

t_paquete *wait(char* recurso){
    t_ws* ws = new_ws(recurso);
    t_buffer* buffer = malloc(sizeof(t_buffer));
    t_paquete* wsp = crear_paquete(WAIT);

    serializar_wait_o_signal(ws,buffer);
    agregar_a_paquete(wsp,buffer->stream,buffer->size);
    free(ws);
    return wsp;
}

t_paquete *inst_signal(char* recurso){
    t_ws* ws = new_ws(recurso);
    t_buffer* buffer = malloc(sizeof(t_buffer));
    t_paquete* wsp = crear_paquete(SIGNAL);

    serializar_wait_o_signal(ws,buffer);
    agregar_a_paquete(wsp,buffer->stream,buffer->size);
    free(ws);
    return wsp;
}
