# $Copyright: $
# Copyright (c) 1996 - 2004 by Steve Baker
# All Rights reserved
#
# This software is provided as is without any express or implied
# warranties, including, without limitation, the implied warranties
# of merchant-ability and fitness for a particular purpose.

CC=gcc
#CFLAGS=-ggdb -Wall -DLINUX_BIGFILE
CFLAGS=-O2 -Wall -fomit-frame-pointer -DLINUX_BIGFILE
LDFLAGS=-s

# Uncomment for FreeBSD:
#CC=gcc
#CFLAGS=-O2 -Wall -fomit-frame-pointer
#LDFLAGS=-s

# Uncomment for OS X:
#CC=cc
#CFLAGS=-O2 -Wall -fomit-frame-pointer -no-cpp-precomp
#LDFLAGS=

# Uncomment for HP/UX:
#CC=cc
#CFLAGS=-Ae +O2 +DAportable -Wall
#LDFLAGS=

# Uncomment for OS/2:
#CC=gcc
#CFLAGS=-02 -Wall -fomit-frame-pointer -Zomf -Zsmall-conv
#LDFLAGS=-s -Zomf -Zsmall-conv

prefix = /usr/local

VERSION=1.5.0
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
