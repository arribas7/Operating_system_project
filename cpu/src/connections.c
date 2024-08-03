#include "connections.h"

int recibir_req(int socket_cliente)
{
    int size;
    char* req = recibir_buffer(&size, socket_cliente);
    //log_info(logger, "REQ received... %s", req); //esta linea imprime cualquier cosa, pero no afecta al resultado del valor devuelto
    int reqq = atoi(req);
    free(req);
    return reqq;
}

void serializar_request(t_request* request, t_buffer* buffer){
    buffer->offset = 0;
    size_t size = sizeof(u_int32_t) + sizeof(int);
    buffer->size = size;
    buffer->stream = malloc(size);

    //serializo:
    memcpy(buffer->stream + buffer->offset, &(request->pid), sizeof(u_int32_t));
    buffer->offset += sizeof(u_int32_t);

    memcpy(buffer->stream + buffer->offset, &(request->req), sizeof(int));
    buffer->offset += sizeof(int);
}

t_request* deserializar_request(void* stream){
    t_request* request = malloc(sizeof(t_request));
    int offset = 0;

    offset += sizeof(int);

    memcpy(&(request->pid), stream + offset, sizeof(u_int32_t));
    offset += sizeof(u_int32_t);

    memcpy(&(request->req), stream + offset, sizeof(int));
    offset += sizeof(int);

    return request;
}

t_request* new_request (u_int32_t pid, int req){
    t_request* new_req = malloc(sizeof(t_request));
    if (new_req == NULL) {
        return NULL; 
    }

    new_req->pid = pid;
    new_req->req = req;

    return new_req;
}

int requestFrameToMem (int numPag){
    t_paquete* peticion = crear_paquete(TLB_MISS); //this opcode receive in mem
    t_request* request = new_request(pcb_en_ejecucion->pid, numPag);

    t_buffer *buffer = malloc(sizeof(t_buffer));
    serializar_request(request, buffer);

    agregar_a_paquete(peticion, buffer->stream, buffer->size);
    enviar_paquete(peticion, conexion_mem);
    eliminar_paquete(peticion);

    free(buffer->stream);
    free(buffer);

    recibir_operacion(conexion_mem); //FRAME O PROBRA SI FUNCIONA SIN ESTA LINEA YA QUE EL FRAME MEMORIA LO PUEDE ENVIAR POR UN MENSAJE SIMPLEMENTE
    int marco = recibir_req(conexion_mem); 

    log_info(logger, "PID: <%d> - Accion: <%s> - Pagina: <%d> - Marco: <%d>", pcb_en_ejecucion->pid, "OBTENER MARCO", numPag, marco);

    return marco;
}

void putRegValueToMem(int fisicalAddress, int valor){
    t_paquete* peticion = crear_paquete(WRITE); //this opcode receive in mem
    t_request2* request = new_request2(pcb_en_ejecucion->pid, fisicalAddress,valor);

    t_buffer *buffer = malloc(sizeof(t_buffer));
    serializar_request2(request, buffer);
    agregar_a_paquete(peticion, buffer->stream, buffer->size);
    enviar_paquete(peticion, conexion_mem);

    free(buffer->stream);
    free(buffer);

    eliminar_paquete(peticion);
    log_info(logger, "PID: <%d> - Accion: <%s> - Direccion Fisica: <%d> - Valor: <%d>", pcb_en_ejecucion->pid, "WRITE", fisicalAddress, valor);
}

int requestRegToMem (int fisicalAddr){
    t_paquete* peticion = crear_paquete(REG_REQUEST); //this opcode receive in mem
    t_request* request = new_request(pcb_en_ejecucion->pid, fisicalAddr);

    t_buffer *buffer = malloc(sizeof(t_buffer));
    serializar_request(request, buffer);

    agregar_a_paquete(peticion, buffer->stream, buffer->size);
 
    enviar_paquete(peticion, conexion_mem);
    eliminar_paquete(peticion);

    free(buffer->stream);
    free(buffer);

    recibir_operacion(conexion_mem); //REG O PROBRA SI FUNCIONA SIN ESTA LINEA YA QUE EL FRAME MEMORIA LO PUEDE ENVIAR POR UN MENSAJE SIMPLEMENTE
    int valor = recibir_req(conexion_mem); //receive VALOR DEL REG
    
    log_info(logger, "PID: <%d> - Accion: <%s> - Direccion Fisica: <%d> - Valor: <%d>", pcb_en_ejecucion->pid, "READ", fisicalAddr, valor);

    return valor;
}

void serializar_request2(t_request2* request, t_buffer* buffer){
    buffer->offset = 0;
    size_t size = sizeof(u_int32_t) + sizeof(int) + sizeof(int);
    buffer->size = size;
    buffer->stream = malloc(size);

    //serializo:
    memcpy(buffer->stream + buffer->offset, &(request->pid), sizeof(u_int32_t));
    buffer->offset += sizeof(u_int32_t);

    memcpy(buffer->stream + buffer->offset, &(request->req), sizeof(int));
    buffer->offset += sizeof(int);

    memcpy(buffer->stream + buffer->offset, &(request->val), sizeof(int));
    buffer->offset += sizeof(int);
}

t_request2* deserializar_request2(void* stream){
    t_request2* request = malloc(sizeof(t_request2));
    int offset = 0;

    offset += sizeof(int);

    memcpy(&(request->pid), stream + offset, sizeof(u_int32_t));
    offset += sizeof(u_int32_t);

    memcpy(&(request->req), stream + offset, sizeof(int));
    offset += sizeof(int);
    
    memcpy(&(request->val), stream + offset, sizeof(int));
    offset += sizeof(int);


    return request;
}

t_request2* new_request2(u_int32_t pid, int fisAdd, int valor){
    t_request2* new_req = malloc(sizeof(t_request2));
    if (new_req == NULL) {
        return NULL; 
    }

    new_req->pid = pid;
    new_req->req = fisAdd;
    new_req->val = valor;

    return new_req;
}
