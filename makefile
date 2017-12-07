#
#  Makefile for 42minjoy
#
CC = gcc
CFLAGS = -Os -s

tutorial.out : joy.exe 42minjoy.lib tutorial.joy
	./joy.exe <tutorial.joy >$@

joy.exe : joy.c
	$(CC) $(CFLAGS) -o$@ $<

clean :
	rm -f joy.exe 42minjoy.lst tutorial.out
