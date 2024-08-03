#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <commons/config.h>
#include <commons/bitarray.h>
#include <commons/collections/list.h>
#include <dirent.h>
#include "dialfs.h"
#include "bitarray_utils.h"
#include "generic_st.h"
#include <utils/inout.h>

int block_size;
int block_count;
const char *path_base;
extern t_log* logger;
extern t_config* config;
t_bitarray *bitmap;
size_t bitmap_size;
FILE *blocks_file = NULL;

void debug_print_bitmap() {
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
    if (!blocks_file) {
        log_error(logger, "Error: blocks_file es NULL.");
        return;
    }

    long original_pointer = ftell(blocks_file);
    fseek(blocks_file, 0, SEEK_END);
    long file_size = ftell(blocks_file);
    fseek(blocks_file, 0, SEEK_SET);

    char *buffer = malloc(file_size + 1);  // +1 para el terminador nulo
    if (!buffer) {
        log_error(logger, "NO SE PUDO ASIGNAR MEMORIA PARA BUFFER");
        return;
    }

    fread(buffer, sizeof(char), file_size, blocks_file);
    buffer[file_size] = '\0';

    char *log_string = malloc(file_size + 50);
    if (!log_string) {
        log_error(logger, "NO SE PUDO ASIGNAR MEMORIA PARA log_string");
        free(buffer);
        return;
    }

    strcpy(log_string, "[DEBUG] Blocks file content:\n");
    strcat(log_string, buffer);
    strcat(log_string, "\n");

    log_debug(logger, "%s", log_string);

    free(log_string);
    free(buffer);

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

    blocks_file = fopen(blocks_path, "rb+");
    if (blocks_file == NULL) {
        // Si no existe, crearlo en modo escritura/lectura
        blocks_file = fopen(blocks_path, "wb+");
        if (blocks_file) {
            fseek(blocks_file, (block_size * block_count) - 1, SEEK_SET);
            fputc('\0', blocks_file); // asegurar el tamaño que se pide
        }
    } else {
        // Verificar el tamaño del archivo para determinar si es necesario inicializarlo
        fseek(blocks_file, 0, SEEK_END);
        long file_size = ftell(blocks_file);
        if (file_size < (block_size * block_count)) {
            fseek(blocks_file, (block_size * block_count) - 1, SEEK_SET);
            fputc('\0', blocks_file);
        }
    }
    if (blocks_file) {
        fclose(blocks_file);
    }

    char bitmap_path[256];
    snprintf(bitmap_path, sizeof(bitmap_path), "%s/bitmap.dat", path_base);

    // Intentar abrir el archivo en modo lectura/escritura
    int fd = open(bitmap_path, O_RDWR);
    if (fd == -1) {
        // Si no existe, crearlo en modo lectura/escritura
        fd = open(bitmap_path, O_RDWR | O_CREAT, 0600);
        if (fd == -1) {
            log_error(logger, "Error opening bitmap file %s", bitmap_path);
            return;
        }
    }

    bitmap_size = (block_count + 7) / 8;
    // Verificar el tamaño del archivo para determinar si es necesario inicializarlo
    struct stat st;
    if (fstat(fd, &st) == 0) {
        if (st.st_size < bitmap_size) {
            if (ftruncate(fd, bitmap_size) == -1) {
                log_error(logger, "Error setting size of bitmap file %s", bitmap_path);
                close(fd);
                return;
            }
        }
    }

    char *bitmap_data = mmap(NULL, bitmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (bitmap_data == MAP_FAILED) {
        log_error(logger, "Error mapping bitmap file %s", bitmap_path);
        close(fd);
        return;
    }

    if (st.st_size < bitmap_size) {
        memset(bitmap_data + st.st_size, 0, bitmap_size - st.st_size);
    }

    bitmap = bitarray_create_with_mode(bitmap_data, bitmap_size, LSB_FIRST);

    close(fd);
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

    int start_block = -1;

    //Se recorre el bitmap buscando un bloque libre
    for (int i = 0; i < bitarray_get_max_bit(bitmap); i++) {
        if (!bitarray_test_bit(bitmap, i)) {
            start_block = i;

            //Se encuentra el primer bloque libre y se marca ocupado en el bitmap
            bitarray_set_bit(bitmap, i);
            break;
        }
    }

    //Se fija si se pudo encontrar un bloque libre
    if (start_block != -1) {

        //Se sincroniza el bitmap con el FS asegurandose de que se pueda escribir en el disco
        msync(bitmap->bitarray, bitmap_size, MS_SYNC);

        //Se crea el archivo de metadata con un bloque inicial y un tamaño inicial de 0
        create_file_metadata(filename, start_block, 0);
    }
    //debug_print_bitmap();
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

    int num_blocks = get_blocks_needed(file_size, block_size);
    for (int i = start_block; i < start_block + num_blocks; i++) {
        bitarray_clean_bit(bitmap, i);
    }

    msync(bitmap->bitarray, bitmap_size, MS_SYNC);
    remove(metadata_path);
}

int fs_write(const char* filename, uint32_t phys_addr, uint32_t size, uint32_t f_pointer, uint32_t pid, int mem_connection) {
    log_debug(logger, "Phys_addr to read from memory: %d", phys_addr);
    log_info(logger, "PID: %d - Escribir Archivo - Tamaño a Escribir: %d - Puntero Archivo: %d", pid, size, f_pointer);

    t_req_to_r* mem_req = req_to_read(pid, size, phys_addr);
    send_req_to_r(mem_req, mem_connection);
    char* data = receive_word(mem_connection);
    if(strlen(data) == 0) {
        log_error(logger, "ERROR READING FROM MEMORY");
        return -1;
    }
    char metadata_path[256];
    snprintf(metadata_path, sizeof(metadata_path), "%s/%s", path_base, filename);

    t_config *metadata = config_create(metadata_path);
    if (!metadata) {
        log_error(logger, "Error abriendo archivo de metadata: %s", metadata_path);
        free(data);
        return -1;
    }

    int start_block = config_get_int_value(metadata, "BLOQUE_INICIAL");
    int file_size = config_get_int_value(metadata, "TAMANIO_ARCHIVO");

    if (f_pointer + size > file_size) {
        char size_str[12];
        sprintf(size_str, "%d", f_pointer + size);
        config_set_value(metadata, "TAMANIO_ARCHIVO", size_str);
        config_save(metadata);
    }
    config_destroy(metadata);

    char blocks_path[256];
    snprintf(blocks_path, sizeof(blocks_path), "%s/bloques.dat", path_base);

    FILE *blocks = fopen(blocks_path, "rb+");
    if (!blocks) {
        log_error(logger, "Error abriendo archivo de bloques: %s", blocks_path);
        free(data);
        return -1;
    }

    int start_position = start_block * block_size + f_pointer;

    fseek(blocks, start_position, SEEK_SET);
    fwrite(data, sizeof(char), size, blocks);
    fclose(blocks);

    free(data);
    return 0;
}

int fs_read(const char* filename, uint32_t phys_addr, uint32_t size, uint32_t f_pointer, uint32_t pid, int mem_connection) {
    log_info(logger, "PID: %d - Leer Archivo - Tamaño a Leer: %d - Puntero Archivo: %d", pid, size, f_pointer);
    char metadata_path[256];
    snprintf(metadata_path, sizeof(metadata_path), "%s/%s", path_base, filename);

    t_config *metadata = config_create(metadata_path);
    if (!metadata) {
        log_error(logger, "Error abriendo archivo de metadata: %s", metadata_path);
        
        return -1;
    }

    int start_block = config_get_int_value(metadata, "BLOQUE_INICIAL");
    int file_size = config_get_int_value(metadata, "TAMANIO_ARCHIVO");
    config_destroy(metadata);

    if (f_pointer + size > file_size) {
        log_error(logger, "Intento de leer fuera del límite del archivo");
        return -1;
    }

    char blocks_path[256];
    snprintf(blocks_path, sizeof(blocks_path), "%s/bloques.dat", path_base);

    // Abrir el archivo de bloques
    FILE *blocks = fopen(blocks_path, "rb");
    if (!blocks) {
        log_error(logger, "Error abriendo archivo de bloques: %s", blocks_path);
        return -1;
    }

    // Calcular la posición de inicio en el archivo de bloques
    int start_position = start_block * block_size + f_pointer;

    // Leer el contenido del archivo desde la posición calculada
    char *buffer = malloc(size);
    fseek(blocks, start_position, SEEK_SET);
    fread(buffer, sizeof(char), size, blocks);
    fclose(blocks);

    log_debug(logger, "Buffer leído: %s", buffer);
    log_debug(logger, "Phys_addr para escribir en memoria: %d", phys_addr);

    t_req_to_w* mem_req = req_to_write(pid, buffer, phys_addr);
    send_req_to_w(mem_req, mem_connection);
    free(buffer);
    return 0;
}

t_list* list_files(char* file_exception) {
    struct dirent* entry;
    DIR* dir = opendir(path_base);

    if (!dir) {
        return NULL;
    }

    t_list* filenames = list_create();

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            char* filename = strdup(entry->d_name);
            if (file_exception == NULL || strcmp(filename, file_exception) != 0) {
                list_add(filenames, filename);
            }
        }
    }

    closedir(dir);
    return filenames;
}

