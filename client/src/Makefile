#############################################################################
# Cittadella/UX client                   (C) M. Caldarelli and R. Vianello  #
#                                                                           #
# File : Makefile                                                           #
# Targets : all, cittaclient, remote_cittaclient, debug_cittaclient,        #
#           clean, mrproper                                                 #
#                                                                           #
#############################################################################

SHELL = /bin/sh

#compiler name
CC = gcc

PROGRAM = cittaclient
VERSION = 0.3

prefix = /usr/local
bindir = ../bin
libdir = ../../share
mandir = $(prefix)/man
man6dir = $(mandir)/man6

VPATH = $(libdir)


#WFLAGS = -ansi -W -Wtraditional -Wshadow -Wpointer-arith \

WFLAGS = -ansi -pedantic -W -Wtraditional -Wshadow -Wpointer-arith \
	 -Wcast-qual -Wcast-align -Wwrite-strings -Wconversion \
         -Waggregate-return -Wstrict-prototypes -Wmissing-prototypes \
         -Wmissing-declarations -Wnested-externs -Winline \
         -Wunused-but-set-variable

#compiler flags you want to use (other than profiling, such as -Wall)
# Flag DEBUG per tenere le parti di codice dedicate al debugging
# Se definisco UTMPLIB uso utmp.h, altrimentu utmpx.h (sono equivalenti)
# Aggiungere -DLOGINPORT per attivare il login dal demone in.cittad
# (sperimentale)
#MYFLAGS = -g -Wall -pedantic -Wstrict-prototypes -Wmissing-prototypes \
#		  -Wconversion
#MYFLAGS = -Wall -DUTMPLIB -O3 -fomit-frame-pointer
MYFLAGS = -g -pedantic -Wall -Wextra -DUTMPLIB $(TESTFLAGS)

#flags for profiling (see hacker.doc for more information)
#PROFILE = -pg
PROFILE =

# Flags for linking with libefence
#EFENCE = -lefence

#remove the hash marks below if compiling under AIX
#CC = cc
#COMMFLAGS = -D_BSD

#remove the hash mark below if compiling under SVR4
#OSFLAGS = -DSVR4
#LIBS = -lsocket -lnsl

#remove the hash mark below if compiling under IRIX
#LIBS = -lmalloc

#remove the hash mark below if compiling with compression support enabled
LZ = -lz

#############################################################################
CFLAGS = $(MYFLAGS) $(PROFILE) $(OSFLAGS) $(BGCOL)
override CFLAGS += $(patsubst %,-I%,$(subst :, ,$(VPATH)))
LFLAGS = -L$(libdir) -lcitta -lncurses $(LIBS)

COMMONOBJ = ansicol.o aide.o blog_cmd.o cml.o configurazione.o cterminfo.o \
	    edit.o editor.o errore.o floor_cmd.o friends.o generic_cmd.o \
	    inkey.o login.o messaggi.o metadata_ops.o name_list.o \
	    pager.o parm.o prompt.o room_cmd.o signals.o sysconf_client.o \
	    sysop.o tabc.o urna-crea.o urna-gestione.o urna-leggi.o \
	    urna-servizio.o urna-vota.o utente_client.o utility.o memkeep.o

LOCALOBJ = cittaclient.o cittacfg.o cmd_da_server.o comandi.o comunicaz.o \
		conn.o decompress.o local.o

REMOTEOBJ = decompress_fake.o rcittaclient.o rcittacfg.o rcmd_da_server.o rcomandi.o \
		rcomunicaz.o rconn.o remote.o

DEBUGOBJ = cittaclient.o cittacfg.o dcmd_da_server.o comandi.o comunicaz.o \
		dconn.o decompress_fake.o local.o

OBJFILES = $(COMMONOBJ) $(LOCALOBJ)

OBJFILES_R = $(COMMONOBJ) $(REMOTEOBJ)

OBJFILES_D = $(COMMONOBJ) $(DEBUGOBJ)

OBJALL = $(COMMONOBJ) $(LOCALOBJ) $(REMOTEOBJ) $(DEBUGOBJ)

all : $(bindir)/cittaclient $(bindir)/remote_cittaclient \
	$(bindir)/debug_cittaclient

$(bindir) :
	mkdir $(bindir)

$(bindir)/cittaclient : $(OBJFILES) $(bindir)
	$(CC) $(CFLAGS) $(OBJFILES) $(LFLAGS) $(LZ) $(EFENCE) \
		-o $(bindir)/cittaclient

$(bindir)/remote_cittaclient : $(OBJFILES_R)
	$(CC) $(CFLAGS) $(OBJFILES_R) $(LFLAGS) -o $(bindir)/remote_cittaclient

$(bindir)/debug_cittaclient : $(OBJFILES_D)
	$(CC) $(CFLAGS) $(OBJFILES_D) $(LFLAGS) -o $(bindir)/debug_cittaclient

cfg_test:  cittacfg.c cittacfg.h
	$(CC) $(CFLAGS) -DCFG_TEST cittacfg.c -o $(bindir)/cfg_test

cittaclient.o : cittaclient.c cittaclient.h versione.h
	$(CC) -c $(CFLAGS) $(COMMFLAGS) -DLOCAL cittaclient.c
rcittaclient.o : cittaclient.c cittaclient.h versione.h
	$(CC) -c $(CFLAGS) $(COMMFLAGS) cittaclient.c -o rcittaclient.o
