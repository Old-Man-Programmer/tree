# $Copyright: $
# Copyright (c) 1996 - 2009 by Steve Baker
# All Rights reserved
#
# This software is provided as is without any express or implied
# warranties, including, without limitation, the implied warranties
# of merchant-ability and fitness for a particular purpose.

prefix = /usr

CC=gcc

VERSION=1.5.3
TREE_DEST=tree
BINDIR=${prefix}/bin
MAN=tree.1
MANDIR=${prefix}/man/man1

# Uncomment options below for your particular OS:

# Linux defaults:
CFLAGS=-ggdb -Wall -DLINUX -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
#CFLAGS=-O2 -Wall -fomit-frame-pointer -DLINUX -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
#LDFLAGS=-s

# Uncomment for FreeBSD:
#CFLAGS=-O2 -Wall -fomit-frame-pointer
#LDFLAGS=-s
#XOBJS=strverscmp.o

# Uncomment for Solaris:
#CC=cc
#CFLAGS=-xO0 -v
#LDFLAGS=
#XOBJS=strverscmp.o
#MANDIR=${prefix}/share/man/man1

# Uncomment for Cygwin:
#CFLAGS=-O2 -Wall -fomit-frame-pointer -DCYGWIN
#LDFLAGS=-s
#TREE_DEST=tree.exe
#XOBJS=strverscmp.o

# Uncomment for OS X:
#CC=cc
#CFLAGS=-O2 -Wall -fomit-frame-pointer -no-cpp-precomp
#LDFLAGS=
#XOBJS=strverscmp.o

# Uncomment for HP/UX:
#CC=cc
#CFLAGS=-Ae +O2 +DAportable -Wall
#LDFLAGS=
#XOBJS=strverscmp.o

# Uncomment for OS/2:
#CFLAGS=-02 -Wall -fomit-frame-pointer -Zomf -Zsmall-conv
#LDFLAGS=-s -Zomf -Zsmall-conv
#XOBJS=strverscmp.o

# Uncomment for HP NonStop:
#prefix = /opt
#CC=c89
#CFLAGS=-Wextensions -WIEEE_float -g -Wnowarn=1506 -D_XOPEN_SOURCE_EXTENDED=1 \
#	 -Wallow_cplusplus_comments
#LDFLAGS=


#------------------------------------------------------------

all:	tree

tree:	tree.o $(XOBJS)
	$(CC) $(LDFLAGS) -o $(TREE_DEST) tree.o $(XOBJS)

tree.o:	tree.c

strverscmp.o:	strverscmp.c

clean:
	if [ -x $(TREE_DEST) ]; then rm $(TREE_DEST); fi
	if [ -f tree.o ]; then rm *.o; fi
	rm -f *~

install: tree
	install -d $(BINDIR)
	install -d $(MANDIR)
	if [ -e $(TREE_DEST) ]; then \
		install -s $(TREE_DEST) $(BINDIR)/$(TREE_DEST); \
	fi
	install man/$(MAN) $(MANDIR)/$(MAN)

distclean:
	if [ -f tree.o ]; then rm *.o; fi
	rm -f *~
	

dist:	distclean
	tar zcf ../tree-$(VERSION).tgz -C .. `cat .tarball`
