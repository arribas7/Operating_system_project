#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/config.h>
#include <commons/bitarray.h>
#include <dirent.h>
#include "dialfs.h"

int block_size;
int block_count;
extern t_log* logger;
extern t_config* config;

void create_file_metadata(const char *path_base, const char *filename, int start_block, int size) {
   char metadata_path[256];
    snprintf(metadata_path, sizeof(metadata_path), "%s/%s", path_base, filename);

    FILE *file = fopen(metadata_path, "w");
    if (file != NULL) {
        fclose(file);
    }

    t_config *metadata = config_create(metadata_path);

    char start_block_str[12], size_str[12];
    sprintf(start_block_str, "%d", start_block);
    sprintf(size_str, "%d", size);

    config_set_value(metadata, "BLOQUE_INICIAL", start_block_str);
    config_set_value(metadata, "TAMANIO_ARCHIVO", size_str);
    config_save(metadata);
    config_destroy(metadata);
}

void fs_create(const char *path_base, const char *filename, int pid) {
    log_info(logger, "PID: %d - Crear Archivo: %s", pid, filename);

    char bitmap_path[256];
    snprintf(bitmap_path, sizeof(bitmap_path), "%s/bitmap.dat", path_base);

    FILE *bitmap_file = fopen(bitmap_path, "rb+");
    if (!bitmap_file) return;

    fseek(bitmap_file, 0, SEEK_END);
    size_t bitmap_size = ftell(bitmap_file);
    fseek(bitmap_file, 0, SEEK_SET);

    char *bitmap_data = malloc(bitmap_size);
    fread(bitmap_data, 1, bitmap_size, bitmap_file);

    t_bitarray *bitmap = bitarray_create_with_mode(bitmap_data, bitmap_size, LSB_FIRST);

    int start_block = -1;
    for (int i = 0; i < bitarray_get_max_bit(bitmap); i++) {
        if (!bitarray_test_bit(bitmap, i)) {
            start_block = i;
            bitarray_set_bit(bitmap, i);
            break;
        }
    }

    if (start_block != -1) {
        fseek(bitmap_file, 0, SEEK_SET);
        fwrite(bitmap->bitarray, 1, bitmap_size, bitmap_file);
        create_file_metadata(path_base, filename, start_block, 0);
    }

    bitarray_destroy(bitmap);
    free(bitmap_data);
    fclose(bitmap_file);
}

void fs_delete(const char *path_base, const char *filename, int pid) {
    log_info(logger, "PID: %d - Eliminar Archivo: %s", pid, filename);

    char metadata_path[256];
    snprintf(metadata_path, sizeof(metadata_path), "%s/%s", path_base, filename);

    t_config *metadata = config_create(metadata_path);
    if (!metadata) return;

    int start_block = config_get_int_value(metadata, "BLOQUE_INICIAL");
    int size = config_get_int_value(metadata, "TAMANIO_ARCHIVO");
    config_destroy(metadata);

    char bitmap_path[256];
    snprintf(bitmap_path, sizeof(bitmap_path), "%s/bitmap.dat", path_base);

    FILE *bitmap_file = fopen(bitmap_path, "rb+");
    if (!bitmap_file) return;

    fseek(bitmap_file, 0, SEEK_END);
    size_t bitmap_size = ftell(bitmap_file);
    fseek(bitmap_file, 0, SEEK_SET);

    char *bitmap_data = malloc(bitmap_size);
    fread(bitmap_data, 1, bitmap_size, bitmap_file);

    t_bitarray *bitmap = bitarray_create_with_mode(bitmap_data, bitmap_size, LSB_FIRST);

    for (int i = start_block; i < start_block + (size + block_size - 1) / block_size; i++) {
        bitarray_clean_bit(bitmap, i);
    }

    fseek(bitmap_file, 0, SEEK_SET);
    fwrite(bitmap->bitarray, 1, bitmap_size, bitmap_file);

    bitarray_destroy(bitmap);
    free(bitmap_data);
    fclose(bitmap_file);

    remove(metadata_path);
}

