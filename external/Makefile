## humextra external library GNU makefile.
##
## Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
## Creation Date: Mon Jun 29 18:12:35 PDT 2009
## Last Modified: Fri Apr 17 11:50:02 PDT 2015
## Filename:      ...humextra/external/Makefile
##
## Description: This Makefile compiles external libraries used in
##              humdrum extras source code.  In particular:
##              (1) Centerpoint XML parsing code
##              (2) Perl-Compatible Regular Expressions
##
## To run this makefile, type (without quotes) "make", or "gmake" on
## FreeBSD computers.  This makefile is typically run automatically from the
## parent directory's Makefile.
##

PCRE = $(wildcard pcre-8.35)

.PHONY: centerpoint midifile $(PCRE)


all: centerpoint pcre midifile improv


midifile:
	(cd midifile; $(MAKE) library)


centerpoint:
	(cd centerpoint && $(MAKE) static_release)
	mkdir -p ../lib
	# Sleep needed in Ubuntu on virtual box so that library
	# file finished writing from previous command.
	sleep 1
	cp centerpoint/lib/libcslxml.a ../lib
#ifneq ($(wildcard ../lib),)
#	(cd ../lib; ranlib libcslxml.a)
#else
#	@echo Error trying to copy static library for centerpoint
#endif


pcre:
	# The following command helps to avoid having autoreconf or
	# aclocal or other stupid non-installed configuration tools
	# from being triggered to run.
	(cd $(PCRE);  touch *)
	(cd $(PCRE); ./configure)
	(cd $(PCRE); $(MAKE))
	-mkdir ../lib
	cp $(PCRE)/.libs/libpcre.a ../lib/libpcre.a
ifneq ($(wildcard ../lib),)
	(cd ../lib; ranlib libpcre.a)
else
	@echo Error trying to copy static library for pcre
endif

clean: cleancenterpoint cleanmidi cleanpcre improv-clean

superclean: cleancenterpoint cleanpcre remove-improv cleanmidi

cleanmidi:
	(cd midifile; $(MAKE) clean)

cleancenterpoint:
	(cd centerpoint; $(MAKE) clean)

cleanpcre:
#	(cd $(PCRE); $(MAKE) clean)


###########################################################################
##
## Improv library is for compiling humplay.
##


improv: improv-library
improv-library: improv-clone
	-(cd improv && make clean && make library)
	-cp improv/lib/libimprov.a ../lib/
#	-rm -rf ../lib/improv
#	-rm ../lib/libimprov.a
#	mkdir -p ../lib/improv
#	(cd ../lib/improv && ar -x ../../external/improv/lib/libimprov.a)
#	(cd ../lib/improv && rm -f Options.o)
#	(cd ../lib/improv && \
#	   for i in *.o; do if [[ -z `nm $$i 2> /dev/null` ]]; then rm $$i; \
#	   fi; done)
#	(cd ../lib/improv && libtool -static *.o -o ../libimprov.a)
#	-rm -rf ../lib/improv


improv-clean:
ifneq ($(wildcard improv),)
	(cd improv && make superclean)
endif


improv-clone:
ifeq ($(wildcard improv),)
	git clone https://github.com/craigsapp/improv
endif


update: improv-update
improv-update:
ifeq ($(wildcard improv),)
	git pull https://github.com/craigsapp/improv
endif


remove-improv:
	-rm -rf improv


