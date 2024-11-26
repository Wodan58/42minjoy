/*
    module  : joy.c
    version : 1.45
    date    : 11/20/24
*/
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <setjmp.h>

#if 0
#define DEBUG
#endif

/*
 * The library should be read only once. As a consequence, some calls to strdup
 * are needed. These strings are freed in the patch phase.
 */
#define CORRECT_GARBAGE
#define READ_LIBRARY_ONCE
#define OBSOLETE_NOTHING
#define RENAME_INDEX
#define RENAME_SELECT
#if 0
#define ADD_SETAUTOPUT
#endif

#ifdef DEBUG
#define LOGFILE(x)	printf("%s\n", x)
#else
#define LOGFILE(x)
#endif

typedef unsigned char boolean;

#define true  1
#define false 0

#define errormark	"%JOY"

#define lib_filename	"42minjoy.lib"
#define list_filename	"42minjoy.lst"

#define reslength	8
#define maxrestab	10

#define identlength	16
#define maxstdidenttab	32

typedef enum {
    lbrack, rbrack, semic, period, def_equal,
/* compulsory for scanutilities: */
    charconst, stringconst, numberconst, leftparenthesis, identifier
/* hyphen */
} symbol;

/*
 * Renaming: index -> of, select -> opcase
 * Removing: nothing
 *
 * The removal of nothing means that uncons can only be used for non-empty
 * lists.
 */
typedef enum {
    lib_, mul_, add_, sub_, div_, lss_, eql_, and_, body_, cons_, dip_, dup_,
    false_, get_, getch_, i_,
#ifndef RENAME_INDEX
    index_,
#endif
    not_,
#ifndef OBSOLETE_NOTHING
    nothing_,
#endif
#ifdef RENAME_INDEX
    of_,
#endif
#ifdef RENAME_SELECT
    opcase_,
#endif
    or_, pop_, put_, putch_, sametype_,
#ifndef RENAME_SELECT
    select_,
#endif
#ifdef ADD_SETAUTOPUT
    setautoput_,
#endif
    stack_, step_, swap_, true_,
    uncons_, unstack_, boolean_, char_, integer_, list_, unknownident
} standardident;

typedef short memrange;

/*
 * pathname of the joy binary, to be used as prefix to library files. That way
 * it is not necessary to copy all library files to all working directories.
 */
char *pathname;

/*
 * autoput controls whether automatic output of the TOS is required. Initially
 * it is set to 1, but it can be turned off with the setautoput instruction.
 * This is not a directive, because it has to be similar to the full version of
 * Joy.
 */
int autoput = 1;

#include "proto.h"
#define MINJOY
#include "scanutil.c"

static void initialise()
{
    int i;

    LOGFILE("initialise");
    iniscanner();
    strcpy(specials_repeat, "=>");
    erw(".",	period);
    erw(";",	semic);
    erw("==",	def_equal);
    erw("[",	lbrack);
    erw("]",	rbrack);
    est("*",          mul_);
    est("+",          add_);
    est("-",          sub_);
    est("/",          div_);
    est("<",          lss_);
    est("=",          eql_);
    est("and",        and_);
    est("body",       body_);
    est("cons",       cons_);
    est("dip",        dip_);
    est("dup",        dup_);
    est("false",      false_);
    est("get",        get_);
    est("getch",      getch_);
    est("i",          i_);
#ifndef RENAME_INDEX
    est("index",      index_);
#endif
    est("not",        not_);
#ifndef OBSOLETE_NOTHING
    est("nothing",    nothing_);
#endif
#ifdef RENAME_INDEX
    est("of",         of_);
#endif
#ifdef RENAME_SELECT
    est("opcase",     opcase_);
#endif
    est("or",         or_);
    est("pop",        pop_);
    est("put",        put_);
    est("putch",      putch_);
    est("sametype",   sametype_);
#ifndef RENAME_SELECT
    est("select",     select_);
#endif
#ifdef ADD_SETAUTOPUT
    est("setautoput", setautoput_);
#endif
    est("stack",      stack_);
    est("step",       step_);
    est("swap",       swap_);
    est("true",       true_);
    est("uncons",     uncons_);
    est("unstack",    unstack_);
    for (i = mul_; i <= unstack_; i++)
	if (i != (int)stdidents[i].symb)
	    point('F', "bad order in standard idents");
}  /* initialise */

