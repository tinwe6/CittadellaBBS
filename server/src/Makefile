#############################################################################
# Cittadella/UX server                   (C) M. Caldarelli and R. Vianello  #
#                                                                           #
# File : Makefile                                                           #
# Targets : all, cittaserver, fm_admin, clean, mrproper                     #
#                                                                           #
#############################################################################

SHELL = /bin/sh
.SUFFIXES:
.SUFFIXES: .c .o

# Compiler name
CC = gcc

# Directories
srcdir = .
bindir = ../bin
libdir = ../../share
VPATH = $(libdir)

# Compiler flags you want to use (other than profiling, such as -Wall)
# Flag DEBUG per tenere le parti di codice dedicate al debugging
# Flag SANE_STAT to check ustat: remember to create a file sane_stat
# in server/lib/urna/
#MYFLAGS = -g -Wall -DDEBUG -D_GNU_SOURCE
MYFLAGS = -g -pedantic -Wall -Wextra -DDEBUG -DSANE_STAT

# Flags for profiling (see hacker.doc for more information)
#PROFILE =
#PROFILE = -pg

# Flags for linking with libefence
#EFENCE = -lefence

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

#se le routines di <zlib.h> sono disponibili, togliere il cancelletto
LZ = -lz

#############################################################################
CFLAGS = $(MYFLAGS) $(CRYPT) $(PROFILE) $(OSFLAGS) $(TESTFLAGS)
# -lutil per le libutil necessarie al cittademone.
# LFLAGS = -L$(libdir) -lm -lcitta -lutil $(LCRYPT) $(LIBS)
# attenzone, prima -lcitta e poi -lm...
LFLAGS = -L$(libdir)  -lcitta -lm $(LCRYPT) $(LIBS)
override CFLAGS += $(patsubst %,-I%,$(subst :, ,$(VPATH)))

OBJFILES = banner.o blog.o cache_post.o cestino.o cittaserver.o cittaweb.o \
	   cmd_al_client.o coda_testo.o compress.o config.o email.o express.o \
	   file_messaggi.o fileserver.o find.o floors.o floor_ops.o fm_cmd.o \
	   generic.o imap4.o interprete.o march.o mail.o messaggi.o memstat.o \
	   parm.o patacche.o pop3.o post.o remote.o rooms.o room_ops.o \
	   signals.o socket_citta.o sysconfig.o urna-comm.o urna-crea.o \
	   urna-file.o urna-fine.o urna-rw.o urna-servizi.o urna-vota.o \
	   utente.o utente_cmd.o utility.o ut_rooms.o webstr.o x.o \
	   urna-strutture.o urna-posta.o

OBJFILES_CONV = conv_0.3.1.o memstat.o utility.o

OBJFILES_FMADMIN = file_messaggi.o fm_admin.o memstat.o utility.o termioc.o

OBJFILES_INETD = cittad.o

all : $(bindir)/cittaserver $(bindir)/cittad #$(bindir)/conv_0.3.1 #$(bindir)/fm_admin

$(bindir)/cittaserver : $(OBJFILES)
	$(CC) $(CFLAGS) $(OBJFILES) $(LFLAGS) $(LZ) $(EFENCE) -o $@

$(bindir)/cittad : $(OBJFILES_INETD)
	$(CC) $(CFLAGS) $(OBJFILES_INETD) $(EFENCE) -o $@

$(bindir)/conv_0.3.1 : $(OBJFILES_CONV)
	$(CC) $(CFLAGS) $(OBJFILES_CONV) $(LFLAGS) $(EFENCE) -o $@

$(bindir)/fm_admin : $(OBJFILES_FMADMIN)
	$(CC) $(CFLAGS) $(OBJFILES_FMADMIN) $(LFLAGS) -o $@

cittaserver.o : cittaserver.c cittaserver.h versione.h comm.h
	$(CC) -c $(CFLAGS) $(COMMFLAGS) cittaserver.c
