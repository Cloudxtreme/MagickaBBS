# $Id: Makefile,v 1.2 2001/10/25 23:56:29 efalk Exp $

INC = -I..

#OPT = -g
OPT = -O

CFLAGS = $(OPT) $(INC) -DHAVE_STRDUP

AR = ar
RANLIB = ranlib

HDRS =	crctab.h seriallog.h xmodem.h zmodem.h

SRCS = crctab.c seriallog.c zmodem.c zmodemr.c zmodemsys.c zmodemt.c zmutil.c

OBJS = $(SRCS:.c=.o)

libzmodem.a: $(OBJS)
	-rm -f libzmodem.a
	$(AR) cru libzmodem.a $(OBJS)
	$(RANLIB) libzmodem.a

clean:
	rm -f *.o *.s *.i

clobber: clean
	rm -f *.a tags

tags: $(SRCS) $(HDRS)
	ctags *.[ch]

.SUFFIXES: .i .s

.c.i:
	$(CC) -E $(CFLAGS) $*.c > $@

.c.s:
	$(CC) -S $(CFLAGS) $*.c

depend:
	makedepend -- $(CFLAGS) -- $(SRCS)
