#
#  Makefile for 42minjoy
#
CC = gcc
CFLAGS = -Os -s

demo.txt : joy 42minjoy.lib tutorial.joy
	./joy <tutorial.joy >$@

joy : joy.c
	$(CC) $(CFLAGS) -o$@ $<

clean :
	rm -f joy 42minjoy.lst demo.txt
