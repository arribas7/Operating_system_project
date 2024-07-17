#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <commons/config.h>
#include <commons/bitarray.h>
#include <commons/collections/list.h>
#include <dirent.h>
#include "dialfs.h"

int block_size;
int block_count;
const char *path_base;
extern t_log* logger;
extern t_config* config;

// TODO: add mutex
// TODO: Wait a time unit in each operation

void debug_print_bitmap(t_bitarray *bitmap){
    size_t max_bits = bitarray_get_max_bit(bitmap);
    char *log_string = malloc(max_bits + 100);

    sprintf(log_string, "[DEBUG] bitmap print: \n");
    char *ptr = log_string + strlen(log_string);

    for (int i = 0; i < max_bits; i++) {
        if (bitarray_test_bit(bitmap, i)) {
            *ptr++ = '1';
        } else {
            *ptr++ = '0';
        }
    }
    *ptr = '\0';

    log_debug(logger, "%s", log_string);

    free(log_string);
}

void debug_print_blocks(FILE *blocks_file) {
    long original_pointer = ftell(blocks_file);

    fseek(blocks_file, 0, SEEK_END);
    long file_size = ftell(blocks_file);
    fseek(blocks_file, 0, SEEK_SET);

    char *buffer = malloc(file_size);

    fread(buffer, sizeof(char), file_size, blocks_file);
    char *log_string = malloc(file_size + 50);

    strcpy(log_string, "[DEBUG] Blocks file content:\n");
    strncat(log_string, buffer, file_size);
    strcat(log_string, "\n");
    log_debug(logger, log_string);

    free(log_string);
    free(buffer);

    // Volver al puntero original
    fseek(blocks_file, original_pointer, SEEK_SET);
}

void create_directory_if_not_exists(const char *path) {
    struct stat st = {0};

    if (stat(path, &st) == -1) {
        if (mkdir(path, 0700) != 0) {
            log_error(logger, "Error creating directory %s", path);
        }
    }
}

int get_blocks_needed(int size, int b_size) {
    int blocks_needed = (size + b_size - 1) / b_size; // + block_size - 1 es para redondear para arriba la cant de bloques necesarios

    if(blocks_needed == 0) {
        return 1;
    }
    return blocks_needed;
}

void initialize_dialfs(const char *path_base_param, int block_size_param, int block_count_param) {
    path_base = path_base_param;

    create_directory_if_not_exists(path_base);
    char blocks_path[256];
    snprintf(blocks_path, sizeof(blocks_path), "%s/bloques.dat", path_base);

    block_size = block_size_param;
    block_count = block_count_param;
    FILE *blocks_file = fopen(blocks_path, "wb");
    if (blocks_file) {
        fseek(blocks_file, (block_size * block_count) - 1, SEEK_SET);
        fputc('\0', blocks_file);
        fclose(blocks_file);
    }

    char bitmap_path[256];
    snprintf(bitmap_path, sizeof(bitmap_path), "%s/bitmap.dat", path_base);

    FILE *bitmap_file = fopen(bitmap_path, "wb");
    if (bitmap_file) {
        size_t bitmap_size = block_count / 8;
        char *bitmap_data = calloc(bitmap_size, 1); // zero values
        fwrite(bitmap_data, 1, bitmap_size, bitmap_file);
        free(bitmap_data);
        fclose(bitmap_file);
    }
}

void create_file_metadata(const char *filename, int start_block, int size) {
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

void fs_create(const char *filename, uint32_t pid) {
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
        create_file_metadata(filename, start_block, 0);
    }

    bitarray_destroy(bitmap);
    free(bitmap_data);
    fclose(bitmap_file);
}

void fs_delete(const char *filename, uint32_t pid) {
    log_info(logger, "PID: %d - Eliminar Archivo: %s", pid, filename);

    char metadata_path[256];
    snprintf(metadata_path, sizeof(metadata_path), "%s/%s", path_base, filename);

    t_config *metadata = config_create(metadata_path);
    if (!metadata) return;

    int start_block = config_get_int_value(metadata, "BLOQUE_INICIAL");
    int file_size = config_get_int_value(metadata, "TAMANIO_ARCHIVO");
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

    int num_blocks = get_blocks_needed(file_size, block_size);
    for (int i = start_block; i < start_block + num_blocks; i++) {
        bitarray_clean_bit(bitmap, i);
    }

    fseek(bitmap_file, 0, SEEK_SET);
    fwrite(bitmap->bitarray, 1, bitmap_size, bitmap_file);

    bitarray_destroy(bitmap);
    free(bitmap_data);
    fclose(bitmap_file);

    remove(metadata_path);
}

