/*
    module  : malloc.c
    version : 1.1
    date    : 12/13/24
*/
#include <stdio.h>
#include "malloc.h"

#define HEAP_SIZE	15000

heap_t *free_list;

static void init_heap()
{
    static heap_t heap_area[HEAP_SIZE];
    heap_t *cur;

    cur = heap_area;
    cur->size = HEAP_SIZE / sizeof(heap_t);	/* number of heap blocks */
    my_free(cur + 1);				/* release user-area */
}

void *my_malloc(size)
size_t size;
{
    static unsigned char init;
    heap_t *cur, *prev;

    if (!size)
	return 0;
    if (!init) {
	init = 1;
	init_heap();
    }
    size = 1 + (size + sizeof(heap_t) - 1) / sizeof(heap_t);	/* round up */
    for (prev = 0, cur = free_list; cur; prev = cur, cur = cur->next)
	if (cur->size >= size) {
	    if (cur->size <= size + 1) {	/* large enough */
		if (prev)
		    prev->next = cur->next;	/* skip one */
		else
		    free_list = cur->next;	/* skip start */
	    } else {
		cur->size -= size;		/* split */
		cur += cur->size;
		cur->size = size;		/* upper part */
	    }
	    return cur + 1;			/* user area */
	}
    return 0;
}
