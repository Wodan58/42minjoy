/*
    module  : joy.c
    version : 1.29
    date    : 05/30/23
*/
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <setjmp.h>
#include <stdbool.h>
#include <inttypes.h>

/* #define DEBUG */

#ifdef _MSC_VER
#pragma warning(disable : 4244 4996)
#endif

#define CORRECT_GARBAGE
#define READ_LIBRARY_ONCE

#ifdef DEBUG
int debug = 1;
#define LOGFILE(s)	{ if (debug) { FILE *fp = fopen("joy.log",\
			strcmp(s, "main") ? "a" : "w");\
			fprintf(fp, "%s\n", s); fclose(fp); } }
#else
#define LOGFILE(s)
#endif

typedef unsigned char boolean;

#define true  1
#define false 0

#define errormark	"%JOY"

#define lib_filename	"42minjoy.lib"
#define list_filename	"42minjoy.lst"

#define reslength	8
#if 0
#define emptyres	"        "
#endif
#define maxrestab	10

#define identlength	16
#if 0
#define emptyident	"                "
#endif
#define maxstdidenttab	32

typedef enum {
    lbrack, rbrack, semic, period, def_equal,
/* compulsory for scanutilities: */
    charconst, stringconst, numberconst, leftparenthesis, identifier
/* hyphen */
} symbol;

typedef enum {
    lib_, mul_, add_, sub_, div_, lss_, eql_, and_, body_, cons_, dip_,
    dup_, false_, get_, getch_, i_, index_, not_, nothing_, or_, pop_, put_,
    putch_, sametype_, select_, stack_, step_, swap_, true_, uncons_, unstack_,
    boolean_, char_, integer_, list_, unknownident
} standardident;

#define MINJOY
#include "scanutil.c"

static void initialise(void)
{
    unsigned i;

    LOGFILE(__func__);
    iniscanner();
    strcpy(specials_repeat, "=>");
    erw(".",	period);
    erw(";",	semic);
    erw("==",	def_equal);
    erw("[",	lbrack);
    erw("]",	rbrack);
    est("*",        mul_);
    est("+",        add_);
    est("-",        sub_);
    est("/",        div_);
    est("<",        lss_);
    est("=",        eql_);
    est("and",      and_);
    est("body",     body_);
    est("cons",     cons_);
    est("dip",      dip_);
    est("dup",      dup_);
    est("false",    false_);
    est("get",      get_);
    est("getch",    getch_);
    est("i",        i_);
    est("index",    index_);
    est("not",      not_);
    est("nothing",  nothing_);
    est("or",       or_);
    est("pop",      pop_);
    est("put",      put_);
    est("putch",    putch_);
    est("sametype", sametype_);
    est("select",   select_);
    est("stack",    stack_);
    est("step",     step_);
    est("swap",     swap_);
    est("true",     true_);
    est("uncons",   uncons_);
    est("unstack",  unstack_);
    for (i = mul_; i <= unstack_; i++)
	if (i != stdidents[i].symb)
	    point('F', "bad order in standard idents");
}  /* initialise */

#ifndef MAXTABLE
#define MAXTABLE	300
#endif
#ifndef MAXMEM
#define MAXMEM		1999
#endif

typedef short memrange;

typedef struct _REC_table {
    identalfa alf;
    memrange adr;
} _REC_table;

typedef struct _REC_m {
    intptr_t val;
    memrange nxt;
    unsigned char op;
    boolean marked;
} _REC_m;

static _REC_table table[MAXTABLE + 1];
static long lastlibloc, sentinel, lasttable, locatn;
static _REC_m m[MAXMEM + 1];
static memrange firstusernode, freelist, programme;
static memrange s,  /* stack */
		dump;

#if 0
static standardident last_op_executed;
#endif
static long stat_kons, stat_gc, stat_ops, stat_calls;
static clock_t stat_lib;

static char *standardident_NAMES[] = {
    "LIB", "*", "+", "-", "/", "<", "=", "and", "body", "cons", "dip", "dup",
    "false", "get", "getch", "i", "index", "not", "nothing", "or", "pop",
    "put", "putch", "sametype", "select", "stack", "step", "swap", "true",
    "uncons", "unstack", "BOOLEAN", "CHAR", "INTEGER", "LIST", "UNKNOWN"
};

