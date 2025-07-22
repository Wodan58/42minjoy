/*
    module  : joy.c
    version : 1.57
    date    : 07/21/25
*/
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <setjmp.h>
#ifndef ATARI
#include <stdint.h>
#endif
#include <sys/stat.h>	/* filetime         */
#include "gc.h"		/* GC_malloc_atomic */

#if 0
#define DEBUG_G		/* garbage collect  */
#define DEBUG_M		/* memory / symbols */
#define DEBUG_U		/* (un)used symbols */
#define DEBUG_S		/* stack dump @ end */
#define UNDEF_E		/* undeferror       */
#endif

#ifdef NPROTO
#define VOIDP
#else
#define VOIDP		void
#endif

#define fatal_err	2
#define runtime_err	1
#define no_error	0

#define persistent	2
#define true		1
#define false		0

#define errormark	"%JOY"

#define lib_filename	"42minjoy.lib"
#define list_filename	"42minjoy.lst"

#define reslength	2	/* reserved words  */
#define maxrestab	5	/* precise number  */

#define stdlength	11	/* standard idents */
#define identlength	21	/* used to be 16   */
#define maxstdidenttab	104	/* precise number  */

/*
 * Maxint reports the maximum value an integer can hold. Integers are kept in
 * value_t and only two different sizes are expected.
 *
 * ATARI 1040 STF running under Minix 1.5.
 */
#ifdef ATARI
#define format_int	"%ld"
#define format_file	"%lx"
#define format_long	"%*.*ld"
#define MAX_LONG	2147483647L
#define TO_POINTER(x)	((void *)(long)(x))
#define TO_INTEGER(x)	((value_t)(long)(x))
#else
#define format_int	"%lld"
#define format_file	"%llx"
#define format_long	"%*.*lld"
#define MAX_LONG	9223372036854775807LL
#define TO_POINTER(x)	((void *)(intptr_t)(x))
#define TO_INTEGER(x)	((value_t)(intptr_t)(x))
#endif

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
    lib_, mul_, add_, sub_, div_, lss_, eql_, all_, and_, argv_, assign_,
    binrec_, body_, case_, casting_, cleave_, compare_, concat_, cond_,
    condlinrec_, condnestrec_, cons_, construct_, dip_, divmod_, drop_, dup_,
    equal_, false_, fclose_, feof_, ferror_, fflush_, fgetch_, fgets_,
    filetime_, filter_, fopen_, format_, fput_, fputch_, fputchars_, fread_,
    fremove_, frename_, fseek_, ftell_, fwrite_, genrec_, get_, getch_,
    getenv_, gmtime_, help_, i_, ifte_, in_, intern_, linrec_, localtime_,
    map_, maxint_, mktime_, name_, not_, of_, opcase_, or_, pick_, pop_,
    primrec_, put_, putch_, putchars_, quit_, sametype_, size_, some_, split_,
    stack_, stderr_, stdin_, stdout_, step_, strftime_, strtol_, swap_,
    tailrec_, take_, time_, times_, treegenrec_, treerec_, treestep_, true_,
    typeof_, unary2_, unary3_, unary4_, unassign_, uncons_, undefs_, unstack_,
    while_, xor_,
    boolean_, char_, integer_, list_, string_, file_, funct_, unknownident
} standardident;

typedef unsigned short memrange;

/*
 * Value_t should be at least as large as a pointer, because sometimes it is
 * used to store a pointer.
 */
#ifdef ATARI
typedef long value_t;
#else
typedef long long value_t;
#endif

typedef void (*proc_t)(VOIDP);

/*
 * Pathname of the joy binary, to be used as prefix to library files. That way
 * it is not necessary to copy all library files to all working directories.
 */
static char *pathname;
static char **g_argv;	/* command line */
static int g_argc;

#include "proto.h"
#define MINJOY
#include "scanutil.c"

static void initialise()
{
    int j;

    iniscanner();
    strcpy(specials_repeat, "=>");
    erw(".",	period);
    erw(";",	semic);
    erw("==",	def_equal);
    erw("[",	lbrack);
    erw("]",	rbrack);
    est("*",           mul_);
    est("+",           add_);
    est("-",           sub_);
    est("/",           div_);
    est("<",           lss_);
    est("=",           eql_);
    est("all",         all_);
    est("and",         and_);
    est("argv",        argv_);
    est("assign",      assign_);
    est("binrec",      binrec_);
    est("body",        body_);
    est("case",        case_);
    est("casting",     casting_);
    est("cleave",      cleave_);
    est("compare",     compare_);
    est("concat",      concat_);
    est("cond",        cond_);
    est("condlinrec",  condlinrec_);
    est("condnestrec", condnestrec_);
    est("cons",        cons_);
    est("construct",   construct_);
    est("dip",         dip_);
    est("div",         divmod_);
    est("drop",        drop_);
    est("dup",         dup_);
    est("equal",       equal_);
    est("false",       false_);
    est("fclose",      fclose_);
    est("feof",        feof_);
    est("ferror",      ferror_);
    est("fflush",      fflush_);
    est("fgetch",      fgetch_);
    est("fgets",       fgets_);
    est("filetime",    filetime_);
    est("filter",      filter_);
    est("fopen",       fopen_);
    est("format",      format_);
    est("fput",        fput_);
    est("fputch",      fputch_);
    est("fputchars",   fputchars_);
    est("fread",       fread_);
    est("fremove",     fremove_);
    est("frename",     frename_);
    est("fseek",       fseek_);
    est("ftell",       ftell_);
    est("fwrite",      fwrite_);
    est("genrec",      genrec_);
    est("get",         get_);
    est("getch",       getch_);
    est("getenv",      getenv_);
    est("gmtime",      gmtime_);
    est("help",        help_);
    est("i",           i_);
    est("ifte",        ifte_);
    est("in",          in_);
    est("intern",      intern_);
    est("linrec",      linrec_);
    est("localtime",   localtime_);
    est("map",         map_);
    est("maxint",      maxint_);
    est("mktime",      mktime_);
    est("name",        name_);
    est("not",         not_);
    est("of",          of_);
    est("opcase",      opcase_);
    est("or",          or_);
    est("pick",        pick_);
    est("pop",         pop_);
    est("primrec",     primrec_);
    est("put",         put_);
    est("putch",       putch_);
    est("putchars",    putchars_);
    est("quit",        quit_);
    est("sametype",    sametype_);
    est("size",        size_);
    est("some",        some_);
    est("split",       split_);
    est("stack",       stack_);
    est("stderr",      stderr_);
    est("stdin",       stdin_);
    est("stdout",      stdout_);
    est("step",        step_);
    est("strftime",    strftime_);
    est("strtol",      strtol_);
    est("swap",        swap_);
    est("tailrec",     tailrec_);
    est("take",        take_);
    est("time",        time_);
    est("times",       times_);
    est("treegenrec",  treegenrec_);
    est("treerec",     treerec_);
    est("treestep",    treestep_);
    est("true",        true_);
    est("typeof",      typeof_);
    est("unary2",      unary2_);
    est("unary3",      unary3_);
    est("unary4",      unary4_);
    est("unassign",    unassign_);
    est("uncons",      uncons_);
    est("undefs",      undefs_);
    est("unstack",     unstack_);
    est("while",       while_);
    est("xor",         xor_);
    for (j = mul_; j < xor_; j++)
	if (j != (int)stdidents[j].symb ||
			strcmp(stdidents[j].alf, stdidents[j + 1].alf) > 0)
	    point('F', "bad order in standard idents");
}  /* initialise */

/*
 * There are 3 tables: reserved words, standard identifiers, and user defined
 * symbols. MAXTABLE is for user defined symbols. Strings are not garbage
 * collected. They remain in use until the end of the program.
 */
#ifndef MAXTABLE
#define MAXTABLE	500
#endif
#ifndef MAXMEM
#define MAXMEM		2075
#endif

typedef struct _REC_table {
    identalfa alf;
    memrange adr;
#ifdef DEBUG_U
    unsigned char used;
#endif
} _REC_table;
    
typedef struct _REC_m {
    value_t val;
    unsigned char marked;
    unsigned char op;  /* standardident */
    memrange nxt;
} _REC_m;

static _REC_table table[MAXTABLE];
static int lastlibloc, sentinel, lasttable, locatn;
static _REC_m m[MAXMEM];
static memrange firstusernode, freelist, programme;
static memrange s,  /* stack */
		dump;

#if defined(DEBUG_G) || defined(DEBUG_S)
static standardident last_op_executed;
#endif
static long stat_kons, stat_gc, stat_ops, stat_calls;
static time_t stat_lib;

static char *standardident_NAMES[] = {
    "LIB", "*", "+", "-", "/", "<", "=", "all", "and", "argv", "assign",
    "binrec", "body", "case", "casting", "cleave", "compare", "concat", "cond",
    "condlinrec", "condnestrec", "cons", "construct", "dip", "divmod", "drop",
    "dup", "equal", "false", "fclose", "feof", "ferror", "fflush", "fgetch",
    "fgets", "filetime", "filter", "fopen", "format", "fput", "fputch",
    "fputchars", "fread", "fremove", "frename", "fseek", "ftell", "fwrite",
    "genrec", "get", "getch", "getenv", "gmtime", "help", "i", "ifte", "in",
    "intern", "linrec", "localtime", "map", "maxint", "mktime", "name", "not",
    "of", "opcase", "or", "pick", "pop", "primrec", "put", "putch", "putchars",
    "quit", "sametype", "size", "some", "split", "stack", "stderr", "stdin",
    "stdout", "step", "strftime", "strtol", "swap", "tailrec", "take", "time",
    "times", "treegenrec", "treerec", "treestep", "true", "typeof", "unary2",
    "unary3", "unary4", "unassign", "uncons", "undefs", "unstack", "while",
    "xor",
    "BOOLEAN", "CHAR", "INTEGER", "LIST", "STRING", "FILE", "FUNCTION",
    "UNKNOWN"
};

#ifdef DEBUG_M
void DumpM()
{
    memrange j;
    FILE *fp = fopen("42minjoy.dmp", "w");

    fprintf(fp, "Table\n");
    fprintf(fp, "  nr %-*.*s  adr\n", identlength, identlength, "name");
    for (j = 1; j < lasttable; j++)
	fprintf(fp, "%4d %-*.*s %4d\n", j, identlength, identlength,
		table[j].alf, table[j].adr);
    fprintf(fp, "\nMemory\n");
    fprintf(fp, "  nr %-*.*s           value next M\n", stdlength, stdlength,
	    "name");
    for (j = 1; j < MAXMEM; j++) {
	if (!m[j].marked)
	    continue;
	fprintf(fp, "%4d %-*.*s ", j, stdlength, stdlength,
		standardident_NAMES[m[j].op]);
	if (m[j].op == string_)
	    fprintf(fp, "%-15.15s", (char *)m[j].val);
	else
	    fprintf(fp, format_int, m[j].val);
	fprintf(fp, " %4d %c\n", m[j].nxt, m[j].marked == persistent ? 'P' :
		'T');
    }
    fclose(fp);
}
#endif

#ifdef DEBUG_U
void DumpU()
{
    int j;
    FILE *fp = fopen("42minjoy.use", "w");

    fprintf(fp, "Table\n");
    fprintf(fp, "  nr %-*.*s  adr\n", identlength, identlength, "name");
    for (j = 1; j < lasttable; j++) {
	if (table[j].used)
	    continue;
	fprintf(fp, "%4d %-*.*s %4d\n", j, identlength, identlength,
		table[j].alf, table[j].adr);
    }
    fclose(fp);
}
#endif