void fs_truncate(const char *path_base, const char *filename, int new_size, int pid) {
    log_info(logger, "PID: %d - Truncar Archivo: %s - Tamaño: %d", pid, filename, new_size);

    char metadata_path[256];
    snprintf(metadata_path, sizeof(metadata_path), "%s/%s", path_base, filename);

    t_config *metadata = config_create(metadata_path);
    if (!metadata) return;

    int start_block = config_get_int_value(metadata, "BLOQUE_INICIAL");
    int size = config_get_int_value(metadata, "TAMANIO_ARCHIVO");
    config_destroy(metadata);

    int new_blocks = (new_size + block_size - 1) / block_size;
    int old_blocks = (size + block_size - 1) / block_size;

    char bitmap_path[256];
    snprintf(bitmap_path, sizeof(bitmap_path), "%s/bitmap.dat", path_base);

    FILE *bitmap_file = fopen(bitmap_path, "rb+");
    if (!bitmap_file) return;

    fseek(bitmap_file, 0, SEEK_END);
    size_t bitmap_size = ftell(bitmap_file);
    fseek(bitmap_file, 0, SEEK_SET);

    char *bitmap_data = malloc(bitmap_size);
    fread(bitmap_data, 1, bitmap_size, bitmap_file);

    t_bitarray *bitmap = bitarray_create_with_mode(bitmap_data, bitmap_size, LSB_FIRST);

    if (new_blocks > old_blocks) {
        for (int i = old_blocks; i < new_blocks; i++) {
            if (bitarray_test_bit(bitmap, start_block + i)) {
                fclose(bitmap_file);
                bitarray_destroy(bitmap);
                free(bitmap_data);
                compact_dialfs(path_base, pid);
                fs_truncate(path_base, filename, new_size, pid);
                return;
            }
        }
        for (int i = old_blocks; i < new_blocks; i++) {
            bitarray_set_bit(bitmap, start_block + i);
        }
    } else if (new_blocks < old_blocks) {
        for (int i = new_blocks; i < old_blocks; i++) {
            bitarray_clean_bit(bitmap, start_block + i);
        }
    }

    fseek(bitmap_file, 0, SEEK_SET);
    fwrite(bitmap->bitarray, 1, bitmap_size, bitmap_file);

    bitarray_destroy(bitmap);
    free(bitmap_data);
    fclose(bitmap_file);

    metadata = config_create(metadata_path);
    char new_size_str[12];
    sprintf(new_size_str, "%d", new_size);
    config_set_value(metadata, "TAMANIO_ARCHIVO", new_size_str);
    config_save(metadata);
    config_destroy(metadata);
}

void fs_write(const char *path_base, int phys_addr, const char *data, size_t size, int pid) {
    log_info(logger, "PID: %d - Escribir Archivo - Tamaño a Escribir: %zu - Puntero Archivo: %d", pid, size, phys_addr);

    char blocks_path[256];
    snprintf(blocks_path, sizeof(blocks_path), "%s/bloques.dat", path_base);

    FILE *blocks = fopen(blocks_path, "rb+");
    if (!blocks) return;

    fseek(blocks, phys_addr, SEEK_SET);
    fwrite(data, sizeof(char), size, blocks);
    fclose(blocks);
}

void fs_read(const char *path_base, int phys_addr, char *buffer, size_t size, int pid) {
    log_info(logger, "PID: %d - Leer Archivo - Tamaño a Leer: %zu - Puntero Archivo: %d", pid, size, phys_addr);

    char blocks_path[256];
    snprintf(blocks_path, sizeof(blocks_path), "%s/bloques.dat", path_base);

    FILE *blocks = fopen(blocks_path, "rb");
    if (!blocks) return;

    fseek(blocks, phys_addr, SEEK_SET);
    fread(buffer, sizeof(char), size, blocks);
    fclose(blocks);
}

