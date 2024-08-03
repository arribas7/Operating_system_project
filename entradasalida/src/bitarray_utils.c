#include <stdlib.h>
#include <stdio.h>
#include <commons/log.h>
#include <commons/config.h>
#include <utils/client.h>
#include <utils/server.h>
#include <utils/inout.h>
#include <commons/bitarray.h>
#include "bitarray_utils.h"
#include <string.h>
#include <stddef.h>

extern t_bitarray *bitmap;
extern const char *path_base;
extern int current_block;
extern FILE *blocks_file;
extern int block_size;

char *bitmap_string(void) {
    size_t max_bits = bitarray_get_max_bit(bitmap);
    char *log_string = malloc(max_bits + 1);
    if (!log_string) {
        return NULL;
    }

    for (size_t i = 0; i < max_bits; ++i) {
        if (bitarray_test_bit(bitmap, i)) {
            log_string[i] = '1';
        } else {
            log_string[i] = '0';
        }
    }
    log_string[max_bits] = '\0';

    return log_string;
}

int find_hole(const char *str) {
    int i = 0;
    
    // Buscar la primera secuencia de '0's que esté rodeada por '1's
    while (str[i] != '\0') {
        // Verificar si encontramos un '0' que podría ser el inicio de un hueco
        if (str[i] == '0') {
            int start = i;
            // Avanzar hasta el final de la secuencia de '0's
            while (str[i] == '0') {
                i++;
            }
            // Verificar si la secuencia de '0's está rodeada por '1's
            if (str[i] == '1' && start > 0 && str[start - 1] == '1') {
                // Hemos encontrado un hueco
                return start;
            }
        } else {
            i++;
        }
    }
    
    // No se encontró ningún hueco
    return -1;
}

int hole_size(const char *bitmap_string, size_t start_pos) {
    // Verificar si la cadena es nula o la posición de inicio es negativa o inválida
    if (bitmap_string == NULL || start_pos >= strlen(bitmap_string) || start_pos < 1) {
        return -1; // Posición inválida
    }

    int size = 0;
    size_t i = start_pos;

    // Contar el tamaño del hueco
    while (bitmap_string[i] == '0') {
        size++;
        i++;
    }

    return size;
}

int find_file_start_block(char* filename) {
    // Check para saber si es bitmap.dat o bloques.dat y abortar la función
    if (strcmp(filename, "bitmap.dat") == 0 || strcmp(filename, "bloques.dat") == 0) {
        return -1;
    }

    // Se construye la ruta a la metadata
    char metadata_path[256];
    snprintf(metadata_path, sizeof(metadata_path), "%s/%s", path_base, filename); // path_base es variable global de este archivo

    // Bloque inicial
    t_config *metadata = config_create(metadata_path);
    if (metadata == NULL) {
        return -1; // Error al crear la configuración
    }

    int start_block = config_get_int_value(metadata, "BLOQUE_INICIAL");

    // Liberar la memoria de la configuración
    config_destroy(metadata);

    return start_block;
}

bool position_list_priority(char* filename_a, char* filename_b) {
    if (strcmp(filename_a, "bitmap.dat") == 0 || strcmp(filename_a, "bloques.dat") == 0) {
        return false;
    }

    if (strcmp(filename_b, "bitmap.dat") == 0 || strcmp(filename_b, "bloques.dat") == 0) {
        return false;
    }

    int a_pos = find_file_start_block(filename_a);
    int b_pos = find_file_start_block(filename_b);

    if (a_pos < 0 || b_pos < 0) {
        log_error(logger, "INVALID POSITION");
        log_error(logger, "FILE %s POSITION %d", filename_a, a_pos);
        log_error(logger, "FILE %s POSITION %d", filename_b, b_pos);
        return false;
    }
    
    return a_pos < b_pos;
}

void sort_filenames_by_position(t_list* list_files){
    list_sort(list_files, position_list_priority);
}

