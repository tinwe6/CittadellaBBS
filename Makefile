#############################################################################
# Cittadella/UX BBS                      (C) M. Caldarelli and R. Vianello  #
# Client/Server application                                                 #
#                                                                           #
# File : Makefile                                                           #
# Targets : all, cittaclient, cittaserver, clean, mrproper, dist            #
#                                                                           #
#############################################################################

# Remove the hash mark below if compiling under Linux
#OSFLAGS = -DLINUX -std=gnu99 -D_GNU_SOURCE

# Remove the hash mark below if compiling under Mac OS X
OSFLAGS = -DMACOSX -std=c99

# Remove the hash mark below if compiling under Windows with Cygwin:
#OSFLAGS = -DWINDOWS

# Remove the hash mark to compile for terminals with white background.
#BGCOL = -DWHITEBG

# Remove hash mark to perform the tests
#TESTFLAGS = -DTESTS

#############################################################################

export OSFLAGS
export BGCOL
export TESTFLAGS

SHELL = /bin/sh
#MAKE = make -j 2
.SUFFIXES:
.SUFFIXES: .c .o

VERSION = 0.2

docdir = doc
mandir = man
clientdir = client/src
serverdir = server/src
daemondir = server/src
sharedir = share
installdir = ../citta
helpdir = server/lib/help

all: libcitta cittaclient cittaserver aggiorna_help

libcitta:
	@$(MAKE) -C $(sharedir)

cittaclient: libcitta
	@$(MAKE) -C $(clientdir)

cittaserver: libcitta
	@$(MAKE) -C $(serverdir)

cittad:
	@$(MAKE) -C $(daemondir)

strip:
	@$(MAKE) -C $(clientdir) strip
	@$(MAKE) -C $(serverdir) strip

.PHONY: install installdir bininstall \
	doc_clean clean mrproper doc_clean backup aggiorna_help

install: installdir bininstall

installdir:
	mkdir $(installdir)
	mkdir $(installdir)/server
	mkdir $(installdir)/server/bin
	mkdir $(installdir)/server/log
	mkdir $(installdir)/server/tmp
	mkdir $(installdir)/server/lib
	mkdir $(installdir)/server/lib/banners
	mkdir $(installdir)/server/lib/files
	mkdir $(installdir)/server/lib/floors
	mkdir $(installdir)/server/lib/floors/floor_info
	mkdir $(installdir)/server/lib/help
	mkdir $(installdir)/server/lib/images
	mkdir $(installdir)/server/lib/messaggi
	mkdir $(installdir)/server/lib/msgfiles
	mkdir $(installdir)/server/lib/rooms
	mkdir $(installdir)/server/lib/rooms/msgdata
	mkdir $(installdir)/server/lib/rooms/room_info
	mkdir $(installdir)/server/lib/server
	mkdir $(installdir)/server/lib/urna
	mkdir $(installdir)/server/lib/utenti
	mkdir $(installdir)/server/lib/utenti/blog
	mkdir $(installdir)/server/lib/utenti/mail
	mkdir $(installdir)/server/lib/utenti/profile
	mkdir $(installdir)/server/lib/utenti/utr_data
	-cp server/lib/banners/banners $(installdir)/server/lib/banners
	-cp server/lib/help/* $(installdir)/server/lib/help
	-cp server/lib/images/* $(installdir)/server/lib/images
	-cp server/lib/messaggi/* $(installdir)/server/lib/messaggi
	-cp ./server/bin/touchlogs $(installdir)/server/bin
	cd $(installdir)/server && ./bin/touchlogs
	mkdir $(installdir)/client
	mkdir $(installdir)/client/bin
	mkdir $(installdir)/man
	-cp ./server/bin/logrotate $(installdir)/server/bin/
	-cp ./server/bin/backup $(installdir)/server/bin/

bininstall: aggiorna_help #urna_to_ascii
	-rm -f $(installdir)/client/bin/cittaclient
	-rm -f $(installdir)/client/bin/remote_cittaclient
	-cp ./client/bin/cittaclient $(installdir)/client/bin/
	-cp ./client/bin/remote_cittaclient $(installdir)/client/bin/
#	-chmod a+s $(installdir)/client/bin/remote_cittaclient
	-rm -f $(installdir)/server/bin/remote_cittaclient
	-rm -f $(installdir)/server/bin/cittaserver
	-rm -f $(installdir)/server/bin/cittad
	-rm -f $(installdir)/server/bin/conv*
	-cp ./client/bin/remote_cittaclient $(installdir)/server/bin/
	-cp ./server/bin/cittaserver $(installdir)/server/bin/
	-cp ./server/bin/cittad $(installdir)/server/bin/
	-cp ./server/bin/conv* $(installdir)/server/bin/
	-cp -f $(mandir)/* $(installdir)/$(mandir)
	-cp -f $(helpdir)/manuale $(docdir)/dot_commands \
	       $(docdir)/lista_comandi $(installdir)/$(helpdir)/
#	-cp ./server/src/conv_0.3.2 $(installdir)/server/bin
#	-cp ./server/src/urna_to_ascii $(installdir)/server/bin/

doc_clean :
	-rm -f ./*~ $(docdir)/*~ $(docdir)/devel/*~ $(docdir)/client/*~ \
	       	    $(mandir)/*~
	-rm -f $(helpdir)/manuale $(helpdir)/dot_commands \
	       $(helpdir)/lista_comandi

clean: doc_clean
	@$(MAKE) -C $(sharedir) clean
	@$(MAKE) -C $(clientdir) clean
	@$(MAKE) -C $(serverdir) clean
#	@$(MAKE) -C $(daemondir) clean

mrproper: doc_clean
	@$(MAKE) -C $(sharedir) mrproper
	@$(MAKE) -C $(clientdir) mrproper
	@$(MAKE) -C $(serverdir) mrproper
#	@$(MAKE) -C $(daemondir) mrproper
	cd server && ./bin/purge_bbsdata

distclean: mrproper
	-rm -rf .fname cittadella-*

dist: distclean
	@echo cittadella-`sed \
		-e '/version_string/ !d' \
		-e 's/[^0-9.]*\([0-9.]*\).*/\1/' \
		-e q \
		share/versione.h` > .fname
	-rm -f `cat .fname`
	mkdir `cat .fname`
	cp -rf ./Makefile ./COPYING ./CREDITS client server \
	        $(sharedir) $(mandir) $(docdir) `cat .fname`