void initialize_dialfs(const char *path_base, int block_size_param, int block_count_param) {
    char blocks_path[256];
    snprintf(blocks_path, sizeof(blocks_path), "%s/bloques.dat", path_base);

    block_size = block_size_param;
    block_count = block_count_param;
    FILE *blocks_file = fopen(blocks_path, "wb");
    if (blocks_file) {
        fseek(blocks_file, block_size * block_count - 1, SEEK_SET);
        fputc('\0', blocks_file);
        fclose(blocks_file);
    }

    char bitmap_path[256];
    snprintf(bitmap_path, sizeof(bitmap_path), "%s/bitmap.dat", path_base);

    FILE *bitmap_file = fopen(bitmap_path, "wb");
    if (bitmap_file) {
        size_t bitmap_size = (block_count + 7) / 8;
        char *bitmap_data = calloc(1, bitmap_size);
        fwrite(bitmap_data, 1, bitmap_size, bitmap_file);
        free(bitmap_data);
        fclose(bitmap_file);
    }
}

char** list_files(const char* path_base) {
    struct dirent* entry;
    DIR* dir = opendir(path_base);

    if (!dir) {
        return NULL;
    }

    char** filenames = malloc(sizeof(char*) * 256); // Arbitrary limit
    int count = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            filenames[count] = strdup(entry->d_name);
            count++;
        }
    }
    filenames[count] = NULL;

    closedir(dir);
    return filenames;
}

void compact_dialfs(const char *path_base, int pid) {
    log_info(logger, "PID: %d - Inicio Compactación.", pid);

    char blocks_path[256];
    snprintf(blocks_path, sizeof(blocks_path), "%s/bloques.dat", path_base);

    char bitmap_path[256];
    snprintf(bitmap_path, sizeof(bitmap_path), "%s/bitmap.dat", path_base);

    FILE *bitmap_file = fopen(bitmap_path, "rb+");
    if (!bitmap_file) return;

    fseek(bitmap_file, 0, SEEK_END);
    size_t bitmap_size = ftell(bitmap_file);
    fseek(bitmap_file, 0, SEEK_SET);

    char *bitmap_data = malloc(bitmap_size);
    fread(bitmap_data, 1, bitmap_size, bitmap_file);

    t_bitarray *bitmap = bitarray_create_with_mode(bitmap_data, bitmap_size, LSB_FIRST);

    FILE *blocks_file = fopen(blocks_path, "rb+");
    if (!blocks_file) {
        fclose(bitmap_file);
        return;
    }

    char **filenames = list_files(path_base);
    int current_block = 0;

    for (char **filename = filenames; *filename != NULL; filename++) {
        if (strcmp(*filename, "bitmap.dat") == 0 || strcmp(*filename, "bloques.dat") == 0) {
            continue;
        }

        char metadata_path[256];
        snprintf(metadata_path, sizeof(metadata_path), "%s/%s", path_base, *filename);

        t_config *metadata = config_create(metadata_path);
        int start_block = config_get_int_value(metadata, "BLOQUE_INICIAL");
        int size = config_get_int_value(metadata, "TAMANIO_ARCHIVO");
        int blocks_needed = (size + block_size - 1) / block_size;

        if (start_block != current_block) {
            char *buffer = malloc(blocks_needed * block_size);
            fseek(blocks_file, start_block * block_size, SEEK_SET);
            fread(buffer, block_size, blocks_needed, blocks_file);
            fseek(blocks_file, current_block * block_size, SEEK_SET);
            fwrite(buffer, block_size, blocks_needed, blocks_file);
            free(buffer);
        }

        for (int i = 0; i < blocks_needed; i++) {
            bitarray_clean_bit(bitmap, start_block + i);
            bitarray_set_bit(bitmap, current_block + i);
        }

        current_block += blocks_needed;

        char current_block_str[12];
        sprintf(current_block_str, "%d", current_block);
        config_set_value(metadata, "BLOQUE_INICIAL", current_block_str);
        config_save(metadata);
        config_destroy(metadata);
    }

    fseek(bitmap_file, 0, SEEK_SET);
    fwrite(bitmap->bitarray, 1, bitmap_size, bitmap_file);

    bitarray_destroy(bitmap);
    free(bitmap_data);
    fclose(bitmap_file);
    fclose(blocks_file);
    
    usleep(config_get_int_value(config, "RETRASO_COMPACTACION"));
    log_info(logger, "PID: %d - Fin Compactación.", pid);
}