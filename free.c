/*
    module  : free.c
    version : 1.1
    date    : 12/13/24
*/
#include <stdio.h>
#include "malloc.h"

void my_free(tmp)
void *tmp;
{
    heap_t *cur, *ptr = tmp;

    if (!ptr--)				/* null pointer */
	return;
    if (!free_list) {			/* empty free list */
	free_list = ptr;
	ptr->next = 0;			/* initialize free list */
	return;
    }
    for (cur = free_list; cur->next && cur->next < ptr; cur = cur->next)
	;
    if (ptr + ptr->size == cur->next) {
	ptr->size += cur->next->size;
	ptr->next = cur->next->next;	/* swallow upper */
    } else
	ptr->next = cur->next;		/* connect to upper */
    if (cur + cur->size == ptr) {
	cur->size += ptr->size;
	cur->next = ptr->next;		/* swallow pointer */
    } else
	cur->next = ptr;		/* insert pointer */
}
