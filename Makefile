
VERSION=0.1

all:
	gcc -Wall -O2 -o autocutsel autocutsel.c -I/usr/X11R6/include -L/usr/X11R6/lib -lXext -lX11 -lXt -lXmu -lXaw

install: autocutsel
	cp autocutsel /usr/X11R6/bin

dist:
	rm -fr autocutsel-$(VERSION)
	mkdir autocutsel-$(VERSION)
	cp COPYING README ChangeLog TODO Makefile autocutsel.c autocutsel-$(VERSION)
	tar -cvzf autocutsel-$(VERSION).tar.gz autocutsel-$(VERSION)


	