conn.o : conn.c
	$(CC) -c $(CFLAGS) -DLOCAL conn.c
dconn.o : conn.c
	$(CC) -c $(CFLAGS) -DDEBUG conn.c -o dconn.o
rconn.o : conn.c
	$(CC) -c $(CFLAGS) conn.c -o rconn.o
cmd_da_server.o :cmd_da_server.c
	$(CC) -c $(CFLAGS) -DLOCAL cmd_da_server.c
rcmd_da_server.o :cmd_da_server.c
	$(CC) -c $(CFLAGS) cmd_da_server.c -o rcmd_da_server.o
dcmd_da_server.o :cmd_da_server.c
	$(CC) -c $(CFLAGS) -DDEBUG cmd_da_server.c -o dcmd_da_server.o
cittacfg.o : cittacfg.c cittacfg.h
	$(CC) -c $(CFLAGS) -DLOCAL cittacfg.c
rcittacfg.o : cittacfg.c cittacfg.h
	$(CC) -c $(CFLAGS) cittacfg.c -o rcittacfg.o
comandi.o : comandi.c
	$(CC) -c $(CFLAGS) -DLOCAL comandi.c
rcomandi.o : comandi.c
	$(CC) -c $(CFLAGS) comandi.c -o rcomandi.o
comunicaz.o : comunicaz.c
	$(CC) -c $(CFLAGS) -DLOCAL comunicaz.c
rcomunicaz.o : comunicaz.c
	$(CC) -c $(CFLAGS) comunicaz.c -o rcomunicaz.o

aide.o : aide.c
	$(CC) -c $(CFLAGS) aide.c
ansicol.o : ansicol.c
	$(CC) -c $(CFLAGS) ansicol.c
blog_cmd.o : blog_cmd.c blog_cmd.h
	$(CC) -c $(CFLAGS) blog_cmd.c
cml.o : cml.c cml.h
	$(CC) -c $(CFLAGS) cml.c
configurazione.o : configurazione.c configurazione.h
	$(CC) -c $(CFLAGS) configurazione.c
cterminfo.o : cterminfo.c cterminfo.h
	$(CC) -c $(CFLAGS) cterminfo.c
edit.o : edit.c edit.h
	$(CC) -c $(CFLAGS) edit.c
editor.o : editor.c editor.h
	$(CC) -c $(CFLAGS) editor.c
errore.o : errore.c errore.h
	$(CC) -c $(CFLAGS) errore.c
floor_cmd.o : floor_cmd.c
	$(CC) -c $(CFLAGS) floor_cmd.c
friends.o : friends.c friends.h
	$(CC) -c $(CFLAGS) friends.c
generic_cmd.o : generic_cmd.c
	$(CC) -c $(CFLAGS) generic_cmd.c
inkey.o : inkey.c inkey.h
	$(CC) -c $(CFLAGS) inkey.c
local.o : local.c local.h
	$(CC) -c $(CFLAGS) -DLOCAL local.c
login.o : login.c login.h
	$(CC) -c $(CFLAGS) login.c
messaggi.o : messaggi.c
	$(CC) -c $(CFLAGS) messaggi.c
metadata_ops.o : metadata_ops.c
	$(CC) -c $(CFLAGS) metadata_ops.c
name_list.o : name_list.c name_list.h
	$(CC) -c $(CFLAGS) name_list.c
pager.o : pager.c pager.h
	$(CC) -c $(CFLAGS) pager.c
parm.o : parm.c parm.h
	$(CC) -c $(CFLAGS) parm.c
prompt.o : prompt.c prompt.h
	$(CC) -c $(CFLAGS) prompt.c
remote.o : remote.c remote.h
	$(CC) -c $(CFLAGS) remote.c
room_cmd.o : room_cmd.c
	$(CC) -c $(CFLAGS) room_cmd.c
signals.o : signals.c
	$(CC) -c $(CFLAGS) signals.c
sysconf_client.o : sysconf_client.c
	$(CC) -c $(CFLAGS) sysconf_client.c
sysop.o : sysop.c sysop.h
	$(CC) -c $(CFLAGS) sysop.c
tabc.o : tabc.c tabc.h
	$(CC) -c $(CFLAGS) tabc.c
terminale.o : terminale.c
	$(CC) -c $(CFLAGS) terminale.c
urna-crea.o : urna-crea.c
	$(CC) -c $(CFLAGS) urna-crea.c
urna_errori.o : urna_errori.c
	$(CC) -c $(CFLAGS) urna_errori.c
urna-gestione.o : urna-gestione.c
	$(CC) -c $(CFLAGS) urna-gestione.c
urna-leggi.o : urna-leggi.c
	$(CC) -c $(CFLAGS) urna-leggi.c
urna-servizio.o : urna-servizio.c
	$(CC) -c $(CFLAGS) urna-servizio.c
urna-vota.o : urna-vota.c
	$(CC) -c $(CFLAGS) urna-vota.c
utente_client.o : utente_client.c
	$(CC) -c $(CFLAGS) utente_client.c
utility.o : utility.c
	$(CC) -c $(CFLAGS) utility.c

.PHONY :clean mrproper strip
install :

uninstall :


clean :
	-rm -f $(OBJALL) *~

mrproper : clean
	-rm -f $(bindir)/cittaclient $(bindir)/remote_cittaclient \
	       $(bindir)/debug_cittaclient

strip :
	-strip $(bindir)/*
