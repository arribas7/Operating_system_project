#include <files.h>

FILE *file;
InstructionDictionary dict;

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

// Function to get a complete instruction by index
const char *get_complete_instruction(const InstructionDictionary *dict, int index) {
    if (index >= 0 && index < dict->size) {
        return dict->instructions[index].complete_line;
    } else {
        return "__ERROR__";
    }
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
    printf("Step 3: %s\n",file_path);
    FILE *file = fopen(file_path, "r");
    if (file != NULL) {
        printf("File successfully opened: %s\n", file_path);
    } else {
        perror("Error opening file");
    }
    return file;
}

void handle_create_process(const char *file_path, uint32_t pid){
       printf("Step 2: %s\n",file_path);
       //OPEN FILE THAT CORRESPONDS TO PATH_INFO - PID FROM KERNEL
       file = open_file(file_path);
       // FILE *file = fopen("scripts-pruebas/file1", "r");
          if (file != NULL) {
              printf("File to intructions succesfully opened.\n");
              InstructionDictionary dictionary;
              instruction_dictionary_init(&dictionary, 10);  
              load_instructions_from_file(&dictionary, file); 
              fclose(file); 

              for (int pc = 0; pc < dictionary.size; pc++) {
              const char *complete_instruction = get_complete_instruction(&dictionary, pc);
              printf("PC: %d, Complete Instruction: %s\n", pc, complete_instruction);
              }
              instruction_dictionary_free(&dictionary);

            } else {
              perror("Error opening File to intructions");
             }
  
               
}