#ifndef MAXTABLE
#define MAXTABLE	300
#endif
#ifndef MAXMEM
#define MAXMEM		2000
#endif

typedef struct _REC_table {
    identalfa alf;
    memrange adr;
} _REC_table;

typedef struct _REC_m {
    long val;
    memrange nxt;
    unsigned char op;	/* standardident */
    boolean marked;
} _REC_m;

static _REC_table table[MAXTABLE + 1];
static int lastlibloc, sentinel, lasttable, locatn;
static _REC_m m[MAXMEM + 1];
static memrange firstusernode, freelist, programme;
static memrange s,  /* stack */
		dump;

#if 0
static standardident last_op_executed;
#endif
static long stat_kons, stat_gc, stat_ops, stat_calls;
static time_t stat_lib;

static char *standardident_NAMES[] = {
    "LIB", "*", "+", "-", "/", "<", "=", "and", "body", "cons", "dip", "dup",
    "false", "get", "getch", "i",
#ifndef RENAME_INDEX
    "index",
#endif
    "not",
#ifndef OBSOLETE_NOTHING
    "nothing",
#endif
#ifdef RENAME_INDEX
    "of",
#endif
#ifdef RENAME_SELECT
    "opcase",
#endif
    "or", "pop", "put", "putch", "sametype",
#ifndef RENAME_SELECT
    "select",
#endif
#ifdef ADD_SETAUTOPUT
    "setautoput",
#endif
    "stack", "step", "swap", "true",
    "uncons", "unstack", "BOOLEAN", "CHAR", "INTEGER", "LIST", "UNKNOWN"
};

#ifdef DEBUG
void DumpM()
{
    int i;
    FILE *fp = fopen("joy.dmp", "w");

    LOGFILE("DumpM");
    fprintf(fp, "Table\n");
    fprintf(fp, "  nr %-*.*s  adr\n", identlength, identlength, "name");
    for (i = 1; i <= MAXTABLE && table[i].adr; i++)
	fprintf(fp, "%4d %-*.*s %4d\n", i, identlength, identlength,
		table[i].alf, table[i].adr);
    fprintf(fp, "\nMemory\n");
    fprintf(fp, "  nr %-*.*s      value next M\n", identlength,
	    identlength, "name");
    for (i = 1; i <= MAXMEM && m[i].marked; i++)
	fprintf(fp, "%4d %-*.*s %10ld %4d %c\n", i, identlength, identlength,
		standardident_NAMES[m[i].op], m[i].val, m[i].nxt,
		m[i].marked ? 'T' : 'F');
    fclose(fp);
}
#endif

