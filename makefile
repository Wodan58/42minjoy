#
#   module  : makefile
#   version : 1.5
#   date    : 01/14/25
#
.POSIX:
.SUFFIXES:

# Use CC environment variable
# CC = gcc -pg
CFLAGS = -O3 -Wall -Wextra -Wpedantic -Werror -Wno-old-style-definition -Wno-knr-promoted-parameter
OBJECTS = joy.o gc.o malloc.o free.o setraw.o

joy: $(OBJECTS)
	$(CC) -o$@ $(OBJECTS)

joy.o: scanutil.c

clean:
	rm -f *.o

.SUFFIXES: .c .o

.c.o:
	$(CC) -o$@ $(CFLAGS) -c $<
