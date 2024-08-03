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
/*
void free_double_pointer(char **array, int size) {
    for (int i = 0; i < size; i++)
        free(array[i]); 
    free(array);
}
*/
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
    reg_proceso_actual = (t_register *)malloc(sizeof(t_register));
    if (reg_proceso_actual == NULL) {
        fprintf(stderr, "Error al asignar memoria\n");
        return -1;
    }

    reg_proceso_actual->AX = 0;
    reg_proceso_actual->BX = 0;
    reg_proceso_actual->CX = 0;
    reg_proceso_actual->DI = 0;
    reg_proceso_actual->DX = 0;
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
    //log_info(logger, "Before fetching instruction.. %s", instruccion_actual);
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
    instr_decode = string_n_split(instruccion_actual, 6, " ");
    cant_parametros = string_array_size(instr_decode) - 1;
    instruccion_decodificada = buscar(instr_decode[0], listaComandos);
    //log_info(logger, "Decode instruction fetched.. %s", listaComandos[instruccion_decodificada]);
}

t_paquete *execute(t_pcb *pcb)
{
    t_paquete *response = NULL;
    pcb_en_ejecucion = pcb;
    switch(cant_parametros){
        case 0:
            log_info(logger, "PID: <%d> - <EXECUTE>: <%s> ", pcb->pid, listaComandos[instruccion_decodificada]);
        break;
        case 1:
            log_info(logger, "PID: <%d> - <EXECUTE>: <%s> - <%s>", pcb->pid, listaComandos[instruccion_decodificada], instr_decode[1]);
        break;
        case 2:
            log_info(logger, "PID: <%d> - <EXECUTE>: <%s> - <%s>, <%s>", pcb->pid, listaComandos[instruccion_decodificada], instr_decode[1], instr_decode[2]);
        break;
        case 3:
            log_info(logger, "PID: <%d> - <EXECUTE>: <%s> - <%s>, <%s>, <%s>", pcb->pid, listaComandos[instruccion_decodificada], instr_decode[1], instr_decode[2], instr_decode[3]);
        break;
        case 5:
            log_info(logger, "PID: <%d> - <EXECUTE>: <%s> - <%s>, <%s>, <%s>, <%s>, <%s>", pcb->pid, listaComandos[instruccion_decodificada], instr_decode[1], instr_decode[2], instr_decode[3],instr_decode[4], instr_decode[5]);
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
//FALTA AGREGAR DX Y PC A LOS REGISTROS A LEER O ESCRIBIR
void set(char* registro, char* valor){
    if (strcmp(registro, "PC") == 0)
        reg_proceso_actual->PC = atoi(valor);
    if (strcmp(registro, "AX") == 0)
        reg_proceso_actual->AX = (uint8_t)atoi(valor);
    if (strcmp(registro, "BX") == 0) 
        reg_proceso_actual->BX = atoi(valor);
    if (strcmp(registro, "CX") == 0) 
        reg_proceso_actual->CX = atoi(valor);
    if (strcmp(registro, "DX") == 0) 
        reg_proceso_actual->DX = atoi(valor);        
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

    log_info(logger, "%s in actual process: %d", registro, atoi(valor)); //funciona
}

void mov_in(char* registro, char* reg2){
    //int valor = 0; //quitar luego de hacer el siguiente TO DO
    int direccion_logica = obtener_valor_reg(reg2);

    int fisicalAddr = mmu(string_itoa(direccion_logica));
    
    //request to mem
    //int valor = requestRegToMem(fisicalAddr); //TO DO
    int valor = requestRegToMem(fisicalAddr);

    if (strcmp(registro, "AX") == 0)
        reg_proceso_actual->AX = valor;
    if (strcmp(registro, "BX") == 0) 
        reg_proceso_actual->BX = valor;
    if (strcmp(registro, "CX") == 0) 
        reg_proceso_actual->CX = valor;
    if (strcmp(registro, "DX") == 0)
        reg_proceso_actual->DX = valor;
    if (strcmp(registro, "EAX") == 0)
        reg_proceso_actual->EAX = valor;
    if (strcmp(registro, "EBX") == 0)
        reg_proceso_actual->EBX = valor;
    if (strcmp(registro, "ECX") == 0)
        reg_proceso_actual->ECX = valor;
    if (strcmp(registro, "EDX") == 0)
        reg_proceso_actual->EDX = valor;
    if (strcmp(registro, "SI") == 0)
        reg_proceso_actual->SI = valor;
    if (strcmp(registro, "DI") == 0)
        reg_proceso_actual->DI = valor;
}

void mov_out(char* reg1, char* reg2){

    int valor;

    valor = obtener_valor_reg(reg2);
    int direccion_logica = obtener_valor_reg(reg1);

    int fisicalAddress = mmu(string_itoa(direccion_logica));

    putRegValueToMem(fisicalAddress,valor);
}

int obtener_valor_reg(char* reg){
    if (strcmp(reg, "AX") == 0)
        return reg_proceso_actual->AX;
    if (strcmp(reg, "BX") == 0) 
        return reg_proceso_actual->BX;
    if (strcmp(reg, "CX") == 0) 
        return reg_proceso_actual->CX;
    if (strcmp(reg, "DX") == 0) 
        return reg_proceso_actual->DX;
    if (strcmp(reg, "EAX") == 0)
        return reg_proceso_actual->EAX;
    if (strcmp(reg, "EBX") == 0)
        return reg_proceso_actual->EBX;
    if (strcmp(reg, "ECX") == 0)
        return reg_proceso_actual->ECX;
    if (strcmp(reg, "EDX") == 0)
        return reg_proceso_actual->EDX;
    if (strcmp(reg, "SI") == 0)
        return reg_proceso_actual->SI;
    if (strcmp(reg, "DI") == 0)
        return reg_proceso_actual->DI;
    return 0;
}

void sum(char* destReg, char* origReg){                 

    int valor1,valor2,suma;

    valor1 = obtener_valor_reg(destReg);
    valor2 = obtener_valor_reg(origReg);

    suma = valor1 + valor2;

    set(destReg,string_itoa(suma));  
}

void sub(char* destReg, char* origReg){

    int valor1,valor2,resta;

    valor1 = obtener_valor_reg(destReg);
    valor2 = obtener_valor_reg(origReg);

    resta = valor1 - valor2;

    set(destReg,string_itoa(resta));   
}

void jnz(char* reg, char* inst){
    //log_info(logger, "PC in actual process before: %d", reg_proceso_actual->PC); //funciona

    if(obtener_valor_reg(reg))
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
    free(buffer->stream);
    free(buffer);
}

void serializar_copy_string(t_copy_string* copy_string, t_buffer* buffer){
    size_t size = sizeof(uint32_t) + sizeof(int) * 3;
    buffer->size = size;
    buffer->stream = malloc(size);

    buffer->offset = 0;
    //serializo:
    memcpy(buffer->stream + buffer->offset, &(copy_string->pid), sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);

    memcpy(buffer->stream + buffer->offset, &(copy_string->tamanio), sizeof(int));
    buffer->offset += sizeof(int);

    memcpy(buffer->stream + buffer->offset, &(copy_string->fisical_si), sizeof(int));
    buffer->offset += sizeof(int);

    memcpy(buffer->stream + buffer->offset, &(copy_string->fisical_di), sizeof(int));
    buffer->offset += sizeof(int);
}

t_copy_string* deserializar_copy_string(void* stream){
    t_copy_string* copy_string = malloc(sizeof(t_copy_string));
    int offset = sizeof(int); // tamanio

    memcpy(&(copy_string->pid), stream + offset, sizeof(u_int32_t));
    offset += sizeof(u_int32_t);

    memcpy(&(copy_string->tamanio), stream + offset, sizeof(int));
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
    copy_string->tamanio = tamanio;
    copy_string->fisical_di = mmu(string_itoa(pcb_en_ejecucion->reg->DI)); //agregar registro DI
    copy_string->fisical_si = mmu(string_itoa(pcb_en_ejecucion->reg->SI)); //AGREGAR REGISTRO SI AL PCB

    return copy_string;
}

// INSTRUCTIONS PACKAGE - TO IO

t_paquete *io_gen_sleep(char* name, char* time) 
{
    t_paquete* package = crear_paquete(IO_GEN_SLEEP); // This op_code receive in KERNEL
    uint32_t ui_time = (uint32_t) atoi(time);
    t_instruction* instruction = create_instruction_IO(pcb_en_ejecucion->pid, IO_GEN_SLEEP, name, ui_time, 0, 0, "", 0);

    t_buffer *buffer = malloc(sizeof(t_buffer));
    serialize_instruction_IO(instruction, buffer);
    agregar_a_paquete(package, buffer->stream, buffer->size);
    
    delete_instruction_IO(instruction);
    free(buffer->stream);
    free(buffer);

    return package;

    //log_info(logger, "PID: <%d> - Accion: <%s> - IO: <%s>", pcb_en_ejecucion->pid, "IO_GEN_SLEEP", interfaz);
    /*
    recibir_operacion(kernel_dispatch_socket);
    recibir_mensaje(kernel_dispatch_socket); //receive ack from kermel
    */
}

t_paquete *io_stdin_read(char* name, char* logicalAdressReg, char* size) 
{
    t_paquete* io_stdin_read_paq = crear_paquete(IO_STDIN_READ);
    int logicalAddress =  obtener_valor_reg(logicalAdressReg);
    uint32_t reg_size =  obtener_valor_reg(size);
    t_buffer* buffer = malloc(sizeof(t_buffer));

    t_instruction* instruction = create_instruction_IO(pcb_en_ejecucion->pid, IO_STDIN_READ, name, 0, mmu(string_itoa(logicalAddress)), reg_size, "", 0);
    serialize_instruction_IO(instruction, buffer);
    agregar_a_paquete(io_stdin_read_paq, buffer->stream, buffer->size);

    delete_instruction_IO(instruction);
    free(buffer->stream);
    free(buffer);

    return io_stdin_read_paq;
}

t_paquete *io_stdin_write(char* name, char* logicalAdressReg, char* size) 
{
    t_paquete* io_stdin_write_paq = crear_paquete(IO_STDOUT_WRITE);
    int logicalAddress =  obtener_valor_reg(logicalAdressReg);
    uint32_t reg_size =  obtener_valor_reg(size);
    t_buffer* buffer = malloc(sizeof(t_buffer));

    t_instruction* instruction = create_instruction_IO(pcb_en_ejecucion->pid, IO_STDOUT_WRITE, name, 0, mmu(string_itoa(logicalAddress)), reg_size, "", 0);
    serialize_instruction_IO(instruction, buffer);
    agregar_a_paquete(io_stdin_write_paq, buffer->stream, buffer->size);

    delete_instruction_IO(instruction);
    free(buffer->stream);
    free(buffer);

    return io_stdin_write_paq;
}

t_paquete *io_fs_create(char* name, char* file_name) 
{
    t_paquete* io_fs_create = crear_paquete(IO_FS_CREATE);
    t_buffer* buffer = malloc(sizeof(t_buffer));

    t_instruction* instruction = create_instruction_IO(pcb_en_ejecucion->pid, IO_FS_CREATE, name, 0, 0, 0, file_name, 0);
    serialize_instruction_IO(instruction, buffer);
    agregar_a_paquete(io_fs_create, buffer->stream, buffer->size);

    delete_instruction_IO(instruction);
    free(buffer->stream);
    free(buffer);

    return io_fs_create;
}

t_paquete *io_fs_delete(char* name, char* file_name) 
{
    t_paquete* io_fs_delete = crear_paquete(IO_FS_DELETE);
    t_buffer* buffer = malloc(sizeof(t_buffer));    

    t_instruction* instruction = create_instruction_IO(pcb_en_ejecucion->pid, IO_FS_DELETE, name, 0, 0, 0, file_name, 0);
    serialize_instruction_IO(instruction, buffer);
    agregar_a_paquete(io_fs_delete,buffer->stream,buffer->size);
    
    delete_instruction_IO(instruction);
    free(buffer->stream);
    free(buffer);

    return io_fs_delete;
}

t_paquete *io_fs_truncate(char* name, char* file_name, char* size) 
{
    t_paquete* io_fs_truncate = crear_paquete(IO_FS_TRUNCATE);
    uint32_t reg_size =  obtener_valor_reg(size);
    t_buffer* buffer = malloc(sizeof(t_buffer));    

    t_instruction* instruction = create_instruction_IO(pcb_en_ejecucion->pid, IO_FS_TRUNCATE, name, 0, 0, reg_size, file_name, 0);
    serialize_instruction_IO(instruction, buffer);
    agregar_a_paquete(io_fs_truncate, buffer->stream, buffer->size);

    delete_instruction_IO(instruction);
    free(buffer->stream);
    free(buffer);

    return io_fs_truncate;
}

t_paquete *io_fs_write(char* name, char* file_name, char* logicalAddressReg, char* size, char* file_pointer) 
{
    t_paquete* io_fs_write = crear_paquete(IO_FS_WRITE);
    int logicalAddress =  obtener_valor_reg(logicalAddressReg);
    uint32_t reg_size =  obtener_valor_reg(size);
    t_buffer* buffer = malloc(sizeof(t_buffer));    

    t_instruction* instruction = create_instruction_IO(pcb_en_ejecucion->pid, IO_FS_WRITE, name, 0, mmu(string_itoa(logicalAddress)), reg_size, file_name, obtener_valor_reg(file_pointer));
    serialize_instruction_IO(instruction, buffer);
    agregar_a_paquete(io_fs_write, buffer->stream, buffer->size);

    delete_instruction_IO(instruction);
    free(buffer->stream);
    free(buffer);

    return io_fs_write;
}

t_paquete *io_fs_read(char* name, char* file_name, char* logicalAddressReg, char* size, char* file_pointer) 
{
    t_paquete* io_fs_read = crear_paquete(IO_FS_READ);
    int logicalAddress =  obtener_valor_reg(logicalAddressReg);
    uint32_t reg_size =  obtener_valor_reg(size);
    t_buffer* buffer = malloc(sizeof(t_buffer));    

    t_instruction* instruction = create_instruction_IO(pcb_en_ejecucion->pid, IO_FS_READ, name, 0, mmu(string_itoa(logicalAddress)), reg_size, file_name, obtener_valor_reg(file_pointer));
    serialize_instruction_IO(instruction, buffer);
    agregar_a_paquete(io_fs_read, buffer->stream, buffer->size);
 
    delete_instruction_IO(instruction);
    free(buffer->stream);
    free(buffer);

    return io_fs_read;
}

// ----- -----

t_paquete *release(){
    return crear_paquete(RELEASE);
}

//RESIZE, WAIT Y SIGNAL:

t_resize* new_resize(u_int32_t tamanio){
    t_resize* resize = malloc(sizeof(t_resize));

    resize->pid = pcb_en_ejecucion->pid;
    resize->tamanio = tamanio;

    //printf("r p..%d",resize->pid);
    //printf("r t..%d",resize->tamanio);

    return resize;
} 

t_paquete *resize(char* tamanio){
    //solicitar a mem ajustar el tamaño del proceso a tamanio, deberia ser un opCode que reciba mem

    //armar paquete con opcode RESIZE
    t_paquete* resizep = crear_paquete(RESIZE);
    t_buffer* buffer = malloc(sizeof(t_buffer));
    t_resize* resize = new_resize(atoi(tamanio));

    //printf("...........%d",resize->tamanio);

    serializar_resize(resize,buffer);
    agregar_a_paquete(resizep,buffer->stream,buffer->size);
    enviar_paquete(resizep,conexion_mem);

    //if out of memory como respuesta
    eliminar_paquete(resizep);

    recibir_operacion(conexion_mem);
    recibir_ack_resize(conexion_mem);

    if(!strcmp(ack,"Out_of_memory")){
        //devolver contexto ej a kernel informando esto.
        log_info(logger, "OUT OF MEMORY"); 
        return crear_paquete(OUT_OF_MEMORY);
    } else {
        log_info(logger, "PID: <%d> - Accion: <%s> - New Size: <%d>", pcb_en_ejecucion->pid, "RESIZE", atoi(tamanio));   
    }

    return NULL;
}

void serializar_resize(t_resize* resize, t_buffer* buffer){
    buffer->offset = 0;
    size_t size = sizeof(int) * 2;

    buffer->size = size;
    buffer->stream = malloc(size);

    //serializo:
    memcpy(buffer->stream + buffer->offset, &(resize->pid), sizeof(int));
    buffer->offset += sizeof(int);

    memcpy(buffer->stream + buffer->offset, &(resize->tamanio), sizeof(int));
    buffer->offset += sizeof(int);
}

t_resize* deserializar_resize(void* stream){
    t_resize* resize = malloc(sizeof(t_resize));
    int offset = 0;

    memcpy(&(resize->pid), stream + offset, sizeof(int));
    offset += sizeof(int);

    memcpy(&(resize->tamanio), stream + offset, sizeof(int));
    offset += sizeof(int);

    return resize;
}

void recibir_ack_resize(int conexion_mem){
    int size;
    ack = recibir_buffer(&size, conexion_mem);
    //printf("---%s",ack);
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
