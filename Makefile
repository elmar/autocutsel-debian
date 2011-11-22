

# On IRIX:
# cc  -O2 -o autocutsel autocutsel.c -I/usr/include/X11 -L/usr/lib/X11 -lXext
# -lX11 -lXt -lXmu -lXaw

PREFIX  = /usr/X11R6/bin
CC      = gcc
INSTALL = install
LDFLAGS = -L/usr/X11R6/lib -lXext -lX11 -lXt -lXmu -lXaw
CFLAGS  = -Wall -O2 -I/usr/X11R6/include
DISTRIB = COPYING README ChangeLog TODO Makefile autocutsel.c
VERSION=0.2

autocutsel: autocutsel.o
	$(CC) -o autocutsel autocutsel.o $(LDFLAGS)

autocutsel.o: autocutsel.c
	$(CC) -c autocutsel.c $(CFLAGS)

clean:
	rm -f autocutsel autocutsel.o

install: autocutsel
	$(INSTALL) -m 0755 autocutsel $(PREFIX)

dist:
	rm -fr autocutsel-$(VERSION)
	mkdir autocutsel-$(VERSION)
	cp $(DISTRIB) autocutsel-$(VERSION)
	tar -cvzf autocutsel-$(VERSION).tar.gz autocutsel-$(VERSION)