void compact_dialfs(char* file_exception) {
    log_info(logger, "Inicio Compactación.");

    //Se construye la ruta al archivo de bloques
    char blocks_path[256];
    snprintf(blocks_path, sizeof(blocks_path), "%s/bloques.dat", path_base);

    //Se abre el archivo de bloques en modo de lectura y escritura
    blocks_file = fopen(blocks_path, "rb+");
    if (!blocks_file) {
        return;
    }

    //Se obtiene una lista de nombre de archivos
    //Opcionalmente se puede excluir un archivo
    t_list* filenames = list_files(file_exception);
    if (filenames == NULL) {
        fclose(blocks_file);
        return;
    }

    //Variable para rastrear el bloque actual durante compactación
    int current_block = 0;

    //log_filenames_list(filenames);
    sort_filenames_by_position(filenames);
    //log_filenames_list(filenames);
    list_iterate(filenames, (void*)compact_file);
    //log_filenames_list(filenames);

    msync(bitmap->bitarray, bitmap_size, MS_SYNC);
    fclose(blocks_file);
    
    usleep(config_get_int_value(config, "RETRASO_COMPACTACION") * 1000);
    log_info(logger, "Fin Compactación.");
    list_destroy_and_destroy_elements(filenames, free);
}

int find_first_free_block(int blocks_needed) {
    for (int i = 0; i <= bitarray_get_max_bit(bitmap); i++) {
        bool block_free = true;
        for (int j = 0; j < blocks_needed; j++) {
            if (bitarray_test_bit(bitmap, i + j)) {
                block_free = false;
                break;
            }
        }
        if (block_free) {
            return i;
        }
    }
    return -1;
}

