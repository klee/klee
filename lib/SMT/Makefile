LIBRARY=libsmt.a

# Do not optimize the parser code - it compiles forever in some gcc versions
EXTRAFLAGS = -O0

TRANSIENT_CPP = parsesmtlib.cpp lexsmtlib.cpp
TRANSIENT_OBJ = $(TRANSIENT_CPP:.cpp=.o)

TRANSIENT = $(TRANSIENT_CPP) parsesmtlib_defs.h parsesmtlib.output

SRC = $(TRANSIENT_CPP) parser.cpp

HEADERS = parser_temp.h

# The actual source files for the parser that we want to include in a
# distribution
SRC_ORIG = smtlib.lex smtlib.y parser.cpp

#include ../../Makefile.local

#lex
LEX = flex
LFLAGS = # -i  use this for a case-insensitive scanner

#yacc
YACC = bison
YFLAGS = -d -y


##################################################
# Rules for transient files
##################################################

all: $(TRANSIENT_OBJ)

%.o: %.cpp
	$(CXX) $(CFLAGS) -c $< -o $@

lexsmtlib.cpp:  smtlib.lex parsesmtlib_defs.h
		$(LEX) $(LFLAGS) -I -Psmtlib -olexsmtlib.cpp smtlib.lex

parsesmtlib_defs.h: parsesmtlib.cpp

parsesmtlib.cpp: smtlib.y
		$(YACC) $(YFLAGS) -o parsesmtlib.cpp -p smtlib --debug -v smtlib.y
		@if [ -f parsesmtlib.cpp.h ]; then mv parsesmtlib.cpp.h parsesmtlib.hpp; fi
		@mv parsesmtlib.hpp parsesmtlib_defs.h

clean:
	rm -f $(TRANSIENT) *~ *.o

