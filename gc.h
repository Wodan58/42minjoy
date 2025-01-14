/*
    module  : gc.h
    version : 1.2
    date    : 01/14/25
*/
#ifndef GC_H
#define GC_H

#ifdef NPROTO
void *GC_malloc_atomic();
char *GC_strdup();
void GC_free();
#else
void *GC_malloc_atomic(size_t size);
char *GC_strdup(char *str);
void GC_free(void *str);
#endif
#endif
