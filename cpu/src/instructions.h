#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include "mmu_tlb.h"

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

void init_reg_proceso_actual(void);
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


int buscar(char *elemento, char **lista); //to find comando decode

//TO DO:
/, COPY_STRING, IO_STDIN_READ, IO_STDOUT_WRITE.
void resize(int tamanio);
char* recibir_ack_resize(int conexion_mem);



//from long term scheduler (to sincronize):

typedef enum {
    SUCCESS,
    INVALID_RESOURCE,
    INVALID_INTERFACE,
    OUT_OF_MEMORY,
    INTERRUPTED_BY_USER,
    NUM_REASONS // This helps in determining the number of reasons
} exit_reason;

#endif