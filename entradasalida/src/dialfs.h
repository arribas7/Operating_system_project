#ifndef DIALFS_H
#define DIALFS_H

#include <stdint.h>
#include <commons/log.h>

/*Crea un archivo en FS 
Recibe nombre y PID*/
void fs_create(const char *filename, uint32_t pid);

/*Borra un archivo en FS 
Recibe nombre y PID*/
void fs_delete(const char *filename, uint32_t pid);

/*
Diseñada para cambiar el tamaño de un archivo en el sistema de archivos. 
Recibe el filename, el nuevo tamaño (en bytes) y un pid.
1. Actualiza la metadata para reflejar su nuevo tamaño
2. Libera bloques de memoria si el nuevo tamaño es menor
3.1 Asigna nuevos bloques de memoria si el tamaño es mayor
3.2 Si no hay bloques contiguos disponibles, realiza la compactación
4. Actualizar el bitmap para reflejar los cambios en la asignación de bloques
*/
void fs_truncate(const char *filename, uint32_t new_size, uint32_t pid);

int fs_write(const char *filename, uint32_t phys_addr, uint32_t size, uint32_t f_pointer, uint32_t pid, int mem_connection);
int fs_read(const char *filename, uint32_t phys_addr, uint32_t size, uint32_t f_pointer, uint32_t pid, int mem_connection);
void initialize_dialfs(const char *path_base_param, int block_size_param, int block_count_param);
void compact_dialfs();

#endif // DIALFS_H
