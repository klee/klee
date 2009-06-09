#===-- lib/SMT/Makefile ------------------------------------*- Makefile -*--===#

LEVEL=../..

LIBRARYNAME=kleaverSMT
DONT_BUILD_RELINKED=1
BUILD_ARCHIVE=1

include $(LEVEL)/Makefile.common

# Gross, but I don't want to build proper rules for this, and I don't want users
# to have to have bison/flex, so for now require developers to make these
# manually (at least initially).

smtlib_parser.cpp smtlib_parser.h: smtlib.y
	bison -d -o smtlib_parser.cpp -p smtlib smtlib.y
	mv smtlib_parser.hpp smtlib_parser.h

smtlib_lexer.cpp: smtlib.lex smtlib_parser.h
	flex -I -Psmtlib -osmtlib_lexer.cpp smtlib.lex

.PHONY: regen
regen: smtlib_lexer.cpp smtlib_parser.cpp smtlib_parser.h
