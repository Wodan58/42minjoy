/*
    module  : malloc.h
    version : 1.1
    date    : 12/13/24
*/
#ifndef MALLOC_H_
#define MALLOC_H_

typedef struct heap_t {
    size_t size;
    struct heap_t *next;
} heap_t;

extern heap_t *free_list;

#ifdef NPROTO
void *my_malloc();
void my_free();
#else
void *my_malloc(size_t size);
void my_free(void *ptr);
#endif

#endif				/* !MALLOC_H_ */