static void lookup()
{
    int i, j;

    LOGFILE("lookup");
#ifdef READ_LIBRARY_ONCE
    if (!sentinel) {
	id = unknownident;
	return;
    }
#endif
    locatn = 0;
    if (sentinel > 0) {	 /* library has been read */
	strcpy(table[sentinel].alf, ident);
	locatn = lasttable;
	while (strcmp(table[locatn].alf, ident))
	    locatn--;
    }
    if (locatn > sentinel)
	id = lib_;
    else {
	i = 1;
	j = lastlibloc;
	do {
	    locatn = (i + j) / 2;
	    if (strcmp(ident, table[locatn].alf) <= 0)
		j = locatn - 1;
	    if (strcmp(ident, table[locatn].alf) >= 0)
		i = locatn + 1;
	} while (i <= j);
	if (i - 1 > j)
	    id = lib_;
	else {	/* binarysearch through standardidentifiers */
	    i = 1;
	    j = laststdident;
	    do {
		locatn = (i + j) / 2;
		if (strcmp(ident, stdidents[locatn].alf) <= 0)
		    j = locatn - 1;
		if (strcmp(ident, stdidents[locatn].alf) >= 0)
		    i = locatn + 1;
	    } while (i <= j);
	    if (i - 1 > j)
		id = stdidents[locatn].symb;
	    else {
#ifndef READ_LIBRARY_ONCE
		if (!sentinel)
		    id = unknownident;
		else {
#endif
		    if (lasttable == MAXTABLE)
			point('F', "too many library symbols");
		    strcpy(table[++lasttable].alf, ident);
		    table[locatn = lasttable].adr = 0;
		    id = lib_;
#ifndef READ_LIBRARY_ONCE
		}
#endif
	    }
	}  /* ELSE */
    }  /* ELSE */
    if (writelisting > 4)
	fprintf(listing, "lookup : %-*.*s at %d\n", identlength, identlength,
		standardident_NAMES[id], locatn);
}  /* lookup */

static void wn(f, n)
FILE *f;
memrange n;
{
    LOGFILE("wn");
#ifdef READ_LIBRARY_ONCE
    if (m[n].op == unknownident)
	fprintf(f, "%5d %-*.*s %10d %10d %c", n, identlength,
	    identlength, (char *)m[n].val, 0, m[n].nxt,
	    m[n].marked ? 'T' : 'F');
    else
#endif
	fprintf(f, "%5d %-*.*s %10ld %10d %c", n, identlength,
	    identlength, standardident_NAMES[m[n].op], m[n].val,
	    m[n].nxt, m[n].marked ? 'T' : 'F');
    if (m[n].op == lib_)
	fprintf(f, "   %-*.*s %4d", identlength, identlength,
		table[m[n].val].alf, table[m[n].val].adr);
    putc('\n', f);
}

static void writenode(n)
memrange n;
{
    LOGFILE("writenode");
    wn(stdout, n);
    if (writelisting > 0) {
	putc('\t', listing);
	wn(listing, n);
    }
}  /* writenode */

static void mark(n)
memrange n;
{
    LOGFILE("mark");
    while (n > 0) {
	if (writelisting > 4)
	    writenode(n);
	if (m[n].op == list_ && !m[n].marked)
	    mark((memrange)m[n].val);
	m[n].marked = true;
	n = m[n].nxt;
    }
}  /* mark */

static memrange kons(o, v, n)
standardident o;
long v;
memrange n;
{
    memrange i;
    long collected;

    LOGFILE("kons");
    if (!freelist) {
	if (!sentinel)
	    goto einde;
#if 0
	fprintf(stderr, "gc, last_op_executed = %-*.*s\n", identlength,
		identlength, standardident_NAMES[last_op_executed]);
#endif
	if (writelisting > 2) {
	    writeident("GC start");
	    writeline();
	}
	mark(programme);
	mark(s);
	mark(dump);
	/* mark parameters */
	mark(n);
	if (o == list_)
	    mark((memrange)v);
	if (writelisting > 3) {
	    writeident("finished marking");
	    writeline();
	}
	collected = 0;
	for (i = firstusernode; i <= MAXMEM; i++) {
	    if (!m[i].marked) {
		m[i].nxt = freelist;
		freelist = i;
		collected++;
	    }
	    m[i].marked = false;
	    if (m[i].nxt == i)
		point('F', "internal error - selfreference");
	}
	if (writelisting > 2) {
	    writeinteger(collected);
	    putch(' ');
	    writeident("nodes collected");
	    writeline();
	}
	if (!freelist)
einde:
	    point('F', "dynamic memory exhausted");
	stat_gc++;
    }
    i = freelist;
    if (o == list_ && v == i)
	point('F', "internal error - selfreference");
    if (i == n)
	point('F', "internal error - circular");
    freelist = m[i].nxt;
    m[i].op = (unsigned char)o;
    m[i].val = v;
    m[i].nxt = n;
    if (writelisting > 4)
	writenode(i);
    stat_kons++;
    return i;
}  /* kons */

