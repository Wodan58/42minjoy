/*
    module  : scanutil.c
    version : 1.21
    date    : 02/10/25
*/
/* scanutil.c: Included file for scan utilities */

#define maxincludelevel	  5
#define maxlinelength	  361	/* used to be 132 */
#define linenumwidth	  4

#define linenumspace	  "    "
#define linenumsep	  "    "
#define underliner	  "****    "

#define maxoutlinelength  60
#define messagelength	  30
#define dirlength	  10	/* used to be 16 */

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

static FILE *srcfile, *listing;

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
static _REC_reswords reswords[maxrestab + 1];
static int lastresword;
#if defined(MINPAS) || defined(MINJOY)
static _REC_stdidents stdidents[maxstdidenttab + 1];
#endif
static int laststdident;
#if defined(NETVER) || defined(MINJOY)
static int g_trace;
#endif
#ifdef MINJOY
static char stringbuf[maxlinelength + 1];
#endif

static int errorcount, outlinelength, statistics;
static time_t beg_time, end_time;

/* - - - - -   MODULE ERROR    - - - - - */

static void point_to_symbol(repeatline, f, diag, mes)
boolean repeatline;
FILE *f;
char diag;
char *mes;
{
    char c;
    int j, k;

    if (repeatline) {
	fprintf(f, "\n%*d%s", linenumwidth, linenumber, linenumsep);
	for (j = 0; j < ll; j++)
	    putc(line[j], f);
	putc('\n', f);
    }
    fputs(underliner, f);
    for (j = 0, k = cc - 2; j < k; j++)
	if ((c = line[j]) < ' ')
	    putc(c, f);
	else
	    putc(' ', f);
    fprintf(f, "^\n");
    fprintf(f, "%s-%c  %-*.*s\n", errormark, diag, messagelength,
	messagelength, mes);
    if (diag == 'F')
	fprintf(f, "execution aborted\n");
} /* point_to_symbol */

void point(diag, mes)
int diag;
char *mes;
{
    if (diag != 'I')
	errorcount++;
    if (includelevel)
	printf("INCLUDE file : \"%-*.*s\"\n", identlength, identlength,
	    inputs[includelevel - 1].nam);
    point_to_symbol(true, stdout, diag, mes);
    if (writelisting) {
	point_to_symbol(must_repeat_line, listing, diag, mes);
	must_repeat_line = true;
    }
    if (diag == 'F')
	my_exit(2);	/* fatal error */
} /* point */

/* - - - - -   MODULE SCANNER  - - - - - */

/*
    iniscanner - initialize global variables
*/
static void iniscanner()
{
    time(&beg_time);
    /*
     * Initial input file is stdin, unless a filename parameter is present.
     * In that case input comes from that file. Neither stdin nor the input
     * file is stored in the inputs table.
     */
    srcfile = stdin;
    if ((listing = fopen(list_filename, "w")) == 0) {
	fprintf(stderr, "%s (not open for writing)\n", list_filename);
	my_exit(2);	/* fatal error */
    }
    chr = ' ';
    cc = 1;
    ll = 1; /* to enable fatal message during initialisation */
    alternative_radix = initial_alternative_radix;
} /* iniscanner */

static void erw(str, symb)
char *str;
symbol symb;
{
    if (++lastresword > maxrestab)
	point('F', "too many reserved words");
    strncpy(reswords[lastresword].alf, str, reslength);
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
    stdidents[laststdident].symb = symb;
} /* est */
#endif

static void newfile(str, flag)
char *str;
int flag;
{
    FILE *fp;
    char *ptr;

    if (!flag) {
	if ((srcfile = fopen(str, "r")) == 0) {
	    fprintf(stderr, "%s (not open for reading)\n", str);
	    my_exit(2);		/* fatal error */
	}
	adjustment = 0;
	return;
    }
    /*
     * Str may contain a pathname. The pathname must be stripped, because
     * identlength may not be large enough to hold a pathname. The name is
     * only used in error messages and does not need a pathname.
     */
    if ((ptr = strrchr(str, '/')) != 0)
	ptr++;
    else
	ptr = str;
    strncpy(inputs[includelevel].nam, ptr, identlength);
    inputs[includelevel].lastlinenumber = linenumber;
    /*
     * Try to open the file. If that fails, it is tried again, with pathname
     * prepended.
     */
    if ((fp = fopen(str, "r")) == 0) {
	/*
	 * The pathname is prepended and the open is tried again. If that also
	 * fails, the program fails. Pathname itself is a global variable.
	 */
	if (pathname) {
	    flag = strlen(pathname) + strlen(str) + 1;
	    ptr = (char *)GC_malloc_atomic(flag);
	    sprintf(ptr, "%s%s", pathname, str);
	    fp = fopen(ptr, "r");
	    GC_free(ptr);
	}
	if (!fp) {
	    fprintf(stderr, "%s (not open for reading)\n", str);
	    my_exit(2);		/* fatal error */
	}
    }
    inputs[includelevel].fil = fp;
    adjustment = 1;
} /* newfile */

