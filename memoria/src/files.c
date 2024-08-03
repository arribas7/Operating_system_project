#include <files.h>

FILE *file;
InstructionDictionary dict;

PIDToDict pid_instruction_table[MAX_PROCESSES]; //se carga un archivo de instrucciones para un pid específico 
int pid_dict_count = 0;
/*
Key: In this implementation, the key is the index (program counter) used to access the instructions.
Value: The value is the complete instruction string stored in the complete_line field of the Instruction structure.
*/

void instruction_dictionary_init(InstructionDictionary *dict, int capacity) {
    dict->instructions = (Instruction *)malloc(sizeof(Instruction) * capacity);
    dict->size = 0;
    dict->capacity = capacity;
}
void instructions_put(InstructionDictionary *dict, const char *complete_line) {
    if (dict->size >= dict->capacity) {
        // Resize the array if necessary
        dict->capacity *= 2;
        dict->instructions = (Instruction *)realloc(dict->instructions, sizeof(Instruction) * dict->capacity);
    }
    dict->instructions[dict->size].complete_line = strdup(complete_line);
    dict->size++;
}
void instruction_dictionary_free(InstructionDictionary *dict) {
    for (int i = 0; i < dict->size; i++) {
        free(dict->instructions[i].complete_line);
    }
    free(dict->instructions);
}

void load_instructions_from_file(InstructionDictionary *dict, FILE *file) {
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        // Remove the newline character at the end of the string
        line[strcspn(line, "\n")] = 0;
        instructions_put(dict, line);
    }
}

//relaciona cada diccionario a un pid
void add_pid_instruction_dict(uint32_t pid, InstructionDictionary *dict) {
    if (pid_dict_count >= MAX_PROCESSES) {
        fprintf(stderr, "Maximum number of processes reached\n");
        return;
    }
    pid_instruction_table[pid_dict_count].pid = pid;
    pid_instruction_table[pid_dict_count].instruction_dict = dict;
    //pid_instruction_table[pid_dict_count].instruction_dict = *dict;
    pid_dict_count++;
}
/*
void send_instruction(const char *instruction, int socket_cliente) {
     t_paquete *paquete = crear_paquete(MENSAJE);
     agregar_a_paquete(paquete, (void *)instruction, strlen(instruction) + 1);
     enviar_paquete(paquete, socket_cliente);
     eliminar_paquete(paquete);
}
*/
FILE *open_file(const char *file_path) {
   // printf("Step 3: %s\n",file_path);
    FILE *file = fopen(file_path, "r");
    if (file != NULL) {
        printf("File successfully opened: %s\n", file_path);
    } else {
        perror("Error opening file");
    }
    return file;
}


/*
// OLD:Function to get a complete instruction by index
const char *get_complete_instruction(const InstructionDictionary *dict, int index) {
    if (index >= 0 && index < dict->size) {
        return dict->instructions[index].complete_line;
    } else {
        return "__ERROR__";
    }
}
*/

// NUEVO: Función para obtener el diccionario de instrucciones por PID
InstructionDictionary *get_dictionary(uint32_t pid) {
    for (int i = 0; i < pid_dict_count; i++) {
        if (pid_instruction_table[i].pid == pid) {
            return pid_instruction_table[i].instruction_dict;
        }
    }
    return NULL;
}

const char *get_complete_instruction(uint32_t pid, int pc) {
    InstructionDictionary *dict = get_dictionary(pid);

    if (dict != NULL) {
        if (pc >= 0 && pc < dict->size) {
            return dict->instructions[pc].complete_line;
        }
    }
    return NULL;
}
/*
void free_pid_instruction_table() {
    for (int i = 0; i < pid_dict_count; i++) {
        InstructionDictionary *dict = pid_instruction_table[i].instruction_dict;
        for (int j = 0; j < dict->size; j++) {
            free(dict->instructions[j].complete_line);
        }
        free(dict->instructions);

        // Reiniciar el tamaño y capacidad del diccionario
        dict->size = 0;
        dict->capacity = 0;
    }
    // Reiniciar el contador de PIDs
    pid_dict_count = 0;
}
*/

void handle_create_process(const char *file_path, uint32_t pid, t_config* config){
      // printf("Step 2: %s\n",file_path);
    file = open_file(file_path);
    int pc = 0;
    if (file != NULL) {
        printf("File to intructions succesfully opened.\n");
        InstructionDictionary *dictionary = malloc(sizeof(InstructionDictionary));
        instruction_dictionary_init(dictionary, 10);  
        load_instructions_from_file(dictionary, file); 
        fclose(file); 
        add_pid_instruction_dict(pid, dictionary); //Nueva funcion
        for (pc = 0; pc < dictionary->size; pc++) {
            const char *complete_instruction = get_complete_instruction(pid, pc);
            //printf("PID: %u, PC: %d, Complete Instruction: %s\n", pid, pc, complete_instruction);
        }
        //instruction_dictionary_free(&dictionary);
        // free_pid_instruction_table();
    } else
        perror("Error opening File to intructions");

    int tamanio_proceso = 0;
    int tamanio_marco = config_get_int_value(config,"TAM_PAGINA");
    TablaPaginas* tabla = crearTablaPaginas(pid,tamanio_proceso,tamanio_marco);    
    //destroy_page_table(tabla);
}
/*
void cleanup() {
    free_pid_instruction_table();
    // Otras limpiezas adicionales si es necesario
}
*/