static char *my_strdup(str)
char *str;
{
    char *ptr;
    size_t leng;

    LOGFILE("my_strdup");
    leng = strlen(str);
    if ((ptr = (char *)malloc(leng + 1)) != 0)
        strcpy(ptr, str);
    return ptr;
}

static void readfactor(where)
memrange *where;
{
    memrange here;

    LOGFILE("readfactor");
    switch (sym) {
    case lbrack:
	getsym();
	*where = kons(list_, 0L, 0);
	m[*where].marked = true;
	if (sym == lbrack || sym == identifier || sym == charconst ||
		sym == numberconst) {	/* sym == hyphen */
	    readterm(&here);
	    m[*where].val = here;
	}
	break;

    case identifier:
	lookup();
#ifdef READ_LIBRARY_ONCE
	if (id == unknownident)
	    *where = kons(id, (long)my_strdup(ident), 0);
	else
#endif
	    *where = kons(id, (long)locatn, 0);
	break;

    case charconst:
	*where = kons(char_, num, 0);
	break;

    case numberconst:
	*where = kons(integer_, num, 0);
	break;

#if 0
    case hyphen:
	*where = kons(sub_, sub_, 0);
	break;
#endif

    case period:
	*where = 0;
	return;

    default:
	point('F', "internal in readfactor");
    }  /* CASE */
    m[*where].marked = true;
}  /* readfactor */

static void readterm(first)
memrange *first;
{   /* readterm */
    /* was forward */
    memrange i;

    LOGFILE("readterm");
    /* this is LL0 */
    readfactor(first);
    if ((i = *first) == 0)
	return;
    getsym();
    while (sym == lbrack || sym == identifier || sym == charconst ||
		sym == numberconst) {	/* sym == hyphen */
	readfactor(&m[i].nxt);
	i = m[i].nxt;
	getsym();
    }
}  /* readterm */

static void writeterm(n, nl)
memrange n;
boolean nl;
{
    LOGFILE("writeterm");
    while (n > 0) {
	writefactor(n, false);
	if (m[n].nxt > 0)
	    putch(' ');
	n = m[n].nxt;
    }
    if (nl)
	writeline();
}  /* writeterm */

static void writefactor(n, nl)
memrange n;
boolean nl;
{   /* was forward */
    LOGFILE("writefactor");
    if (n > 0) {
	switch (m[n].op) {
	case list_:
	    putch('[');
	    writeterm((memrange)m[n].val, false);
	    putch(']');
	    break;

	case boolean_:
	    if (m[n].val == 1)
		writeident("true");
	    else
		writeident("false");
	    break;

	case char_:
	    if (m[n].val == '\n')
		writeline();
	    else
		putch((int)m[n].val);
	    break;

	case integer_:
	    writeinteger(m[n].val);
	    break;

	case lib_:
	    writeident(table[m[n].val].alf);
	    break;

	case unknownident:
	    writeident((char *)m[n].val);
	    break;

	default:
	    writeident(stdidents[m[n].val].alf);
	    break;
	}  /* CASE */
    }
    if (nl)
	writeline();
}  /* writefactor */

#ifdef READ_LIBRARY_ONCE
static void patchterm(n)
memrange n;
{
    LOGFILE("patchterm");
    while (n > 0) {
	patchfactor(n);
	n = m[n].nxt;
    }
}  /* patchterm */

