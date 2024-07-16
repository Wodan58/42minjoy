/*
    module  : scanutil.c
    version : 1.10
    date    : 07/12/24
*/
/* File: Included file for scan utilities */

#define maxincludelevel	  5
#define maxlinelength	  132
#define linenumwidth	  4

#define linenumspace	  "    "
#define linenumsep	  "    "
#define underliner	  "****    "

#define maxoutlinelength  60
#define messagelength	  30

#define initial_alternative_radix  2

#if 0
#define maxchartab	  1000
#define maxstringtab	  100
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
static bool must_repeat_line;

static long scantimevariables['Z' + 1 - 'A'];
static long alternative_radix, linenumber;
static char line[maxlinelength + 1];
static int cc, ll;
static int chr;
static identalfa ident;
#if defined(MINPAS) || defined(MINJOY)
static standardident id;
#endif
#if 0
static resalfa res;
#endif
static char specials_repeat[maxrestab + 1];
static symbol sym;
static long num;
static _REC_reswords reswords[maxrestab + 1];
static int lastresword;
#if defined(MINPAS) || defined(MINJOY)
static _REC_stdidents stdidents[maxstdidenttab + 1];
#endif
static int laststdident;
#ifdef NETVER
static long trace = 1;
#endif
#if 0
static char resword_inverse[37];
static long stringtab[maxstringtab + 1];
static char chartab[maxchartab + 1];

static toops toop;
#endif

static long errorcount, outlinelength, statistics;
static clock_t start_clock, end_clock;

/* - - - - -   MODULE ERROR    - - - - - */

static void point_to_symbol(bool repeatline, FILE *f, char diag, char *mes)
{
    char c;
    int i, j;

    if (repeatline) {
	fprintf(f, "%*ld%s", linenumwidth, linenumber, linenumsep);
	for (i = 0; i < ll; i++)
	    putc(line[i], f);
	putc('\n', f);
    }
    fputs(underliner, f);
    for (i = 0, j = cc - 2; i < j; i++)
	if ((c = line[i]) < ' ')
	    putc(c, f);
	else
	    putc(' ', f);
    fprintf(f, "^\n");
    fprintf(f, "%s-%c  %-*.*s\n", errormark, diag, messagelength,
	messagelength, mes);
    if (diag == 'F')
	fprintf(f, "execution aborted\n");
} /* point_to_symbol */

static void point(char diag, char *mes)
{
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
} /* point */

/* - - - - -   MODULE SCANNER  - - - - - */

static void closelisting(void)
{
    if (listing)
	fclose(listing);
    listing = 0;
}

/*
    iniscanner - initialize global variables
*/
static void iniscanner(void)
{
    start_clock = clock();
    if ((listing = fopen(list_filename, "w")) == NULL) {
	fprintf(stderr, "%s (not open for writing)\n", list_filename);
	exit(0);
    }
    atexit(closelisting);
    writelisting = 0;
    chr = ' ';
    linenumber = 0;
    cc = 1;
    ll = 1; /* to enable fatal message during initialisation */
    memset(specials_repeat, 0, sizeof(specials_repeat)); /* def: no repeats */
    includelevel = 0;
#if 0
    /*
     * Initial input file is stdin. This is not stored in the inputs table.
     */
    inputs[0].fil = stdin;
    strcpy(inputs[0].nam, "stdin");
    inputs[0].lastlinenumber = 1;
#endif
    adjustment = 0;
    alternative_radix = initial_alternative_radix;
    lastresword = 0;
    laststdident = 0;
    outlinelength = 0;
    memset(scantimevariables, 0, sizeof(scantimevariables));
    errorcount = 0;
    must_repeat_line = false;
} /* iniscanner */

static void erw(char *str, symbol symb)
{
    if (++lastresword > maxrestab)
	point('F', "too many reserved words");
    strncpy(reswords[lastresword].alf, str, reslength);
    reswords[lastresword].alf[reslength] = 0;
    reswords[lastresword].symb = symb;
} /* erw */

#if defined(MINPAS) || defined(MINJOY)
static void est(char *str, standardident symb)
{
    if (++laststdident > maxstdidenttab)
	point('F', "too many identifiers");
    strncpy(stdidents[laststdident].alf, str, identlength);
    stdidents[laststdident].alf[identlength] = 0;
    stdidents[laststdident].symb = symb;
} /* est */
#endif

static void release(void)
{
    int i;

    for (i = 0; i < maxincludelevel; i++)
	if (inputs[i].fil) {
	    fclose(inputs[i].fil);
	    inputs[i].fil = 0;
	}
}

