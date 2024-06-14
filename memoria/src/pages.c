#include <pages.h>
#include <memoria.h>
/*Each page in the main memory (physical memory) is mapped to a frame. 
Páginas: Se refieren a las divisiones lógicas de un proceso en la memoria virtual. 
Cada proceso se divide en páginas del mismo tamaño.
Frames: Se refieren a las divisiones físicas de la memoria principal (RAM).
Cada página de un proceso se carga en un frame de la memoria principal.
*/


int initPaging(void) {
    // Allocate main memory
 
    printf("TAMAÑO MEMORIA TOTAL: %d\n",value_memory.memory_size);

       mainMemory = malloc(value_memory.memory_size);
    if (mainMemory == NULL) {
        perror("Memory allocation failed!\n");
        return 0;
    }
    
    int pageSize = value_memory.page_size;
    int frameCount = value_memory.memory_size / pageSize; 
    //Each bit in the bit array represents one frame (0 for free, 1 for occupied).

    log_info(logger, "Memory has %d frames of %d bytes", frameCount, pageSize);


    //se divide cant_frames_ppal por 8 para obtener la cantidad de bytes necesarios para almacenar todos los bits necesarios para representar los marcos de memoria.
      char* mapabits = (char*)malloc(frameCount / 8); //CHEQUEAR ESTA CUENTA
    if (mapabits == NULL) {
        perror("ERROR: No se pudo asignar memoria.\n");
        return 1;
    }
      /*frameCount is the number of frames in the main memory.. 
       memset: Initializes this memory block to 0, indicating all frames are free.
       Each bit in the bit array represents one frame (0 for free, 1 for occupied).
     */


    memset(mapabits, 0, frameCount / 8); 
    // After memset
    /*
    Memory Frame Allocation After memset
    -----------------------------------------------------------
    | Byte 0          | Byte 1          | Byte 2          | ...
    -----------------------------------------------------------
    | 0 0 0 0 0 0 0 0 | 0 0 0 0 0 0 0 0 | 0 0 0 0 0 0 0 0 | ...
    -----------------------------------------------------------
    | 0 1 2 3 4 5 6 7 | 8 9 10 11 12 13 14 15 | 16 17 18 19 20 21 22 23 | ...
    (Bits representing frames; all bits set to 0)
    */
    //Se usa funciones  bitarray de las commons 
    frames = bitarray_create_with_mode(mapabits, frameCount / 8, MSB_FIRST);

   // pageTable = list_create();


    return 1;
}