#include "resources_manager.h"
#include <commons/config.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include <commons/log.h>
#include <state_lists.h>
#include <utils/kernel.h>
#include <pthread.h>

extern t_list* resources_list;
extern pthread_mutex_t mutex_resources;

extern t_config *config;
extern t_log* logger;

extern t_list* list_BLOCKED;
extern pthread_mutex_t mutex_blocked;

extern t_list* list_READY;
extern pthread_mutex_t mutex_ready;

t_resource* new_resource(char* name, int instances) {
    t_resource* resource = malloc(sizeof(t_resource));
    resource->name = strdup(name);
    resource->instances = instances;
    resource->blocked_queue = queue_create();
    resource->assigned_pcbs = list_create();
    pthread_mutex_init(&resource->mutex, NULL);
    return resource;
}

void initialize_resources() {
    resources_list = list_create();

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
    list_destroy_and_destroy_elements(resource->assigned_pcbs, (void*)free);
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

t_result resource_wait(t_pcb* pcb, char* name) {
    t_result result;
    t_resource* resource = find_resource(name);

    if (resource == NULL) {
        return RESOURCE_NOT_FOUND;
    }
    
    pthread_mutex_lock(&resource->mutex);
    
    int new_instances_value = resource->instances - 1; 
    if (new_instances_value < 0) {
        queue_push(resource->blocked_queue, pcb);
        result = RESOURCE_BLOCKED;
    } else {
        resource->instances = new_instances_value;
        list_add(resource->assigned_pcbs, pcb->pid);
        result = RESOURCE_SUCCESS_OP;
    }
    
    pthread_mutex_unlock(&resource->mutex);
    
    return result;
}

t_result resource_signal_by_instance(t_pcb* pcb, t_resource* resource){
    t_result result = RESOURCE_SUCCESS_OP;
    pthread_mutex_lock(&resource->mutex);

    bool is_pid(void* elem) {
        return (int)elem == pcb->pid;
    }
    list_remove_by_condition(resource->assigned_pcbs, is_pid);
    
    resource->instances++;
    if (!queue_is_empty(resource->blocked_queue)) {
        t_pcb* released_pcb = queue_pop(resource->blocked_queue);
        resource->instances--;
        list_add(resource->assigned_pcbs, released_pcb->pid);

        pthread_mutex_lock(&mutex_blocked);
        t_pcb *pcb_released = list_remove_by_pid(list_BLOCKED, released_pcb->pid);
        if (pcb_released != NULL){
            move_pcb(pcb_released, BLOCKED, READY, list_READY, &mutex_ready);
        } else {
            log_warning(logger, "PCB released from resource not found on blocked list");
        }
        pthread_mutex_unlock(&mutex_blocked);
    }

    pthread_mutex_unlock(&resource->mutex);
    return result;
}

t_result resource_signal(t_pcb* pcb, char* name) {
    t_resource* resource = find_resource(name);
    if (resource == NULL) {
        return RESOURCE_NOT_FOUND;
    }

    return resource_signal_by_instance(pcb, resource);
}

bool pid_list_exists(t_list* list, int searchPid) {

	if (list == NULL || list_is_empty(list)) {
		return false;
	}
	
	for (int i = 0; i < list->elements_count; i++) {
		int pid = (int)list_get(list, i);
		if (searchPid == pid) {
            return true;
        }
	}
	return false;
}

void release_all_resources(t_pcb *pcb) {
    void release_pcb_from_resource(t_resource* resource) {
        pthread_mutex_lock(&resource->mutex);
        bool isAssigned = pid_list_exists(resource->assigned_pcbs, pcb->pid);
        pthread_mutex_unlock(&resource->mutex);

        if(isAssigned){
            resource_signal_by_instance(pcb, resource);
        } else {
            pthread_mutex_lock(&resource->mutex);
            list_remove_by_pid(resource->blocked_queue->elements, pcb->pid);
            pthread_mutex_unlock(&resource->mutex);
        }
    }

    pthread_mutex_lock(&mutex_resources);
    list_iterate(resources_list, (void*)release_pcb_from_resource);
    pthread_mutex_unlock(&mutex_resources);
}

