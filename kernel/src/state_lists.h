#ifndef STATELISTS_H
#define STATELISTS_H

#include <commons/collections/list.h>
#include <stdio.h>
#include <commons/log.h>
#include <utils/kernel.h>

void initialize_lists();

//Adds an element at the end of the list
//Returns the updated list
//Usage example: list_push(list_NEW, &newPcb);
t_list *list_push(t_list *list, void *element);

//Removes the last element at the end of the list and returns it
//Returns the element which was removed
void *list_pop(t_list *list);

//Log contents in kernel.log to know what's inside the list at any given time
//Doesn't return anything
void *log_list_contents(t_log *logger, t_list *list, pthread_mutex_t mutex);

//Returns true if there exists a pcb with the given pid anywhere on the list
bool list_has_pid(t_list* list, int pid);

//Searches for a specific pid in a given list of pcbs and returns the index where it's been found
//Returns -1 in the case of error or not finding the pid
int list_pid_element_index(t_list* list, int pid);

//Searches for a specific pid in a given list of pcbs and returns the t_pcb where it's been found
//Returns NULL in the case of error or not finding the pid
void* list_pid_element(t_list* list, int pid);

//Searches for the element with the provided pid on the list and removes it from the list adequately
//Returns the removed element
//Returns NULL if the element wasn't found
void *list_remove_by_pid(t_list* list, int pid);

//Empties the state list and deletes every single pcb contained in it with delete_pcb()
//The list itself is not destroyed so it can be reutilized
//Broke the usual naming convention to specify that this isn't from list.h but state_list.h
void state_list_clean(t_list * list);

//Deletes all pcbs contained within the list itself with delete_pcb() and then deletes the list itself
//The list is destroyed so it cannot be reutilized without reinitializing
//Broke the usual naming convention to specify that this isn't from list.h but state_list.h
void state_list_destroy(t_list * list);

//Removes the first element from a list and returns it
//Returns NULL if the list is empty
void *list_get_first(t_list *list);

//Removes the last item from a list and returns it
//Returns NULL if the list is empty
void *list_pop_last(t_list *list);

void move_pcb(t_pcb* pcb, t_state prev_status, t_state destination_status, t_list* destination_list, pthread_mutex_t* mutex);

void move_pcb_from_to_by_pid(int pid, t_state from_status, t_list* from_list, pthread_mutex_t* from_mutex, t_state to_status, t_list* to_list, pthread_mutex_t* to_mutex);
/* ---------------- Lists Usage example ---------------- */
    /*t_pcb *testpcb = new_pcb(1,0,"");
    t_pcb *testpcb2 = new_pcb(2,0,"");
    t_pcb *testpcb3 = new_pcb(3,0,"");

    list_push(list_NEW, testpcb);
    list_push(list_NEW, testpcb2);
    list_push(list_NEW, testpcb3);

    t_pcb *testpcb4 = list_pop(list_NEW);

    log_list_contents(logger, list_NEW);
    log_info(logger, "Popped element pid: %u", testpcb4->pid);

    bool has_pid_1 = list_has_pid(list_NEW, 1);
    //0 For False, 1 For True, apparently. Trust me I've looked it up.
    log_info(logger, "The list having a pid==1 is %d. (true being 1 and false being 0)", has_pid_1);

    int index_address_for_1 = list_pid_element_index(list_NEW, 1);
    log_info(logger, "The index of pid==1 is %d", index_address_for_1);

    list_remove_by_pid(list_NEW, 1);
    log_list_contents(logger, list_NEW);
    state_list_destroy(list_NEW);
    */
/* ---------------- End of List Usage example ---------------- */

#endif