int move_file_to_free_space(const char *filename, char* buffer, int new_blocks_needed, int actual_size) {
    int new_start_block = find_first_free_block(new_blocks_needed);
    if (new_start_block == -1) {
        log_error(logger, "No hay suficiente espacio libre después de la compactación.");
        return -1;
    }

    char blocks_path[256];
    snprintf(blocks_path, sizeof(blocks_path), "%s/bloques.dat", path_base);

    //FILE *blocks_file = fopen(blocks_path, "rb+");
    blocks_file = fopen(blocks_path, "rb+");

    fseek(blocks_file, new_start_block * block_size, SEEK_SET);
    fwrite(buffer, sizeof(char), actual_size, blocks_file);

    free(buffer);
    fclose(blocks_file);

    msync(bitmap->bitarray, bitmap_size, MS_SYNC);
    debug_print_bitmap();

    char metadata_path[256];
    snprintf(metadata_path, sizeof(metadata_path), "%s/%s", path_base, filename);
    t_config *metadata = config_create(metadata_path);

    char new_start_block_str[12];

    sprintf(new_start_block_str, "%d", new_start_block);
    config_set_value(metadata, "BLOQUE_INICIAL", new_start_block_str);

    log_warning(logger, "NEW START BLOCK OF FILE %s IS %d (DECIMAL) %s (STRING)", filename, new_start_block, new_start_block_str);

    config_save(metadata);
    config_destroy(metadata);

    for (int j = 0; j < new_blocks_needed; j++) {
        bitarray_set_bit(bitmap, new_start_block + j);
    }

    return new_start_block;
}

