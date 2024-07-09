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
    pthread_mutex_t mutex;
} t_resource;

typedef enum {
    RESOURCE_SUCCESS_OP,
    RESOURCE_NOT_FOUND,
    RESOURCE_BLOCKED,
    RESOURCE_RELEASED
} t_result;

typedef struct {
    t_pcb* pcb_released;
    t_result result;
} t_resource_op_return;

void initialize_resources();
t_resource* new_resource(char* name, int instances);
t_resource_op_return resource_wait(t_pcb* pcb, char* name);
t_resource_op_return resource_signal(char* name);
void destroy_resource(t_resource* resource);
void destroy_resource_list();