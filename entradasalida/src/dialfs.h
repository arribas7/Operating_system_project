#ifndef DIALFS_H
#define DIALFS_H

#include <stdint.h>
#include <commons/log.h>

void fs_create(const char *filename, uint32_t pid);
void fs_delete(const char *filename, uint32_t pid);
void fs_truncate(const char *filename, uint32_t new_size, uint32_t pid);
void fs_write(uint32_t phys_addr, uint32_t size, uint32_t f_pointer, uint32_t pid);
void fs_read(uint32_t phys_addr, uint32_t size, uint32_t f_pointer, uint32_t pid);
void initialize_dialfs(const char *path_base_param, int block_size_param, int block_count_param);
void compact_dialfs(uint32_t pid);

#endif // DIALFS_H