void fs_truncate(const char *filename, uint32_t new_size, uint32_t pid) {
    log_info(logger, "PID: %d - Truncar Archivo: %s - Tamaño: %d", pid, filename, new_size);

    char metadata_path[256];
    snprintf(metadata_path, sizeof(metadata_path), "%s/%s", path_base, filename);

    t_config *metadata = config_create(metadata_path);
    if (!metadata) return;

    int start_block = config_get_int_value(metadata, "BLOQUE_INICIAL");
    int actual_size = config_get_int_value(metadata, "TAMANIO_ARCHIVO");
    config_destroy(metadata);

    int new_blocks_needed = get_blocks_needed(new_size, block_size);
    int old_blocks_needed = get_blocks_needed(actual_size, block_size);

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
    // TODO: debug remove
    debug_print_bitmap(bitmap);
    // Chequeamos si hay espacio desde el bit actual, y si no llegamos a cubrir todo los bloques, compactamos.
    if (new_blocks_needed > old_blocks_needed) {
        for (int i = old_blocks_needed; i < new_blocks_needed; i++) {
            if (bitarray_test_bit(bitmap, start_block + i)) {
                fclose(bitmap_file);
                bitarray_destroy(bitmap);
                free(bitmap_data);
                compact_dialfs(pid);
                fs_truncate(filename, new_size, pid);
                return;
            }
        }
        for (int i = old_blocks_needed; i < new_blocks_needed; i++) {
            bitarray_set_bit(bitmap, start_block + i);
        }
    } else if (new_blocks_needed < old_blocks_needed) {
        for (int i = new_blocks_needed; i < old_blocks_needed; i++) {
            bitarray_clean_bit(bitmap, start_block + i);
        }
    }

    fseek(bitmap_file, 0, SEEK_SET);
    fwrite(bitmap->bitarray, 1, bitmap_size, bitmap_file);

    debug_print_bitmap(bitmap);
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

void fs_write(uint32_t phys_addr, uint32_t size, uint32_t f_pointer, uint32_t pid) {
    log_debug(logger, "Phys_addr to read from memory: %d", phys_addr);
    log_info(logger, "PID: %d - Escribir Archivo - Tamaño a Escribir: %d - Puntero Archivo: %d", pid, size, f_pointer);

    char *data = malloc(size);
    snprintf(data, size, "%d: test text.", pid);
    // TODO: Call memory to retrieve data with phys_address and size.
    // TODO: required? data[size - 1] = '\0';

    char blocks_path[256];
    snprintf(blocks_path, sizeof(blocks_path), "%s/bloques.dat", path_base);

    FILE *blocks = fopen(blocks_path, "rb+");
    if (!blocks) return;

    fseek(blocks, f_pointer, SEEK_SET);
    fwrite(data, sizeof(char), size, blocks);
    fclose(blocks);
}

void fs_read(uint32_t phys_addr, uint32_t size, uint32_t f_pointer, uint32_t pid) {
    log_info(logger, "PID: %d - Leer Archivo - Tamaño a Leer: %d - Puntero Archivo: %d", pid, size, f_pointer);
    char blocks_path[256];
    snprintf(blocks_path, sizeof(blocks_path), "%s/bloques.dat", path_base);

    FILE *blocks = fopen(blocks_path, "rb");
    if (!blocks) {
        log_error(logger, "error opening blocks path: %s", blocks_path);
        return;
    }

    debug_print_blocks(blocks);

    char *buffer = malloc(size);
    fseek(blocks, f_pointer, SEEK_SET);
    fread(buffer, sizeof(char), size, blocks);
    fclose(blocks);

    log_debug(logger, "Buffer read: %s", buffer);
    log_debug(logger, "Phys_addr to write on memory: %d", phys_addr);
    // TODO: call memory to write buffer
    // Example: write_to_memory(phys_addr, buffer, size);
    free(buffer);
}

t_list* list_files() {
    struct dirent* entry;
    DIR* dir = opendir(path_base);

    if (!dir) {
        return NULL;
    }

    t_list* filenames = list_create();

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            char* filename = strdup(entry->d_name);
            list_add(filenames, filename);
        }
    }

    closedir(dir);
    return filenames;
}

void compact_dialfs(uint32_t pid) {
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

    t_list* filenames = list_files();
    if (filenames == NULL) {
        fclose(bitmap_file);
        fclose(blocks_file);
        return;
    }
    int current_block = 0;

    void compact_file(char* filename) {
        if (strcmp(filename, "bitmap.dat") == 0 || strcmp(filename, "bloques.dat") == 0) {
            return;
        }

        char metadata_path[256];
        snprintf(metadata_path, sizeof(metadata_path), "%s/%s", path_base, filename);

        t_config *metadata = config_create(metadata_path);
        int start_block = config_get_int_value(metadata, "BLOQUE_INICIAL");
        int size = config_get_int_value(metadata, "TAMANIO_ARCHIVO");
        int blocks_needed = get_blocks_needed(size, block_size);

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

    list_iterate(filenames, (void*)compact_file);

    list_destroy_and_destroy_elements(filenames, free);

    fseek(bitmap_file, 0, SEEK_SET);
    fwrite(bitmap->bitarray, 1, bitmap_size, bitmap_file);

    bitarray_destroy(bitmap);
    free(bitmap_data);
    fclose(bitmap_file);
    fclose(blocks_file);
    
    usleep(config_get_int_value(config, "RETRASO_COMPACTACION"));
    log_info(logger, "PID: %d - Fin Compactación.", pid);
}