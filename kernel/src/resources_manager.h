#include <commons/config.h>
#include <commons/collections/queue.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include <pthread.h>
#include <utils/kernel.h>

typedef struct {
    char* name;
    int instances;
    t_queue* blocked_queue;
    t_list* assigned_pcbs;
    pthread_mutex_t mutex;
} t_resource;

typedef enum {
    RESOURCE_SUCCESS_OP,
    RESOURCE_NOT_FOUND,
    RESOURCE_BLOCKED,
    RESOURCE_RELEASED
} t_result;

void initialize_resources();
t_resource* new_resource(char* name, int instances);
t_result resource_wait(t_pcb* pcb, char* name);
t_result resource_signal(t_pcb *pcb, char* name);
void destroy_resource(t_resource* resource);
void destroy_resource_list();
void release_all_resources(t_pcb *pcb);
