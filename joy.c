/*
    module  : joy.c
    version : 1.51
    date    : 12/14/24
*/
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <setjmp.h>
#include "malloc.h"

#if 0
#define DEBUG
#define AUTOP
#endif

#ifdef DEBUG
#define LOGFILE(x)	printf("%s\n", x)
#else
#define LOGFILE(x)
#endif

#define true		1
#define false		0

#define errormark	"%JOY"

#define lib_filename	"42minjoy.lib"
#define list_filename	"42minjoy.lst"

#define reslength	8
#define maxrestab	10

#define identlength	16
#define maxstdidenttab	100

/*
 * Strings that are read in the library are set to uncollectable. Strings that
 * are unmarked can be freed. They are then set to uncollectable in order to
 * prevent double free.
 */
#define maxrefcnt	2
#define strmarked	1
#define strunmark	0

typedef unsigned char boolean;

typedef enum {
    lbrack, rbrack, semic, period, def_equal,
/* compulsory for scanutilities: */
    charconst, stringconst, numberconst, leftparenthesis, identifier
/* hyphen */
} symbol;

/*
 * Addition: getch, putch
 * Renaming: index -> of, select -> opcase
 * Removing: nothing
 *
 * The removal of nothing means that uncons can only be used on non-empty
 * lists.
 */
typedef enum {
    lib_, mul_, add_, sub_, div_, lss_, eql_, and_, argv_, binrec_, body_,
    cleave_, cons_, dip_, dup_, false_, feof_, fgetch_, fopen_, get_, getch_,
    i_, not_, of_, opcase_, or_, pop_, put_, putch_, sametype_,
#ifdef AUTOP
    setautoput_,
#endif
    stack_, stdin_, step_, strtol_, swap_, true_, unary2_, uncons_, unstack_,
    while_, boolean_, char_, integer_, list_, string_, file_, unknownident
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
char **g_argv;	/* command line */
int g_argc;	/* command line */

/*
 * The value_t size should be as large as a pointer, because sometimes it is
 * used to store a pointer.
 */
#if _MSC_VER >= 1941
typedef long long value_t;
#else
typedef long value_t;
#endif

#include "proto.h"
#define MINJOY
#include "scanutil.c"

static void initialise()
{
    int i;

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
    est("argv",       argv_);
    est("binrec",     binrec_);
    est("body",       body_);
    est("cleave",     cleave_);
    est("cons",       cons_);
    est("dip",        dip_);
    est("dup",        dup_);
    est("false",      false_);
    est("feof",       feof_);
    est("fgetch",     fgetch_);
    est("fopen",      fopen_);
    est("get",        get_);
    est("getch",      getch_);
    est("i",          i_);
    est("not",        not_);
    est("of",         of_);
    est("opcase",     opcase_);
    est("or",         or_);
    est("pop",        pop_);
    est("put",        put_);
    est("putch",      putch_);
    est("sametype",   sametype_);
#ifdef AUTOP
    est("setautoput", setautoput_);
#endif
    est("stack",      stack_);
    est("stdin",      stdin_);
    est("step",       step_);
    est("strtol",     strtol_);
    est("swap",       swap_);
    est("true",       true_);
    est("unary2",     unary2_);
    est("uncons",     uncons_);
    est("unstack",    unstack_);
    est("while",      while_);
    for (i = mul_; i <= while_; i++)
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
    value_t val;
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
    "LIB", "*", "+", "-", "/", "<", "=", "and", "argv", "binrec", "body",
    "cleave", "cons", "dip", "dup", "false", "feof", "fgetch", "fopen", "get",
    "getch", "i", "not", "of", "opcase", "or", "pop", "put", "putch",
    "sametype",
#ifdef AUTOP
    "setautoput",
#endif
    "stack", "stdin", "step", "strtol", "swap", "true", "unary2", "uncons",
    "unstack", "while", "BOOLEAN", "CHAR", "INTEGER", "LIST", "STRING", "FILE",
    "UNKNOWN"
};

#ifdef DEBUG
void DumpM()
{
    int i;
    FILE *fp = fopen("joy.dmp", "w");

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
		standardident_NAMES[m[i].op], (long)m[i].val, m[i].nxt,
		m[i].marked ? 'T' : 'F');
    fclose(fp);
}
#endif

