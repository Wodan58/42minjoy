/*
    module  : gc.c
    version : 1.3
    date    : 02/10/25
*/
#include <string.h>
#include "malloc.h"

/*
 * Set to allow playing tutorial.joy with minimal library support.
 */
#ifndef MAXSTRTAB
#define MAXSTRTAB	500
#endif

#ifdef NPROTO
void point();
#else
void point(int diag, char *mes);
#endif

void mark_string();

static int strindex;
static char *strtable[MAXSTRTAB + 1];

/*
 * Mark all strings that have been allocated. Scan the table and free strings
 * that have not been marked. Also compact the table. Unmark strings that have
 * been marked, unless they have been marked as permanent. Register the new
 * size of the table.
 * The table can also be used to report about strings that are still allocated.
 * As these are strings, they can easily be printed.
 */
static void my_gc()
{
    int j;
    char *str;

    strindex = 0;
    mark_string();
    while ((str = strtable[strindex]) != 0) {
	if (*str == 0) {
	    my_free(str);
	    for (j = strindex; strtable[j]; j++)
		strtable[j] = strtable[j + 1];
	} else {
	    if (*str == 1) 
		*str = 0;
	    strindex++;
	}
    }
}

/*
 * In case my_malloc returns 0 or in case the string table is full, garbage
 * collect strings.
 *
 * All roots must be searched (programme, s, dump) and whenever a string is
 * found, it must be marked. Strings in the symbol table are marked as
 * persistent. This is needed only once.
 *
 * Strings that are not marked are freed.
 *
 * Strings can be marked by allocating an extra byte in front and use that for
 * marking; if strings are also stored in a table, then the table can be used
 * during scanning.
 *
 * So, instead of producing a fatal error, garbage collect strings.
 */
void *GC_malloc_atomic(size)
size_t size;	
{
    char *ptr;

    /*
     * Allocate an extra byte, used for marking.
     */
    if ((ptr = my_malloc(size + 1)) == 0) {
	my_gc();
	if ((ptr = my_malloc(size + 1)) == 0)
	    point('F', "too many characters in strings");
    }
    /*
     * Tabulate the return value from my_malloc.
     */
    if (strindex >= MAXSTRTAB) {
	my_gc();
	if (strindex >= MAXSTRTAB)
	    point('F', "too many strings");
    }
    strtable[strindex++] = ptr;		/* remember return from my_malloc */
    *ptr++ = 1;				/* allocate from index 0 and mark */
    return (void *)ptr;			/* return user area */
}

char *GC_strdup(str)
char *str;
{
    char *ptr;
    size_t size;

    size = strlen(str) + 1; 
    ptr = GC_malloc_atomic(size);
    return strcpy(ptr, str);
}

/*
 * Explicit free is not possible, because the pointer is still present in the
 * string table.
 */
void GC_free(ptr)
void *ptr;
{
    ((char *)ptr)[-1] = 0;		/* set to unmarked */
}
