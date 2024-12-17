/*
    module  : scanutil.c
    version : 1.19
    date    : 12/14/24
*/
/* scanutil.c: Included file for scan utilities */

#define maxincludelevel	  5
#define maxlinelength	  132
#define linenumwidth	  4

#define linenumspace	  "    "
#define linenumsep	  "    "
#define underliner	  "****    "

#define maxoutlinelength  60
#define messagelength	  30

#define initial_alternative_radix  2

typedef char identalfa[identlength + 1];
typedef char resalfa[reslength + 1];

typedef struct _REC_inputs {
    FILE *fil;
    identalfa nam;
    int lastlinenumber;
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
static int includelevel, adjustment, writelisting;
static boolean must_repeat_line;

static value_t scantimevariables['Z' + 1 - 'A'];
static int alternative_radix, linenumber;
static char line[maxlinelength + 1];
static int cc, ll;
static int chr;
static identalfa ident;
#if defined(MINPAS) || defined(MINJOY)
static standardident id;
#endif
static char specials_repeat[maxrestab + 1];
static symbol sym;
static value_t num;
#ifdef MINJOY
static char *str;
#endif
static _REC_reswords reswords[maxrestab + 1];
static int lastresword;
#if defined(MINPAS) || defined(MINJOY)
static _REC_stdidents stdidents[maxstdidenttab + 1];
#endif
static int laststdident;
#ifdef NETVER
static int trace = 1;
#endif
#ifdef MINJOY
static char stringbuf[maxlinelength + 1];
#endif

static int errorcount, outlinelength, statistics;
static time_t beg_time, end_time;

/* - - - - -   MODULE STRINGS  - - - - - */

static char *stralloc(count)
int count;
{
    char *ptr;

    if ((ptr = my_malloc(count)) == 0)
	point('F', "too many strings");
    return ptr;
}

static char *strsave(str)
char *str;
{
    char *ptr;

    ptr = stralloc(strlen(str) + 2);
    *ptr++ = strunmark;		/* skip one char */
    strcpy(ptr, str);
    return ptr;			/* return user area */
}

static void strfree(ptr)
char *ptr;
{
    if (!*--ptr) {		/* pointer is unmarked */
	*ptr = maxrefcnt;	/* prevent double free */
	my_free(ptr);		/* free pointer */
    }
}

/* - - - - -   MODULE ERROR    - - - - - */

static void point_to_symbol(repeatline, f, diag, mes)
boolean repeatline;
FILE *f;
char diag;
char *mes;
{
    char c;
    int i, j;

    if (repeatline) {
	fprintf(f, "%*d%s", linenumwidth, linenumber, linenumsep);
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

static void point(diag, mes)
char diag;
char *mes;
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
	my_exit(0);
} /* point */

/* - - - - -   MODULE SCANNER  - - - - - */

static void closelisting(VOIDPARM)
{
    if (listing)
	fclose(listing);
    listing = 0;
}

/*
    iniscanner - initialize global variables
*/
static void iniscanner()
{
    beg_time = time(0);
    if ((listing = fopen(list_filename, "w")) == 0) {
	fprintf(stderr, "%s (not open for writing)\n", list_filename);
	my_exit(0);
    }
    my_atexit(closelisting);
    writelisting = 0;
    chr = ' ';
    linenumber = 0;
    cc = 1;
    ll = 1; /* to enable fatal message during initialisation */
    memset(specials_repeat, 0, sizeof(specials_repeat)); /* def: no repeats */
    includelevel = 0;
    /*
     * Initial input file is stdin. This is not stored in the inputs table.
     */
#if 0
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

static void erw(str, symb)
char *str;
symbol symb;
{
    if (++lastresword > maxrestab)
	point('F', "too many reserved words");
    strncpy(reswords[lastresword].alf, str, reslength);
    reswords[lastresword].alf[reslength] = 0;
    reswords[lastresword].symb = symb;
} /* erw */

#if defined(MINPAS) || defined(MINJOY)
static void est(str, symb)
char *str;
standardident symb;
{
    if (++laststdident > maxstdidenttab)
	point('F', "too many identifiers");
    strncpy(stdidents[laststdident].alf, str, identlength);
    stdidents[laststdident].alf[identlength] = 0;
    stdidents[laststdident].symb = symb;
} /* est */
#endif

static void release(VOIDPARM)
{
    int i;

    for (i = 0; i < maxincludelevel; i++)
	if (inputs[i].fil) {
	    fclose(inputs[i].fil);
	    inputs[i].fil = 0;
	}
}

static void newfile(str, flag)
char *str;
int flag;
{
    static unsigned char init;
    FILE *fp;
    char *ptr;

    if (!init) {
	init = 1;
	my_atexit(release);
    }
    if (!flag) {
	if (!freopen(str, "r", stdin)) {
	    fprintf(stderr, "%s (not open for reading)\n", str);
	    my_exit(0);
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
    if ((fp = fopen(str, "r")) == 0) {
	/*
	 * If the include file does not contain a pathname yet, the pathname
	 * is prepended and the open is tried again. If that also fails, the
	 * program fails. Pathname itself is a global variable.
	 */
	if (pathname && ptr == str) {
	    flag = strlen(pathname) + strlen(str) + 1;
	    ptr = stralloc(flag);
	    sprintf(ptr, "%s%s", pathname, str);
	    fp = fopen(ptr, "r");
	}
	if (!fp) {
	    fprintf(stderr, "%s (not open for reading)\n", str);
	    my_exit(0);
	}
    }
    inputs[includelevel].fil = fp;
    adjustment = 1;
} /* newfile */

#define dirlength	16

static void perhapslisting()
{
    int i;

    if (writelisting > 0) {
	fprintf(listing, "%*d%s", linenumwidth, linenumber, linenumsep);
	for (i = 0; i < ll; i++)
	    putc(line[i], listing);
	putc('\n', listing);
	must_repeat_line = false;
	fflush(listing);
    }
}

static void getch()
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
		my_exit(0);
	} else {
	    f = inputs[includelevel - 1].fil;
	    if (fgets(line, maxlinelength, f)) {
		if ((ptr = strchr(line, '\n')) != 0)
		    *ptr = 0;
		ll = strlen(line);
		perhapslisting();
	    } else {
		fclose(f);
		inputs[includelevel - 1].fil = 0;
		adjustment = -1;
	    }
	}
	line[ll++] = ' ';
    } /* IF */
    chr = line[cc++];
} /* getch */

static value_t value()
{
    /* this is a  LL(0) parser */
    value_t result = 0;

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

static void directive()
{
    int i;
    char c, dir[dirlength + 1];

    getch();
    i = 0;
    do {
	if (i < dirlength)
	    dir[i++] = (char)chr;
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
		ident[i++] = (char)chr;
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
	c = (char)chr;
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
	writelisting = (int)value();
	if (!i)
	    perhapslisting();
    } else if (!strcmp(dir, "STATISTICS"))
	statistics = (int)value();
    else if (!strcmp(dir, "RADIX"))
	alternative_radix = (int)value();
    else
	point('F', "unknown directive");
    getch();
} /* directive */

static void getsym()
{
#ifdef MINJOY
    char c;
#endif
    boolean negated;
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

#ifdef MINJOY
    case '"':
	stringbuf[i = 0] = 0;	/* initialize string */
	getch();
	while (chr != '"') {
	    if ((c = chr) == '\\')
		c = value();
	    else
		getch();
	    if (i >= maxlinelength)
		point('F', "too many characters in string");
	    stringbuf[i++] = c;
	}  /* WHILE */
	getch();
	stringbuf[i] = 0;	/* finalize string */
	str = strsave(stringbuf);
	sym = stringconst;
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
	num = 0;
	do {
	    num = num * 10 + chr - '0';
	    getch();
	} while (isdigit(chr));
	if (negated)
	    num = -num;
	sym = numberconst;
	break;

    case '&': /* number in alternative radix */
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
	sym = numberconst;
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
again:	    if (index < identlength)
		ident[index++] = (char)chr;
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
		 * The label "einde" cannot be moved one line further down.
		 */
einde:		if (chr == '_' || isalnum(chr)) {
		    do {
			if (index < identlength)
			    ident[index++] = (char)chr;
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
static void putch(c)
int c;
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

static void writeline()
{
    putchar('\n');
    if (writelisting > 0)
	putc('\n', listing);
    outlinelength = 0;
}

static void writeident(str)
char *str;
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

static void writenatural(n)
value_t n;
{
    if (n >= 10)
	writenatural(n / 10);
    putch((int)(n % 10 + '0'));
}

static void writeinteger(i)
value_t i;
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

static void fin(f)
FILE *f;
{
    time_t c;

    if (errorcount > 0)
	fprintf(f, "%d error(s)\n", errorcount);
    end_time = time(0);
    if ((c = end_time - beg_time) > 0)
	fprintf(f, "%lu seconds CPU\n", (unsigned long)c);
}

static void finalise()
{
    /* finalise */
    fflush(stdout);
    fin(stderr);
    if (listing && writelisting > 0)
	fin(listing);
    /* finalise */
}

/* End Included file for scan utilities */
