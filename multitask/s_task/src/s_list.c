/* Copyright xhawk, MIT license */

#include "s_list.h"

/*******************************************************************/
/* list                                                            */
/*******************************************************************/

s_list_t *s_list_get_prev (s_list_t *list) {
    return list->prev;
}

s_list_t *s_list_get_next (s_list_t *list) {
    return list->next;
}

void s_list_set_prev (s_list_t *list, s_list_t *other) {
    list->prev = other;
}

void s_list_set_next (s_list_t *list, s_list_t *other) {
    list->next = other;
}

/* Initilization a list */
void s_list_init(s_list_t *list) {
    s_list_set_prev(list, list);
    s_list_set_next(list, list);
}

/* Connect or disconnect two lists. */
void s_list_toggle_connect(s_list_t *list1, s_list_t *list2) {
    s_list_t *prev1 = s_list_get_prev(list1);
    s_list_t *prev2 = s_list_get_prev(list2);
    s_list_set_next(prev1, list2);
    s_list_set_next(prev2, list1);
    s_list_set_prev(list1, prev2);
    s_list_set_prev(list2, prev1);
}

/* Connect two lists. */
void s_list_connect (s_list_t *list1, s_list_t *list2) {
    s_list_toggle_connect (list1, list2);
}

/* Disconnect tow lists. */
void s_list_disconnect (s_list_t *list1, s_list_t *list2) {
    s_list_toggle_connect (list1, list2);
}

/* Same as s_list_connect */
void s_list_attach (s_list_t *node1, s_list_t *node2) {
    s_list_connect (node1, node2);
}

/* Make node in detach mode */
void s_list_detach (s_list_t *node) {
    s_list_disconnect (node, s_list_get_next(node));
}

/* Check if list is empty */
int s_list_is_empty (s_list_t *list) {
    return (s_list_get_next(list) == list);
}

int s_list_size(s_list_t *list) {
    int n = 0;
    s_list_t *next;
    for(next = s_list_get_next(list); next != list; next = s_list_get_next(next))
        ++n;
    return n;
}