#	ln $(clientdir)/* `cat .fname`
	tar zchf `cat .fname`.tgz `cat .fname`
	-rm -rf `cat .fname` .fname

client_predist: mrproper
	-rm -rf `cat .fname` .fname
	@echo cittaclient-`sed \
		-e '/version_string/ !d' \
		-e 's/[^0-9.]*\([0-9.]*\).*/\1/' \
		-e q \
		share/versione.h` > .fname
	-rm -rf `cat .fname`
	mkdir `cat .fname`
	ln $(clientdir)/*.c $(clientdir)/*.h \
	$(sharedir)/*.c $(sharedir)/*.h $(mandir)/cittaclient.6 \
	`cat .fname`
	rm `cat .fname`/remote.*
	mkdir `cat .fname`/doc
	ln $(docdir)/COPYING.CLIENT `cat .fname`/COPYING
	ln $(docdir)/lista_comandi `cat .fname`/doc/COMANDI
	ln $(docdir)/dot_commands `cat .fname`/doc/DOT_COMMANDS
	ln $(docdir)/client/cittaclient.rc `cat .fname`/doc
	ln $(docdir)/client/cittalogo.png `cat .fname`/doc
	ln $(clientdir)/Makefile.dist `cat .fname`/Makefile

client_dist : client_predist
	tar zchf `cat .fname`-src.tgz `cat .fname`
	-rm -rf `cat .fname` .fname

client_bzdist : client_predist
	tar ychf `cat .fname`.tar.bz2 `cat .fname`
	-rm -rf `cat .fname` .fname

client_bindist : cittaclient strip
	-rm -rf `cat .fname` .fname
#	@echo cittaclient-$(VERSION) > .fname
#	@echo cittaclient > .fname
#	mkdir `cat .fname`
	@echo cittaclient-`sed \
		-e '/version_string/ !d' \
		-e 's/[^0-9.]*\([0-9.]*\).*/\1/' \
		-e q \
		share/versione.h` > .fname
	-rm -f `cat .fname`
	mkdir `cat .fname`
	ln client/bin/cittaclient $(mandir)/cittaclient.6 \
		$(docdir)/README $(docdir)/client/cittaclient.rc \
		$(docdir)/lista_comandi $(docdir)/dot_commands  \
		`cat .fname`
	tar zchf `cat .fname`-i386.tgz `cat .fname`
	-rm -rf `cat .fname` .fname

backup : mrproper
	cd .. && tar cf - cittadella/ | bzip2 -9 > \
	cittadella-`date '+%y.%m.%d'`.tbz
	sync

aggiorna_help :
	-cp -f $(docdir)/README $(helpdir)/manuale
	-cp -f $(docdir)/dot_commands $(helpdir)/dot_commands
	-cp -f $(docdir)/lista_comandi $(helpdir)/lista_comandi

conv_0.3.2 : libcitta
	@$(MAKE) -C $(serverdir) conv_0.3.2


urna_to_ascii: libcitta
	@$(MAKE) -C $(serverdir) urna_to_ascii