static void lookup()
{
    int i, j;

    if (!sentinel) {
	id = unknownident;
	return;
    }
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
		if (lasttable == MAXTABLE)
		    point('F', "too many library symbols");
		strcpy(table[++lasttable].alf, ident);
		table[locatn = lasttable].adr = 0;
		id = lib_;
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
    if (m[n].op == unknownident)
	fprintf(f, "%5d %-*.*s %10d %10d %c", n, identlength,
	    identlength, (char *)m[n].val, 0, m[n].nxt,
	    m[n].marked ? 'T' : 'F');
    else
	fprintf(f, "%5d %-*.*s %10ld %10d %c", n, identlength,
	    identlength, standardident_NAMES[m[n].op], (long)m[n].val,
	    m[n].nxt, m[n].marked ? 'T' : 'F');
    if (m[n].op == lib_)
	fprintf(f, "   %-*.*s %4d", identlength, identlength,
		table[m[n].val].alf, table[m[n].val].adr);
    putc('\n', f);
}

static void writenode(n)
memrange n;
{
    wn(stdout, n);
    if (writelisting > 0) {
	putc('\t', listing);
	wn(listing, n);
    }
}  /* writenode */

static void mark(n)
memrange n;
{
    char *ptr;

    while (n > 0) {
	if (writelisting > 4)
	    writenode(n);
	if (m[n].op == string_) {
	    ptr = (char *)m[n].val;
	    if (ptr[-1] == strunmark)	/* if unmarked */
		ptr[-1] = strmarked;	/* set marked */
	}
	if (m[n].op == list_ && !m[n].marked)
	    mark((memrange)m[n].val);
	m[n].marked = true;
	n = m[n].nxt;
    }
}  /* mark */

