/*
    module  : joy.c
    version : 1.27
    date    : 07/30/21
*/
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <setjmp.h>
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

/* File: Included file for scan utilities */

#define maxincludelevel	  5
#define maxlinelength	  132
#define linenumwidth	  4

#define linenumspace	  "    "
#define linenumsep	  "    "
#define underliner	  "****    "
#if 0
#define tab_in_listing	  "    ----    "
#endif

#define maxoutlinelength  60
#define messagelength	  30

#define initial_alternative_radix  2

#if 0
#define maxchartab	  1000
#define maxstringtab	  100
#define maxnodtab	  1000
#endif

typedef char identalfa[identlength + 1];
typedef char resalfa[reslength + 1];

#if 0
typedef char message[messagelength + 1];

typedef struct toops {
    long symbols, types, strings, chars;
} toops;
#endif

typedef struct _REC_inputs {
    FILE *fil;
    identalfa nam;
    long lastlinenumber;
} _REC_inputs;

typedef struct _REC_reswords {
    resalfa alf;
    symbol symb;
} _REC_reswords;

typedef struct _REC_stdidents {
    identalfa alf;
    standardident symb;
} _REC_stdidents;

static FILE *listing;

static _REC_inputs inputs[maxincludelevel];
static long includelevel, adjustment, writelisting;
static boolean must_repeat_line;

static long scantimevariables['Z' + 1 - 'A'];
static long alternative_radix, linenumber;
static char line[maxlinelength + 1];
static size_t cc, ll;
static int my_ch;
static identalfa ident;
static standardident id;
static char specials_repeat[maxrestab + 1];
static symbol sym;
static long num;
static _REC_reswords reswords[maxrestab + 1];
static int lastresword;
static _REC_stdidents stdidents[maxstdidenttab + 1];
static int laststdident;
#if 0
static long trace;
static long stringtab[maxstringtab + 1];
static char chartab[maxchartab + 1];

static toops toop;
#endif

static long errorcount, outlinelength, statistics;
static clock_t start_clock, end_clock;

/* - - - - -   MODULE ERROR    - - - - - */

static void point_to_symbol(boolean repeatline, FILE *f, char diag, char *mes)
{
    char c;
    int i, j;

    LOGFILE(__func__);
    if (repeatline) {
	fprintf(f, "%*ld%s", linenumwidth, linenumber, linenumsep);
	for (i = 0; i < ll; i++)
	    putc(line[i], f);
	putc('\n', f);
    }
    fputs(underliner, f);
    j = (int)cc;
    for (i = 0, j -= 2; i < j; i++)
	if ((c = line[i]) < ' ')
	    putc(c, f);
	else
	    putc(' ', f);
    fprintf(f, "^\n");
    fprintf(f, "%s-%c  %-*.*s\n", errormark, diag, messagelength,
	    messagelength, mes);
    if (diag == 'F')
	fprintf(f, "execution aborted\n");
}  /* point_to_symbol */

static void point(char diag, char *mes)
{
    LOGFILE(__func__);
    if (diag != 'I')
	errorcount++;
    if (includelevel > 0)
	printf("INCLUDE file : \"%-*.*s\"\n", identlength, identlength,
	       inputs[includelevel - 1].nam);
    point_to_symbol(true, stdout, diag, mes);
    if (writelisting > 0) {
	point_to_symbol(must_repeat_line, listing, diag, mes);
	must_repeat_line = true;
    }
    if (diag == 'F')
        exit(0);
}  /* point */

/* - - - - -   MODULE SCANNER  - - - - - */

static void closelisting(void)
{
    fclose(listing);
}

/*
    iniscanner - initialize global variables
*/
static void iniscanner(void)
{
    LOGFILE(__func__);
    if ((listing = fopen(list_filename, "w")) == NULL) {
	fprintf(stderr, "%s (not open for writing)\n", list_filename);
	exit(0);
    }
    atexit(closelisting);
    writelisting = 0;
    my_ch = ' ';
    linenumber = 0;
    cc = 1;
    ll = 1;		    /* to enable fatal message during initialisation */
    memset(specials_repeat, 0, sizeof(specials_repeat));  /* def: no repeats */
    includelevel = 0;
    adjustment = 0;
    alternative_radix = initial_alternative_radix;
    lastresword = 0;
    laststdident = 0;
    outlinelength = 0;
    memset(scantimevariables, 0, sizeof(scantimevariables));
    errorcount = 0;
    must_repeat_line = false;
}  /* iniscanner */

static void erw(char *a, symbol symb)
{
    LOGFILE(__func__);
    if (++lastresword > maxrestab)
	point('F', "too many reserved words");
    strncpy(reswords[lastresword].alf, a, reslength);
    reswords[lastresword].alf[reslength] = 0;
    reswords[lastresword].symb = symb;
}  /* erw */