banner.o : banner.c banner.h versione.h
	$(CC) -c $(CFLAGS) banner.c
blog.o : blog.c blog.h
	$(CC) -c $(CFLAGS) blog.c
cache_post.o : cache_post.c cache_post.h
	$(CC) -c $(CFLAGS) cache_post.c
cestino.o : cestino.c cestino.h
	$(CC) -c $(CFLAGS) cestino.c
cittad.o : cittad.c
	$(CC) -c $(CFLAGS) cittad.c
cittaweb.o : cittaweb.c cittaweb.h
	$(CC) -c $(CFLAGS) cittaweb.c
cmd_al_client.o : cmd_al_client.c cmd_al_client.h
	$(CC) -c $(CFLAGS) cmd_al_client.c
coda_testo.o : coda_testo.c coda_testo.h
	$(CC) -c $(CFLAGS) coda_testo.c
config.o : config.c versione.h
	$(CC) -c $(CFLAGS) config.c
email.o : email.c email.h
	$(CC) -c $(CFLAGS) email.c
express.o : express.c express.h
	$(CC) -c $(CFLAGS) express.c
file_messaggi.o : file_messaggi.c file_messaggi.h
	$(CC) -c $(CFLAGS) file_messaggi.c
fileserver.o : fileserver.c fileserver.h
	$(CC) -c $(CFLAGS) fileserver.c
find.o : find.c find.h
	$(CC) -c $(CFLAGS) find.c
floors.o : floors.c floors.h
	$(CC) -c $(CFLAGS) floors.c
floor_ops.o : floor_ops.c floor_ops.h
	$(CC) -c $(CFLAGS) floor_ops.c
fm_cmd.o : fm_cmd.c fm_cmd.h
	$(CC) -c $(CFLAGS) fm_cmd.c
generic.o : generic.c generic.h versione.h
	$(CC) -c $(CFLAGS) generic.c
imap4.o : imap4.c imap4.h
	$(CC) -c $(CFLAGS) imap4.c
info_port.o : info_port.c info_port.h
	$(CC) -c $(CFLAGS) info_port.c
interprete.o : interprete.c interprete.h
	$(CC) -c $(CFLAGS) interprete.c
mail.o : mail.c mail.h
	$(CC) -c $(CFLAGS) mail.c
march.o : march.c march.h rooms.h room_flags.h
	$(CC) -c $(CFLAGS) march.c
messaggi.o : messaggi.c messaggi.h
	$(CC) -c $(CFLAGS) messaggi.c
memstat.o : memstat.c memstat.h
	$(CC) -c $(CFLAGS) memstat.c
parm.o : parm.c parm.h
	$(CC) -c $(CFLAGS) parm.c
patacche.o : patacche.c patacche.h
	$(CC) -c $(CFLAGS) patacche.c
pop3.o : pop3.c pop3.h
	$(CC) -c $(CFLAGS) pop3.c
post.o : post.c post.h
	$(CC) -c $(CFLAGS) post.c
remote.o : remote.c remote.h
	$(CC) -c $(CFLAGS) remote.c
rooms.o : rooms.c rooms.h room_flags.h
	$(CC) -c $(CFLAGS) rooms.c
room_ops.o : room_ops.c room_ops.h rooms.h room_flags.h
	$(CC) -c $(CFLAGS) room_ops.c
signals.o : signals.c signals.h
	$(CC) -c $(CFLAGS) signals.c
socket_citta.o : socket_citta.c socket_citta.h
	$(CC) -c $(CFLAGS) socket_citta.c
sysconfig.o : sysconfig.c sysconfig.h
	$(CC) -c $(CFLAGS) sysconfig.c
urna-comm.o : urna-comm.c urna-comm.h
	$(CC) -c $(CFLAGS) urna-comm.c
urna-crea.o : urna-crea.c urna-crea.h
	$(CC) -c $(CFLAGS) urna-crea.c
