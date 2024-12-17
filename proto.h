/*
    module  : proto.h
    version : 1.7
    date    : 12/14/24
*/
#ifndef PROTO_H
#define PROTO_H

#ifdef NPROTO
#define VOIDPARM
#else
#define VOIDPARM	void
#endif

#ifdef NPROTO
static char *stralloc();
static char *strsave();
static void strfree();
static void point_to_symbol();
static int my_atexit();
static void my_exit();
static void point();
static void getsym();
static void newfile();
static void putch();
static void writenatural();
static void fin();
static void erw();
static void est();
static void wn();
static void writenode();
static void mark();
static void writeident();
static void writeinteger();
static memrange kons();
static void readterm();
static void readfactor();
static void writefactor();
static void writeterm();
static void patchfactor();
static void patchterm();
static memrange ok();
static unsigned char o();
static long v();
static memrange n();
static memrange l();
static boolean b();
static void binary();
static long get_i();
static void joy();
static void readlibrary();
static void writestatistics();
static void perhapsstatistics();
#else
#ifdef MINIX
static void point_to_symbol(int repeatline, FILE *f, int diag, char *mes);
static void point(int diag, char *mes);
static void writenatural(long n);
static void wn(FILE *f, int n);
static void writenode(int n);
static void mark(int n);
static void writeinteger(long i);
static memrange kons(standardident o, long v, int n);
static void writefactor(int n, int nl);
static void writeterm(int n, int nl);
static void patchfactor(int n);
static void patchterm(int n);
static memrange ok(int x);
static unsigned char o(int x);
static long v(int x);
static memrange n(int x);
static memrange l(int x);
static boolean b(int x);
static void binary(standardident o, long v);
static long get_i(int x);
static void joy(int nod);
#else
static void point_to_symbol(boolean repeatline, FILE *f, char diag, char *mes);
static void point(char diag, char *mes);
static void writenatural(value_t n);
static void wn(FILE *f, memrange n);
static void writenode(memrange n);
static void mark(memrange n);
static void writeinteger(value_t i);
static memrange kons(standardident o, value_t v, memrange n);
static void writefactor(memrange n, boolean nl);
static void writeterm(memrange n, boolean nl);
static void patchfactor(memrange n);
static void patchterm(memrange n);
static memrange ok(memrange x);
static unsigned char o(memrange x);
static value_t v(memrange x);
static memrange n(memrange x);
static memrange l(memrange x);
static boolean b(memrange x);
static void binary(standardident o, value_t v);
static value_t get_i(memrange x);
static void joy(memrange nod);
#endif
static char *stralloc(int count);
static char *strsave(char *str);
static void strfree(char *ptr);
static int my_atexit(void (*proc)(void));
static void my_exit(int code);
static void getsym(void);
static void newfile(char *str, int flag);
static void putch(int c);
static void fin(FILE *f);
static void erw(char *str, symbol symb);
static void est(char *str, standardident symb);
static void writeident(char *str);
static void readterm(memrange *first);
static void readfactor(memrange *where);
static void readlibrary(char *str);
static void writestatistics(FILE *f);
static void perhapsstatistics(void);
#endif
#endif
