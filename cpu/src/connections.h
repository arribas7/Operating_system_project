#ifndef CONNECTIONS_H
#define CONNECTIONS_H

#include "instructions.h"

typedef struct{
    u_int32_t pid;
    int req;
} t_request;

t_request* deserializar_request(void* stream);
void serializar_request(t_request* request, t_buffer* buffer);
t_request* new_request (u_int32_t pid, int req);
int recibir_req(int socket_cliente);
int requestFrameToMem (int numPag);
void putRegValueToMem(int fisicalAddress, int valor);
int requestRegToMem (int fisicalAddr);

#endif