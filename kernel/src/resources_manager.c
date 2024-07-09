#include "resources_manager.h"
#include <commons/config.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include <pthread.h>

t_list* resources_list;
pthread_mutex_t mutex_resources;
extern t_config *config;

t_resource* new_resource(char* name, int instances) {
    t_resource* resource = malloc(sizeof(t_resource));
    resource->name = strdup(name);
    resource->instances = instances;
    resource->blocked_queue = queue_create();
    pthread_mutex_init(&resource->mutex, NULL);
    return resource;
}

void initialize_resources() {
    resources_list = list_create();
    pthread_mutex_init(&mutex_resources, NULL);

    char** resource_names = config_get_array_value(config, "RECURSOS");
    char** resource_instances_str = config_get_array_value(config, "INSTANCIAS_RECURSOS");

    for (int i = 0; resource_names[i] != NULL; i++) {
        int instances = atoi(resource_instances_str[i]);
        t_resource* resource = new_resource(resource_names[i], instances);
        list_add(resources_list, resource);
    }

    string_array_destroy(resource_names);
    string_array_destroy(resource_instances_str);
}

void destroy_resource(t_resource* resource) {
    free(resource->name);
    queue_destroy_and_destroy_elements(resource->blocked_queue, (void*)free);
    pthread_mutex_destroy(&resource->mutex);
    free(resource);
}

void destroy_resource_list() {
    list_destroy_and_destroy_elements(resources_list, (void*) destroy_resource);
}

t_resource* find_resource(char* name) {
    bool is_resource(void* elem) {
        t_resource* resource = (t_resource*)elem;
        return strcmp(resource->name, name) == 0;
    }

    pthread_mutex_lock(&mutex_resources);
    t_resource* resource = list_find(resources_list, is_resource);
    pthread_mutex_unlock(&mutex_resources);

    return resource;
}

t_resource_op_return resource_wait(t_pcb* pcb, char* name) {
    t_resource_op_return op_return;
    op_return.result = RESOURCE_SUCCESS_OP;
    t_resource* resource = find_resource(name);

    if (resource == NULL) {
        op_return.result = RESOURCE_NOT_FOUND;
        return op_return;
    }
    pthread_mutex_lock(&resource->mutex);
    resource->instances--;
    if (resource->instances < 0) {
        queue_push(resource->blocked_queue, pcb);
        op_return.result = RESOURCE_BLOCKED;
    }
    pthread_mutex_unlock(&resource->mutex);
    return op_return;
}

t_resource_op_return resource_signal(char* name) {
    t_resource_op_return op_return;
    op_return.result = RESOURCE_SUCCESS_OP;

    t_resource* resource = find_resource(name);
    if (resource == NULL) {
        op_return.result = RESOURCE_NOT_FOUND;
        return op_return;
    }

    pthread_mutex_lock(&resource->mutex);
    resource->instances++;
    if (!queue_is_empty(resource->blocked_queue)) {
        op_return.result = RESOURCE_RELEASED;
        op_return.pcb_released = queue_pop(resource->blocked_queue);
    }
    pthread_mutex_unlock(&resource->mutex);
    return op_return;
}