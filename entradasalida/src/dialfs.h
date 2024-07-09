#ifndef DIALFS_H
#define DIALFS_H

#include <stdint.h>
#include <commons/log.h>

void io_fs_create(const char *path_base, const char *filename, int pid, t_log *logger);
void io_fs_delete(const char *path_base, const char *filename, int pid, t_log *logger);
void io_fs_truncate(const char *path_base, const char *filename, int new_size, int pid, t_log *logger);
void io_fs_write(const char *path_base, int phys_addr, const char *data, size_t size, int pid, t_log *logger);
void io_fs_read(const char *path_base, int phys_addr, char *buffer, size_t size, int pid, t_log *logger);
void initialize_dialfs(const char *path_base, int block_size, int block_count);

#endif // DIALFS_H
