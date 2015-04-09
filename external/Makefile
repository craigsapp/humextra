## humextra external library GNU makefile.
##
## Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
## Creation Date: Mon Jun 29 18:12:35 PDT 2009
## Last Modified: Thu Apr  9 06:24:42 PDT 2015
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

.PHONY: centerpoint $(PCRE)

all: centerpoint pcre

centerpoint:
	(cd centerpoint; $(MAKE) static_release)
	-mkdir ../lib
	cp centerpoint/lib/libcslxml.a ../lib
ifneq ($(wildcard ../lib),)
	(cd ../lib; ranlib libcslxml.a)
else
	@echo Error trying to copy static library for centerpoint
endif

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

clean: cleancenterpoint cleanpcre

cleancenterpoint:
	(cd centerpoint; $(MAKE) clean)

cleanpcre:
#	(cd $(PCRE); $(MAKE) clean)

