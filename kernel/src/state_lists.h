#ifndef STATELISTS_H
#define STATELISTS_H

#include <commons/collections/list.h>
#include <stdio.h>
#include <commons/log.h>

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
void *log_list_contents(t_log *logger, t_list *list);

#endif