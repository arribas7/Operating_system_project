// kernel.h tiene las estructuras y funciones relacionadas a la comunicación entre módulos.
#ifndef UTILS_CPU_H_
#define UTILS_CPU_H_

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <commons/log.h>
#include "client.h"

typedef struct {
    uint32_t PC;
    uint8_t AX;
    uint8_t BX;
    uint8_t CX;
    uint8_t DX;
    uint8_t EAX;
    uint8_t EBX;
    uint8_t ECX;
    uint8_t EDX;
    uint32_t SI;
    uint32_t DI;
} t_reg_cpu;

typedef struct 
{
    uint32_t pid;
    op_code code;
    uint32_t length_name;
    char* name;
    uint32_t time;              // GEN
    uint32_t physical_address;  // STD - FS
    uint32_t size;              // STD - FS
    uint32_t length_f_name;     // FS
    char* f_name;               // FS
    uint32_t f_pointer;         // FS
} t_instruction;

/*
typedef struct
{
    u_int32_t pid;
    int tamanio;
    int fisical_dir;
    u_int32_t interfaz_length;
    char* interfaz;    
} t_io_stdin;

typedef struct
{
    u_int32_t interfaz_length;
    char* interfaz;
    u_int32_t nombre_archivo_length;
    char* nombre_archivo;
    u_int32_t direccion_fisica;
    u_int32_t tamanio_bytes;
    u_int32_t puntero_archivo;
} t_interfaz; // FS
*/

typedef struct{
    uint32_t pid;
    int tamanio;
    int fisical_si;
    int fisical_di;
} t_copy_string;

typedef struct{
    int pid;
    int tamanio;
} t_resize;

typedef struct{
    u_int32_t recurso_length;
    char* recurso;
} t_ws;

// INSTRUCTION

t_instruction* create_instruction_IO(uint32_t pid, op_code code, char* name, uint32_t time, uint32_t physical_address, uint32_t size, char* f_name, uint32_t f_pointer);
t_instruction* deserialize_instruction_IO (void* stream);
void serialize_instruction_IO(t_instruction* interface, t_buffer* buffer);
t_instruction* receive_instruction_IO(int socket_cliente);
void send_instruction_IO(t_instruction* instruction, int socket_cliente);
void delete_instruction_IO(t_instruction* instruction);

t_reg_cpu* nuevo_reg(uint8_t pc);
void serializar_reg(t_reg_cpu* reg, t_buffer* buffer);
t_reg_cpu* deserializar_reg(void* stream);
void eliminar_reg(t_reg_cpu *reg);

t_ws* new_ws(char* recurso);
void serializar_wait_o_signal(t_ws* ws, t_buffer* buffer);
t_ws* deserializar_wait_o_signal(void* stream);
t_ws* recibir_wait_o_signal(int socket_cliente);
void destroy_ws(t_ws* ws);

/*
void serializar_io_stdin(t_io_stdin* io_stdin, t_buffer* buffer);
t_io_stdin* deserialize_io_stdin(void* stream);
t_io_stdin* new_io_stdin(u_int32_t pid, char* interfaz, int tamanio, int logical_address, int fisical_dir);
t_io_stdin* recibir_io_stdin(int socket_cliente)

void serializar_interfaz(t_interfaz* interfaz, t_buffer* buffer);   
t_interfaz* deserializar_interfaz(void* stream);
t_interfaz* new_interfaz(char* interfazs, char* nombre_archivo, u_int32_t direccion_fisica, u_int32_t tamanio_bytes, u_int32_t puntero_archivo);
t_interfaz* recibir_interfaz(int socket_cliente);
*/

#endif