static void patchfactor(n)
memrange n;
{   /* was forward */
    LOGFILE("patchfactor");
    if (n > 0) {
	switch (m[n].op) {
	case list_:
	    patchterm((memrange)m[n].val);
	    break;

	case unknownident:
	    strncpy(ident, (char *)m[n].val, identlength);
	    ident[identlength] = 0;
	    free((char *)m[n].val);
	    m[n].val = 0;
	    lookup();
	    m[n].op = (unsigned char)id;
	    m[n].val = locatn;
	    break;
	}  /* CASE */
    }
}  /* patchfactor */
#endif

static void readlibrary(str)
char *str;
{
#ifdef READ_LIBRARY_ONCE
    FILE *fp;
#endif
    char *lib;
    int loc, must_free = 0;

    LOGFILE("readlibrary");
#if 0
    if (writelisting > 5)
	fprintf(listing, "first pass through library:\n");
#endif
    /*
     * Check that the library file is present. If not, this should not be a
     * fatal error. Some global variables need to be set in case that happens.
     * If the library is not present in the current directory, but is present
     * in the same directory as the joy binary, it is read from there. This
     * also applies to files that are included in the library.
     */
    lastlibloc = 0;
#ifdef READ_LIBRARY_ONCE
    if ((fp = fopen(str, "r")) == 0) {
	/*
	 * Prepend the pathname and try again. The library files are expected
	 * to exist somewhere.
	 */
	if (pathname) {
	    loc = strlen(pathname) + strlen(str) + 1;
	    lib = (char *)malloc(loc);
	    sprintf(lib, "%s%s", pathname, str);
	    if ((fp = fopen(lib, "r")) == 0) {
		free(lib);
		goto failed;
	    }
	    str = lib;
	    must_free = 1;	/* str must be given to free */
	    goto done;
	}
failed:
	firstusernode = freelist;
	cc = ll;
	sentinel = lastlibloc + 1;
	lasttable = sentinel;
	adjustment = 0;
	return;
    }
done:
    /*
     * The file was checked to exist. It is now closed, to be reopened again
     * in newfile. It is possible that str now has a pathname prepended. That
     * pathname should also be used in subsequent opens of newfile.
     */
    fclose(fp);
#endif
    newfile(str, 1);
    getsym();
#if 0
    do {
#else
    while (sym != period) {
#endif
	if (writelisting > 8)
	    fprintf(listing, "seen : %-*.*s\n", identlength, identlength,
		    ident);
	if (lastlibloc > 0)
	    if (strcmp(ident, table[lastlibloc].alf) <= 0)
		point('F', "bad order in library");
	if (lastlibloc == MAXTABLE)
	    point('F', "too many library symbols");
	strcpy(table[++lastlibloc].alf, ident);
#ifdef READ_LIBRARY_ONCE
	loc = lastlibloc;
#else
	do
	    getsym();
	while (sym != semic && sym != period);
	if (sym == semic)
	    getsym();
#if 0
    } while (sym != period);
#else
    }
#endif
    if (writelisting > 5)
	fprintf(listing, "second pass through library:\n");
    newfile(str, 1);
#if 0
    do {
	getsym();
#else
    getsym();
    while (sym != period) {
#endif
	if (sym != identifier)
	    point('F', "pass 2: identifier expected");
	lookup();
	loc = locatn;
#endif
	/*
	 * In case of READ_LIBRARY_ONCE, continue here.
	 */
	getsym();
	if (sym != def_equal)
	    point('F', "pass 2: \"==\" expected");
	getsym();
	readterm(&table[loc].adr);
	if (writelisting > 8)
	    writeterm(table[loc].adr, true);
#if 0
    } while (sym != period);
#else
	if (sym != period)
	    getsym();
    }
#endif
    firstusernode = freelist;
    if (writelisting > 5)
	fprintf(listing, "firstusernode = %d,  total memory = %d\n",
		firstusernode, MAXMEM);
    cc = ll;
#ifdef READ_LIBRARY_ONCE
    sentinel = lastlibloc + 1;
    lasttable = sentinel;
    for (loc = 1; loc < lasttable; loc++)
	patchterm(table[loc].adr);
    adjustment = -1;
#else
    adjustment = -2;  /* back to file "input" */
#endif
    if (must_free)
	free(str);
}  /* readlibrary */

