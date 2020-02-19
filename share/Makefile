#############################################################################
# Cittadella/UX library                  (C) M. Caldarelli and R. Vianello  #
#                                                                           #
# File : Makefile                                                           #
# Targets : all, libcitta.a, clean, mrproper                                #
#                                                                           #
#############################################################################

SHELL = /bin/sh
.SUFFIXES:
.SUFFIXES: .c .o

# Compiler name
CC = gcc

# Directories
srcdir = .
libdir = ../lib

# Compiler flags you want to use (other than profiling, such as -Wall)
# Flag DEBUG per tenere le parti di codice dedicate al debugging
MYFLAGS = -g -Wall -DDEBUG

# Flags for profiling (see hacker.doc for more information)
#PROFILE = -pg
PROFILE =

#remove the hash marks below if compiling under AIX
#CC = cc
#COMMFLAGS = -D_BSD

#remove the hash mark below if compiling under SVR4
#OSFLAGS = -DSVR4
#LIBS = -lsocket -lnsl

#remove the has mark below if compiling under IRIX
#LIBS = -lmalloc

#se le routines di <crypt.h> sono disponibili, togliere il cancelletto
#CRYPT  = -DCRYPT
#LCRYPT = -lcrypt

#############################################################################

CFLAGS = $(MYFLAGS) $(CRYPT) $(PROFILE) $(OSFLAGS)
LFLAGS = $(LCRYPT)

OBJFILES = argz.o cache.o cml2html.o cml_entities.o extract.o hash.o histo.o \
	   metadata.o prefixtree.o terminale.o text.o versione.o

all : libcitta.a

libcitta.a : $(OBJFILES)
	ar r libcitta.a $(OBJFILES)
ifeq ($(OSFLAGS),-DMACOSX)
	ranlib libcitta.a
endif
argz.o : argz.c argz.h
	$(CC) -c $(CFLAGS) argz.c
cache.o : cache.c cache.h
	$(CC) -c $(CFLAGS) cache.c
cml2html.o : cml2html.c cml2html.h
	$(CC) -c $(CFLAGS) cml2html.c
cml_entities.o : cml_entities.c cml_entities.h
	$(CC) -c $(CFLAGS) cml_entities.c
extract.o : extract.c extract.h
	$(CC) -c $(CFLAGS) extract.c
hash.o : hash.c hash.h
	$(CC) -c $(CFLAGS) hash.c
histo.o : histo.c histo.h
	$(CC) -c $(CFLAGS) histo.c
metadata.o : metadata.c metadata.h
	$(CC) -c $(CFLAGS) metadata.c
prefixtree.o : prefixtree.c prefixtree.h
	$(CC) -c $(CFLAGS) prefixtree.c
terminale.o : terminale.c terminale.h
	$(CC) -c $(CFLAGS) terminale.c
text.o : text.c text.h
	$(CC) -c $(CFLAGS) text.c
versione.o : versione.c versione.h
	$(CC) -c $(CFLAGS) versione.c

.PHONY : clean mrproper

clean :
	-rm -f $(OBJFILES) *~

mrproper : clean
	-rm -f libcitta.a