void fs_truncate(const char *filename, uint32_t new_size, uint32_t pid) {
    log_info(logger, "PID: %d - Truncar Archivo: %s - Tamaño: %d", pid, filename, new_size);

    //Construye la ruta al archivo de metadata
    char metadata_path[256];
    snprintf(metadata_path, sizeof(metadata_path), "%s/%s", path_base, filename);

    //Carga la ruta al archivo de metadata
    t_config *metadata = config_create(metadata_path);
    if (!metadata) return;

    //Se obtiene el bloque inicial y el tamaño del archivo desde la metadata
    int start_block = config_get_int_value(metadata, "BLOQUE_INICIAL");
    int actual_size = config_get_int_value(metadata, "TAMANIO_ARCHIVO");
    config_destroy(metadata);

    //Calcula el tamaño en bloques de memoria que se necesita para el nuevo tamaño y el viejo
    int new_blocks_needed = get_blocks_needed(new_size, block_size);
    int old_blocks_needed = get_blocks_needed(actual_size, block_size);

    log_debug(logger, "bitmap before:");
    debug_print_bitmap();

    //Si se necesita más espacio
    if (new_blocks_needed > old_blocks_needed) {

        //For Loop que recorre la cantidad de espacio nueva requerida
        for (int i = old_blocks_needed; i < new_blocks_needed; i++) {

            //Si el bloque está siendo utilizado
            if (bitarray_test_bit(bitmap, start_block + i)) {
                //*****************SE LEE EL ARCHIVO Y SE ALMACENA EN UN BUFFER*****************

                char *buffer = malloc(old_blocks_needed * block_size);

                /*
                log_info(logger, "BEGINNING BUFFERING");
                log_info(logger, "Blocks File: %p", blocks_file);
                log_info(logger, "Blocks file is at %ld", ftell(blocks_file));
                log_info(logger, "Trying to fseek %d", start_block * block_size);
                log_info(logger, "Block Size %d", block_size);
                */
                

                if (blocks_file == NULL){
                    free(buffer);
                    log_error(logger, "NULL FILE POINTER");
                    return;
                }

                if (buffer == NULL) {
                    log_error(logger, "Failed to allocate buffer");
                    return;
                }

                char blocks_path[256];
                snprintf(blocks_path, sizeof(blocks_path), "%s/bloques.dat", path_base);
                blocks_file = fopen(blocks_path, "wb");

                //Se mueve el puntero del archivo al start block
                fseek(blocks_file, start_block * block_size, SEEK_SET);

                //Se lee desde el inicio la cantidad de bloques necesarios y se almacena en buffer
                fread(buffer, block_size, old_blocks_needed, blocks_file);

                //*****************SE LIMPIA EL BITMAP*****************

                for (int j = 0; j < old_blocks_needed; j++) {
                    bitarray_clean_bit(bitmap, start_block + j);
                }

                //*****************COMPACTACIÓN*****************

                //Se compacta el sistema de archivos exceptuando al archivo actual
                compact_dialfs(filename);

                //*****************ESCRITURA*****************

                //Se intenta mover el archivo a un espacio libre después de la compactación
                move_file_to_free_space(filename, buffer, new_blocks_needed, actual_size);
                break;
            }
        }
        //Se marcan los nuevos bloques del archivo como utilizados en el bitmap
        /*
        for (int i = old_blocks_needed; i < new_blocks_needed; i++) {
            bitarray_set_bit(bitmap, start_block + i);
        }
        */
    } else if (new_blocks_needed < old_blocks_needed) {
        //Si se necesitan menos bloques que antes, transcurrir el espacio descartado
        for (int i = new_blocks_needed; i < old_blocks_needed; i++) {
            //Liberar los bloques
            bitarray_clean_bit(bitmap, start_block + i);
        }
    }

    //Sincronizar Bitmap con FS
    msync(bitmap->bitarray, bitmap_size, MS_SYNC);
    log_debug(logger, "bitmap after:");
    debug_print_bitmap();

    //Actualizar Metadata
    metadata = config_create(metadata_path);
    char new_size_str[12];
    sprintf(new_size_str, "%d", new_size);
    config_set_value(metadata, "TAMANIO_ARCHIVO", new_size_str);
    config_save(metadata);
    config_destroy(metadata);

    t_list* completeFilenames = list_files(NULL);
    log_filenames_list(completeFilenames);
    list_destroy_and_destroy_elements(completeFilenames, free);

}