#ifdef DEBUG
void DumpM(void)
{
    long i;
    FILE *fp = fopen("joy.dmp", "w");

    LOGFILE(__func__);
    fprintf(fp, "Table\n");
    fprintf(fp, "  nr %-*.*s  adr\n", identlength, identlength, "name");
    for (i = 1; i <= MAXTABLE && table[i].adr; i++)
	fprintf(fp, "%4ld %-*.*s %4ld\n", i, identlength, identlength,
		table[i].alf, (long)table[i].adr);
    fprintf(fp, "\nMemory\n");
    fprintf(fp, "  nr %-*.*s      value next M\n", identlength,
	    identlength, "name");
    for (i = 1; i <= MAXMEM && m[i].marked; i++)
	fprintf(fp, "%4ld %-*.*s %10ld %4ld %c\n", i, identlength, identlength,
		standardident_NAMES[m[i].op], (long)m[i].val, (long)m[i].nxt,
		m[i].marked ? 'T' : 'F');
    fclose(fp);
}
#endif

static void lookup(void)
{
    int i, j;

    LOGFILE(__func__);
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
	fprintf(listing, "lookup : %-*.*s at %ld\n", identlength, identlength,
		standardident_NAMES[id], locatn);
}  /* lookup */

static void wn(FILE *f, memrange n)
{
    LOGFILE(__func__);
#ifdef READ_LIBRARY_ONCE
    if (m[n].op == unknownident)
	fprintf(f, "%5ld %-*.*s %10ld %10ld %c", (long)n, identlength,
	    identlength, (char *)m[n].val, 0L, (long)m[n].nxt,
	    m[n].marked ? 'T' : 'F');
    else
#endif
	fprintf(f, "%5ld %-*.*s %10ld %10ld %c", (long)n, identlength,
	    identlength, standardident_NAMES[m[n].op], (long)m[n].val,
	    (long)m[n].nxt, m[n].marked ? 'T' : 'F');
    if (m[n].op == lib_)
	fprintf(f, "   %-*.*s %4ld", identlength, identlength,
		table[m[n].val].alf, (long)table[m[n].val].adr);
    putc('\n', f);
}

static void writenode(memrange n)
{
    LOGFILE(__func__);
    wn(stdout, n);
    if (writelisting > 0) {
	putc('\t', listing);
	wn(listing, n);
    }
}  /* writenode */

static void mark(memrange n)
{
    LOGFILE(__func__);
    while (n > 0) {
	if (writelisting > 4)
	    writenode(n);
	if (m[n].op == list_ && !m[n].marked)
	    mark(m[n].val);
	m[n].marked = true;
	n = m[n].nxt;
    }
}  /* mark */

static memrange kons(standardident o, intptr_t v, memrange n)
{
    memrange i;
    long collected;

    LOGFILE(__func__);
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
	    mark(v);
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
    m[i].op = o;
    m[i].val = v;
    m[i].nxt = n;
    if (writelisting > 4)
	writenode(i);
    stat_kons++;
    return i;
}  /* kons */

static char *my_strdup(char *str)
{
    char *ptr;
    size_t leng;

    leng = strlen(str);
    if ((ptr = malloc(leng + 1)) != 0)
        strcpy(ptr, str);
    return ptr;
}

static void readterm(memrange *);

static void readfactor(memrange *where)
{
    memrange here;

    LOGFILE(__func__);
    switch (sym) {

    case lbrack:
	getsym();
	*where = kons(list_, 0, 0);
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
	    *where = kons(id, (intptr_t)my_strdup(ident), 0);
	else
#endif
	    *where = kons(id, locatn, 0);
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

    default:
	point('F', "internal in readfactor");
    }  /* CASE */
    m[*where].marked = true;
}  /* readfactor */

static void readterm(memrange *first)
{   /* readterm */
    /* was forward */
    memrange i;

    LOGFILE(__func__);
    /* this is LL0 */
    readfactor(first);
    i = *first;
    getsym();
    while (sym == lbrack || sym == identifier || sym == charconst ||
		sym == numberconst) {	/* sym == hyphen */
	readfactor(&m[i].nxt);
	i = m[i].nxt;
	getsym();
    }
}  /* readterm */