static jmp_buf JL10;

static memrange ok(x)
memrange x;
{
    if (x < 1)
	point('F', "null address being referenced");
    return x;
}  /* ok */

static unsigned char o(x)
memrange x;
{
    return m[ok(x)].op;
}  /* o */

static long get_i(x)
memrange x;
{
    if (o(x) == integer_)
	return m[x].val;
    point('R', "integer value required");
    longjmp(JL10, 1);
}  /* i */

static memrange l(x)
memrange x;
{
    if (o(x) == list_)
	return (memrange)m[x].val;
    point('R', "list value required");
    longjmp(JL10, 1);
}  /* l */

static memrange n(x)
memrange x;
{
    if (m[ok(x)].nxt >= 0)
	return m[x].nxt;
    point('R', "negative next value");
    longjmp(JL10, 1);
}  /* n */

static long v(x)
memrange x;
{
    return m[ok(x)].val;
}  /* v */

static boolean b(x)
memrange x;
{
    return (boolean)(v(x) > 0);
}  /* b */

static void binary(o, v)
standardident o;
long v;
{
    s = kons(o, v, n(n(s)));
}

static void joy(nod)
memrange nod;
{
#ifdef RENAME_INDEX
    long val;
#endif
    memrange temp1, temp2;

    LOGFILE("joy");
    while (nod > 0) {  /* WHILE */
	if (writelisting > 3) {
	    writeident("joy:");
	    putch(' ');
	    writefactor(nod, true);
	}
	if (writelisting > 4) {
	    writeident("stack:");
	    putch(' ');
	    writeterm(s, true);
	    writeident("dump:");
	    putch(' ');
	    writeterm(dump, true);
	}
#if 0
	last_op_executed = m[nod].op;
#endif
	switch (m[nod].op) {
#ifndef OBSOLETE_NOTHING
	case nothing_:
#endif
	case char_:
	case integer_:
	case list_:
	    s = kons(m[nod].op, m[nod].val, s);
	    break;

	case true_:
	case false_:
	    s = kons(boolean_, (long)(m[nod].op == true_), s);
	    break;

	case pop_:
	    s = n(s);
	    break;

	case dup_:
	    s = kons(o(s), v(s), s);
	    break;

	case swap_:
	    s = kons(o(n(s)), v(n(s)), kons(o(s), v(s), n(n(s))));
	    break;

	case stack_:
	    s = kons(list_, (long)s, s);
	    break;

	case unstack_:
	    s = l(s);
	    break;

	/* OPERATIONS: */
	case not_:
	    s = kons(boolean_, (long)!b(s), n(s));
	    break;

	case mul_:
	    binary(integer_, get_i(n(s)) * get_i(s));
	    break;

	case add_:
	    binary(integer_, v(n(s)) + get_i(s));
	    break;

	case sub_:
	    binary(integer_, v(n(s)) - get_i(s));
	    break;

	case div_:
	    binary(integer_, get_i(n(s)) / get_i(s));
	    break;

	case and_:
	    binary(boolean_, (long)(b(n(s)) & b(s)));
	    break;

	case or_:
	    binary(boolean_, (long)(b(n(s)) | b(s)));
	    break;

	case lss_:
	    if (o(s) == lib_)
		binary(boolean_, (long)(strcmp(table[v(n(s))].alf,
					       table[v(s)].alf) < 0));
	    else
		binary(boolean_, (long)(v(n(s)) < v(s)));
	    break;

	case eql_:
	    binary(boolean_, (long)(v(n(s)) == v(s)));
	    break;

	case sametype_:
	    binary(boolean_, (long)(o(n(s)) == o(s)));
	    break;

	case cons_:
#ifndef OBSOLETE_NOTHING
	    if (o(n(s)) == nothing_)
		s = kons(list_, (long)l(s), n(n(s)));
	    else
#endif
		s = kons(list_, (long)kons(o(n(s)), v(n(s)), (memrange)v(s)),
				n(n(s)));
	    break;

	case uncons_:
#ifndef OBSOLETE_NOTHING
	    if (!v(s))
		s = kons(list_, 0L, kons(nothing_, (long)nothing_, n(s)));
	    else
#endif
		s = kons(list_, (long)n(l(s)), kons(o(l(s)), v(l(s)), n(s)));
	    break;

#ifdef RENAME_SELECT
	case opcase_:
#else
	case select_:
#endif
	    temp1 = l(s);
	    while (o(l(temp1)) != o(n(s)))
		temp1 = n(temp1);
	    s = kons(list_, (long)n(l(temp1)), n(s));
	    break;

#ifdef RENAME_INDEX
	case of_:
	    temp1 = l(s);
	    for (val = v(n(s)); val > 0; val--)
		temp1 = n(temp1);
	    s = kons(o(temp1), v(temp1), n(n(s)));
	    break;
#else
	case index_:
	    if (v(n(s)) < 1)
		s = kons(o(l(s)), v(l(s)), n(n(s)));
	    else
		s = kons(o(n(l(s))), v(n(l(s))), n(n(s)));
	    break;
#endif

	case body_:
	    s = kons(list_, (long)table[v(s)].adr, n(s));
	    break;

#ifdef ADD_SETAUTOPUT
	case setautoput_:
	    autoput = v(s);
	    s = n(s);
	    break;
#endif

	case put_:
	    writefactor(s, false);
	    s = n(s);
	    break;

	case putch_:
	    putch((int)v(s));
	    s = n(s);
	    break;

	case get_:
	    getsym();
	    readfactor(&temp1);
	    s = kons(o(temp1), v(temp1), s);
	    break;

	case getch_:
	    getch();
	    s = kons(integer_, (long)chr, s);
	    break;

	/* COMBINATORS: */
	case i_:
/*
 * It is possible that during evaluation of the program that is processed by i,
 * that program is garbage collected. This is prevented by placing that program
 * on the dump.
 */
#ifdef CORRECT_GARBAGE
	    dump = kons(o(s), (long)l(s), dump);
#endif
	    temp1 = s;
	    s = n(s);
	    joy(l(temp1));
#ifdef CORRECT_GARBAGE
	    dump = n(dump);
#endif
	    break;

	case dip_:
	    dump = kons(o(n(s)), v(n(s)), dump);
	    dump = kons(list_, (long)l(s), dump);
	    s = n(n(s));
	    joy(l(dump));
	    dump = n(dump);
	    s = kons(o(dump), v(dump), s);
	    dump = n(dump);
	    break;

	case step_:
	    dump = kons(o(s), (long)l(s), dump);
	    dump = kons(o(n(s)), (long)l(n(s)), dump);
	    temp1 = l(s);
	    temp2 = l(n(s));
	    s = n(n(s));
	    while (temp2 > 0) {
		s = kons(m[temp2].op, m[temp2].val, s);
		joy(temp1);
		temp2 = m[temp2].nxt;
	    }
	    dump = n(n(dump));
	    break;

	case lib_:
	    joy(table[m[nod].val].adr);
	    break;

	default:
	    point('F', "internal error in interpreter");
	}  /* CASE */
	stat_ops++;
	nod = m[nod].nxt;
    }
    stat_calls++;
}  /* joy */

