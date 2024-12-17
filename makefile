#
#   module  : makefile
#   version : 1.4
#   date    : 12/14/24
#
.POSIX:
.SUFFIXES:

# Use CC environment variable
# CC = gcc -pg
CFLAGS = -O3 -Wall -Wextra -Wpedantic -Werror -Wno-unused-parameter -Wno-old-style-definition -Wno-knr-promoted-parameter
OBJECTS = joy.o malloc.o free.o

joy: $(OBJECTS)
	$(CC) -o$@ $(OBJECTS)

joy.o: scanutil.c

clean:
	rm -f *.o

.SUFFIXES: .c .o

.c.o:
	$(CC) -o$@ $(CFLAGS) -c $<