urna-file.o : urna-file.c urna-file.h
	$(CC) -c $(CFLAGS) urna-file.c
urna-fine.o : urna-fine.c urna-fine.h
	$(CC) -c $(CFLAGS) urna-fine.c
urna-rw.o : urna-rw.c urna-rw.h
	$(CC) -c $(CFLAGS) urna-rw.c
urna-posta.o: urna-posta.c urna-posta.h
	$(CC) -c $(CFLAGS) urna-posta.c
urna-servizi.o : urna-servizi.c urna-servizi.h
	$(CC) -c $(CFLAGS) urna-servizi.c
urna-strutture.o : urna-strutture.c urna-strutture.h
	$(CC) -c $(CFLAGS) urna-strutture.c
urna-vota.o : urna-vota.c urna-vota.h
	$(CC) -c $(CFLAGS) urna-vota.c
ut_rooms.o : ut_rooms.c ut_rooms.h
	$(CC) -c $(CFLAGS) ut_rooms.c
utente.o : utente.c utente.h
	$(CC) -c $(CFLAGS) utente.c
utente_cmd.o : utente_cmd.c utente_cmd.h
	$(CC) -c $(CFLAGS) utente_cmd.c
utility.o : utility.c utility.h
	$(CC) -c $(CFLAGS) utility.c
webstr.o : webstr.c webstr.h
	$(CC) -c $(CFLAGS) webstr.c
x.o : x.c x.h
	$(CC) -c $(CFLAGS) x.c
urna_to_ascii.o: urna_to_ascii.c
	$(CC) -c $(CFLAGS) urna_to_ascii.c
fm_admin.o : fm_admin.c
	$(CC) -c $(CFLAGS) fm_admin.c
termioc.o : termioc.c
	$(CC) -c $(CFLAGS) termioc.c

.PHONY : clean mrproper strip

install_dir :
	mkdir $(libdir)

clean :
	-rm -f $(OBJFILES) $(OBJFILES_CONV) *~

mrproper : clean mrproperconv cleanconv
	-rm -f $(bindir)/cittaserver $(bindir)/cittad $(bindir)/fm_admin \
		$(bindir)/conv*


strip :
	-strip $(bindir)/*

urna_to_ascii : urna_to_ascii.o urna-file.o memstat.o utility.o ../../share/text.o
	$(CC)  $(CFLAGS) -o urna_to_ascii urna_to_ascii.o urna-file.o memstat.o utility.o ../../share/text.o



CONV_0_3_2_MYFLAGS = -DUSE_REFERENDUM

CONV_0_3_2_OBJFILES =  urna-file.o memstat.o utility.o conv_0.3.2.o  \
	            conv_new_write.0.3.2.o conv_conv_0.3.2.o conv_read_old_0.3.2.o

conv_0.3.2 : $(CONV_0_3_2_OBJFILES)
	$(CC) $(CFLAGS) $(CONV_0_3_2_OBJFILES) $(LFLAGS) $(EFENCE) \
    -o conv_0.3.2

conv_0.3.2.o : conv_0.3.2.c
	$(CC) -c $(CFLAGS) $(CONV_0_3_2_MYFLAGS) $(COMMFLAGS) conv_0.3.2.c

conv_read_old_0.3.2.o : conv_read_old_0.3.2.c
	$(CC) -c $(CFLAGS) $(CONV_0_3_2_MYFLAGS) conv_read_old_0.3.2.c

conv_conv_0.3.2.o : conv_conv_0.3.2.c
	$(CC) -c $(CFLAGS) $(CONV_0_3_2_MYFLAGS) conv_conv_0.3.2.c

conv_new_write_0.3.2.o : conv_new_write_0.3.2.c
	$(CC) -c $(CFLAGS) $(CONV_0_3_2_MYFLAGS) conv_new_write_0.3.2.c

mrproperconv:
	-rm -f ./conv_0.3.2

cleanconv:
	-rm -f $(CONV_0_3_2_OBJFILES)