static void writestatistics(f)
FILE *f;
{
    time_t c[2];

    /*
     * end_time - beg_time has already been reported as the time needed to
     * execute (unless the time measured was 0). This duration includes reading
     * the library. Here it is split between reading the library and execution
     * proper.
     */
    LOGFILE("writestatistics");
    c[0] = end_time - beg_time;
    c[1] = stat_lib - beg_time;
    if (c[1] > 0)
	fprintf(f, "%lu seconds CPU to read library\n", c[1]);
    c[0] -= c[1];
    if (c[0] > 0)
	fprintf(f, "%lu seconds CPU to execute\n", c[0]);
    fprintf(f, "%lu user nodes available\n", MAXMEM - firstusernode + 1L);
    fprintf(f, "%lu garbage collections\n", stat_gc);
    fprintf(f, "%lu nodes used\n", stat_kons);
    fprintf(f, "%lu calls to joy interpreter\n", stat_calls);
    fprintf(f, "%lu operations executed\n", stat_ops);
}  /* writestatistics */

int main(argc, argv)
int argc;
char *argv[];
{  /* main */
    memrange i;
    int j;
    char *ptr;

    LOGFILE("main");
    beg_time = time(0);
    initialise();
    my_atexit(perhapsstatistics);
    for (i = 1; i <= MAXMEM; i++) {
	m[i].marked = false;
	m[i].nxt = i + 1;
    }
    freelist = 1;
    m[MAXMEM].nxt = 0;
    writelisting = 0;
    stat_kons = 0;
    stat_gc = 0;
    stat_ops = 0;
    stat_calls = 0;
    sentinel = 0;
    firstusernode = 0;
    /*
     * Extract the pathname from the joy binary.
     */
    if ((ptr = strrchr(argv[0], '/')) != 0) {
	j = ptr - argv[0];
	pathname = (char *)malloc(j + 2);
	strncpy(pathname, argv[0], j + 1);
	pathname[j + 1] = 0;
    }
    /*
     * The library is 42minjoy.lib. If not present, only programs can be used,
     * no definitions.
     */
    readlibrary(lib_filename);
    stat_lib = time(0);
    if (writelisting > 2)
	for (j = 1; j <= lastlibloc; j++) {
	    fprintf(listing, "\"%-*.*s\" :\n", identlength, identlength,
		    table[j].alf);
	    writeterm(table[j].adr, true);
	}
    sentinel = lastlibloc + 1;
    lasttable = sentinel;
    s = 0;
    dump = 0;
#ifdef DEBUG
    DumpM();
#endif
    /*
     * A filename parameter is possible: it contains programs to be executed.
     * The file replaces standard input.
     */
    if (argc > 1)
	newfile(argv[1], 0);
    if (pathname)
	free(pathname);
    setjmp(JL10);
    while (1) {
	getsym();
#if 0
	last_op_executed = get_;
#endif
	programme = 0;
	readterm(&programme);
	if (writelisting > 2) {
	    writeident("interpreting:");
	    writeline();
	    writeterm(programme, true);
	}
	if (dump != 0) {
	    printf("dump error: should be empty!\n");
	    writeterm(dump, true);
	    dump = 0;
	}
	outlinelength = 0;
	joy(programme);
	/*
	 * Add automatic output of TOS and a newline. This under the control of
	 * the autoput flag.
	 */
	if (s && autoput) {
	    writefactor(s, true);
	    s = n(s);
	    fflush(stdout);
	}
	if (outlinelength > 0)
	    writeline();
	if (writelisting > 2) {
	    writeident("stack:");
	    writeline();
	    writeterm(s, true);
	}
    }
    return 0;
}

static void perhapsstatistics()
{
    LOGFILE("perhapsstatistics");
    finalise();
    if (statistics > 0) {
	fflush(stdout);
	writestatistics(stderr);
	if (writelisting > 0)
	    writestatistics(listing);
    }
}

#define MAXTAB	10

static int my_index;
static void (*my_table[MAXTAB])(VOIDPARM);

static int my_atexit(proc)
void (*proc)(VOIDPARM);
{
    if (my_index == MAXTAB)
	return 1;
    my_table[my_index++] = proc;
    return 0;
}

static void my_exit(code)
int code;	
{
    while (--my_index >= 0)
	(*my_table[my_index])();
    exit(code);
}
