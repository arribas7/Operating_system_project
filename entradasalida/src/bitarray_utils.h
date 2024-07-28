#ifndef BITARRAY_UTILS_H
#define BITARRAY_UTILS_H

#include <stdint.h>
#include <commons/log.h>
#include <commons/bitarray.h>
#include <stdio.h>
#include <string.h>

//extern t_bitarray bitmap;

//Devuelve en una string el bitmap actual para posterior análisis
char *bitmap_string(void);

//Función para encontrar la posición inicial del primer "hueco" en una cadena de '1's y '0's
//Devuelve -1 si no hay ningún hueco
int find_hole(const char *str);

//Analiza el tamaño del hueco cuya posición inicial está dada por parametro
//Devuelve -1 si la posición es inválida
//Devuelve 0 si la posición no es la de un hueco
int hole_size(const char *bitmap_string, size_t start_pos);

/*
    Toma un archivo, levanta su metadata y lo corre hacia la izquierda
    todo lo posible para compactar memoria.
*/
void compact_file(char* filename);

/*
    Ordena una lista de filenames para que aparezcan según su posición en el bitmap
*/
void sort_filenames_by_position(t_list* list_files);

void log_filenames(t_list* list_files);

#endif //BITARRAY_UTILS_H