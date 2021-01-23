#ifndef INC_S_LIST_H_
#define INC_S_LIST_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef GET_PARENT_ADDR
#define GET_PARENT_ADDR(pMe,tParent,eMyName) \
    ((tParent *)((char *)(pMe) - (ptrdiff_t)&((tParent *)0)->eMyName))
#endif

struct tag_list;
typedef struct tag_s_list {
    struct tag_s_list *next;
    struct tag_s_list *prev;
} s_list_t;

s_list_t *s_list_get_prev (s_list_t *list);
s_list_t *s_list_get_next (s_list_t *list);
void s_list_set_prev (s_list_t *list, s_list_t *other);
void s_list_set_next (s_list_t *list, s_list_t *other);
/* Initilization a list */
void s_list_init(s_list_t *list);
/* Connect two lists. */
void s_list_connect (s_list_t *list1, s_list_t *list2);
/* Disconnect tow lists. */
void s_list_disconnect (s_list_t *list1, s_list_t *list2);
/* Same as s_list_connect */
void s_list_attach (s_list_t *node1, s_list_t *node2);
/* Make node in detach mode */
void s_list_detach (s_list_t *node);
/* Check if list is empty */
int s_list_is_empty (s_list_t *list);
int s_list_size(s_list_t *list);


#ifdef __cplusplus
}
#endif
#endif