static void est(char *a, standardident symb)
{
    LOGFILE(__func__);
    if (++laststdident > maxstdidenttab)
	point('F', "too many identifiers");
    strncpy(stdidents[laststdident].alf, a, identlength);
    stdidents[laststdident].alf[identlength] = 0;
    stdidents[laststdident].symb = symb;
}  /* est */

static void release(void)
{
    int i;

    for (i = 0; i < maxincludelevel; i++)
	if (inputs[i].fil)
	    fclose(inputs[i].fil);
}

static void newfile(char *a)
{
    static int init;
#if 0
    int i;
    char str[256];
#endif

    LOGFILE(__func__);
    if (!init) {
	init = 1;
	atexit(release);
    }
    strncpy(inputs[includelevel].nam, a, identlength);
    inputs[includelevel].nam[identlength] = 0;
    inputs[includelevel].lastlinenumber = linenumber;
#if 0
    sprintf(str, "%-*.*s", identlength, identlength, a);
    for (i = strlen(str) - 1; isspace((int)str[i]); i--)
	;
    str[i + 1] = 0;
    for (i = 0; i < includelevel; i++)
	if (!strcmp(inputs[i].nam, inputs[includelevel].nam)) {
	    fclose(inputs[i].fil);
	    inputs[i].fil = NULL;
	    break;
	}
#endif
    if (inputs[includelevel].fil != NULL) {
	fclose(inputs[includelevel].fil);
	inputs[includelevel].fil = NULL;
    }
    if ((inputs[includelevel].fil = fopen(a, "r")) == NULL) {
	fprintf(stderr, "%s (not open for reading)\n", a);
	exit(0);
    }
    adjustment = 1;
}  /* newfile */

static void getsym(void);

#if 0
#define emptydir	"                "
#endif
#define dirlength	16

static void perhapslisting(void)
{
    size_t i, j;

    LOGFILE(__func__);
    if (writelisting > 0) {
        fprintf(listing, "%*ld", linenumwidth, linenumber);
        for (i = ll - 1; i > 0 && line[i] <= ' '; i--)
            ;
        if (line[j = i + 1] != 0) {
            fprintf(listing, "%s", linenumsep);
	    for (i = 0; i < j; i++)
		putc(line[i], listing);
	}
	putc('\n', listing);
	must_repeat_line = false;
	fflush(listing);
    }
}

