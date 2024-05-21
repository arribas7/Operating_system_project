#include <stdio.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <state_lists.h>
#include <utils/kernel.h>


//extern t_log *logger; // Extern indicates that logger will be defined elsewhere in execution

t_list *list_NEW = NULL;
t_list *list_READY = NULL;
t_list *list_RUNNING = NULL;
t_list *list_BLOCKED = NULL;
t_list *list_EXIT = NULL;

void initialize_lists() {
    list_NEW = list_create();
    list_READY = list_create();
    list_RUNNING = list_create();
    list_BLOCKED = list_create();
    list_EXIT = list_create();
}

t_list *list_push(t_list *list, void *element) {
    list_add(list, element);
    return list;
}

void *list_pop(t_list *list) {

    if (list == NULL || list->elements_count == 0) {return NULL;}
    
    t_link_element *popped_element = (t_link_element*)list_get(list, list->elements_count - 1);
	t_pcb *pcb = (t_pcb *)popped_element->data;

    bool success = list_remove_element(list, popped_element);

	if (success)
	{
		return pcb;
	}
	else
	{
		return NULL;
	}
}

void *log_list_contents(t_log *logger, t_list *list) {
    if (list == NULL) {
        log_info(logger, "List is NULL. It needs to be initialized with initialize_lists();");
        return NULL;
    }

	if (list->elements_count == 0) {
		log_info(logger, "List is empty");
		return NULL;
	}

	log_info(logger, "-The list is %d elements long", list->elements_count);
	log_info(logger, "-----------List Start-----------");

    for (int i = 0; i < list->elements_count; i++) {

		t_link_element *element = (t_link_element*)list_get(list, i);
		t_pcb *pcb = (t_pcb *)element->data;

		if (pcb == NULL) {
            log_error(logger, "Data of element at index %d is NULL", i);
            continue; // Skip to the next iteration
        }
		
		log_info(logger, "***************************");
        log_info(logger, "--PCB #%d", i);
		log_info(logger, "---pid: %d", pcb->pid);
		//log_info(logger, "---pc: %d", pcb->pc);
		//log_info(logger, "---quantum: %d", pcb->quantum);
    }
	log_info(logger, "-----------List End-----------");
}

bool list_has_pid(t_list* list, int pid) {

	if (list == NULL || list_is_empty(list)) {
		return false;
	}
	
	for (int i = 0; i < list->elements_count; i++) {
		t_link_element *element = (t_link_element*)list_get(list, i);
		t_pcb *pcb = (t_pcb *)element->data;

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
		t_link_element *element = (t_link_element*)list_get(list, i);
		t_pcb *pcb = (t_pcb *)element->data;

		if (pcb == NULL) { //Invalid pcb for some reason
            continue; // Skip to the next iteration
        }

		if (pcb->pid == pid) {
            return i;
        }
	}
	return -1;
}

void *list_remove_by_pid(t_list* list, int pid) {
	int position = list_pid_element_index(list, pid);

	if (position == -1) {
		return NULL;
	}

	return list_remove(list, position);
}

/*
	typedef struct {
		t_link_element *head;
		int elements_count;
	} t_list;

	typedef struct {
		t_list *list;
		t_link_element **actual;
		t_link_element **next;
		int index;
	} t_list_iterator;

    struct link_element{
		void *data;
		struct link_element *next;
	};
	typedef struct link_element t_link_element;

	typedef struct {
    u_int32_t pid;
    u_int32_t pc;
    u_int32_t quantum;
    //    char *path; 
    t_register *reg;
} t_pcb;
*/