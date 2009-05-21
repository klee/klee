include ../Makefile.common

SRCS = constantbv.cpp
OBJS = $(SRCS:.cpp=.o)

libconstantbv.a: $(OBJS)
	$(AR) rc $@ $^
	$(RANLIB) $@

clean:	
	rm -rf *.o *~ *.a .#*

constantbv.o: constantbv.h