static void newfile(char *str, int flag)
{
    static unsigned char init;
    FILE *fp;
    char *ptr;

    if (!init) {
	init = 1;
	atexit(release);
    }
    if (!flag) {
	if (!freopen(str, "r", stdin)) {
	    fprintf(stderr, "%s (not open for reading)\n", str);
	    exit(0);
	}
	adjustment = 0;
	return;
    }
    /*
     * str may contain a pathname. The pathname must be stripped, because
     * identlength is not large enough to hold a pathname. The name is only
     * used in error messages and does not need a pathname.
     */
    if ((ptr = strrchr(str, '/')) != 0)
	ptr++;
    else
	ptr = str;
    strncpy(inputs[includelevel].nam, ptr, identlength);
    inputs[includelevel].lastlinenumber = linenumber;
    if ((fp = fopen(str, "r")) == NULL) {
	/*
	 * If the include file does not contain a pathname yet, the pathname
	 * is prepended and the open is tried again. If that also fails, the
	 * program fails. Pathname itself is a global variable.
	 */
	if (ptr == str) {
	    flag = strlen(pathname) + strlen(str) + 1;
	    ptr = malloc(flag);
	    sprintf(ptr, "%s%s", pathname, str);
	    fp = fopen(ptr, "r");
	    free(ptr);
	}
	if (!fp) {
	    fprintf(stderr, "%s (not open for reading)\n", str);
	    exit(0);
	}
    }
    inputs[includelevel].fil = fp;
    adjustment = 1;
} /* newfile */

static void getsym(void);

#define dirlength	16

static void perhapslisting(void)
{
    int i;

    if (writelisting > 0) {
	fprintf(listing, "%*ld%s", linenumwidth, linenumber, linenumsep);
	for (i = 0; i < ll; i++)
	    putc(line[i], listing);
	putc('\n', listing);
	must_repeat_line = false;
	fflush(listing);
    }
}

static void getch(void)
{
    FILE *f;
    char *ptr;

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
	    if (fgets(line, maxlinelength, stdin)) {
		if ((ptr = strchr(line, '\n')) != 0)
		    *ptr = 0;
		ll = strlen(line);
		perhapslisting();
	    } else
		exit(0);
	} else {
	    f = inputs[includelevel - 1].fil;
	    if (fgets(line, maxlinelength, f)) {
		if ((ptr = strchr(line, '\n')) != 0)
		    *ptr = 0;
		ll = strlen(line);
		perhapslisting();
	    } else {
		fclose(f);
		inputs[includelevel - 1].fil = NULL;
		adjustment = -1;
	    }
	}
	line[ll++] = ' ';
    } /* IF */
    chr = line[cc++];
} /* getch */

static long value(void)
{
    /* this is a  LL(0) parser */
    long result = 0;

    do
	getch();
    while (chr <= ' ');
    if (chr == '\'' || chr == '&' || isdigit(chr)) {
	getsym();
	result = num;
	goto einde;
    }
    if (isupper(chr)) {
	result = scantimevariables[chr - 'A'];
	goto einde;
    }
    if (chr == '(') {
	result = value();
	while (chr <= ' ')
	    getch();
	if (chr == ')')
	    getch();
	else
	    point('E', "right parenthesis expected");
	goto einde;
    }
    switch (chr) {
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
#if 0
    case '?':
	if (scanf("%ld", &result) != 1)
	    result = 0;
	break;
#endif
    default:
	point('F', "illegal start of scan expr");
    } /* CASE */
einde:
    return result;
} /* value */

static void directive(void)
{
    int i;
    char c, dir[dirlength + 1];

    getch();
    i = 0;
    do {
	if (i < dirlength)
	    dir[i++] = chr;
	getch();
    } while (chr == '_' || isupper(chr));
    dir[i] = 0;
    if (!strcmp(dir, "IF")) {
	if (value() < 1)
	    cc = ll; /* readln */
    } else if (!strcmp(dir, "INCLUDE")) {
	if (includelevel == maxincludelevel)
	    point('F', "too many include files");
	while (chr <= ' ')
	    getch();
	i = 0;
	do {
	    if (i < identlength)
		ident[i++] = chr;
	    getch();
	} while (chr > ' ');
	ident[i] = 0;
	newfile(ident, 1);
    } else if (!strcmp(dir, "PUT")) {
	fflush(stdout);
	for (i = cc - 1; i < ll; i++)
	    fputc(line[i], stderr);
	fputc('\n', stderr);
	cc = ll;
    } else if (!strcmp(dir, "SET")) {
	while (chr <= ' ')
	    getch();
	if (!isupper(chr))
	    point('E', "\"A\" .. \"Z\" expected");
	c = chr;
	getch();
	while (chr <= ' ')
	    getch();
	if (chr != '=')
	    point('E', "\"=\" expected");
	scantimevariables[c - 'A'] = value();
#if 0
    } else if (!strcmp(dir, "TRACE")) {
	trace = value();
#endif
    } else if (!strcmp(dir, "LISTING")) {
	i = writelisting;
	writelisting = value();
	if (!i)
	    perhapslisting();
    } else if (!strcmp(dir, "STATISTICS"))
	statistics = value();
    else if (!strcmp(dir, "RADIX"))
	alternative_radix = value();
    else
	point('F', "unknown directive");
    getch();
} /* directive */