static void getch(void)
{
#if 0
    int c;
#endif
    FILE *f;

    LOGFILE(__func__);
    if (cc == ll) {
	if (adjustment != 0) {
	    if (adjustment == -1)
		linenumber = inputs[includelevel - 1].lastlinenumber;
	    else
		linenumber = 0;
	    includelevel += adjustment;
	    adjustment = 0;
	}
	linenumber++;
	ll = 0;
	cc = 0;
	if (!includelevel) {
#if 1
	    if (fgets(line, maxlinelength, stdin)) {
		ll = strlen(line);
		perhapslisting();
	    }
#else
	    while ((c = getchar()) != EOF && c != '\n')
		if (ll < maxlinelength && c != '\r')
		    line[ll++] = c;
	    if (c == EOF)
		point('F', "unexpected end of file");
	    perhapslisting();
#endif
	} else {
	    f = inputs[includelevel - 1].fil;
#if 1
	    if (fgets(line, maxlinelength, f)) {
		ll = strlen(line);
		perhapslisting();
	    } else {
#else
	    while ((c = getc(f)) != EOF && c != '\n')
		if (ll < maxlinelength && c != '\r')
		    line[ll++] = c;
	    if (c == EOF) {
		c = 0;
#endif
		fclose(f);
		inputs[includelevel - 1].fil = NULL;
		adjustment = -1;
	    }
#if 0
	      else
		perhapslisting();
#endif
	}
	line[ll++] = ' ';
    }  /* IF */
    my_ch = line[cc++];
}  /* getch */

static long value(void)
{
    /* this is a  LL(0) parser */
    long result = 0;

    LOGFILE(__func__);
    do
	getch();
    while (my_ch <= ' ');
    if (my_ch == '\'' || my_ch == '&' || isdigit(my_ch)) {
	getsym();
	result = num;
	goto einde;
    }
    if (isupper(my_ch)) {
	result = scantimevariables[my_ch - 'A'];
#if 0
	getsym();
#endif
	goto einde;
    }
    if (my_ch == '(') {
	result = value();
	while (my_ch <= ' ')
	    getch();
	if (my_ch == ')')
	    getch();
	else
	    point('E', "right parenthesis expected");
	goto einde;
    }
    switch (my_ch) {

    case '+':
	result = value() + value();
	break;

    case '-':
	result = value();
	result -= value();
	break;

    case '*':
	result = value() * value();
	break;

    case '/':
	result = value();
	result /= value();
	break;

    case '=':
	result = value() == value();
	break;

    case '>':
	result = value();
	result = result > value();
	break;

    case '<':
	result = value();
	result = result < value();
	break;

    case '?':
	if (scanf("%ld", &result) != 1)
	    result = 0;
	break;

    default:
	point('F', "illegal start of scan expr");
    }  /* CASE */
einde:
    return result;
}  /* value */

static void directive(void)
{
    int i, j;
    char c, dir[dirlength + 1];

    LOGFILE(__func__);
    getch();
    i = 0;
#if 0
    strncpy(dir, emptydir, dirlength);
#endif
    do {
	if (i < dirlength)
	    dir[i++] = my_ch;
	getch();
    } while (my_ch == '_' || isupper(my_ch));
    dir[i] = 0;
    if (!strcmp(dir, "IF")) {
	if (value() < 1)
	    cc = ll;  /* readln */
    } else if (!strcmp(dir, "INCLUDE")) {
	if (includelevel == maxincludelevel)
	    point('F', "too many include files");
	while (my_ch <= ' ')
	    getch();
	i = 0;
#if 0
	strncpy(ident, emptyident, identlength);
#endif
	do {
	    if (i < identlength)
		ident[i++] = my_ch;
	    getch();
	} while (my_ch > ' ');
	ident[i] = 0;
	newfile(ident);
    } else if (!strcmp(dir, "PUT")) {
	j = (int)ll;
        for (i = j - 1; i > 0 && line[i] <= ' '; i--)
            ;
	j = i + 1;
	i = (int)cc;
	for (i--; i < j; i++)
	    fputc(line[i], stderr);
	fputc('\n', stderr);
	cc = ll;
    } else if (!strcmp(dir, "SET")) {
	while (my_ch <= ' ')
	    getch();
	if (!isupper(my_ch))
	    point('E', "\"A\" .. \"Z\" expected");
	c = my_ch;
	getch();
	while (my_ch <= ' ')
	    getch();
	if (my_ch != '=')
	    point('E', "\"=\" expected");
	scantimevariables[c - 'A'] = value();
    }
#if 0
    else if (!strncmp(dir, "TRACE           ", dirlength))
	trace = value();
#endif
    else if (!strcmp(dir, "LISTING")) {
	i = writelisting;
	writelisting = value();
	if (!i)
	    perhapslisting();
    }
    else if (!strcmp(dir, "STATISTICS"))
	statistics = value();
    else if (!strcmp(dir, "RADIX"))
	alternative_radix = value();
    else
	point('F', "unknown directive");
    getch();
}  /* directive */

#if 0
#undef emptydir
#undef dirlength
#endif

static void getsym(void)
{
#if 0
    char c;
#endif
    int i, j, k;
    boolean negated;

    LOGFILE(__func__);
begin:
    ident[i = 0] = 0;
    while (my_ch <= ' ')
	getch();
    switch (my_ch) {

    case '\'':
	getch();
	if (my_ch == '\\')
	    num = value();
	else {
	    num = my_ch;
	    getch();
	}
	if (my_ch == '\'')
	    getch();
	sym = charconst;
	break;

#if 0
    case '"':
	if (toop.strings == maxstringtab)
	    point('F', "too many strings              ");
	stringtab[num = ++toop.strings] = toop.chars + 1;
	getch();
	while (ch != '"') {
	    if ((c = ch) == '\\')
		c = value();
	    else
		getch();
	    if (++toop.chars > maxchartab)
		point('F', "too many characters in strings");
	    chartab[toop.chars] = c;
	}  /* WHILE */
	getch();
	stringtab[num + 1] = toop.chars;
	sym = stringconst;
	/* FOR i := stringtab[num] TO stringtab[num+1] DO
	    write(chartab[i]) */
	break;
#endif

    case '(':
	getch();
	if (my_ch == '*') {
	    getch();
	    do {
		while (my_ch != '*')
		    getch();
		getch();
	    } while (my_ch != ')');
	    getch();
	    goto begin;
	}
	sym = leftparenthesis;
	break;

    case '-':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
	if (my_ch != '-')
	    negated = false;
	else {
	    getch();
	    if (!isdigit(my_ch)) {	
#if 0
		strncpy(res, emptyres, reslength);
		strncpy(ident, emptyident, identlength);
#endif
		ident[i++] = '-';
		ident[i] = 0;
		/* sym = hyphen; */
		goto einde;
	    }
	    negated = true;
	}
	sym = numberconst;
	num = 0;
	do {
	    num = num * 10 + my_ch - '0';
	    getch();
	} while (isdigit(my_ch));
	if (negated)
	    num = -num;
	break;

    case '&':  /* number in alternative radix */
	sym = numberconst;
	num = 0;
	getch();
	while (isdigit(my_ch) || isupper(my_ch)) {
	    if (my_ch >= 'A' && my_ch <= 'Z')
		my_ch += '9' - 'A' + 1;
	    if (my_ch >= alternative_radix + '0')
		point('E', "exceeding alternative radix");
	    num = alternative_radix * num + my_ch - '0';
	    getch();
	}
	break;

    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
    case 'g':
    case 'h':
    case 'i':
    case 'j':
    case 'k':
    case 'l':
    case 'm':
    case 'n':
    case 'o':
    case 'p':
    case 'q':
    case 'r':
    case 's':
    case 't':
    case 'u':
    case 'v':
    case 'w':
    case 'x':
    case 'y':
    case 'z':
	sym = identifier;
#if 0
	strncpy(ident, emptyident, identlength);
#endif
	do {
	    if (i < identlength)
		ident[i++] = my_ch;
	    getch();
	} while (my_ch == '_' || isalnum(my_ch));
	ident[i] = 0;
	break;

    case '%':
	directive();
	goto begin;

    default:
#if 0
	strncpy(res, emptyres, reslength);
	strncpy(ident, emptyident, identlength);
#endif
einde:
	if (isupper(my_ch))
	    do {
		if (i < identlength) {
		    ident[i++] = my_ch;
		    ident[i] = 0;
		}
		getch();
	    } while (my_ch == '_' || isalnum(my_ch));
	else if (my_ch > ' ')
	    do {
		if (i < identlength) {
		    ident[i++] = my_ch;
		    ident[i] = 0;
		}
		getch();
	    } while (strchr(specials_repeat, my_ch));
	i = 1;
	j = lastresword;
	do {
	    k = (i + j) / 2;
	    if (strcmp(ident, reswords[k].alf) <= 0)
		j = k - 1;
	    if (strcmp(ident, reswords[k].alf) >= 0)
		i = k + 1;
	} while (i <= j);
	if (i - 1 > j)	/* OTHERWISE */
	    sym = reswords[k].symb;
	else
	    sym = identifier;
	break;
    }  /* CASE */
}  /* getsym */

/* - - - - -   MODULE OUTPUT   - - - - - */

static void putch(char c)
{
    LOGFILE(__func__);
    putchar(c);
    if (writelisting > 0) {
	if (!outlinelength)
	    fprintf(listing, "%s%s", linenumspace, linenumsep);
	putc(c, listing);
    }
    if (c == '\n')
	outlinelength = 0;
    else
	outlinelength++;
}

static void writeline(void)
{
    LOGFILE(__func__);
    putchar('\n');
    if (writelisting > 0)
	putc('\n', listing);
    outlinelength = 0;
}

static void writeident(char *a)
{
    int i;
    size_t length;

    LOGFILE(__func__);
#if 0
    length = identlength;
    while (a[length - 1] <= ' ')
	length--;
#else
    length = strlen(a);
#endif
    if (outlinelength + length > maxoutlinelength)
	writeline();
    for (i = 0; i < length; i++)
	putch(a[i]);
}

#if 0
static void writeresword(char *a)
{
    long i, length;

    length = reslength;
    while (a[length - 1] <= ' ')
	length--;
    if (outlinelength + length > maxoutlinelength)
	writeline();
    for (i = 0; i < length; i++)
	putch(a[i]);
}
#endif

static void writenatural(unsigned long n)
{
    LOGFILE(__func__);
    if (n >= 10)
	writenatural(n / 10);
    putch(n % 10 + '0');
}

static void writeinteger(long i)
{
    LOGFILE(__func__);
    if (outlinelength + 12 > maxoutlinelength)
	writeline();
    if (i >= 0)
	writenatural(i);
    else {
	putch('-');
	writenatural(-i);
    }
}  /* writeinteger */

static void fin(FILE *f)
{
    LOGFILE(__func__);
    if (errorcount > 0)
	fprintf(f, "%ld error(s)\n", errorcount);
    end_clock = clock() - start_clock;
    fprintf(f, "%ld microseconds CPU\n", end_clock);
}

static void finalise(void)
{
    LOGFILE(__func__);
    /* finalise */
    fin(stderr);
    if (writelisting > 0)
	fin(listing);
    /* finalise */
}

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
	    s = kons(integer_, my_ch, s);
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
    fprintf(f, "%lu microseconds CPU to read library\n", stat_lib);
    fprintf(f, "%lu microseconds CPU to execute\n", end_clock - stat_lib);
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
    stat_lib = clock() - start_clock;
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
