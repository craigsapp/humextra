## main humextra GNU makefile for Linux on Intel computers.
##
## Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
## Creation Date: Sun Apr  3 00:44:44 PST 2005
## Last Modified: Wed Aug 25 14:24:19 PDT 2010
## Filename:      ...humextra/Makefile
##
## Description: This Makefile can create the Humdrum Extras library or 
##              programs which use the Humdrum Extras library with linux 
##              using g++ (gcc 2.7.2.1 or later).
##
## To run this makefile, type (without quotes) "make library" (or 
## "gmake library" on FreeBSD computers), then "make programs".
##

# targets which don't actually refer to files
.PHONY : src-programs lib src-library include bin scripts update libupdate updatelib libup uplib

###########################################################################
#                                                                         #
#                                                                         #

all: info library examples

info:
	@echo ""
	@echo This makefile will create either the Humdrum Extra library file
	@echo or will compile the Humdrum Extra programs.  You may
	@echo have to make the library first if it does not exist.
	@echo Type one of the following:
	@echo "   $(MAKE) library"
	@echo or
	@echo "   $(MAKE) programs"
	@echo ""
	@echo To compile a specific program called xxx, type:
	@echo "   $(MAKE) xxx"
	@echo ""
	@echo Typing \"make\" alone with compile both the library and all programs.
	@echo ""

library: 
	$(MAKE) -f Makefile.library

update: library-update programs-update

examples-update: programs-update
example-update:  programs-update
program-update:  programs-update
programs-update: 
	touch src-programs/*.cpp
	$(MAKE) -f Makefile.programs

libupdate: library-update
updatelib: library-update
uplib: library-update
libup: library-update
library-update:
	$(MAKE) -f Makefile.library library

scripts:
	$(MAKE) -f Makefile.programs scripts

clean:
	$(MAKE) -f Makefile.library clean
	-rm -rf bin
	-rm -rf lib
ifneq ($(wildcard external),)
	(cd external; make clean)
endif

examples: programs
programs:
	$(MAKE) -f Makefile.programs

%: 
	-mkdir -p bin
	@echo compiling file $@
	$(MAKE) -f Makefile.programs $@
	

#                                                                         #
#                                                                         #
###########################################################################



