#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include "mmu_tlb.h"
#include "connections.h"

void fetch(t_pcb *pcb);
void decode(t_pcb *pcb);
void execute(t_pcb *pcb);

extern t_log *logger;
extern t_log *loggerError;
extern t_config *config;

extern t_reg_cpu* reg_proceso_actual;
extern t_pcb* pcb_en_ejecucion;

extern int conexion_mem;
extern int cliente_fd;

extern char *instruccion_actual;
extern int instruccion_decodificada;
extern char **instr_decode;
extern int cant_parametros; //de la instruccion que llega desde memoria

typedef enum
{
    _SET,
    _MOV_IN,
    _MOV_OUT,
    _SUM,
    _SUB,
    _JNZ,
    _RESIZE,
    _COPY_STRING,
    _WAIT,
    _SIGNAL,
    _IO_GEN_SLEEP,
    _IO_STDIN_READ,
    _IO_STDOUT_WRITE,
    _IO_FS_CREATE,
    _IO_FS_DELETE,
    _IO_FS_TRUNCATE,
    _IO_FS_WRITE,
    _IO_FS_READ,
    _EXIT
} t_comando;

extern char *listaComandos[];

typedef struct{
    u_int32_t pid;
    int tamaño;
    int fisical_si;
    int fisical_di;
} t_copy_string;

typedef struct{
    u_int32_t pid;
    int tamanio;
    int fisical_dir;
    u_int32_t interfaz_length;
    char* interfaz;    
} t_io_stdin;

typedef struct{
    u_int32_t pid;
    u_int32_t motivo_length;
    char* motivo;    //crear una lista de interrupciones como cree la lista de comandos
} t_interrupt;

typedef struct{
    u_int32_t interfaz_length;
    char* interfaz;
    u_int32_t nombre_archivo_length;
    char* nombre_archivo;
    u_int32_t direccion_fisica;
    u_int32_t tamanio_bytes;
    u_int32_t puntero_archivo;
} t_interfaz; //para las fs


int init_reg_proceso_actual(void);
void io_gen_sleep(char* interfaz, char* job_unit);
void set(char* registro, char* valor);
void mov_in(char* registro, char* si);
t_instruction* new_instruction_IO(u_int32_t pid,char* interfaz, char* job_unit);
void recibir_instruccion(int socket_cliente);
void mov_in(char* registro, char* logicalAddress);
void mov_out(char* logicalAddr, char* reg);
int valueOfReg (char* reg);
void sum(char* destReg, char* origReg);
void sub(char* destReg, char* origReg);
void jnz(char* reg, char* inst);
void copy_string (char* tamanio);


int buscar(char *elemento, char **lista); //to find comando decode
/*typedef struct{
    u_int32_t pid;
    u_int32_t tamanio;
} t_resize;*/

t_resize* new_resize(u_int32_t tamanio);
void resize(char* tamanio);
void serializar_resize(t_resize* resize, t_buffer* buffer);
t_resize* deserializar_resize(void* stream);
char* recibir_ack_resize(int conexion_mem);
t_copy_string* new_copy_string(int tamanio);
void serializar_copy_string(t_copy_string* copy_string, t_buffer* buffer);
t_copy_string* deserializar_copy_string(void* stream);

void serializar_io_stdin(t_io_stdin* io_stdin, t_buffer* buffer);
t_io_stdin* deserialize_io_stdin(void* stream);
t_io_stdin* new_io_stdin(char* interfaz, int tamanio, int logical_address);

void io_stdin_read(char* interfaz, char* logicalAdress, int tamanio);
void io_stdin_write(char* interfaz, char* logicalAdress, int tamanio);


void serializar_interrupcion(t_interrupt* interrupt, t_buffer* buffer);
t_interrupt* deserializar_interrupcion(void* stream);
t_interrupt* new_interupt(u_int32_t pid, char* motivo);

void serializar_interfaz(t_interfaz* interfaz, t_buffer* buffer);   
t_interfaz* deserializar_interfaz(void* stream);
t_interfaz* new_interfaz(char* interfazs, char* nombre_archivo, u_int32_t direccion_fisica, u_int32_t tamanio_bytes, u_int32_t puntero_archivo);

void io_fs_create(char* interfaz, char* nombre_archivo);
void io_fs_delete(char* interfaz, char* nombre_archivo);
void io_fs_truncate(char* interfaz, char* nombre_archivo, char* registro_tamanio);
void io_fs_write(char* interfaz, char* nombre_archivo, char* registro_direccion, char* registro_tamanio, char* registro_puntero_archivo);
void io_fs_read(char* interfaz, char* nombre_archivo, char* registro_direccion, char* registro_tamanio, char* registro_puntero_archivo);
void exxxit(t_pcb* pcb_en_ejecucion);

typedef struct{
    u_int32_t recurso_length;
    char* recurso;
} t_ws;

void wait (char* recurso);
void inst_signal (char* recurso);
t_ws* new_ws(char* recurso);
void serializar_wait_o_signal(t_ws* ws, t_buffer* buffer);
t_ws* deserializar_wait_o_signal(void* stream);
//from long term scheduler (to sincronize):

typedef enum {
    SUCCESS,
    INVALID_RESOURCE,
    INVALID_INTERFACE,
    OUT_OF_MEMORY,
    INTERRUPTED_BY_USER,
    NUM_REASONS // This helps in determining the number of reasons
} exit_reason;


void check_interrupt (void);

#endif