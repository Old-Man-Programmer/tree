# $Copyright: $
# Copyright (c) 1996 - 1999 by Steve Baker
# All Rights reserved
#
# This software is provided as is without any express or implied
# warranties, including, without limitation, the implied warranties
# of merchant-ability and fitness for a particular purpose.

CC=gcc
CFLAGS=-O2 -Wall -fomit-frame-pointer #-m486
LDFLAGS=-s

VERSION=1.3
TREE_DEST=tree
BINDIR=/usr/local/bin
MAN=tree.1
MANDIR=/usr/man/man1

all:	tree

tree:	tree.o
	$(CC) $(LDFLAGS) -o $(TREE_DEST) tree.o

tree.o:	tree.c

clean:
	if [ -e $(TREE_DEST) ]; then rm $(TREE_DEST); fi
	if [ -e tree.o ]; then rm *.o; fi
	rm -f *~

install:
	install -d $(BINDIR) $(MANDIR)
	if [ -e $(TREE_DEST) ]; then \
		install -s $(TREE_DEST) $(BINDIR)/$(TREE_DEST); \
	fi
	install $(MAN) $(MANDIR)/$(MAN)

distclean:
	if [ -e tree.o ]; then rm *.o; fi
	rm -f *~
	

dist:	distclean
	tar zcf ../tree-$(VERSION).tgz -C .. tree-$(VERSION)/
