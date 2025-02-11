/*
    module  : malloc.c
    version : 1.3
    date    : 02/10/25
*/
#include <stdio.h>
#include "malloc.h"

/*
 * Set to allow playing tutorial.joy with minimal library support.
 */
#ifndef MAXHEAP
#define MAXHEAP 	2000
#endif

heap_t *free_list;

static void init_heap()
{
    static heap_t heap_area[MAXHEAP + 1];
    heap_t *cur;

    cur = heap_area;
    cur->size = MAXHEAP;	/* number of heap blocks */
    my_free(cur + 1);		/* release user-area */
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
	    return (void *)(cur + 1);		/* user area */
	}
    return 0;
}
