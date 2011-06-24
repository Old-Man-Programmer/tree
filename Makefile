# $Copyright: $
# Copyright (c) 1996 - 2011 by Steve Baker
# All Rights reserved
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

prefix = /usr

CC=gcc

VERSION=1.6.0
TREE_DEST=tree
BINDIR=${prefix}/bin
MAN=tree.1
MANDIR=${prefix}/man/man1
OBJS=tree.o unix.o html.o xml.o hash.o color.o

# Uncomment options below for your particular OS:

# Linux defaults:
#CFLAGS=-ggdb -Wall -DLINUX -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS=-O4 -Wall  -DLINUX -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
LDFLAGS=-s

# Uncomment for FreeBSD:
#CFLAGS=-O2 -Wall -fomit-frame-pointer
#LDFLAGS=-s
#OBJS+=strverscmp.o

# Uncomment for Solaris:
#CC=cc
#CFLAGS=-xO0 -v
#LDFLAGS=
#OBJS+=strverscmp.o
#MANDIR=${prefix}/share/man/man1

# Uncomment for Cygwin:
#CFLAGS=-O2 -Wall -fomit-frame-pointer -DCYGWIN
#LDFLAGS=-s
#TREE_DEST=tree.exe
#OBJS+=strverscmp.o

# Uncomment for OS X:
#CC=cc
#CFLAGS=-O2 -Wall -fomit-frame-pointer -no-cpp-precomp
#LDFLAGS=
#OBJS+=strverscmp.o

# Uncomment for HP/UX:
#CC=cc
#CFLAGS=-O2 -DAportable -Wall
#LDFLAGS=
#OBJS+=strverscmp.o

# Uncomment for OS/2:
#CFLAGS=-02 -Wall -fomit-frame-pointer -Zomf -Zsmall-conv
#LDFLAGS=-s -Zomf -Zsmall-conv
#OBJS+=strverscmp.o

# Uncomment for HP NonStop:
#prefix = /opt
#CC=c89
#CFLAGS=-Wextensions -WIEEE_float -g -Wnowarn=1506 -D_XOPEN_SOURCE_EXTENDED=1 \
#	 -Wallow_cplusplus_comments
#LDFLAGS=
#OBJS+=strverscmp.o


#------------------------------------------------------------

all:	tree

tree:	$(OBJS)
	$(CC) $(LDFLAGS) -o $(TREE_DEST) $(OBJS)

$(OBJS): %.o:	%.c tree.h
	$(CC) $(CFLAGS) -c -o $@ $<

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
	install doc/$(MAN) $(MANDIR)/$(MAN)

distclean:
	if [ -f tree.o ]; then rm *.o; fi
	rm -f *~
	

dist:	distclean
	tar zcf ../tree-$(VERSION).tgz -C .. `cat .tarball`
