# $Copyright: $
# Copyright (c) 1996 - 2001 by Steve Baker
# All Rights reserved
#
# This software is provided as is without any express or implied
# warranties, including, without limitation, the implied warranties
# of merchant-ability and fitness for a particular purpose.

CC=gcc
CFLAGS=-O2 -Wall -fomit-frame-pointer #-m486
LDFLAGS=-s
# Uncomment for OS X:
#CC=cc
#CFLAGS=-O2 -Wall -fomit-frame-pointer -no-cpp-precomp
#LDFLAGS=

prefix = /usr/local

VERSION=1.4
TREE_DEST=tree
BINDIR=${prefix}/bin
MAN=tree.1
MANDIR=${prefix}/man/man1

all:	tree

tree:	tree.o
	$(CC) $(LDFLAGS) -o $(TREE_DEST) tree.o

tree.o:	tree.c

clean:
	if [ -x $(TREE_DEST) ]; then rm $(TREE_DEST); fi
	if [ -f tree.o ]; then rm *.o; fi
	rm -f *~

install:
	install -d $(BINDIR)
	install -d $(MANDIR)
	if [ -e $(TREE_DEST) ]; then \
		install -s $(TREE_DEST) $(BINDIR)/$(TREE_DEST); \
	fi
	install $(MAN) $(MANDIR)/$(MAN)

distclean:
	if [ -f tree.o ]; then rm *.o; fi
	rm -f *~
	

dist:	distclean
	tar zcf ../tree-$(VERSION).tgz -C .. `cat .tarball`