static void getsym(void)
{
#if 0
    char c;
#endif
    bool negated;
    int i, j, k, index;

begin:
    ident[index = 0] = 0;	/* start with empty ident and index at 0 */
    while (chr <= ' ')
	getch();
    switch (chr) {
    case '\'':
	getch();
	if (chr == '\\')
	    num = value();
	else {
	    num = chr;
	    getch();
	}
#ifndef MINJOY
	if (chr == '\'')	/* no optional closing quote */
	    getch();
#endif
	sym = charconst;
	break;

#if 0
    case '"':
	if (toop.strings == maxstringtab)
	    point('F', "too many strings");
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
	if (chr == '*') {
	    getch();
	    do {
		while (chr != '*')
		    getch();
		getch();
	    } while (chr != ')');
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
	if (chr != '-')
	    negated = false;
	else {
	    getch();
	    if (!isdigit(chr)) {
		ident[index++] = '-';
		ident[index] = 0;
		goto einde;
	    }
	    negated = true;
	}
	sym = numberconst;
	num = 0;
	do {
	    num = num * 10 + chr - '0';
	    getch();
	} while (isdigit(chr));
	if (negated)
	    num = -num;
	break;

    case '&': /* number in alternative radix */
	sym = numberconst;
	num = 0;
	getch();
	while (isdigit(chr) || isupper(chr)) {
	    if (isupper(chr))
		chr += '9' - 'A' + 1;
	    if (chr >= alternative_radix + '0')
		point('E', "exceeding alternative radix");
	    num = alternative_radix * num + chr - '0';
	    getch();
	}
	break;

    case '%':
	directive();
	goto begin;

    default:
	if (chr == '_' || isalnum(chr))
	    goto einde;
	if (chr > ' ') {
	    /*
	     * A special character has been read that may be a reserved word
	     * or it may be the start of an identifier. The table of reserved
	     * words is searched after reading each character.
	     */
again:		    
	    if (index < identlength)
		ident[index++] = chr;
	    getch();
	    ident[index] = 0;
	    i = 1;
	    j = lastresword;
	    do {
		k = (i + j) / 2;
		if (strcmp(ident, reswords[k].alf) <= 0)
		    j = k - 1;
		if (strcmp(ident, reswords[k].alf) >= 0)
		    i = k + 1;
	    } while (i <= j);
	    if (i - 1 > j)
		sym = reswords[k].symb;
	    else {
		/*
		 * A reserved word was not recognized, but a special character
		 * was read. In that case, add the character to the word and
		 * try again to recognize a reserved word.
		 */
		if (strchr(specials_repeat, chr))
		    goto again;
		/*
		 * No second special character was read. It is still possible
		 * to build an identifier that starts with one special char.
		 */
einde:		
		if (chr == '_' || isalnum(chr)) {
		    do {
			if (index < identlength)
			    ident[index++] = chr;
			getch();
		    } while (chr == '_' || isalnum(chr));
		}
		ident[index] = 0;
		sym = identifier;
	    }
	}
	break;
    } /* CASE */
} /* getsym */

#ifndef MINJOY
static void check(symbol sy, symset ss, char *er)
{
    if (sym == sy)
	getsym();
    else {
	point('E', er);
	if (P_inset(sym, ss))
	    getsym();
    }
} /* check */

#if defined(MINPAS) || defined(DATBAS)
static void test(symset s1, symset s2, char *er)
{
    symset set;

    if (!P_inset(sym, s1)) {
	point('E', er);
	P_setunion(set, s1, s2);
	if (!P_inset(sym, set)) {
	    do
		getsym();
	    while (!P_inset(sym, set));
	    point('I', "skipped symbols to here");
	}
    }
} /* test */
#endif
#endif

/* - - - - -   MODULE OUTPUT   - - - - - */

#ifndef MINPAS
static void putch(int c)
{
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
    putchar('\n');
    if (writelisting > 0)
	putc('\n', listing);
    outlinelength = 0;
}

static void writeident(char *str)
{
    int i, length;

    length = strlen(str);
    if (outlinelength + length > maxoutlinelength)
	writeline();
    for (i = 0; i < length; i++)
	putch(str[i]);
}

#ifdef DATBAS
static void writeresword(char *str)
{
    int i, length;

    for (length = reslength; length && str[length] <= ' '; length--)
	;
    if (outlinelength + length > maxoutlinelength)
	writeline();
    for (i = 0; i < length; i++)
	putch(str[i]);
}
#endif

static void writenatural(intptr_t n)
{
    if (n >= 10)
	writenatural(n / 10);
    putch(n % 10 + '0');
}

static void writeinteger(intptr_t i)
{
    if (outlinelength + 12 > maxoutlinelength)
	writeline();
    if (i >= 0)
	writenatural(i);
    else {
	putch('-');
	writenatural(-i);
    }
} /* writeinteger */
#endif

static void fin(FILE *f)
{
    double lib;

    if (errorcount > 0)
	fprintf(f, "%ld error(s)\n", errorcount);
    lib = end_clock = clock();
    if ((lib -= start_clock) > 0) {
	lib = lib * 1000 / CLOCKS_PER_SEC;
	fprintf(f, "%.0f milliseconds CPU\n", lib);
    }
}

static void finalise(void)
{
    /* finalise */
    fflush(stdout);
    fin(stderr);
    if (listing && writelisting > 0)
	fin(listing);
    /* finalise */
}

/* End Included file for scan utilities */