void log_filename(char* filename){
    //Check para saber si es bitmap.dat o bloques.dat y abortar la funcion
    if (strcmp(filename, "bitmap.dat") == 0 || strcmp(filename, "bloques.dat") == 0) {
        return;
    }

    //Se construye la ruta a la metadata
    char metadata_path[256];
    snprintf(metadata_path, sizeof(metadata_path), "%s/%s", path_base, filename); //path_base es variable global de este archivo

    //Bloque inicial y tamaño del archivo desde la metadata
    t_config *metadata = config_create(metadata_path);
    if (metadata == NULL) {
        return; // Error al crear la configuración
    }

    int start_block = config_get_int_value(metadata, "BLOQUE_INICIAL");
    int size = config_get_int_value(metadata, "TAMANIO_ARCHIVO");

    //Bloques necesarios
    int blocks_needed = get_blocks_needed(size, block_size);

    log_info(logger, "FILE %s SIZE %d AT BLOCK %d", filename, blocks_needed, start_block);

    // Liberar la memoria de la configuración
    config_destroy(metadata);
}

void log_filenames_list(t_list* list_files){
    log_info(logger, "--------------------------");
    list_iterate(list_files, log_filename);
    log_info(logger, "--------------------------");
}

//Función para compactar cada archivo individualmente
void compact_file(char* filename) {

    //**************SE LEVANTA LA METADATA DEL ARCHIVO**************

    //Check para saber si es bitmap.dat o bloques.dat y abortar la funcion
    if (strcmp(filename, "bitmap.dat") == 0 || strcmp(filename, "bloques.dat") == 0) {
        return;
    }

    //Se construye la ruta a la metadata
    char metadata_path[256];
    snprintf(metadata_path, sizeof(metadata_path), "%s/%s", path_base, filename); //path_base es variable global de este archivo

    //Bloque inicial y tamaño del archivo desde la metadata
    t_config *metadata = config_create(metadata_path);
    if (metadata == NULL) {
        return; // Error al crear la configuración
    }

    int start_block = config_get_int_value(metadata, "BLOQUE_INICIAL");
    int size = config_get_int_value(metadata, "TAMANIO_ARCHIVO");

    //Bloques necesarios
    int blocks_needed = get_blocks_needed(size, block_size);

    //**************SE LEEN LOS DATOS DEL ARCHIVO**************

    //Si el bloque de inicio del archivo no es el bloque actual (comienza en 0)
    //Se arma un buffer con el espacio necesario para el archivo
    char *buffer = malloc(blocks_needed * block_size);
    if (!buffer) {
        config_destroy(metadata); // Free metadata if allocation fails
        return;
    }

    //Se mueve el puntero del archivo al start block
    fseek(blocks_file, start_block * block_size, SEEK_SET);

    //Se lee desde el inicio la cantidad de bloques necesarios y se almacena en buffer
    fread(buffer, block_size, blocks_needed, blocks_file);

    log_info(logger, "El archivo %s ocupa %d bloques", filename, blocks_needed);

    //**************SE BORRAN LOS DATOS DEL ARCHIVO DEL BITMAP**************

    for (int i = 0; i < blocks_needed; i++) {
        bitarray_clean_bit(bitmap, start_block + i);
    }

    //**************SE BUSCA EL INICIO DEL ARCHIVO BITMAP O EL INICIO DE UN HUECO**************

    while (start_block > 0 && bitarray_test_bit(bitmap, start_block) == 0 && bitarray_test_bit(bitmap, start_block - 1) == 0) {
        start_block--;
    }

    //**************SE ESCRIBEN LOS DATOS EN ESTA NUEVA POSICION DEL BITMAP**************

    //Se mueve el puntero del archivo al start block
    fseek(blocks_file, start_block * block_size, SEEK_SET);

    //Se escribe el contenido del buffer en el current block
    fwrite(buffer, block_size, blocks_needed, blocks_file);

    for (int i = 0; i < blocks_needed; i++) {
        bitarray_set_bit(bitmap, start_block + i);
    }

    //**************SE ACTUALIZA LA METADATA**************
    free(buffer); // Free the buffer after its use

    //Se crea current_block_str y se le carga current_block
    char current_block_str[12];
    //sprintf(current_block_str, "%d", current_block);
    sprintf(current_block_str, "%d", start_block);

    //Se cambia el valor del bloque inicial en la metadata del archivo
    config_set_value(metadata, "BLOQUE_INICIAL", current_block_str);
    config_save(metadata);
    config_destroy(metadata);
    
    //current_block += blocks_needed;
}