static void lookup()
{
    int j, k, result;

    if (!sentinel) {
	id = unknownident;
	return;
    }
    locatn = 0;
    if (sentinel) {	/* library has been read */
	strcpy(table[sentinel].alf, ident);
	locatn = lasttable;
	while (strcmp(table[locatn].alf, ident))
	    locatn--;
    }
    if (locatn > sentinel)
	id = lib_;
    else {
	j = result = 1;
	k = lastlibloc;
	while (k >= j) {
	    locatn = j + (k - j) / 2;
	    if ((result = strcmp(ident, table[locatn].alf)) == 0)
		break;
	    if (result < 0)
		k = locatn - 1;
	    else
		j = locatn + 1;
	}
	if (!result)
	    id = lib_;
	else {	/* binarysearch through standardidentifiers */
	    j = result = 1;
	    k = laststdident;
	    while (k >= j) {
		locatn = j + (k - j) / 2;
		if ((result = strcmp(ident, stdidents[locatn].alf)) == 0)
		    break;
		if (result < 0)
		    k = locatn - 1;
		else
		    j = locatn + 1;
	    }
	    if (!result)
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
    if (writelisting > 3)
	fprintf(listing, "lookup : %-*.*s at %d\n", identlength, identlength,
		standardident_NAMES[id], locatn);
}  /* lookup */

static void wn(f, node)
FILE *f;
memrange node;
{
    if (m[node].op == unknownident)
	fprintf(f, "%5d %-*.*s %10d %10d %c", node, identlength,
	    identlength, (char *)TO_POINTER(m[node].val), 0, m[node].nxt,
	    m[node].marked == persistent ? 'P' : m[node].marked ? 'T' : 'F');
    else if (m[node].op == list_)
	fprintf(f, "%5d %-*.*s %10d %10d %c", node, identlength,
	    identlength, standardident_NAMES[m[node].op], (memrange)m[node].val,
	    m[node].nxt, m[node].marked == persistent ? 'P' : m[node].marked ?
	    'T' : 'F');
    else {
	fprintf(f, "%5d %-*.*s ", node, identlength, identlength,
		standardident_NAMES[m[node].op]);
	fprintf(f, format_int, m[node].val);
	fprintf(f, " %10d %c", m[node].nxt, m[node].marked == persistent ?
		'P' : m[node].marked ? 'T' : 'F');
    }
    if (m[node].op == lib_)
	fprintf(f, "   %-*.*s %4d", identlength, identlength,
		table[m[node].val].alf, table[m[node].val].adr);
    putc('\n', f);
}

static void writenode(node)
memrange node;
{
    wn(stdout, node);
    if (writelisting) {
	putc('\t', listing);
	wn(listing, node);
    }
}  /* writenode */

static void mark(node)
memrange node;
{
    while (node) {
	if (writelisting > 3)
	    writenode(node);
	if (m[node].op == list_ && !m[node].marked)
	    mark((memrange)m[node].val);
	if (!m[node].marked)
	    m[node].marked = 1;	/* set marked */
	node = m[node].nxt;
    }
}  /* mark */

static memrange kons(op, val, node)
standardident op;
value_t val;
memrange node;
{
    memrange j;
    value_t collected;

    if (!freelist && sentinel) {
#ifdef DEBUG_G
	printf("gc, last_op_executed: %-*.*s\n", identlength,
		identlength, standardident_NAMES[last_op_executed]);
#endif
	if (writelisting > 1)
	    writeident("GC start\n");
	mark(programme);
	mark(s);
	mark(dump);
	/* mark parameters */
	mark(node);
	if (op == list_)
	    mark((memrange)val);
	/* mark variables */
	for (j = 1; j < lasttable; j++)
	    if (table[j].alf[0] == '_')
		mark(table[j].adr);
	if (writelisting > 2)
	    writeident("finished marking\n");
	/*
	 * Scan memory and move all unused nodes to the freelist.
	 */
	collected = 0;
	for (j = firstusernode; j < MAXMEM; j++) {
	    if (!m[j].marked) {
		m[j].nxt = freelist;
		freelist = j;
		collected++;
	    } else if (m[j].marked == 1)
		m[j].marked = 0;	/* set unmarked */
	    if (m[j].nxt == j)
		point('F', "internal error - selfreference");
	}
	if (writelisting > 1) {
	    writeinteger(collected);
	    writeident(" nodes collected\n");
	}
	stat_gc++;
    }
    if ((j = freelist) == 0)
	point('F', "dynamic memory exhausted");
    if (op == list_ && val == j)
	point('F', "internal error - selfreference");
    if (j == node)
	point('F', "internal error - circular");
    freelist = m[j].nxt;
    m[j].op  = (unsigned char)op;
    m[j].val = val;
    m[j].nxt = node;
    if (writelisting > 3)
	writenode(j);
    stat_kons++;
    return j;
}  /* kons */

static void readfactor(where)
memrange *where;
{
    char *str;
    memrange here;

    switch (sym) {
    case lbrack:
	getsym();
	*where = kons(list_, (value_t)0, 0);
	m[*where].marked = sentinel ? 1 : persistent;
	if (sym == lbrack || sym == identifier  || sym == charconst ||
			     sym == numberconst || sym == stringconst) {
		/* sym == hyphen */
	    readterm(&here);
	    m[*where].val = here;
	}
	break;

    case identifier:
	lookup();
	if (id == unknownident)
	    *where = kons(id, TO_INTEGER(GC_strdup(ident)), 0);
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
    /*
     * When reading the library, strings can be marked as persistent.
     */
    case stringconst:
	str = GC_strdup(stringbuf);
#ifndef BDWGC
	str[-1] = sentinel ? 1 : persistent;
#endif
	*where = kons(string_, TO_INTEGER(str), 0);
	break;

    case semic:
    case period:
	*where = 0;
	return;

    default:
	point('F', "internal in readfactor");
    }  /* CASE */
    m[*where].marked = sentinel ? 1 : persistent;
}  /* readfactor */

static void readterm(first)
memrange *first;
{   /* readterm */
    /* was forward */
    memrange j;

    /* this is LL0 */
    readfactor(first);
    if ((j = *first) == 0)
	return;
    getsym();
    while (sym == lbrack || sym == identifier  || sym == charconst ||
			    sym == numberconst || sym == stringconst) {
	    /* sym == hyphen */
	readfactor(&m[j].nxt);
	j = m[j].nxt;
	getsym();
    }
}  /* readterm */

static void writeterm(node, nl)
memrange node;
boolean nl;
{
    while (node) {
	writefactor(node, false);
	if (m[node].nxt)
	    putch(' ');
	node = m[node].nxt;
    }
    if (nl)
	writeline();
}  /* writeterm */

static void writefactor(node, nl)
memrange node;
boolean nl;
{   /* was forward */
    char *str, *ptr;

    if (node) {
	switch (m[node].op) {
	case list_:
	    putch('[');
	    writeterm((memrange)m[node].val, false);
	    putch(']');
	    break;

	case boolean_:
	    writeident(m[node].val == 1 ? "true" : "false");
	    break;

	case char_:
	    if (!isspace((int)m[node].val))
		putch('\'');
	    putch((int)m[node].val);
	    break;

	case integer_:
	    writeinteger(m[node].val);
	    break;

	case string_:
	    str = TO_POINTER(m[node].val);
	    ptr = GC_malloc_atomic(strlen(str) + 3);
	    sprintf(ptr, "\"%s\"", str);
	    writeident(ptr);
	    GC_free(ptr);
	    break;

	case file_:
	    writeident("FILE:");
	    writeinteger(m[node].val);
	    break;

	case lib_:
	    writeident(table[m[node].val].alf);
	    break;

	case funct_:
	    writeident("FUNCTION");
	    break;

	case unknownident:
	    writeident(TO_POINTER(m[node].val));
	    break;

	default:
	    writeident(stdidents[m[node].val].alf);
	    break;
	}  /* CASE */
    }
    if (nl)
	writeline();
}  /* writefactor */

static void patchterm(node)
memrange node;
{
    while (node) {
	patchfactor(node);
	node = m[node].nxt;
    }
}  /* patchterm */

static void patchfactor(node)
memrange node;
{   /* was forward */
    if (node) {
	switch (m[node].op) {
	case list_:
	    patchterm((memrange)m[node].val);
	    break;

	case unknownident:
	    strncpy(ident, TO_POINTER(m[node].val), identlength);
	    ident[identlength] = 0;
	    GC_free(TO_POINTER(m[node].val));
	    lookup();
	    m[node].op = (unsigned char)id;
	    m[node].val = locatn;
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
     *
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
	    lib = GC_malloc_atomic(loc);
	    sprintf(lib, "%s%s", pathname, str);
	    if ((fp = fopen(lib, "r")) != 0) {
		str = lib;
		goto done;
	    }
	    GC_free(lib);
	}
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
     * pathname can also be used in subsequent opens of newfile.
     */
    fclose(fp);
    newfile(str, 1);
    if (lib)
	GC_free(lib);
    getsym();
    while (sym != period) {
	if (writelisting > 5)
	    fprintf(listing, "seen : %-*.*s\n", identlength, identlength,
		    ident);
	if (lastlibloc)
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
	if (writelisting > 5)
	    writeterm(table[loc].adr, true);
	if (sym != period)
	    getsym();
    }
    firstusernode = freelist;
    if (writelisting > 4)
	fprintf(listing, "firstusernode = %d, total memory = %d\n",
		firstusernode, MAXMEM - 1);
    cc = ll;
    sentinel = lastlibloc + 1;
    lasttable = sentinel;
    adjustment = -1;
    for (loc = 1; loc < lasttable; loc++)
	patchterm(table[loc].adr);
}  /* readlibrary */

static jmp_buf JL10;

#ifdef NCHECK
#define o(x)	m[x].op
#define b(x)	(m[x].val > 0)
#define i(x)	m[x].val
#define l(x)	((memrange)m[x].val)
#define n(x)	m[x].nxt
#define v(x)	m[x].val
#else
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

static boolean b(x)
memrange x;
{
    return (boolean)(v(x) > 0);
}  /* b */

static value_t i(x)
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
#endif

/*
 * Function called from SetRaw.
 * Remembers the screen dimensions.
 */
void do_push_int(val)
int val;
{
    s = kons(integer_, (value_t)val, s);
}

static void binary(op, val)
standardident op;
value_t val;
{
    s = kons(op, val, n(n(s)));
}

static boolean condition(prog)
memrange prog;
{
    memrange save;
    boolean result;

    save = s;
    dump = kons(list_, (value_t)save, dump);
    joy(prog);
    result = b(s);
    s = save;
    dump = n(dump);
    return result;
}

/**
true  :  ->  true
Pushes the value true.
*/
static void do_true()
{
    s = kons(boolean_, (value_t)1, s);
}

/**
false  :  ->  false
Pushes the value false.
*/
static void do_false()
{
    s = kons(boolean_, (value_t)0, s);
}

/**
pop  :  X  ->
Removes X from top of the stack.
*/
static void do_pop()
{
    s = n(s);
}

/**
dup  :  X  ->  X X
Pushes an extra copy of X onto stack.
*/
static void do_dup()
{
    s = kons(o(s), v(s), s);
}

/**
swap  :  X Y  ->  Y X
Interchanges X and Y on top of the stack.
*/
static void do_swap()
{
    memrange temp;

    temp = kons(o(s), v(s), n(n(s)));
    s = kons(o(n(s)), v(n(s)), temp);
}

/**
stack  :  .. X Y Z  ->  .. X Y Z [Z Y X ..]
Pushes the stack as a list.
*/
static void do_stack()
{
    s = kons(list_, (value_t)s, s);
}

/**
unstack  :  [X Y ..]  ->  ..Y X
The list [X Y ..] becomes the new stack.
*/
static void do_unstack()
{
    s = l(s);
}

/**
not  :  X  ->  Y
Y is the complement of set X, logical negation for truth values.
*/
static void do_not()
{
    s = kons(boolean_, (value_t)!b(s), n(s));
}

/**
*  :  I J  ->  K
Integer K is the product of integers I and J.  Also supports float.
*/
static void do_mul()
{
    binary(integer_, i(n(s)) * i(s));
}

/**
+  :  M I  ->  N
Numeric N is the result of adding integer I to numeric M.
Also supports float.
*/
static void do_add()
{
    binary(integer_, v(n(s)) + i(s));
}

/**
-  :  M I  ->  N
Numeric N is the result of subtracting integer I from numeric M.
Also supports float.
*/
static void do_sub()
{
    binary(integer_, v(n(s)) - i(s));
}

/**
div  :  I J  ->  K L
Integers K and L are the quotient and remainder of dividing I by J.
*/
static void do_div()
{
    binary(integer_, i(n(s)) / i(s));
}

/**
and  :  X Y  ->  Z
Z is the intersection of sets X and Y, logical conjunction for truth values.
*/
static void do_and()
{
    binary(boolean_, (value_t)(b(n(s)) & b(s)));
}

/**
or  :  X Y  ->  Z
Z is the union of sets X and Y, logical disjunction for truth values.
*/
static void do_or()
{
    binary(boolean_, (value_t)(b(n(s)) | b(s)));
}

static int Compare(one, two)
memrange one;
memrange two;
{
    int result;
    value_t num1, num2;
    char *str1 = 0, *str2 = 0;

    num2 = v(two);
    switch (o(two)) {
    case lib_:
	str2 = table[num2].alf;
	break;
    case string_:
	str2 = TO_POINTER(v(two));
	break;
    }
    num1 = v(one);
    switch (o(one)) {
    case lib_:
	str1 = table[num1].alf;
	break;
    case string_:
	str1 = TO_POINTER(v(one));
	break;
    }
    if (str1 && str2) {
	result = strcmp(str1, str2);
	result = result < 0 ? -1 : result > 0;
    } else if (num1 < num2)
	result = -1;
    else
	result = num1 > num2;
    return result;
}

/**
<  :  X Y  ->  B
Either both X and Y are numeric or both are strings or symbols.
Tests whether X less than Y.  Also supports float.
*/
static void do_lss()
{
    int result;

    result = Compare(n(s), s) < 0;
    s = kons(boolean_, (value_t)result, n(n(s)));
}

/**
=  :  X Y  ->  B
Either both X and Y are numeric or both are strings or symbols.
Tests whether X equal to Y.  Also supports float.
*/
static void do_eql()
{
    int result;

    result = Compare(n(s), s) == 0;
    s = kons(boolean_, (value_t)result, n(n(s)));
}

/**
sametype  :  X Y  ->  B
Tests whether X and Y have the same type.
*/
static void do_sametype()
{
    binary(boolean_, (value_t)(o(n(s)) == o(s)));
}

static void split(num, first, second)
int num;
int *first;
int *second;
{
    *first = num / 64;
    *second = num - num / 64 * 64;
}

static void expand(str, val)
char *str;
value_t val;
{
    int num, tmp[6];

    if ((num = val) < 128)
	sprintf(str, "%c", num);
    else if (num < 2048) {
	split(num, &tmp[0], &tmp[1]);
	sprintf(str, "%c%c", tmp[0] + 192, tmp[1] + 128);
    } else if (num < 65536) {
	split(num, &tmp[0], &tmp[1]);
	split(tmp[0], &tmp[2], &tmp[3]);
	sprintf(str, "%c%c%c", tmp[2] + 224, tmp[3] + 128, tmp[1] + 128);
    } else {
	split(num, &tmp[0], &tmp[1]);
	split(tmp[0], &tmp[2], &tmp[3]);
	split(tmp[2], &tmp[4], &tmp[5]);
	sprintf(str, "%c%c%c%c", tmp[4] + 240, tmp[5] + 128, tmp[3] + 128,
		tmp[1] + 128);
    }
}

/**
cons  :  X A  ->  B
Aggregate B is A with a new member X (first member for sequences).
*/
static void do_cons()
{
    memrange temp;
    char *str, tmp[10], *ptr;

    if (o(s) == string_) {
	str = TO_POINTER(v(s));
	expand(tmp, v(n(s)));
	ptr = GC_malloc_atomic(strlen(tmp) + strlen(str) + 1);
	sprintf(ptr, "%s%s", tmp, str);
	s = kons(string_, TO_INTEGER(ptr), n(n(s)));
    } else {
	temp = kons(o(n(s)), v(n(s)), l(s));
	s = kons(list_, (value_t)temp, n(n(s)));
    }
}

static int unpack(str, count)
unsigned char *str;
int *count;
{
    int num, tmp[4];

    if (str[0] >= 0xF0) {
	tmp[0] = str[0] & 0x7;	/* strip 5 bits */
	tmp[1] = str[1] & 0x3F;
	tmp[2] = str[2] & 0x3F;
	tmp[3] = str[3] & 0x3F;
	num = (tmp[0] << 18) | (tmp[1] << 12) | (tmp[2] << 6) | tmp[3];
	*count = 4;
    } else if (str[0] >= 0xE0) {
	tmp[0] = str[0] & 0xF;	/* strip 4 bits */
	tmp[1] = str[1] & 0x3F;
	tmp[2] = str[2] & 0x3F;
	num = (tmp[0] << 12) | (tmp[1] << 6) | tmp[2];
	*count = 3;
    } else if (str[0] >= 0xC0) {
	tmp[0] = str[0] & 0x1F;	/* strip 3 bits */
	tmp[1] = str[1] & 0x3F;
	num = (tmp[0] << 6) | tmp[1];
	*count = 2;
    } else {
	num = str[0];
	*count = 1;
    }
    return num;
}

/**
uncons  :  A  ->  F R
F and R are the first and the rest of non-empty aggregate A.
*/
static void do_uncons()
{
    char *str;
    memrange temp;
    int num, count;

    if (o(s) == string_) {
	str = TO_POINTER(v(s));
	num = unpack((unsigned char *)str, &count);
	temp = kons(count > 1 ? integer_ : char_, (value_t)num, n(s));
	s = kons(string_, TO_INTEGER(GC_strdup(str + count)), temp);
    } else {
	temp = kons(o(l(s)), v(l(s)), n(s));
	s = kons(list_, (value_t)n(l(s)), temp);
    }
}

/**
opcase  :  X [..[X Xs]..]  ->  X [Xs]
Indexing on type of X, returns the list [Xs].
*/
static void do_opcase()
{
    memrange aggr;

    aggr = l(s);
    while (o(l(aggr)) != o(n(s)))
	aggr = n(aggr);
    s = kons(list_, (value_t)n(l(aggr)), n(s));
}

/**
of  :  I A  ->  X
X (= A[I]) is the I-th member of aggregate A.
*/
static void do_of()
{
    char *str;
    value_t val;
    memrange aggr;
    int num, count;

    val = v(n(s));
    if (o(s) == string_) {
	for (str = TO_POINTER(v(s)); val > 0; val--, str += count)
	    num = unpack((unsigned char *)str, &count);
	num = unpack((unsigned char *)str, &count);
	s = kons(count > 1 ? integer_ : char_, (value_t)num, n(n(s)));
    } else {
	for (aggr = l(s); val > 0; val--)
	    aggr = n(aggr);
	s = kons(o(aggr), v(aggr), n(n(s)));
    }
}

/**
body  :  U  ->  [P]
Quotation [P] is the body of user-defined symbol U.
*/
static void do_body()
{
    s = kons(list_, (value_t)table[v(s)].adr, n(s));
}

/**
put  :  X  ->
Writes X to output, pops X off stack.
*/
static void do_put()
{
    writefactor(s, false);
    s = n(s);
}

/**
putch  :  N  ->
N : numeric, writes character whose ASCII is N.
*/
static void do_putch()
{
    char str[10];

    expand(str, v(s));
    printf("%s", str);
    s = n(s);
}

/**
get  :  ->  F
Reads a factor from input and pushes it onto stack.
*/
static void do_get()
{
    memrange temp;

    getsym();
    readfactor(&temp);
    s = kons(o(temp), v(temp), s);
}

/**
getch  :  ->  N
Reads a character from input and puts it onto stack.
*/
static void do_getch()
{
    getch();
    s = kons(integer_, (value_t)chr, s);
}

/**
i  :  [P]  ->  ...
Executes P. So, [P] i  ==  P.
*/
static void do_i()
{
    memrange prog;

    prog = l(s);
    dump = kons(list_, (value_t)prog, dump);
    s = n(s);
    joy(prog);
    dump = n(dump);
}

/**
dip  :  X [P]  ->  ...  X
Saves X, executes P, pushes X back.
*/
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

/**
step  :  A [P]  ->  ...
Sequentially putting members of aggregate A onto stack,
executes P for each member of A.
*/
static void do_step()
{
    char *str;
    int num, count;
    memrange prog, aggr;

    prog = l(s);
    dump = kons(list_, (value_t)prog, dump);
    s = n(s);
    if (o(s) == string_) {
	str = TO_POINTER(v(s));
	for (s = n(s); *str; str += count) {
	    num = unpack((unsigned char *)str, &count);
	    s = kons(count > 1 ? integer_ : char_, (value_t)num, s);
	    joy(prog);
	}
    } else {
	aggr = l(s);
	dump = kons(list_, (value_t)aggr, dump);
	for (s = n(s); aggr; aggr = n(aggr)) {
	    s = kons(o(aggr), v(aggr), s);
	    joy(prog);
	}
	dump = n(dump);
    }
    dump = n(dump);
}

/**
argv  :  ->  A
Creates an aggregate A containing the interpreter's command line arguments.
*/
static void do_argv()
{
    int j;
    memrange *dump1;

    dump = kons(list_, (value_t)dump, 0);
    dump1 = &m[dump].nxt;
    for (j = 0; j < g_argc; j++) {
	*dump1 = kons(string_, TO_INTEGER(g_argv[j]), 0);
	dump1 = &m[*dump1].nxt;
    }
    s = kons(list_, (value_t)n(dump), s);
    dump = l(dump);
}

/**
strtol  :  S I  ->  J
String S is converted to the integer J using base I.
If I = 0, assumes base 10,
but leading "0" means base 8 and leading "0x" means base 16.
*/
static void do_strtol()
{
#ifdef ATARI
    binary(integer_, (value_t)strtol(TO_POINTER(v(n(s))), 0, v(s)));
#else
    binary(integer_, (value_t)strtoll(TO_POINTER(v(n(s))), 0, v(s)));
#endif
}

/**
while  :  [B] [D]  ->  ...
While executing B yields true executes D.
*/
static void do_while()
{
    memrange prog[2];

    prog[1] = l(s);
    dump = kons(list_, (value_t)prog[1], dump);
    s = n(s);
    prog[0] = l(s);
    dump = kons(list_, (value_t)prog[0], dump);
    s = n(s);
    while (condition(prog[0]))
	joy(prog[1]);
    dump = n(n(dump));
}

/**
unary2  :  X1 X2 [P]  ->  R1 R2
Executes P twice, with X1 and X2 on top of the stack.
Returns the two values R1 and R2.
*/
static void do_unary2()
{
    /*  Y Z [P]  unary2  ==>  Y' Z'  */
    memrange prog, parm, save, outp[2];

    prog = l(s);
    dump = kons(list_, (value_t)prog, dump);	/* save prog */
    parm = s = n(s);
    dump = kons(list_, (value_t)parm, dump);	/* save Z */
    s = n(s);					/* just Y on top */
    save = n(s);
    dump = kons(list_, (value_t)save, dump);	/* save stack */
    joy(prog);					/* execute P */
    outp[0] = s;
    dump = kons(list_, (value_t)outp[0], dump);	/* save P(Y) */
    s = kons(o(parm), v(parm), save);		/* just Z on top */
    joy(prog);					/* execute P */
    outp[1] = s;
    dump = kons(list_, (value_t)outp[1], dump);	/* save P(Z) */
    s = kons(o(outp[0]), v(outp[0]), save);	/* Y' */
    s = kons(o(outp[1]), v(outp[1]), s);	/* Z' */
    dump = n(n(n(n(n(dump)))));
}

/**
cleave  :  X [P1] [P2]  ->  R1 R2
Executes P1 and P2, each with X on top, producing two results.
*/
static void do_cleave()
{
    /*  X [P1] [P2]  cleave  ==>  X1 X2  */
    memrange prog[2], save, outp[2];

    prog[1] = l(s);
    dump = kons(list_, (value_t)prog[1], dump);
    s = n(s);
    prog[0] = l(s);
    dump = kons(list_, (value_t)prog[0], dump);
    save = s = n(s);
    dump = kons(list_, (value_t)save, dump);	/* save stack */
    joy(prog[0]);				/* [P1] */
    outp[0] = s;
    dump = kons(list_, (value_t)outp[0], dump);	/* save X1 */
    s = save;					/* restore stack */
    joy(prog[1]);				/* [P2] */
    outp[1] = s;
    dump = kons(list_, (value_t)outp[1], dump);	/* save X2 */
    s = kons(o(outp[0]), v(outp[0]), n(save));	/* X1 */
    s = kons(o(outp[1]), v(outp[1]), s);	/* X2 */
    dump = n(n(n(n(n(dump)))));
}

/**
stdin  :  ->  S
Pushes the standard input stream.
*/
static void do_stdin()
{
    s = kons(file_, TO_INTEGER(stdin), s);
}

/**
fopen  :  P M  ->  S
The file system object with pathname P is opened with mode M (r, w, a, etc.)
and stream object S is pushed; if the open fails, file:NULL is pushed.
*/
static void do_fopen()
{
    FILE *fp;
    char *path, *mode;

    mode = TO_POINTER(v(s));
    s = n(s);
    path = TO_POINTER(v(s));
    fp = fopen(path, mode);
    s = kons(file_, TO_INTEGER(fp), n(s));
}

/**
fgetch  :  S  ->  S C
C is the next available character from stream S.
*/
static void do_fgetch()
{
    FILE *fp;
    int num, count;
    unsigned char str[10];

    fp = TO_POINTER(v(s));
    str[0] = getc(fp);
    if (str[0] >= 0xF0) {
	str[1] = getc(fp);
	str[2] = getc(fp);
	str[3] = getc(fp);
    } else if (str[0] >= 0xE0) {
	str[1] = getc(fp);
	str[2] = getc(fp);
    } else if (str[0] >= 0xC0)
	str[1] = getc(fp);
    num = unpack(str, &count);
    s = kons(count > 1 ? integer_ : char_, (value_t)num, s);
}

/**
feof  :  S  ->  S B
B is the end-of-file status of stream S.
*/
static void do_feof()
{
    FILE *fp;

    fp = TO_POINTER(v(s));
    s = kons(boolean_, (value_t)feof(fp), s);
}

static void binrecaux(prog)
memrange prog[];
{
    if (condition(prog[0]))
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

/**
binrec  :  [P] [T] [R1] [R2]  ->  ...
Executes P. If that yields true, executes T.
Else uses R1 to produce two intermediates, recurses on both,
then executes R2 to combine their results.
*/
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

/**
primrec  :  X [I] [C]  ->  R
Executes I to obtain an initial value R0.
For integer X uses increasing positive integers to X, combines by C for new R.
For aggregate X uses successive members and combines by C for new R.
*/
static void do_primrec()
{
    char *str;
    int num, count, j, k = 0;
    memrange prog[2], data, cur;

    prog[1] = l(s);
    dump = kons(list_, (value_t)prog[1], dump);
    s = n(s);
    prog[0] = l(s);
    dump = kons(list_, (value_t)prog[0], dump);
    data = s = n(s);
    s = n(s);
    switch (o(data)) {
    case string_:
	for (str = TO_POINTER(v(data)); *str; str += count) {
	    num = unpack((unsigned char *)str, &count);
	    s = kons(count > 1 ? integer_ : char_, (value_t)num, s);
	    k++;
	}
	break;
    case list_:
	dump = kons(list_, l(data), dump);
	for (cur = l(data); cur; cur = n(cur)) {
	    s = kons(o(cur), v(cur), s);
	    k++;
	}
	dump = n(dump);
	break;
    case integer_:
	for (j = v(data); j > 0; j--) {
	    s = kons(integer_, (value_t)j, s);
	    k++;
	}
	break;
    }
    joy(prog[0]);
    dump = n(dump);
    for (j = 0; j < k; j++)
	joy(prog[1]);
    dump = n(dump);
}

/**
ifte  :  [B] [T] [F]  ->  ...
Executes B. If that yields true, then executes T else executes F.
*/
static void do_ifte()
{
    boolean result;
    memrange prog[3];

    prog[2] = l(s);
    dump = kons(list_, (value_t)prog[2], dump);
    s = n(s);
    prog[1] = l(s);
    dump = kons(list_, (value_t)prog[1], dump);
    s = n(s);
    prog[0] = l(s);
    dump = kons(list_, (value_t)prog[0], dump);
    s = n(s);
    result = condition(prog[0]);
    dump = n(dump);
    joy(result ? prog[1] : prog[2]);
    dump = n(n(dump));
}

static void linrecaux(prog)
memrange prog[];
{
    if (condition(prog[0]))
	joy(prog[1]);
    else {
	joy(prog[2]);
	linrecaux(prog);
	joy(prog[3]);
    }
}

/**
linrec  :  [P] [T] [R1] [R2]  ->  ...
Executes P. If that yields true, executes T.
Else executes R1, recurses, executes R2.
*/
static void do_linrec()
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
    linrecaux(prog);
    dump = n(n(n(n(dump))));
}

/**
putchars  :  "abc.."  ->
Writes abc.. (without quotes)
*/
static void do_putchars()
{
    char *str;

    str = TO_POINTER(v(s));
    printf("%s", str);
    s = n(s);
}

/**
split  :  A [B]  ->  A1 A2
Uses test B to split aggregate A into sametype aggregates A1 and A2.
*/
static void do_split()
{
    char *str, *yes_str, *no_str;
    int num, count, yes_ptr = 0, no_ptr = 0;
    memrange prog, save, aggr, *dump1, *dump2;

    prog = l(s);
    dump = kons(list_, (value_t)prog, dump);
    s = n(s);
    save = n(s);
    dump = kons(list_, (value_t)save, dump);
    if (o(s) == string_) {
	str = TO_POINTER(v(s));
	no_str = GC_strdup(str);			/* false */
	yes_str = GC_strdup(str);			/* true */
	for (; *str; str += count) {
	    num = unpack((unsigned char *)str, &count);
	    s = kons(count > 1 ? integer_ : char_, (value_t)num, save);
	    joy(prog);
	    if (b(s)) {
		strncpy(&yes_str[yes_ptr], str, count);
		yes_ptr += count;
	    } else {
		strncpy(&no_str[no_ptr], str, count);
		no_ptr += count;
	    }
	}
	yes_str[yes_ptr] = 0;
	no_str[no_ptr] = 0;
	s = kons(string_, TO_INTEGER(yes_str), save);	/* true */
	s = kons(string_, TO_INTEGER(no_str), s);	/* false */
    } else {
	aggr = l(s);
	dump = kons(list_, (value_t)aggr, dump);
	dump = kons(list_, (value_t)dump, 0);		/* false */
	dump2 = &m[dump].nxt;
	dump = kons(list_, (value_t)dump, 0);		/* true */
	dump1 = &m[dump].nxt;
	for (; aggr; aggr = n(aggr)) {
	    s = kons(o(aggr), v(aggr), save);
	    joy(prog);
	    if (b(s)) {
		*dump1 = kons(o(aggr), v(aggr), 0);
		dump1 = &m[*dump1].nxt;
	    } else {
		*dump2 = kons(o(aggr), v(aggr), 0);
		dump2 = &m[*dump2].nxt;
	    }
	}
	s = kons(list_, (value_t)n(dump), save);	/* true */
	dump = l(dump);
	s = kons(list_, (value_t)n(dump), s);		/* false */
	dump = l(dump);
	dump = n(dump);
    }
    dump = n(n(dump));
}

/**
fread  :  S I  ->  S L
I bytes are read from the current position of stream S
and returned as a list of I integers.
*/
static void do_fread()
{
    FILE *fp;
    char *buf;
    int j, count;
    memrange *dump1;

    count = i(s);			/* number of characters to read */
    s = n(s);
    fp = TO_POINTER(v(s));		/* file descriptor */
    s = n(s);
    buf = GC_malloc_atomic(count);	/* buffer for characters to read */
    count = fread(buf, 1, count, fp);	/* number of characters read */
    dump = kons(list_, (value_t)dump, 0);
    dump1 = &m[dump].nxt;
    for (j = 0; j < count; j++) {
	*dump1 = kons(integer_, (value_t)buf[j], 0);
	dump1 = &m[*dump1].nxt;
    }
    s = kons(list_, (value_t)n(dump), s);
    dump = l(dump);
    GC_free(buf);
}

/**
fseek  :  S P W  ->  S B
Stream S is repositioned to position P relative to whence-point W,
where W = 0, 1, 2 for beginning, current position, end respectively.
*/
static void do_fseek()
{
    FILE *fp;
    int whence;
    value_t pos;

    whence = i(s);
    s = n(s);
    pos = v(s);
    s = n(s);
    fp = TO_POINTER(v(s));
    pos = fseek(fp, pos, whence);
    s = kons(boolean_, (value_t)(pos != 0), s);
}

/**
fclose  :  S  ->
Stream S is closed and removed from the stack.
*/
static void do_fclose()
{
    FILE *fp;

    fp = TO_POINTER(v(s));
    fclose(fp);
    s = n(s);
}

/**
filter  :  A [B]  ->  A1
Uses test B to filter aggregate A producing sametype aggregate A1.
*/
static void do_filter()
{
    char *str, *yes_str;
    int num, count, yes_ptr = 0;
    memrange prog, save, aggr, *dump1;

    prog = l(s);
    dump = kons(list_, (value_t)prog, dump);
    s = n(s);
    save = n(s);
    dump = kons(list_, (value_t)save, dump);
    if (o(s) == string_) {
	str = TO_POINTER(v(s));
	yes_str = GC_strdup(str);			/* true */
	for (; *str; str += count) {
	    num = unpack((unsigned char *)str, &count);
	    s = kons(count > 1 ? integer_ : char_, (value_t)num, save);
	    joy(prog);
	    if (b(s)) {
		strncpy(&yes_str[yes_ptr], str, count);
		yes_ptr += count;
	    }
	}
	yes_str[yes_ptr] = 0;
	s = kons(string_, TO_INTEGER(yes_str), save);	/* true */
    } else {
	aggr = l(s);
	dump = kons(list_, (value_t)aggr, dump);
	dump = kons(list_, (value_t)dump, 0);		/* true */
	dump1 = &m[dump].nxt;
	for (; aggr; aggr = n(aggr)) {
	    s = kons(o(aggr), v(aggr), save);
	    joy(prog);
	    if (b(s)) {
		*dump1 = kons(o(aggr), v(aggr), 0);
		dump1 = &m[*dump1].nxt;
	    }
	}
	s = kons(list_, (value_t)n(dump), save);
	dump = l(dump);
	dump = n(dump);
    }
    dump = n(n(dump));
}

/**
fremove  :  P  ->  B
The file system object with pathname P is removed from the file
system. B is a boolean indicating success or failure.
*/
static void do_fremove()
{
    char *str;
    int result;

    str = TO_POINTER(v(s));
#ifdef ATARI
    result = unlink(str);
#else
    result = remove(str);
#endif
    s = kons(boolean_, (value_t)!result, n(s));
}

/**
frename  :  P1 P2  ->  B
The file system object with pathname P1 is renamed to P2.
B is a boolean indicating success or failure.
*/
static void do_frename()
{
    int result;
    char *p1, *p2;

    p2 = TO_POINTER(v(s));
    s = n(s);
    p1 = TO_POINTER(v(s));
    result = rename(p1, p2);
    s = kons(boolean_, (value_t)!result, n(s));
}

/**
drop  :  A N  ->  B
Aggregate B is the result of deleting the first N elements of A.
*/
static void do_drop()
{
    memrange aggr;
    int j, k, count;
    unsigned char *str;

    count = i(s);
    s = n(s);
    if (o(s) == string_) {
	str = TO_POINTER(v(s));
	for (k = j = 0; j < count && str[k]; j++, k++) {
	    if (str[k] >= 0xF0)
		k += 3;
	    else if (str[k] >= 0xE0)
		k += 2;
	    else if (str[k] >= 0xC0)
		k++;
	}
	str = (unsigned char *)GC_strdup((char *)&str[k]);
	s = kons(string_, TO_INTEGER(str), n(s));
    } else {
	aggr = l(s);
	while (count-- > 0 && aggr)
	    aggr = n(aggr);
	s = kons(list_, (value_t)aggr, n(s));
    }
}

/**
take  :  A N  ->  B
Aggregate B is the result of retaining just the first N elements of A.
*/
static void do_take()
{
    int j, k, count;
    unsigned char *str;
    memrange aggr, *dump1;

    count = i(s);
    s = n(s);
    if (o(s) == string_) {
	str = TO_POINTER(v(s));
	for (k = j = 0; j < count && str[k]; j++, k++) {
	    if (str[k] >= 0xF0)
		k += 3;
	    else if (str[k] >= 0xE0)
		k += 2;
	    else if (str[k] >= 0xC0)
		k++;
	}
	if (str[k]) {
	    str = (unsigned char *)GC_strdup((char *)str);
	    str[k] = 0;
	    s = kons(string_, TO_INTEGER(str), n(s));
	}
    } else {
	dump = kons(list_, (value_t)dump, 0);
	dump1 = &m[dump].nxt;
	for (aggr = l(s); aggr && count; aggr = n(aggr), count--) {
	    *dump1 = kons(o(aggr), v(aggr), 0);
	    dump1 = &m[*dump1].nxt;
	}
	s = kons(list_, (value_t)n(dump), n(s));
	dump = l(dump);
    }
}

/**
maxint  :  ->  maxint
Pushes largest integer (platform dependent). Typically it is 32 bits.
*/
static void do_maxint()
{
    s = kons(integer_, (value_t)MAX_LONG, s);
}

/**
div  :  I J  ->  K L
Integers K and L are the quotient and remainder of dividing I by J.
*/
static void do_divmod()
{
    value_t parm[2], quot, remn;

    parm[1] = v(s);
    s = n(s);
    parm[0] = v(s);
    quot = parm[0] / parm[1];
    remn = parm[0] % parm[1];
    s = kons(integer_, quot, n(s));
    s = kons(integer_, remn, s);
}

/**
xor  :  X Y  ->  Z
Z is the symmetric difference of sets X and Y,
logical exclusive disjunction for truth values.
*/
static void do_xor()
{
    binary(boolean_, (value_t)(b(n(s)) ^ b(s)));
}

/**
name  :  sym  ->  "sym"
For operators and combinators, the string "sym" is the name of item sym,
for literals sym the result string is its type.
*/
static void do_name()
{
    char *str;

    str = o(s) == lib_ ? table[v(s)].alf : standardident_NAMES[o(s)];
    s = kons(string_, TO_INTEGER(str), n(s));
}

/**
time  :  ->  I
Pushes the current time (in seconds since the Epoch).
*/
static void do_time()
{
    time_t t;

    time(&t);
    s = kons(integer_, (value_t)t, s);
}

/**
stdout  :  ->  S
Pushes the standard output stream.
*/
static void do_stdout()
{
    s = kons(file_, TO_INTEGER(stdout), s);
}

/**
stderr  :  ->  S
Pushes the standard error stream.
*/
static void do_stderr()
{
    s = kons(file_, TO_INTEGER(stderr), s);
}

/**
quit  :  ->
Exit from Joy.
*/
static void do_quit()
{
    my_exit(fatal_err);	/* not a fatal error, but should also end the program */
}

/**
intern  :  "sym"  ->  sym
Pushes the item whose name is "sym".
*/
static void do_intern()
{
    strncpy(ident, TO_POINTER(v(s)), identlength);
    ident[identlength] = 0;
    lookup();
    s = kons((unsigned char)id, (value_t)locatn, n(s));
}

/**
unary3  :  X1 X2 X3 [P]  ->  R1 R2 R3
Executes P three times, with Xi, returns Ri (i = 1..3).
*/
static void do_unary3()
{
    /*  X Y Z [P]  unary3  ==>  X' Y' Z'  */
    memrange prog, parm[2], save, outp[3];

    prog = l(s);
    dump = kons(list_, (value_t)prog, dump);	/* save prog */
    parm[1] = s = n(s);
    dump = kons(list_, (value_t)parm[1], dump);	/* save Z */
    parm[0] = s = n(s);
    dump = kons(list_, (value_t)parm[0], dump);	/* save Y */
    s = n(s);					/* just X on top */
    save = n(s);
    dump = kons(list_, (value_t)save, dump);	/* save stack */
    joy(prog);					/* execute P */
    outp[0] = s;
    dump = kons(list_, (value_t)outp[0], dump);	/* save P(X) */
    s = kons(o(parm[0]), v(parm[0]), save);	/* just Y on top */
    joy(prog);					/* execute P */
    outp[1] = s;
    dump = kons(list_, (value_t)outp[1], dump);	/* save P(Y) */
    s = kons(o(parm[1]), v(parm[1]), save);	/* just Z on top */
    joy(prog);					/* execute P */
    outp[2] = s;
    dump = kons(list_, (value_t)outp[2], dump);	/* save P(Z) */
    s = kons(o(outp[0]), v(outp[0]), save);	/* X' */
    s = kons(o(outp[1]), v(outp[1]), s);	/* Y' */
    s = kons(o(outp[2]), v(outp[2]), s);	/* Z' */
    dump = n(n(n(n(n(n(n(dump)))))));
}

/**
unary4  :  X1 X2 X3 X4 [P]  ->  R1 R2 R3 R4
Executes P four times, with Xi, returns Ri (i = 1..4).
*/
static void do_unary4()
{
    /*  X Y Z W [P]  unary4  ==>  X' Y' Z' W'  */
    memrange prog, parm[3], save, outp[4];

    prog = l(s);
    dump = kons(list_, (value_t)prog, dump);	/* save prog */
    parm[2] = s = n(s);
    dump = kons(list_, (value_t)parm[2], dump);	/* save W */
    parm[1] = s = n(s);
    dump = kons(list_, (value_t)parm[1], dump);	/* save Z */
    parm[0] = s = n(s);
    dump = kons(list_, (value_t)parm[0], dump);	/* save Y */
    s = n(s);					/* just X on top */
    save = n(s);
    dump = kons(list_, (value_t)save, dump);	/* save stack */
    joy(prog);					/* execute P */
    outp[0] = s;
    dump = kons(list_, (value_t)outp[0], dump);	/* save P(X) */
    s = kons(o(parm[0]), v(parm[0]), save);	/* just Y on top */
    joy(prog);					/* execute P */
    outp[1] = s;
    dump = kons(list_, (value_t)outp[1], dump);	/* save P(Y) */
    s = kons(o(parm[1]), v(parm[1]), save);	/* just Z on top */
    joy(prog);					/* execute P */
    outp[2] = s;
    dump = kons(list_, (value_t)outp[2], dump);	/* save P(Z) */
    s = kons(o(parm[2]), v(parm[2]), save);	/* just W on top */
    joy(prog);					/* execute P */
    outp[3] = s;
    dump = kons(list_, (value_t)outp[3], dump);	/* save P(W) */
    s = kons(o(outp[0]), v(outp[0]), save);	/* X' */
    s = kons(o(outp[1]), v(outp[1]), s);	/* Y' */
    s = kons(o(outp[2]), v(outp[2]), s);	/* Z' */
    s = kons(o(outp[3]), v(outp[3]), s);	/* W' */
    dump = n(n(n(n(n(n(n(n(n(dump)))))))));
}

/**
casting  :  X Y  ->  Z
Z takes the value from X and uses the value from Y as its type.
*/
static void do_casting()
{
    unsigned char op;

    switch (v(s)) {
    case 2:
	op = lib_;
	break;

    case 4:
	op = boolean_;
	break;

    case 5:
	op = char_;
	break;

    case 6:
	op = integer_;
	break;

    case 8:
	op = string_;
	break;

    case 9:
	op = list_;
	break;

    case 11:
	op = file_;
	break;

    default:
	op = unknownident;
	break;
    }
    s = n(s);
    s = kons(op, v(s), n(s));
}

/**
case  :  X [..[X Y]..]  ->  Y i
Indexing on the value of X, execute the matching Y.
*/
static void do_case()
{
    memrange aggr;

    aggr = l(s);
    s = n(s);
    while (n(aggr) && Compare(l(aggr), s))
	aggr = n(aggr);
    if (n(aggr) < 1)
	joy(l(aggr));
    else {
	s = n(s);
	joy(n(l(aggr)));
    }
}

/**
cond  :  [..[[Bi] Ti]..[D]]  ->  ...
Tries each Bi. If that yields true, then executes Ti and exits.
If no Bi yields true, executes default D.
*/
static void do_cond()
{
    memrange aggr, save;

    aggr = l(s);
    dump = kons(list_, (value_t)aggr, dump);
    save = n(s);
    dump = kons(list_, (value_t)save, dump);
    for (; n(aggr); aggr = n(aggr)) {
	s = save;
	joy(l(l(aggr)));
	if (b(s))
	    break;
    }
    s = save;
    if (n(aggr))
	joy(n(l(aggr)));
    else
	joy(l(aggr));
    dump = n(n(dump));
}

/**
some  :  A [B]  ->  X
Applies test B to members of aggregate A, X = true if some pass.
*/
static void do_some()
{
    char *str;
    int num, count;
    boolean result = 0;
    memrange prog, save, aggr;

    prog = l(s);
    dump = kons(list_, (value_t)prog, dump);
    s = n(s);
    save = n(s);
    dump = kons(list_, (value_t)save, dump);
    if (o(s) == string_) {
	for (str = TO_POINTER(v(s)); *str; str += count) {
	    num = unpack((unsigned char *)str, &count);
	    s = kons(count > 1 ? integer_ : char_, (value_t)num, save);
	    joy(prog);
	    result = b(s);
	    if (result)
		break;
	}
    } else {
	aggr = l(s);
	dump = kons(list_, (value_t)aggr, dump);
	for (; aggr; aggr = n(aggr)) {
	    s = kons(o(aggr), v(aggr), save);
	    joy(prog);
	    result = b(s);
	    if (result)
		break;
	}
	dump = n(dump);
    }
    s = kons(boolean_, (value_t)result, save);
    dump = n(n(dump));
}

/**
all  :  A [B]  ->  X
Applies test B to members of aggregate A, X = true if all pass.
*/
static void do_all()
{
    char *str;
    int num, count;
    boolean result = 1;
    memrange prog, save, aggr;

    prog = l(s);
    dump = kons(list_, (value_t)prog, dump);
    s = n(s);
    save = n(s);
    dump = kons(list_, (value_t)save, dump);
    if (o(s) == string_) {
	for (str = TO_POINTER(v(s)); *str; str += count) {
	    num = unpack((unsigned char *)str, &count);
	    s = kons(count > 1 ? integer_ : char_, (value_t)num, save);
	    joy(prog);
	    result = b(s);
	    if (!result)
		break;
	}
    } else {
	aggr = l(s);
	dump = kons(list_, (value_t)aggr, dump);
	for (; aggr; aggr = n(aggr)) {
	    s = kons(o(aggr), v(aggr), save);
	    joy(prog);
	    result = b(s);
	    if (!result)
		break;
	}
	dump = n(dump);
    }
    s = kons(boolean_, (value_t)result, save);
    dump = n(n(dump));
}

/**
getenv  :  "variable"  ->  "value"
Retrieves the value of the environment variable "variable".
*/
static void do_getenv()
{
    char *str;

    str = TO_POINTER(v(s));
    if ((str = getenv(str)) == 0)
	str = "";
    s = kons(string_, TO_INTEGER(GC_strdup(str)), n(s));
}

/**
compare  :  A B  ->  I
I (=-1,0,+1) is the comparison of aggregates A and B.
The values correspond to the predicates <=, =, >=.
*/
static void do_compare()
{
    int result;

    result = Compare(n(s), s);
    s = kons(boolean_, (value_t)result, n(n(s)));
}

static void condnestrecaux(aggr)
memrange aggr;
{
    memrange list, save;

    list = aggr;
    save = s;
    dump = kons(list_, (value_t)save, dump);
    for (; n(aggr); aggr = n(aggr)) {
	s = save;
	joy(l(l(aggr)));
	if (b(s))
	    break;
    }
    s = save;
    dump = n(dump);
    aggr = n(aggr) ? n(l(aggr)) : l(aggr);
    joy(l(aggr));
    for (aggr = n(aggr); aggr; aggr = n(aggr)) {
	condnestrecaux(list);
	joy(l(aggr));
    }
}

/**
condnestrec  :  [ [C1] [C2] .. [D] ]  ->  ...
A generalisation of condlinrec.
Each [Ci] is of the form [[B] [R1] [R2] .. [Rn]] and [D] is of the form
[[R1] [R2] .. [Rn]]. Tries each B, or if all fail, takes the default [D].
For the case taken, executes each [Ri] but recurses between any two
consecutive [Ri] (n > 3 would be exceptional.)
*/
static void do_condnestrec()
{
    memrange aggr;

    aggr = l(s);
    dump = kons(list_, (value_t)aggr, dump);
    s = n(s);
    condnestrecaux(aggr);
    dump = n(dump);
}

static void tailrecaux(prog)
memrange prog[];
{
tailrec:
    if (condition(prog[0]))
	joy(prog[1]);
    else {
	joy(prog[2]);
	goto tailrec;
    }
}

/**
Q3  OK  2720  tailrec  :  [P] [T] [R1]  ->  ...
Executes P. If that yields true, executes T.
Else executes R1, recurses.
*/
static void do_tailrec()
{
    memrange prog[3];

    prog[2] = l(s);
    dump = kons(list_, (value_t)prog[2], dump);
    s = n(s);
    prog[1] = l(s);
    dump = kons(list_, (value_t)prog[1], dump);
    s = n(s);
    prog[0] = l(s);
    dump = kons(list_, (value_t)prog[0], dump);
    s = n(s);
    tailrecaux(prog);
    dump = n(n(n(dump)));
}

/**
treegenrecaux  :  T [[O1] [O2] C]  ->  ...
T is a tree. If T is a leaf, executes O1.
Else executes O2 and then [[[O1] [O2] C] treegenrecaux] C.
*/
static void treegenrecaux()
{
    memrange save;

    save = s;
    dump = kons(list_, (value_t)save, dump);
    s = n(s);
    if (o(s) != list_)
	joy(l(l(save)));	/* [O1] */
    else {
	joy(l(n(l(save))));	/* [O2] */
	s = kons(o(save), v(save), s);
	save = kons(funct_, TO_INTEGER(treegenrecaux), 0);
	s = kons(list_, (value_t)save, s);
	do_cons();
	joy(n(n(l(l(s)))));	/* [C] */
    }
    dump = n(dump);
}

/**
treegenrec  :  T [O1] [O2] [C]  ->  ...
T is a tree. If T is a leaf, executes O1.
Else executes O2 and then [[[O1] [O2] C] treegenrecaux] C.
*/
static void do_treegenrec()
{
    do_cons();
    do_cons();
    treegenrecaux();
}

/**
treerecaux  :  T [[O] C]  ->  ...
T is a tree. If T is a leaf, executes O. Else executes [[[O] C] treerecaux] C.
*/
static void treerecaux()
{
    memrange save;

    if (o(n(s)) == list_) {
	save = kons(funct_, TO_INTEGER(treerecaux), 0);
	s = kons(list_, (value_t)save, s);	/* T [[[O] C] treerecaux] */
	do_cons();
	joy(n(l(l(s))));		/* C */
    } else {
	save = l(l(s));
	dump = kons(list_, (value_t)save, dump);
	s = n(s);
	joy(save);			/* O */
	dump = n(dump);
    }
}

/**
treerec  :  T [O] [C]  ->  ...
T is a tree. If T is a leaf, executes O. Else executes [[[O] C] treerecaux] C.
*/
static void do_treerec()
{
    do_cons();
    treerecaux();
}

static void treestepaux(aggr, prog)
memrange aggr;
memrange prog;
{
    if (o(aggr) == list_)
	for (aggr = l(aggr); aggr; aggr = n(aggr))
	    treestepaux(aggr, prog);
    else {
	s = kons(o(aggr), v(aggr), s);
	joy(prog);
    }
}

/**
treestep  :  T [P]  ->  ...
Recursively traverses leaves of tree T, executes P for each leaf.
*/
static void do_treestep()
{
    memrange prog, aggr;

    prog = l(s);
    dump = kons(list_, (value_t)prog, dump);
    s = n(s);
    aggr = s;
    dump = kons(list_, (value_t)aggr, dump);
    s = n(s);
    treestepaux(aggr, prog);
    dump = n(n(dump));
}

/**
map  :  A [P]  ->  B
Executes P on each member of aggregate A,
collects results in sametype aggregate B.
*/
static void do_map()
{
    int num, count, yes_ptr = 0;
    char *str, *yes_str, tmp[10];
    memrange prog, save, aggr, *dump1;

    prog = l(s);
    dump = kons(list_, (value_t)prog, dump);
    s = n(s);
    save = n(s);
    dump = kons(list_, (value_t)save, dump);
    if (o(s) == string_) {
	str = TO_POINTER(v(s));
	yes_str = GC_malloc_atomic(strlen(str) * 4 + 1);
	for (; *str; str += count) {
	    num = unpack((unsigned char *)str, &count);
	    s = kons(count > 1 ? integer_ : char_, (value_t)num, save);
	    joy(prog);
	    num = v(s);
	    expand(tmp, (value_t)num);
	    strcpy(&yes_str[yes_ptr], tmp);
	    yes_ptr += strlen(tmp);
	}
	yes_str[yes_ptr] = 0;
	s = kons(string_, TO_INTEGER(yes_str), save);
    } else {
	aggr = l(s);
	dump = kons(list_, (value_t)aggr, dump);
	dump = kons(list_, (value_t)dump, 0);
	dump1 = &m[dump].nxt;
	for (; aggr; aggr = n(aggr)) {
	    s = kons(o(aggr), v(aggr), save);
	    joy(prog);
	    *dump1 = kons(o(s), v(s), 0);
	    dump1 = &m[*dump1].nxt;
	}
	s = kons(list_, (value_t)n(dump), save);
	dump = l(dump);
	dump = n(dump);
    }
    dump = n(n(dump));
}

/**
fgets  :  S  ->  S L
L is the next available line (as a string) from stream S.
*/
static void do_fgets()
{
    FILE *fp;
    char *buf, *tmp;
    int leng, size = maxlinelength;

    fp = TO_POINTER(v(s));
    buf = GC_malloc_atomic(size);
    buf[leng = 0] = 0;
    while (fgets(buf + leng, size - leng, fp)) {
	if ((leng = strlen(buf)) > 0 && strchr(buf, '\n'))
	    break;
	if ((tmp = GC_malloc_atomic(size * 2)) == 0)
	    break;
	memcpy(tmp, buf, size);
	buf = tmp;
	size *= 2;
    }
    s = kons(string_, TO_INTEGER(GC_strdup(buf)), s);
}

/**
genrecaux  :  [[B] [T] [R1] R2]  ->  ...
Executes B, if that yields true, executes T.
Else executes R1 and then [[[B] [T] [R1] R2] genrecaux] R2.
*/
static void genrecaux()
{
    memrange prog, save;

    prog = l(s);
    dump = kons(list_, (value_t)prog, dump);
    s = n(s);
    if (condition(l(prog)))	/* [B] */
	joy(l(n(prog)));	/* [T] */
    else {
	joy(l(n(n(prog))));	/* [R1] */
	s = kons(list_, (value_t)prog, s);
	save = kons(funct_, TO_INTEGER(genrecaux), 0);
	s = kons(list_, (value_t)save, s);
	do_cons();
	joy(n(n(n(prog))));	/* [R2] */
    }
    dump = n(dump);
}

/**
genrec  :  [B] [T] [R1] [R2]  ->  ...
Executes B, if that yields true, executes T.
Else executes R1 and then [[[B] [T] [R1] R2] genrecaux] R2.
*/
static void do_genrec()
{
    do_cons();
    do_cons();
    do_cons();
    genrecaux();
}

/**
construct  :  [P] [[P1] [P2] ..]  ->  R1 R2 ..
Saves state of stack and then executes [P].
Then executes each [Pi] to give Ri pushed onto saved stack.
*/
static void do_construct()
{
    memrange prog[2], old_s, new_s;

    prog[1] = l(s);
    dump = kons(list_, (value_t)prog[1], dump);
    s = n(s);
    prog[0] = l(s);
    dump = kons(list_, (value_t)prog[0], dump);
    old_s = n(s);	/* save old stack */
    dump = kons(list_, (value_t)old_s, dump);
    joy(prog[0]);	/* [P]		  */
    new_s = s;		/* save new stack */
    dump = kons(list_, (value_t)new_s, dump);
    for (; prog[1]; prog[1] = n(prog[1])) {
	joy(l(prog[1]));
	old_s = kons(o(s), v(s), old_s);	/* save result */
	s = new_s;	/* restore new stack */
    }
    s = old_s;		/* restore old stack */
    dump = n(n(n(n(dump))));
}

/**
ferror  :  S  ->  S B
B is the error status of stream S.
*/
static void do_ferror()
{
    FILE *fp;
    boolean result;

    fp = TO_POINTER(v(s));
    result = ferror(fp);
    s = kons(boolean_, (value_t)result, s);
}

/**
fflush  :  S  ->  S
Flush stream S, forcing all buffered output to be written.
*/
static void do_fflush()
{
    FILE *fp;

    fp = TO_POINTER(v(s));
    fflush(fp);
}

/**
fwrite  :  S L  ->  S
A list of integers are written as bytes to the current position of stream S.
*/
static void do_fwrite()
{
    int j;
    FILE *fp;
    char *buf;
    memrange aggr, temp;

    temp = aggr = l(s);
    s = n(s);
    fp = TO_POINTER(v(s));
    for (j = 0; temp; temp = n(temp), j++)
	;
    buf = GC_malloc_atomic(j);
    for (j = 0; aggr; aggr = n(aggr), j++)
	buf[j] = (char)v(aggr);
    fwrite(buf, 1, j, fp);
    GC_free(buf);
}

static void printterm(node, fp)
memrange node;
FILE *fp;
{
    while (node) {
	printfactor(node, fp);
	if (n(node))
	    putc(' ', fp);
	node = n(node);
    }
}  /* printterm */

static void printfactor(node, fp)
memrange node;
FILE *fp;
{   /* was forward */
    if (node) {
	switch (o(node)) {
	case list_:
	    putc('[', fp);
	    printterm(l(node), fp);
	    putc(']', fp);
	    break;

	case boolean_:
	    fprintf(fp, "%s", v(node) == 1 ? "true" : "false");
	    break;

	case char_:
	    putc(v(node), fp);
	    break;

	case integer_:
	    fprintf(fp, format_int, v(node));
	    break;

	case string_:
	    fprintf(fp, "\"%s\"", (char *)TO_POINTER(v(node)));
	    break;

	case file_:
	    fprintf(fp, format_file, v(node));
	    break;

	case lib_:
	    fprintf(fp, "%s", table[v(node)].alf);
	    break;

	case funct_:
	    fprintf(fp, "%s", "FUNCTION");
	    break;

	case unknownident:
	    fprintf(fp, "%s", (char *)TO_POINTER(v(node)));
	    break;

	default:
	    fprintf(fp, "%s", stdidents[v(node)].alf);
	    break;
	}  /* CASE */
    }
}  /* printfactor */

/**
fput  :  S X  ->  S
Writes X to stream S, pops X off stack.
*/
static void do_fput()
{
    FILE *fp;
    memrange node;

    node = s;
    s = n(s);
    fp = TO_POINTER(v(s));
    printfactor(node, fp);
}

/**
fputch  :  S C  ->  S
The character C is written to the current position of stream S.
*/
static void do_fputch()
{
    int c;
    FILE *fp;

    c = v(s);
    s = n(s);
    fp = TO_POINTER(v(s));
    putc(c, fp);
}

/**
fputchars  :  S "abc.."  ->  S
The string abc.. (no quotes) is written to the current position of stream S.
*/
static void do_fputchars()
{
    FILE *fp;
    char *str;

    str = TO_POINTER(v(s));
    s = n(s);
    fp = TO_POINTER(v(s));
    fprintf(fp, "%s", str);
}

/**
ftell  :  S  ->  S I
I is the current position of stream S.
*/
static void do_ftell()
{
    FILE *fp;
    value_t val;

    fp = TO_POINTER(v(s));
    val = ftell(fp);
    s = kons(integer_, val, s);
}

/**
typeof  :  X  ->  I
Replace X by its type.
*/
static void do_typeof()
{
    int type;

    switch (o(s)) {
    case lib_:
	type = 2;
	break;

    case funct_:
	type = 3;
	break;

    case boolean_:
	type = 4;
	break;

    case char_:
	type = 5;
	break;

    case integer_:
	type = 6;
	break;

    case string_:
	type = 8;
	break;

    case list_:
	type = 9;
	break;

    case file_:
	type = 11;
	break;

    default:
	type = 1;
	break;
    }
    s = kons(integer_, (value_t)type, n(s));
}

/**
format  :  N C I J  ->  S
S is the formatted version of N in mode C
('d or 'i = decimal, 'o = octal, 'x or
'X = hex with lower or upper case letters)
with maximum width I and minimum width J.
*/
static void do_format()
{
    int prec, width;
    char spec, format[10], *result;

    prec = v(s);
    s = n(s);
    width = v(s);
    s = n(s);
    spec = v(s);
    s = n(s);
    strcpy(format, format_long);
    if ((result = strrchr(format, 'd')) != 0)
	*result = spec;
    result = GC_malloc_atomic(maxlinelength);
    sprintf(result, format, width, prec, v(s));
    s = kons(string_, TO_INTEGER(GC_strdup(result)), n(s));
}

static void unmktime(t)
struct tm *t;
{
    int wday;
    memrange *dump1;

    dump = kons(list_, (value_t)dump, 0);
    dump1 = &m[dump].nxt;
    *dump1 = kons(integer_, (value_t)(t->tm_year + 1900), 0);
    dump1 = &m[*dump1].nxt;
    *dump1 = kons(integer_, (value_t)(t->tm_mon + 1), 0);
    dump1 = &m[*dump1].nxt;
    *dump1 = kons(integer_, (value_t)t->tm_mday, 0);
    dump1 = &m[*dump1].nxt;
    *dump1 = kons(integer_, (value_t)t->tm_hour, 0);
    dump1 = &m[*dump1].nxt;
    *dump1 = kons(integer_, (value_t)t->tm_min, 0);
    dump1 = &m[*dump1].nxt;
    *dump1 = kons(integer_, (value_t)t->tm_sec, 0);
    dump1 = &m[*dump1].nxt;
    *dump1 = kons(boolean_, (value_t)t->tm_isdst, 0);
    dump1 = &m[*dump1].nxt;
    *dump1 = kons(integer_, (value_t)t->tm_yday, 0);
    dump1 = &m[*dump1].nxt;
    if ((wday = t->tm_wday) == 0)
	wday = 7;
    *dump1 = kons(integer_, (value_t)wday, 0);
    s = kons(list_, (value_t)n(dump), n(s));
    dump = l(dump);
}

/**
localtime  :  I  ->  T
Converts a time I into a list T representing local time:
[year month day hour minute second isdst yearday weekday].
Month is 1 = January ... 12 = December;
isdst is a Boolean flagging daylight savings/summer time;
weekday is 1 = Monday ... 7 = Sunday.
*/
static void do_localtime()
{
    time_t t;
    struct tm *tm;

    t = v(s);
    tm = localtime(&t);
    unmktime(tm);
}

/**
gmtime  :  I  ->  T
Converts a time I into a list T representing universal time:
[year month day hour minute second isdst yearday weekday].
Month is 1 = January ... 12 = December;
isdst is false; weekday is 1 = Monday ... 7 = Sunday.
*/
static void do_gmtime()
{
    time_t t;
    struct tm *tm;

    t = v(s);
    tm = gmtime(&t);
    unmktime(tm);
}

static void decode(t)
struct tm *t;
{
    memrange p;

    memset(t, 0, sizeof(struct tm));
    p = l(s);
    if (p && o(p) == integer_) {
	t->tm_year = v(p) - 1900;
	p = n(p);
    }
    if (p && o(p) == integer_) {
	t->tm_mon = v(p) - 1;
	p = n(p);
    }
    if (p && o(p) == integer_) {
	t->tm_mday = v(p);
	p = n(p);
    }
    if (p && o(p) == integer_) {
	t->tm_hour = v(p);
	p = n(p);
    }
    if (p && o(p) == integer_) {
	t->tm_min = v(p);
	p = n(p);
    }
    if (p && o(p) == integer_) {
	t->tm_sec = v(p);
	p = n(p);
    }
    if (p && o(p) == boolean_) {
	t->tm_isdst = v(p);
	p = n(p);
    }
    if (p && o(p) == integer_) {
	t->tm_yday = v(p);
	p = n(p);
    }
    if (p && o(p) == integer_)
	t->tm_wday = v(p) % 7;
}

/**
mktime  :  T  ->  I
Converts a list T representing local time into a time I.
T is in the format generated by localtime.
*/
static void do_mktime()
{
    struct tm t;

    decode(&t);
    s = kons(integer_, (value_t)mktime(&t), n(s));
}

/**
strftime  :  T S1  ->  S2
Formats a list T in the format of localtime or gmtime
using string S1 and pushes the result S2.
*/
static void do_strftime()
{
#ifndef ATARI
    struct tm t;
    char *format, *result;

    format = TO_POINTER(v(s));
    s = n(s);
    decode(&t);
    result = GC_malloc_atomic(maxlinelength);
    strftime(result, maxlinelength, format, &t);
    s = kons(string_, TO_INTEGER(GC_strdup(result)), n(s));
#endif
}

/**
filetime  :  F  ->  T
T is the modification time of file F.
*/
static void do_filetime()
{
    FILE *fp;
    char *str;
    time_t mtime;	/* modification time */
    struct stat *buf;	/* struct stat is big */

    str = TO_POINTER(v(s));
    mtime = 0;
    if ((fp = fopen(str, "r")) != 0) {
	buf = GC_malloc_atomic(sizeof(struct stat));
	if (fstat(fileno(fp), buf) == 0)
	    mtime = buf->st_mtime;
	fclose(fp);
	GC_free(buf);
    }
    s = kons(integer_, (value_t)mtime, n(s));
}

/**
pick  :  X Y Z 2  ->  X Y Z X
Pushes an extra copy of nth (e.g. 2) item X on top of the stack.
*/
static void do_pick()
{
    memrange item;
    value_t j, size;

    size = v(s);	/* pick up the number */
    s = n(s);		/* remove top of stack */
    item = s;		/* if the stack is too small, select the last item */
    for (j = 0; j < size && n(item); j++)	/* top of stack was popped */
	item = n(item); /* possibly select the last item on the stack */
    s = kons(o(item), v(item), s);
}

/**
help  :  ->
Lists all defined symbols, including those from library files.
Then lists all primitives of raw Joy.
(There is a variant: "_help" which lists hidden symbols).
*/
static void do_help()
{
    int loc;

    for (loc = lastlibloc; loc; loc--) {
	putch(' ');
	writeident(table[loc].alf);
    }
    for (loc = xor_; loc; loc--) {
	putch(' ');
	writeident(stdidents[loc].alf);
    }
    writeline();
}

/**
size  :  A  ->  I
Integer I is the number of elements of aggregate A.
*/
static void do_size()
{
    char *str;
    memrange aggr;
    int j = 0, count = 0;

    if (o(s) == string_) {
	for (str = TO_POINTER(v(s)); str[j]; j++)
	    if ((str[j] & 0xC0) != 0x80)
		count++;
    } else
	for (aggr = l(s); aggr; aggr = n(aggr))
	    count++;
    s = kons(integer_, (value_t)count, n(s));
}

/**
concat  :  S T  ->  U
Sequence U is the concatenation of sequences S and T.
*/
static void do_concat()
{
    size_t leng;
    memrange aggr, *dump1;
    char *str, *str1, *str2;

    if (o(s) == string_) {
	str1 = TO_POINTER(v(n(s)));
	str2 = TO_POINTER(v(s));
	leng = strlen(str1) + strlen(str2) + 1;
	str = GC_malloc_atomic(leng);
	sprintf(str, "%s%s", str1, str2);
	s = kons(string_, TO_INTEGER(str), n(n(s)));
    } else {
	if ((aggr = l(n(s))) == 0)
	    s = kons(list_, v(s), n(n(s)));
	else {
	    dump = kons(list_, (value_t)dump, 0);
	    dump1 = &m[dump].nxt;
	    for (; aggr; aggr = n(aggr)) {
		*dump1 = kons(o(aggr), v(aggr), 0);
		dump1 = &m[*dump1].nxt;
	    }
	    *dump1 = l(s);
	    s = kons(list_, (value_t)n(dump), n(n(s)));
	    dump = l(dump);
	}
    }
}

/**
in  :  X A  ->  B
Tests whether X is a member of aggregate A.
*/
static void do_in()
{
    char *str;
    memrange aggr, elem;
    int num, count, result;

    elem = n(s);
    if (o(s) == string_) {
	for (str = TO_POINTER(v(s)); *str; str += count) {
	    num = unpack((unsigned char *)str, &count);
	    if (num == v(elem))
		break;
	}
	result = *str != 0;
    } else {
	for (aggr = l(s); aggr && Compare(aggr, elem); aggr = n(aggr))
	    ;
	result = aggr != 0;
    }
    s = kons(boolean_, (value_t)result, n(n(s)));
}

/*
 * Check whether two lists are equal.
 */
static int is_equal_list(one, two)
memrange one;
memrange two;
{
    if (!one && !two)
	return 1;	/* equal */
    if (!one || !two)
	return 0;	/* not equal */
    if (is_equal(one, two))
	return is_equal_list(n(one), n(two));
    return 0;		/* not equal */
}

/*
 * Check whether two nodes are equal.
 */
static int is_equal(one, two)
memrange one;
memrange two;
{
    if (o(one) == list_ && o(two) == list_)
	return is_equal_list(l(one), l(two));
    return o(one) == o(two) && v(one) == v(two);
}

/**
equal  :  T U  ->  B
(Recursively) tests whether trees T and U are identical.
*/
static void do_equal()
{
    int result;

    result = is_equal(n(s), s);
    s = kons(boolean_, (value_t)result, n(n(s)));
}

/**
times  :  N [P]  ->  ...
N times executes P.
*/
static void do_times()
{
    int j, k;
    memrange prog;

    prog = l(s);
    dump = kons(list_, (value_t)prog, dump);
    s = n(s);
    k = v(s);
    s = n(s);
    for (j = 0; j < k; j++)
	joy(prog);
    dump = n(dump);
}

/**
assign  :  V [N]  ->
Assigns value V to the variable with name N.
*/
static void do_assign()
{
    int index;

    index = v(l(s));
    s = n(s);
    table[index].adr = kons(o(s), v(s), 0);
    s = n(s);
}

/**
unassign  :  [N]  ->
Sets the body of the name N to uninitialized.
*/
void do_unassign()
{
    int index;

    index = v(l(s));
    s = n(s);
    table[index].adr = 0;
}

/**
undefs  :  ->  [..]
Push a list of all undefined symbols in the current symbol table.
*/
static void do_undefs()
{
    int j;
    memrange *dump1;

    dump = kons(list_, (value_t)dump, 0);
    dump1 = &m[dump].nxt;
    for (j = 1; j < lasttable; j++)
	if (!table[j].adr) {
	    *dump1 = kons(string_, TO_INTEGER(GC_strdup(table[j].alf)), 0);
	    dump1 = &m[*dump1].nxt;
	}
    s = kons(list_, (value_t)n(dump), s);
    dump = l(dump);
}

static void joy(node)
memrange node;
{
    memrange temp;

    for (; node; node = n(node)) {	/* FOR */
	if (writelisting > 2) {
	    writeident("joy: ");
	    writefactor(node, true);
	}
	if (writelisting > 3) {
	    writeident("stack: ");
	    writeterm(s, true);
	}
#if defined(DEBUG_G) || defined(DEBUG_S)
	last_op_executed = o(node);
#endif
	switch (o(node)) {
	case funct_:
	    (*(proc_t)TO_POINTER(v(node)))();
	    break;

	case char_:
	case integer_:
	case string_:
	case list_:
	    s = kons(o(node), v(node), s);
	    break;

	case true_:
	    do_true();
	    break;

	case false_:
	    do_false();
	    break;

	case pop_:
	    do_pop();
	    break;

	case dup_:
	    do_dup();
	    break;

	case swap_:
	    do_swap();
	    break;

	case stack_:
	    do_stack();
	    break;

	case unstack_:
	    do_unstack();
	    break;

	/* OPERATIONS: */
	case not_:
	    do_not();
	    break;

	case mul_:
	    do_mul();
	    break;

	case add_:
	    do_add();
	    break;

	case sub_:
	    do_sub();
	    break;

	case div_:
	    do_div();
	    break;

	case and_:
	    do_and();
	    break;

	case or_:
	    do_or();
	    break;

	case lss_:
	    do_lss();
	    break;

	case eql_:
	    do_eql();
	    break;

	case sametype_:
	    do_sametype();
	    break;

	case cons_:
	    do_cons();
	    break;

	case uncons_:
	    do_uncons();
	    break;

	case opcase_:
	    do_opcase();
	    break;

	case of_:
	    do_of();
	    break;

	case body_:
	    do_body();
	    break;

	case put_:
	    do_put();
	    break;

	case putch_:
	    do_putch();
	    break;

	case get_:
	    do_get();
	    break;

	case getch_:
	    do_getch();
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
	    if ((temp = table[v(node)].adr) != 0) {
#ifdef DEBUG_U
		table[v(node)].used = 1;
#endif		    
		joy(temp);
	    }
#ifdef UNDEF_E
	    else {
		printf("\ndefinition needed for %s (%ld)\n",
			table[v(node)].alf, (long)v(node));
		my_exit(runtime_err);
	    }
#endif
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

	case primrec_:
	    do_primrec();
	    break;

	case ifte_:
	    do_ifte();
	    break;

	case linrec_:
	    do_linrec();
	    break;

	case putchars_:
	    do_putchars();
	    break;

	case split_:
	    do_split();
	    break;

	case fread_:
	    do_fread();
	    break;

	case fseek_:
	    do_fseek();
	    break;

	case fclose_:
	    do_fclose();
	    break;

	case filter_:
	    do_filter();
	    break;

	case fremove_:
	    do_fremove();
	    break;

	case frename_:
	    do_frename();
	    break;

	case drop_:
	    do_drop();
	    break;

	case take_:
	    do_take();
	    break;

	case maxint_:
	    do_maxint();
	    break;

	case divmod_:
	    do_divmod();
	    break;

	case xor_:
	    do_xor();
	    break;

	case name_:
	    do_name();
	    break;

	case time_:
	    do_time();
	    break;

	case stdout_:
	    do_stdout();
	    break;

	case stderr_:
	    do_stderr();
	    break;

	case quit_:
	    do_quit();
	    break;

	case intern_:
	    do_intern();
	    break;

	case unary3_:
	    do_unary3();
	    break;

	case unary4_:
	    do_unary4();
	    break;

	case casting_:
	    do_casting();
	    break;

	case case_:
	    do_case();
	    break;

	case cond_:
	    do_cond();
	    break;

	case some_:
	    do_some();
	    break;

	case all_:
	    do_all();
	    break;

	case getenv_:
	    do_getenv();
	    break;

	case compare_:
	    do_compare();
	    break;

	case condlinrec_:
	case condnestrec_:
	    do_condnestrec();
	    break;

	case tailrec_:
	    do_tailrec();
	    break;

	case fgets_:
	    do_fgets();
	    break;

	case treegenrec_:
	    do_treegenrec();
	    break;

	case treerec_:
	    do_treerec();
	    break;

	case treestep_:
	    do_treestep();
	    break;

	case map_:
	    do_map();
	    break;

	case genrec_:
	    do_genrec();
	    break;

	case construct_:
	    do_construct();
	    break;

	case ferror_:
	    do_ferror();
	    break;

	case fflush_:
	    do_fflush();
	    break;

	case fwrite_:
	    do_fwrite();
	    break;

	case fput_:
	    do_fput();
	    break;

	case fputch_:
	    do_fputch();
	    break;

	case fputchars_:
	    do_fputchars();
	    break;

	case ftell_:
	    do_ftell();
	    break;

	case typeof_:
	    do_typeof();
	    break;

	case format_:
	    do_format();
	    break;

	case localtime_:
	    do_localtime();
	    break;

	case gmtime_:
	    do_gmtime();
	    break;

	case mktime_:
	    do_mktime();
	    break;

	case strftime_:
	    do_strftime();
	    break;

	case filetime_:
	    do_filetime();
	    break;

	case pick_:
	    do_pick();
	    break;

	case help_:
	    do_help();
	    break;

	case size_:
	    do_size();
	    break;

	case concat_:
	    do_concat();
	    break;

	case in_:
	    do_in();
	    break;

	case equal_:
	    do_equal();
	    break;

	case times_:
	    do_times();
	    break;

	case assign_:
	    do_assign();
	    break;

	case unassign_:
	    do_unassign();
	    break;

	case undefs_:
	    do_undefs();
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
    fprintf(f, "%lu user nodes available\n", (unsigned long)MAXMEM -
	    firstusernode);
    fprintf(f, "%lu garbage collections\n", stat_gc);
    fprintf(f, "%lu nodes used\n", stat_kons);
    fprintf(f, "%lu calls to joy interpreter\n", stat_calls);
    fprintf(f, "%lu operations executed\n", stat_ops);
}  /* writestatistics */

int main(argc, argv)
int argc;
char *argv[];
{  /* main */
    char *ptr;
    memrange j;

    /*
     * Fatal errors and end-of-file end the program. Other errors allow new
     * programs to be read.
     */
    if ((j = setjmp(JL10)) == fatal_err) {
#ifdef DEBUG_S
	printf("last_op_executed: %-*.*s\n", identlength, identlength,
		standardident_NAMES[last_op_executed]);
	writeident("stack: ");
	writeterm(s, true);
#endif
	goto einde;
    }
    if (j == runtime_err)
	goto begin;
    /*
     * Initialize clock, scanner and symbol table.
     */
    initialise();
    /*
     * Initialize freelist.
     */
    for (freelist = j = 1; j < MAXMEM; j++)
	m[j].nxt = j + 1;
    m[MAXMEM - 1].nxt = 0;
    /*
     * Extract the pathname from the joy binary, to be prepended to include
     * files, in case the include file was not found in the current directory.
     */
    if ((ptr = strrchr(argv[0], '/')) == 0)
	ptr = strrchr(argv[0], '\\');
    if (ptr) {
	/*
	 * Remove the joy binary, keep the separator.
	 */
	ptr[1] = 0;
	/*
	 * Make the pathname available everywhere.
	 */
	pathname = argv[0];
    }
    g_argv = argv;	/* command line */
    g_argc = argc;
    /*
     * The library is 42minjoy.lib. If not present, only programs can be used,
     * no definitions.
     */
    readlibrary(lib_filename);
    time(&stat_lib);
    if (writelisting > 1)
	for (j = 1; j <= lastlibloc; j++) {
	    fprintf(listing, "\"%-*.*s\" :\n", identlength, identlength,
		    table[j].alf);
	    writeterm(table[j].adr, true);
	}
    /*
     * A filename parameter is possible: it contains programs to be executed.
     */
    if (argc > 1)
	newfile(argv[1], 0);
begin:    
    while (1) {
	getsym();
#if defined(DEBUG_G) || defined(DEBUG_S)
	last_op_executed = get_;
#endif
	programme = 0;
	readterm(&programme);
	if (writelisting > 1) {
	    writeident("interpreting: ");
	    writeterm(programme, true);
	}
	if (dump) {
	    printf("dump error: should be empty!\n");
	    writeterm(dump, true);
	    dump = 0;
	}
	outlinelength = 0;
	joy(programme);
	/*
	 * Automatic output of TOS and a newline.
	 */
	if (s) {
	    writefactor(s, true);
	    fflush(stdout);
	    s = n(s);
	}
	if (outlinelength)
	    writeline();
	if (writelisting > 1) {
	    writeident("stack: ");
	    writeterm(s, true);
	}
    }
einde:
#ifdef DEBUG_M
    DumpM();
#endif
#ifdef DEBUG_U
    DumpU();
#endif
    perhapsstatistics();
    SetNormal();
    return 0;
}

static void perhapsstatistics()
{
    finalise();
    if (statistics) {
	fflush(stdout);
	writestatistics(stderr);
	if (writelisting)
	    writestatistics(listing);
    }
}

/*
 * Called from the scan utilities, that do not have access to JL10.
 */
static void my_exit(code)
int code;
{
    longjmp(JL10, code);
}

#ifndef BDWGC
static void mark_str(node)
memrange node;
{
    char *str;

    while (node) {
	if (o(node) == list_)
	    mark_str(l(node));
	if (o(node) == string_) {
	    str = TO_POINTER(v(node));
	    if (!str[-1])
		str[-1] = 1;
	}
	node = n(node);
    }
}  /* mark_str */

void mark_string()
{
    mark_str(programme);
    mark_str(s);
    mark_str(dump);
}
#endif
