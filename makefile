#
#   module  : makefile
#   version : 1.6
#   date    : 02/10/25
#
.POSIX:
.SUFFIXES:

# CC is taken from the environment
CFLAGS = -DMAXMEM=65535 -DNCHECK -O3 -Wall -Wextra -Werror -Wno-old-style-definition -Wno-knr-promoted-parameter
OBJECTS = joy.o setraw.o

joy: $(OBJECTS)
	$(CC) -o$@ $(OBJECTS) -L/usr/local/lib -lgc

joy.o: joy.c scanutil.c

clean:
	rm -f *.o

.SUFFIXES: .c .o

.c.o:
	$(CC) -o$@ $(CFLAGS) -c $<