static void writefactor(memrange n, boolean nl);

static void writeterm(memrange n, boolean nl)
{
    LOGFILE(__func__);
    while (n > 0) {
	writefactor(n, false);
	if (m[n].nxt > 0)
	    putch(' ');
	n = m[n].nxt;
    }
    if (nl)
	writeline();
}  /* writeterm */

static void writefactor(memrange n, boolean nl)
{   /* was forward */
    LOGFILE(__func__);
    if (n > 0) {
	switch (m[n].op) {

	case list_:
	    putch('[');
	    writeterm(m[n].val, false);
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
		putch(m[n].val);
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
static void patchfactor(memrange n);

static void patchterm(memrange n)
{
    LOGFILE(__func__);
    while (n > 0) {
	patchfactor(n);
	n = m[n].nxt;
    }
}  /* patchterm */

static void patchfactor(memrange n)
{   /* was forward */
    LOGFILE(__func__);
    if (n > 0) {
	switch (m[n].op) {

	case list_:
	    patchterm(m[n].val);
	    break;

	case unknownident:
	    strncpy(ident, (char *)m[n].val, identlength);
	    ident[identlength] = 0;
	    free((char *)m[n].val);
	    m[n].val = 0;
	    lookup();
	    m[n].op = id;
	    m[n].val = locatn;
	    break;
	}  /* CASE */
    }
}  /* patchfactor */
#endif

static void readlibrary(char *str)
{
    int loc;

    LOGFILE(__func__);
#if 0
    if (writelisting > 5)
	fprintf(listing, "first pass through library:\n");
#endif
    newfile(str);
    lastlibloc = 0;
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
    newfile(str);
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
	fprintf(listing, "firstusernode = %ld,  total memory = %ld\n",
		(long)firstusernode, (long)MAXMEM);
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
}  /* readlibrary */

static jmp_buf JL10;

static memrange ok(memrange x)
{
    if (x < 1)
	point('F', "null address being referenced");
    return x;
}  /* ok */

static standardident o(memrange x)
{
    return m[ok(x)].op;
}

static long i(memrange x)
{
    if (o(x) == integer_)
	return m[x].val;
    point('R', "integer value required");
    longjmp(JL10, 1);
}  /* i */

static memrange l(memrange x)
{
    if (o(x) == list_)
	return m[x].val;
    point('R', "list value required");
    longjmp(JL10, 1);
}  /* l */

static memrange n(memrange x)
{
    if (m[ok(x)].nxt >= 0)
	return m[x].nxt;
    point('R', "negative next value");
    longjmp(JL10, 1);
}  /* n */

static long v(memrange x)
{
    return m[ok(x)].val;
}

static boolean b(memrange x)
{
    return (boolean)(v(x) > 0);
}

static void binary(standardident o, long v)
{
    s = kons(o, v, n(n(s)));
}

static void joy(memrange nod)
{
    memrange temp1, temp2;

    LOGFILE(__func__);
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

	case nothing_:
	case char_:
	case integer_:
	case list_:
	    s = kons(m[nod].op, m[nod].val, s);
	    break;

	case true_:
	case false_:
	    s = kons(boolean_, m[nod].op == true_, s);
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
	    s = kons(list_, s, s);
	    break;

	case unstack_:
	    s = l(s);
	    break;

	/* OPERATIONS: */
	case not_:
	    s = kons(boolean_, !b(s), n(s));
	    break;

	case mul_:
	    binary(integer_, i(n(s)) * i(s));
	    break;

	case add_:
	    binary(integer_, i(n(s)) + i(s));
	    break;

	case sub_:
	    binary(integer_, i(n(s)) - i(s));
	    break;

	case div_:
	    binary(integer_, i(n(s)) / i(s));
	    break;

	case and_:
	    binary(boolean_, b(n(s)) & b(s));
	    break;

	case or_:
	    binary(boolean_, b(n(s)) | b(s));
	    break;

	case lss_:
	    if (o(s) == lib_)
		binary(boolean_,strcmp(table[v(n(s))].alf,table[v(s)].alf) < 0);
	    else
		binary(boolean_, v(n(s)) < v(s));
	    break;

	case eql_:
	    binary(boolean_, v(n(s)) == v(s));
	    break;

	case sametype_:
	    binary(boolean_, o(n(s)) == o(s));
	    break;

	case cons_:
	    if (o(n(s)) == nothing_)
		s = kons(list_, l(s), n(n(s)));
	    else
		s = kons(list_, kons(o(n(s)), v(n(s)), v(s)), n(n(s)));
	    break;

	case uncons_:
	    if (!v(s))
		s = kons(list_, 0, kons(nothing_, nothing_, n(s)));
	    else
		s = kons(list_, n(l(s)), kons(o(l(s)), v(l(s)), n(s)));
	    break;

	case select_:
	    temp1 = l(s);
	    while (o(l(temp1)) != o(n(s)))
		temp1 = n(temp1);
	    s = kons(list_, n(l(temp1)), n(s));
	    break;

	case index_:
	    if (v(n(s)) < 1)
		s = kons(o(l(s)), v(l(s)), n(n(s)));
	    else
		s = kons(o(n(l(s))), v(n(l(s))), n(n(s)));
	    break;

	case body_:
	    s = kons(list_, table[v(s)].adr, n(s));
	    break;

	case put_:
	    writefactor(s, false);
	    s = n(s);
	    break;

	case putch_:
	    putch(v(s));
	    s = n(s);
	    break;

	case get_:
	    getsym();
	    readfactor(&temp1);
	    s = kons(o(temp1), v(temp1), s);
	    break;

	case getch_:
	    getch();
	    s = kons(integer_, ch, s);
	    break;

	/* COMBINATORS: */
	case i_:
#ifdef CORRECT_GARBAGE
	    dump = kons(o(s), l(s), dump);
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
	    dump = kons(list_, l(s), dump);
	    s = n(n(s));
	    joy(l(dump));
	    dump = n(dump);
	    s = kons(o(dump), v(dump), s);
	    dump = n(dump);
	    break;

	case step_:
	    dump = kons(o(s), l(s), dump);
	    dump = kons(o(n(s)), l(n(s)), dump);
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

static void writestatistics(FILE *f)
{
    LOGFILE(__func__);
    fprintf(f, "%f seconds CPU to read library\n",
           (double)(stat_lib - start_clock) / CLOCKS_PER_SEC);
    fprintf(f, "%f seconds CPU to execute\n",
           (double)(end_clock - stat_lib) / CLOCKS_PER_SEC);
    fprintf(f, "%lu user nodes available\n", MAXMEM - firstusernode + 1L);
    fprintf(f, "%lu garbage collections\n", stat_gc);
    fprintf(f, "%lu nodes used\n", stat_kons);
    fprintf(f, "%lu calls to joy interpreter\n", stat_calls);
    fprintf(f, "%lu operations executed\n", stat_ops);
}  /* writestatistics */

static void perhapsstatistics(void);

int main(int argc, char *argv[])
{  /* main */
    memrange i;
    int j, k = 1;

    LOGFILE(__func__);
    start_clock = clock();
    initialise();
    atexit(perhapsstatistics);
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
    if (argc == 1)
	readlibrary(lib_filename);
    else {
	argc--;
	readlibrary(argv[k++]);
    }
    stat_lib = clock();
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
    if (argc > 1)
	newfile(argv[k]);
    setjmp(JL10);
    do {
	getsym();
	if (sym != period) {
#if 0
	    last_op_executed = get_;
#endif
	    programme = 0;
	    readfactor(&programme);
	    if (writelisting > 2) {
		writeident("interpreting:");
		writeline();
		writefactor(programme, true);
	    }
	    if (dump != 0) {
		printf("dump error: should be empty!\n");
		writeterm(dump, true);
		dump = 0;
	    }
	    outlinelength = 0;
	    joy(m[programme].val);
	    if (outlinelength > 0)
		writeline();
	    if (writelisting > 2) {
		writeident("stack:");
		writeline();
		writeterm(s, true);
	    }
	}  /* IF */
    } while (sym != period);
    finalise();
    return 0;
}

static void perhapsstatistics(void)
{
    LOGFILE(__func__);
    if (statistics > 0) {
	writestatistics(stderr);
	if (writelisting > 0)
	    writestatistics(listing);
    }
}
