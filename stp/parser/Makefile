include ../Makefile.common

SRCS = lexPL.cpp parsePL.cpp let-funcs.cpp main.cpp
OBJS = $(SRCS:.cpp=.o)
LIBS = -L../AST -last -L../sat -lsatsolver -L../simplifier -lsimplifier -L../bitvec -lconsteval -L../constantbv -lconstantbv

all: parser 

parser: lexPL.o parsePL.o let-funcs.o main.o 
		$(CXX) $(CFLAGS) $(LDFLAGS) lexPL.o parsePL.o main.o let-funcs.o $(LIBS) -o parser

main.o: parsePL_defs.h

lexPL.cpp:	PL.lex parsePL_defs.h ../AST/AST.h
		$(LEX) -o lexPL.cpp PL.lex

parsePL_defs.h: y.tab.h
		@cp y.tab.h parsePL_defs.h
parsePL.cpp: y.tab.c
		@cp y.tab.c parsePL.cpp

y.tab.c y.tab.h:	PL.y
		$(YACC) PL.y


clean:	
		rm -rf *.o parsePL_defs.h *~ lexPL.cpp parsePL.cpp *.output parser y.tab.* lex.yy.c .#*
