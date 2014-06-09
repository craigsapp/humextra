## Makefile for webpages at http://extras.humdrum.org/example
## and for regression tests of Humdrum Extras programs.
##
## Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
## Creation Date: Sat Mar 12 09:14:29 PST 2005
## Last Modified: Mon Jun  9 02:18:30 PDT 2014
## Filename:      ...example/Makefile
##
## To run this makefile, type (without quotes) "make library",
## and then "make examples".  To compile on FreeBSD computers, use 
## "gmake" instead of "make".
##

BINDIR=bin

all: index
	$(BINDIR)/makepages ./

index:
	$(BINDIR)/makeallindex ./ Alltemplate.html ../man > index.html

test:  regression
tests: regression
regression:
	$(BINDIR)/run-command-tests 

regression-fails:
	$(BINDIR)/run-command-tests -f 

regression-diff:
	$(BINDIR)/run-command-tests -fd


