#include <string.h>
#include"server.h"

int iniciar_servidor(const char* puerto)
{
    int socket_servidor;

    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(NULL, puerto, &hints, &servinfo);

    // Creamos el socket de escucha del servidor

    for (p = servinfo; p != NULL; p = p->ai_next) {
        socket_servidor = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (socket_servidor == -1){
            continue;
        }

        int opt = 1;
        // Para evitar tener el problema de escuchar el cliente -1
        if (setsockopt(socket_servidor, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
            perror("setsockopt");
            exit(EXIT_FAILURE);
        }

        // Asociamos el socket a un puerto
        if (bind(socket_servidor, p->ai_addr, p->ai_addrlen) == 0){
            break;
        }

        close(socket_servidor);
    }

    if (p == NULL) {
        return -1;
    }

    // Escuchamos las conexiones entrantes
    freeaddrinfo(servinfo);
    log_trace(logger, "Listo para escuchar a mi cliente");
    listen(socket_servidor, 10);

    return socket_servidor;
}

int esperar_cliente(int socket_servidor)
{
    // Aceptamos un nuevo cliente
    int socket_cliente;

    struct sockaddr_in dir_cliente;
    socklen_t tam_direccion = sizeof(struct sockaddr_in);

    socket_cliente = accept(socket_servidor, (struct sockaddr *)&dir_cliente, &tam_direccion);

    if (socket_cliente == -1) {
        perror("accept");
        return -1;
    }

    log_info(logger, "Se conecto un cliente!");

    return socket_cliente;
}

int recibir_operacion(int socket_cliente)
{
    int cod_op;
    if(recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) > 0)
        return cod_op;
    else
    {
        close(socket_cliente);
        return -1;
    }
    /* TO DO 
         // Enviar el handshake al cliente
       handshake = 1;
       size_t bytes = send(socket_cliente, &handshake, sizeof(int32_t), 0);
       if (bytes != sizeof(int32_t)) {
           // Manejar error en el envío del handshake
           return HANDSHAKE_ERROR;
       }


       // Recibir la respuesta del cliente
       bytes = recv(socket_cliente, &result, sizeof(int32_t), MSG_WAITALL);
       if (bytes != sizeof(int32_t)) {
           // Manejar error en la recepción de la respuesta
           return HANDSHAKE_ERROR;
       }


       // Verificar el resultado del handshake
       if (result == HANDSHAKE_OK) {
           printf("Handshake exitoso\n");
       } else {
           printf("Error en el handshake\n");
           close(socket_cliente);
           return -1;
       }

    */
}

void* recibir_buffer(int* size, int socket_cliente)
{
    void * buffer;
    recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
    buffer = malloc(*size);
    recv(socket_cliente, buffer, *size, MSG_WAITALL);

    return buffer;
}

void recibir_mensaje(int socket_cliente)
{
    int size;
    char* buffer = recibir_buffer(&size, socket_cliente);
    log_info(logger, "Me llego el mensaje %s", buffer);
    free(buffer);
}

t_list* recibir_paquete(int socket_cliente)
{
    int size;
    int desplazamiento = 0;
    void * buffer;
    t_list* valores = list_create();
    int tamanio;

    buffer = recibir_buffer(&size, socket_cliente);
    while(desplazamiento < size)
    {
        memcpy(&tamanio, buffer + desplazamiento, sizeof(int));
        desplazamiento+=sizeof(int);
        char* valor = malloc(tamanio);
        memcpy(valor, buffer+desplazamiento, tamanio);
        desplazamiento+=tamanio;
        list_add(valores, valor);
    }
    
    free(buffer);
    return valores;
}

t_pcb* recibir_pcb(int socket_cliente) {
    int size;
    int offset = 0;
    void *buffer = recibir_buffer(&size, socket_cliente);
    if (buffer == NULL) {
        return NULL;
    }

    offset += sizeof(int);

    t_pcb* pcb = deserialize_pcb(buffer, offset);
    free(buffer);
    return pcb;
}