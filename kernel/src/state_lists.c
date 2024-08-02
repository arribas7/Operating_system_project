#include <stdio.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <state_lists.h>
#include <utils/kernel.h>
#include <pthread.h>
#include <semaphore.h>

extern t_log *logger;

extern pthread_mutex_t mutex_blocked;
extern pthread_mutex_t mutex_ready;
extern sem_t sem_ready_process;

extern t_list *list_NEW;
extern t_list *list_READY;
extern t_list *list_BLOCKED;
extern t_list *list_EXIT;

t_list *list_push(t_list *list, void *element) {
    list_add(list, element);
    return list;
}

void *list_pop(t_list *list) {
    if (list == NULL || list->elements_count == 0) {return NULL;}

    t_pcb *popped_element = (t_pcb *)list_remove(list, 0);
    return popped_element;
}

void *list_get_first(t_list *list) {
    if (list == NULL || list->elements_count == 0) {
        return NULL;
    }

    void *first_element = list_remove(list, 0);
    return first_element;
}

void *log_list_contents(t_log *logger, t_list *list, pthread_mutex_t mutex) {
	pthread_mutex_lock(&mutex);
    if (list == NULL) {
        log_info(logger, "List is NULL. It needs to be initialized with initialize_lists();");
		pthread_mutex_unlock(&mutex);
        return NULL;
    }

	if (list->elements_count == 0) {
		log_info(logger, "List is empty");
		pthread_mutex_unlock(&mutex);
		return NULL;
	}

	log_info(logger, "-The list is %d elements long", list->elements_count);
	log_info(logger, "-----------List Start-----------");

    for (int i = 0; i < list->elements_count; i++) {
		t_pcb *pcb = (t_pcb *)list_get(list, i);

		if (pcb == NULL) {
            log_error(logger, "Data of element at index %d is NULL", i);
            continue; // Skip to the next iteration
        }
		
		log_info(logger, "***************************");
        log_info(logger, "--PCB #%d", i + 1);
		log_info(logger, "---pid: %d", pcb->pid);
		log_info(logger, "---prev_state: %s", t_state_to_string(pcb->prev_state));
		log_info(logger, "---path: %s", pcb->path);
    }
	log_info(logger, "***************************");
	log_info(logger, "-----------List End-----------");
	pthread_mutex_unlock(&mutex);
}

void list_element_destroyer(void *element) {
    t_pcb *pcb = (t_pcb*)element;
    delete_pcb(pcb);
}

void state_list_clean(t_list * list) {
	list_clean_and_destroy_elements(list, list_element_destroyer);
}

void state_list_destroy(t_list * list) {
	list_destroy_and_destroy_elements(list, (void*) list_element_destroyer);
}

bool list_has_pid(t_list* list, int pid) {

	if (list == NULL || list_is_empty(list)) {
		return false;
	}
	
	for (int i = 0; i < list->elements_count; i++) {
		t_pcb *pcb = (t_pcb *)list_get(list, i);

		if (pcb == NULL) { //Invalid pcb for some reason
            continue; // Skip to the next iteration
        }

		if (pcb->pid == pid) {
            return true;
        }
	}
	return false;
}

int list_pid_element_index(t_list* list, int pid) {

	if (list == NULL || list_is_empty(list)) {
		return -1;
	}
	
	for (int i = 0; i < list->elements_count; i++) {
		t_pcb *pcb = (t_pcb *)list_get(list, i);

		if (pcb == NULL) { //Invalid pcb for some reason
            continue; // Skip to the next iteration
        }

		if (pcb->pid == pid) {
            return i;
        }
	}
	return -1;
}

void* list_pid_element(t_list* list, int pid) {
	if (list == NULL || list_is_empty(list)) {
		return NULL;
	}
	
	for (int i = 0; i < list->elements_count; i++) {
		t_pcb *pcb = (t_pcb *)list_get(list, i);

		if (pcb == NULL) { //Invalid pcb for some reason
            continue; // Skip to the next iteration
        }

		if (pcb->pid == pid) {
            return pcb;
        }
	}
	return NULL;
}

void *list_remove_by_pid(t_list* list, int pid) {
	int position = list_pid_element_index(list, pid);

	if (position == -1) {
		return NULL;
	}

	return list_remove(list, position);
}

void move_pcb(t_pcb* pcb, t_state prev_status, t_state destination_status, t_list* destination_list, pthread_mutex_t* mutex) {
	log_info(logger, "“PID: <%d> - Estado Anterior: <%s> - Estado Actual: <%s>”", pcb->pid, t_state_to_string(prev_status), t_state_to_string(destination_status));
	pcb->prev_state = prev_status;

	pthread_mutex_lock(mutex);
	list_add(destination_list, pcb);
	pthread_mutex_unlock(mutex);

	if(destination_status == READY){
		log_info(logger, "||||| Ingreso a ready: Cola Ready / Ready prioridad ||||| ");
		log_list_contents(logger, list_READY, mutex_ready);
		sem_post(&sem_ready_process);
	}
}

void move_pcb_from_to_by_pid(int pid, t_state from_status, t_list* from_list, pthread_mutex_t* from_mutex, t_state to_status, t_list* to_list, pthread_mutex_t* to_mutex) {
	log_info(logger, "“PID: <%d> - Estado Anterior: <%s> - Estado Actual: <%s>”", pid, t_state_to_string(from_status), t_state_to_string(to_status));
	
	pthread_mutex_lock(from_mutex);
    t_pcb* pcb = list_remove_by_pid(from_list, pid);
    pthread_mutex_unlock(from_mutex);
	if(pcb != NULL){
		pcb->prev_state = from_status;
		
		pthread_mutex_lock(to_mutex);
		list_add(to_list, pcb);
		pthread_mutex_unlock(to_mutex);
		
		if(to_status == READY){
			log_info(logger, "||||| Ingreso a ready: Cola Ready / Ready prioridad ||||| ");
			log_list_contents(logger, list_READY, mutex_ready);
			sem_post(&sem_ready_process);
		}
	} else {
		log_error(logger, "PID %d doesn't exist on list", pid);
	}
  
}
void *list_pop_last(t_list *list) {
    if (list == NULL || list->elements_count == 0) {
        return NULL;
    }
    t_pcb *popped_element = (t_pcb *)list_remove(list, list->elements_count - 1);
    return popped_element;
}