static void perhapslisting()
{
    int j;

    if (writelisting) {
	fprintf(listing, "%*d%s", linenumwidth, linenumber, linenumsep);
	for (j = 0; j < ll; j++)
	    putc(line[j], listing);
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
	    if (fgets(line, maxlinelength, srcfile)) {
		if ((ptr = strchr(line, '\n')) != 0)
		    *ptr = 0;
		ll = strlen(line);
		perhapslisting();
	    } else
		my_exit(2);	/* fatal error */
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
    if (chr == '"') {
	result = chr;
	getch();
	goto einde;
    }	
    if (chr == '\'' || chr == '&' || isdigit(chr)) {
	getsym();
	result = num;
	goto einde;
    }
    if (isupper(chr)) {
	result = scantimevariables[chr - 'A'];
	getsym();
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
    int j = 0;
    char c, dir[dirlength + 1];

    getch();
    do {
	if (j < dirlength)
	    dir[j++] = (char)chr;
	getch();
    } while (chr == '_' || isupper(chr));
    dir[j] = 0;
    if (!strcmp(dir, "IF")) {
	if (value() < 1)
	    cc = ll; /* readln */
    } else if (!strcmp(dir, "INCLUDE")) {
	if (includelevel == maxincludelevel)
	    point('F', "too many include files");
	while (chr <= ' ')
	    getch();
	j = 0;
	do {
	    if (j < identlength)
		ident[j++] = (char)chr;
	    getch();
	} while (chr > ' ');
	ident[j] = 0;
	newfile(ident, 1);
    } else if (!strcmp(dir, "PUT")) {
	fflush(stdout);
	for (j = cc - 1; j < ll; j++)
	    fputc(line[j], stderr);
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
    } else if (!strcmp(dir, "TRACE")) {
	g_trace = value();
    } else if (!strcmp(dir, "LISTING")) {
	j = writelisting;
	writelisting = (int)value();
	if (!j)
	    perhapslisting();
    } else if (!strcmp(dir, "STATISTICS"))
	statistics = (int)value();
    else if (!strcmp(dir, "RADIX"))
	alternative_radix = (int)value();
    else if (!strcmp(dir, "SETRAW"))
	SetRaw();
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
    int j, k, locat, index, result;

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
	stringbuf[j = 0] = 0;	/* initialize string */
	getch();
	while (chr != '"') {
	    if ((c = chr) == '\\')
		c = value();
	    else
		getch();
	    if (j >= maxlinelength)
		point('F', "too many chars in string");
	    stringbuf[j++] = c;
	}  /* WHILE */
	getch();
	stringbuf[j] = 0;	/* finalize string */
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
	    j = result = 1;
	    k = lastresword;
	    while (k >= j) {
		locat = j + (k - j) / 2;
		if ((result = strcmp(ident, reswords[locat].alf)) == 0)
		    break;
		if (result < 0)
		    k = locat - 1;
		else
		    j = locat + 1;
	    }
	    if (!result)
		sym = reswords[locat].symb;
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
		 *
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
    if (writelisting) {
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
    if (writelisting)
	putc('\n', listing);
    outlinelength = 0;
}

static void writeident(str)
char *str;
{
    int j, length;

    length = strlen(str);
    if (outlinelength + length > maxoutlinelength)
	writeline();
    for (j = 0; j < length; j++)
	putch(str[j]);
}

#ifdef DATBAS
static void writeresword(char *str)
{
    int j, length;

    for (length = reslength; length && str[length] <= ' '; length--)
	;
    if (outlinelength + length > maxoutlinelength)
	writeline();
    for (j = 0; j < length; j++)
	putch(str[j]);
}
#endif

static void writenatural(j)
value_t j;
{
    if (j >= 10)
	writenatural(j / 10);
    putch((int)(j % 10 + '0'));
}

static void writeinteger(j)
value_t j;
{
    if (outlinelength + 19 > maxoutlinelength)
	writeline();
    if (j >= 0)
	writenatural(j);
    else {
	putch('-');
	writenatural(-j);
    }
} /* writeinteger */
#endif

static void fin(f)
FILE *f;
{
    time_t c;

    if (errorcount)
	fprintf(f, "%d error(s)\n", errorcount);
    time(&end_time);
    if ((c = end_time - beg_time) > 0)
	fprintf(f, "%lu seconds CPU\n", (unsigned long)c);
}

static void finalise()
{
    /* finalise */
    fflush(stdout);
    fin(stderr);
    if (listing && writelisting)
	fin(listing);
    /* finalise */
}

/* End Included file for scan utilities */
