/*
    module  : proto.h
    version : 1.9
    date    : 02/10/25
*/
#ifndef PROTO_H
#define PROTO_H

/*
 * setraw.c
 */
void SetRaw();
void SetNormal();

#ifdef NPROTO
static void my_exit();
static void getsym();
static void readterm();
static void writefactor();
static void patchfactor();
static void joy();
static void printfactor();
static void perhapsstatistics();
static int is_equal();
#ifndef NCHECK
static memrange ok();
static unsigned char o();
static long v();
static memrange n();
static memrange l();
static boolean b();
static long i();
#endif
#else
#ifdef MINIX
static void writefactor(int node, int nl);
static void patchfactor(int n);
static void printfactor(int n, FILE *fp);
static int is_equal(int one, int two);
#ifndef NCHECK
static memrange ok(int x);
static unsigned char o(int x);
static long v(int x);
static memrange n(int x);
static memrange l(int x);
static boolean b(int x);
static long i(int x);
#endif
static void joy(int nod);
#else
static void writefactor(memrange node, boolean nl);
static void patchfactor(memrange n);
static void printfactor(memrange n, FILE *fp);
static int is_equal(memrange one, memrange two);
#ifndef NCHECK
static memrange ok(memrange x);
static unsigned char o(memrange x);
static value_t v(memrange x);
static memrange n(memrange x);
static memrange l(memrange x);
static boolean b(memrange x);
static value_t i(memrange x);
#endif
static void joy(memrange nod);
#endif
static void my_exit(int code);
static void getsym(void);
static void readterm(memrange *first);
static void perhapsstatistics(void);
#endif
#endif