static memrange kons(o, v, n)
standardident o;
value_t v;
memrange n;
{
    char *ptr;
    memrange i;
    value_t collected;

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
	/*
	 * Scan memory and move all unused nodes to the freelist. Also free
	 * strings that are referenced only once.
	 */
	collected = 0;
	for (i = firstusernode; i <= MAXMEM; i++) {
	    if (!m[i].marked) {
		if (m[i].op == string_) {
		    ptr = (char *)m[i].val;
		    if (ptr[-1] == strunmark)		/* if unmarked */
			strfree((char *)m[i].val);	/* free unused */
		}
		m[i].nxt = freelist;
		freelist = i;
		collected++;
	    }
	    if (m[i].nxt == i)
		point('F', "internal error - selfreference");
	}
	/*
	 * Scan again and unmark nodes that are in use. Also reset strings.
	 */
	for (i = firstusernode; i <= MAXMEM; i++)
	    if (m[i].marked) {
		if (m[i].op == string_) {
		    ptr = (char *)m[i].val;
		    if (ptr[-1] == strmarked)		/* if marked */
			ptr[-1] = strunmark;		/* set unmarked */
		}
		m[i].marked = false;
	    }
	if (writelisting > 2) {
	    writeinteger(collected);
	    putch(' ');
	    writeident("nodes collected");
	    writeline();
	}
einde:	if (!freelist)
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

static void readfactor(where)
memrange *where;
{
    memrange here;

    switch (sym) {
    case lbrack:
	getsym();
	*where = kons(list_, 0L, 0);
	m[*where].marked = true;
	if (sym == lbrack || sym == identifier || sym == charconst ||
	    sym == numberconst || sym == stringconst) {	/* sym == hyphen */
	    readterm(&here);
	    m[*where].val = here;
	}
	break;

    case identifier:
	lookup();
	if (id == unknownident)
	    *where = kons(id, (value_t)strsave(ident), 0);
	else
	    *where = kons(id, (value_t)locatn, 0);
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
    case stringconst:
	/*
	 * During read of the library, strings must be marked. All copies of
	 * these strings are then also marked and cannot be freed.
	 */
	if (!sentinel)
	    str[-1] = maxrefcnt;
	*where = kons(string_, (value_t)str, 0);
	break;

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

    /* this is LL0 */
    readfactor(first);
    if ((i = *first) == 0)
	return;
    getsym();
    while (sym == lbrack || sym == identifier || sym == charconst ||
	   sym == numberconst || sym == stringconst) {	/* sym == hyphen */
	readfactor(&m[i].nxt);
	i = m[i].nxt;
	getsym();
    }
}  /* readterm */

static void writeterm(n, nl)
memrange n;
boolean nl;
{
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

	case string_:
	    putch('"');
	    writeident((char *)m[n].val);
	    putch('"');
	    break;

	case file_:
	    writeident("FILE:");
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

static void patchterm(n)
memrange n;
{
    while (n > 0) {
	patchfactor(n);
	n = m[n].nxt;
    }
}  /* patchterm */

static void patchfactor(n)
memrange n;
{   /* was forward */
    if (n > 0) {
	switch (m[n].op) {
	case list_:
	    patchterm((memrange)m[n].val);
	    break;

	case unknownident:
	    strncpy(ident, (char *)m[n].val, identlength);
	    ident[identlength] = 0;
	    strfree((char *)m[n].val);
	    lookup();
	    m[n].op = (unsigned char)id;
	    m[n].val = locatn;
	    break;
	}  /* CASE */
    }
}  /* patchfactor */

static void readlibrary(str)
char *str;
{
    int loc;
    FILE *fp;
    char *lib = 0;

    /*
     * Check that the library file is present. If not, this should not be a
     * fatal error. Some global variables need to be set in case that happens.
     * If the library is not present in the current directory, but is present
     * in the same directory as the joy binary, it is read from there. This
     * also applies to files that are included in the library.
     */
    lastlibloc = 0;
    if ((fp = fopen(str, "r")) == 0) {
	/*
	 * Prepend the pathname and try again. The library files are expected
	 * to exist somewhere.
	 */
	if (pathname) {
	    loc = strlen(pathname) + strlen(str) + 1;
	    lib = stralloc(loc);
	    sprintf(lib, "%s%s", pathname, str);
	    if ((fp = fopen(lib, "r")) == 0)
		goto failed;
	    str = lib;
	    goto done;
	}
failed: strfree(lib);
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
    newfile(str, 1);
    getsym();
    while (sym != period) {
	if (writelisting > 8)
	    fprintf(listing, "seen : %-*.*s\n", identlength, identlength,
		    ident);
	if (lastlibloc > 0)
	    if (strcmp(ident, table[lastlibloc].alf) <= 0)
		point('F', "bad order in library");
	if (lastlibloc == MAXTABLE)
	    point('F', "too many library symbols");
	strcpy(table[++lastlibloc].alf, ident);
	loc = lastlibloc;
	getsym();
	if (sym != def_equal)
	    point('F', "pass 2: \"==\" expected");
	getsym();
	readterm(&table[loc].adr);
	if (writelisting > 8)
	    writeterm(table[loc].adr, true);
	if (sym != period)
	    getsym();
    }
    if (lib)
	strfree(lib);
    firstusernode = freelist;
    if (writelisting > 5)
	fprintf(listing, "firstusernode = %d,  total memory = %d\n",
		firstusernode, MAXMEM);
    cc = ll;
    sentinel = lastlibloc + 1;
    lasttable = sentinel;
    for (loc = 1; loc < lasttable; loc++)
	patchterm(table[loc].adr);
    adjustment = -1;
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

static value_t get_i(x)
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

static value_t v(x)
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
value_t v;
{
    s = kons(o, v, n(n(s)));
}

static void do_opcase()
{
    memrange temp;

    temp = l(s);
    while (o(l(temp)) != o(n(s)))
	temp = n(temp);
    s = kons(list_, (value_t)n(l(temp)), n(s));
}

static void do_of()
{
    value_t val;
    memrange temp;

    temp = l(s);
    for (val = v(n(s)); val > 0; val--)
	temp = n(temp);
    s = kons(o(temp), v(temp), n(n(s)));
}

static void do_get()
{
    memrange temp;

    getsym();
    readfactor(&temp);
    s = kons(o(temp), v(temp), s);
}

static void do_i()
{
    memrange prog;

    prog = l(s);
    dump = kons(list_, (value_t)prog, dump);
    s = n(s);
    joy(prog);
    dump = n(dump);
}

static void do_dip()
{
    memrange prog;

    prog = l(s);
    dump = kons(list_, (value_t)prog, dump);
    s = n(s);
    dump = kons(o(s), v(s), dump);
    s = n(s);
    joy(prog);
    s = kons(o(dump), v(dump), s);
    dump = n(n(dump));
}

static void do_step()
{
    memrange prog, parm;

    prog = l(s);
    dump = kons(list_, (value_t)prog, dump);
    s = n(s);
    parm = l(s);
    dump = kons(list_, (value_t)parm, dump);
    for (s = n(s); parm; parm = n(parm)) {
	s = kons(o(parm), v(parm), s);
	joy(prog);
    }
    dump = n(n(dump));
}

static void do_argv()
{
    int i;
    memrange lst = 0;

    for (i = g_argc - 1; i > 0; i--)
	lst = kons(string_, (value_t)strsave(g_argv[i]), lst);
    s = kons(list_, (value_t)lst, s);
}

static void do_strtol()
{
    binary(integer_, (value_t)strtol((char *)v(n(s)), 0, (int)v(s)));
}

static void do_while()
{
    boolean result;
    memrange prog[2], save;

    prog[1] = l(s);
    dump = kons(list_, (value_t)prog[1], dump);
    s = n(s);
    prog[0] = l(s);
    dump = kons(list_, (value_t)prog[0], dump);
    save = s = n(s);
    dump = kons(list_, (value_t)save, dump);
    while (1) {
	save = s;
	joy(prog[0]);
	result = b(s);
	s = save;
	if (!result)
	    break;
	joy(prog[1]);
    }
    dump = n(n(n(dump)));
}

static void do_unary2()
{
    /*  Y Z [P]  unary2  ==>  Y' Z'  */
    memrange prog, parm, save, outp;

    prog = l(s);
    dump = kons(list_, (value_t)prog, dump);	/* save prog */
    parm = s = n(s);
    dump = kons(o(parm), v(parm), dump);	/* save Z */
    s = n(s);
    save = n(s);
    dump = kons(list_, (value_t)save, dump);	/* save stack */
    joy(prog);					/* execute P */
    outp = s;
    dump = kons(o(outp), v(outp), dump);	/* save P(Y) */
    s = kons(o(parm), v(parm), save);		/* just Z on top */
    joy(prog);					/* execute P */
    dump = kons(o(s), v(s), dump);		/* save P(Z) */
    s = kons(o(outp), v(outp), save);		/* Y' */
    s = kons(o(dump), v(dump), s);		/* Z' */
    dump = n(n(n(n(n(dump)))));
}

static void do_cleave()
{
    /*  X [P1] [P2]  cleave  ==>  X1 X2  */
    memrange prog[2], save, outp;

    prog[1] = l(s);
    dump = kons(list_, (value_t)prog[1], dump);
    s = n(s);
    prog[0] = l(s);
    dump = kons(list_, (value_t)prog[0], dump);
    save = s = n(s);
    dump = kons(list_, (value_t)save, dump);	/* save stack */
    joy(prog[0]);				/* [P1] */
    outp = s;
    dump = kons(o(outp), v(outp), dump);	/* save X1 */
    s = save;					/* restore stack */
    joy(prog[1]);				/* [P2] */
    dump = kons(o(s), v(s), dump);		/* save X2 */
    s = kons(o(outp), v(outp), n(save));	/* X1 */
    s = kons(o(dump), v(dump), s);		/* X2 */
    dump = n(n(n(n(n(dump)))));
}

static void do_stdin()
{
    s = kons(file_, (value_t)stdin, s);
}

static void do_fopen()
{
    FILE *fp;
    char *path, *mode;

    mode = (char *)v(s);
    s = n(s);
    path = (char *)v(s);
    fp = fopen(path, mode);
    s = kons(file_, (value_t)fp, n(s));
}

static void do_fgetch()
{
    int c;
    FILE *fp;

    fp = (FILE *)v(s);
    c = getc(fp);
    s = kons(char_, (value_t)c, s);
}

static void do_feof()
{
    FILE *fp;

    fp = (FILE *)v(s);
    s = kons(boolean_, (value_t)feof(fp), s);    
}

static void binrecaux(memrange prog[])
{
    memrange save;
    boolean result;

    save = s;
    dump = kons(list_, (value_t)save, dump);	/* save stack */
    joy(prog[0]);				/* condition */
    result = b(s);				/* get result */
    s = save;					/* restore stack */
    dump = n(dump);
    if (result)
	joy(prog[1]);
    else {
	joy(prog[2]);				/* split */
	dump = kons(o(s), v(s), dump);		/* save second */
	s = n(s);
	binrecaux(prog);			/* first */
	s = kons(o(dump), v(dump), s);		/* push second */
	dump = n(dump);
	binrecaux(prog);			/* second */
	joy(prog[3]);				/* combine */
    }
}

static void do_binrec()
{
    memrange prog[4];

    prog[3] = l(s);
    dump = kons(list_, (value_t)prog[3], dump);
    s = n(s);
    prog[2] = l(s);
    dump = kons(list_, (value_t)prog[2], dump);
    s = n(s);
    prog[1] = l(s);
    dump = kons(list_, (value_t)prog[1], dump);
    s = n(s);
    prog[0] = l(s);
    dump = kons(list_, (value_t)prog[0], dump);
    s = n(s);
    binrecaux(prog);
    dump = n(n(n(n(dump))));
}

static void joy(node)
memrange node;
{
    for (; node; node = n(node)) {	/* FOR */
	if (writelisting > 3) {
	    writeident("joy: ");
	    writefactor(node, true);
	}
	if (writelisting > 4) {
	    writeident("stack: ");
	    writeterm(s, true);
	    writeident("dump: ");
	    writeterm(dump, true);
	}
#if 0
	last_op_executed = m[node].op;
#endif
	switch (o(node)) {
	case char_:
	case integer_:
	case list_:
	case string_:
	    s = kons(o(node), v(node), s);
	    break;

	case true_:
	case false_:
	    s = kons(boolean_, (value_t)(o(node) == true_), s);
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
	    s = kons(list_, (value_t)s, s);
	    break;

	case unstack_:
	    s = l(s);
	    break;

	/* OPERATIONS: */
	case not_:
	    s = kons(boolean_, (value_t)!b(s), n(s));
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
	    binary(boolean_, (value_t)(b(n(s)) & b(s)));
	    break;

	case or_:
	    binary(boolean_, (value_t)(b(n(s)) | b(s)));
	    break;

	case lss_:
	    if (o(s) == lib_)
		binary(boolean_, (value_t)(strcmp(table[v(n(s))].alf,
						  table[v(s)].alf) < 0));
	    else
		binary(boolean_, (value_t)(v(n(s)) < v(s)));
	    break;

	case eql_:
	    binary(boolean_, (value_t)(v(n(s)) == v(s)));
	    break;

	case sametype_:
	    binary(boolean_, (value_t)(o(n(s)) == o(s)));
	    break;

	case cons_:
	    s = kons(list_, (value_t)kons(o(n(s)), v(n(s)), (memrange)v(s)),
		     n(n(s)));
	    break;

	case uncons_:
	    s = kons(list_, (value_t)n(l(s)), kons(o(l(s)), v(l(s)), n(s)));
	    break;

	case opcase_:
	    do_opcase();
	    break;

	case of_:
	    do_of();
	    break;

	case body_:
	    s = kons(list_, (value_t)table[v(s)].adr, n(s));
	    break;

#ifdef AUTOP
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
	    do_get();
	    break;

	case getch_:
	    getch();
	    s = kons(integer_, (value_t)chr, s);
	    break;

	/* COMBINATORS: */
	case i_:
	    do_i();
	    break;

	case dip_:
	    do_dip();
	    break;

	case step_:
	    do_step();
	    break;

	case lib_:
	    joy(table[v(node)].adr);
	    break;

	/* EXTENSIONS: */
	case argv_:
	    do_argv();
	    break;

	case strtol_:
	    do_strtol();
	    break;

	case while_:
	    do_while();
	    break;

	case unary2_:
	    do_unary2();
	    break;

	case cleave_:
	    do_cleave();
	    break;

	case stdin_:
	    do_stdin();
	    break;

	case fopen_:
	    do_fopen();
	    break;

	case fgetch_:
	    do_fgetch();
	    break;

	case feof_:
	    do_feof();
	    break;

	case binrec_:
	    do_binrec();
	    break;

	default:
	    point('F', "internal error in interpreter");
	}  /* CASE */
	stat_ops++;
    }
    stat_calls++;
}  /* joy */

static void writestatistics(f)
FILE *f;
{
    time_t c[2];

    /*
     * end_time - beg_time has already been reported as the time needed to
     * execute (unless the time measured was 0.) This duration includes reading
     * the library. Here it is split between reading the library and execution
     * proper.
     */
    c[0] = end_time - beg_time;
    c[1] = stat_lib - beg_time;
    if (c[1] > 0)
	fprintf(f, "%lu seconds CPU to read library\n", (unsigned long)c[1]);
    c[0] -= c[1];
    if (c[0] > 0)
	fprintf(f, "%lu seconds CPU to execute\n", (unsigned long)c[0]);
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
	/*
	 * Remove the joy binary, keep the separator.
	 */
	ptr[1] = 0;
	/*
	 * Make the pathname available everywhere.
	 */
	pathname = argv[0];
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
    g_argv = argv;	/* command line */
    g_argc = argc;	/* command line */
    /*
     * A filename parameter is possible: it contains programs to be executed.
     * The file replaces standard input.
     */
    if (argc > 1)
	newfile(argv[1], 0);
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
