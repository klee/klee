include ../Makefile.common

SRCS = Solver.C Simplifier.C
OBJS = $(SRCS:.C=.o)

libsatsolver.a: $(OBJS)
	$(AR) rc $@ $^
	$(RANLIB) $@

Solver.o: Solver.C Solver.h Sort.h SolverTypes.h VarOrder.h Global.h Heap.h
Simplifier.o: Simplifier.C Solver.h Sort.h SolverTypes.h VarOrder.h Global.h Heap.h

clean:
	rm -rf *.o *~  *.a  depend.mak .#*
depend:
	makedepend -- $(CFLAGS) -- $(SRCS)
