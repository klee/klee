 # STP (Simple Theorem Prover) top level makefile
 #
 # To make in debug mode, type 'make "CLFAGS=-ggdb"
 # To make in optimized mode, type 'make "CFLAGS=-O2" 

include Makefile.common

USE_PARSER := 0

LIBS := AST/libast.a sat/libsatsolver.a simplifier/libsimplifier.a bitvec/libconsteval.a constantbv/libconstantbv.a c_interface/libcinterface.a
DIRS := AST sat simplifier bitvec c_interface constantbv

ifeq ($(USE_PARSER), 1)
DIRS += parser
endif

# NB: the TAGS target is a hack to get around this recursive make nonsense
# we want all the source and header files generated before we make tags
.PHONY: all
all: lib/libstp.a include/stp/c_interface.h

AST/libast.a:
	@$(MAKE) -q -C `dirname $@` || $(MAKE) -C `dirname $@`
sat/libsatsolver.a: AST/libast.a
	@$(MAKE) -q -C `dirname $@` || $(MAKE) -C `dirname $@`
simplifier/libsimplifier.a: AST/libast.a
	@$(MAKE) -q -C `dirname $@` || $(MAKE) -C `dirname $@`
bitvec/libconsteval.a: AST/libast.a
	@$(MAKE) -q -C `dirname $@` || $(MAKE) -C `dirname $@`
constantbv/libconstantbv.a: AST/libast.a
	@$(MAKE) -q -C `dirname $@` || $(MAKE) -C `dirname $@`
c_interface/libcinterface.a: AST/libast.a
	@$(MAKE) -q -C `dirname $@` || $(MAKE) -C `dirname $@`
parser/parser: $(LIBS)
	@$(MAKE) -q -C `dirname $@` || $(MAKE) -C `dirname $@`

lib/libstp.a: $(LIBS)
	@mkdir -p lib
	rm -f $@
	@for dir in $(DIRS); do \
		$(AR) rc $@ $$dir/*.o; \
	done
	$(RANLIB) $@

bin/stp: parser/parser $(LIBS)
	@mkdir -p bin
	@cp parser/parser $@

include/stp/c_interface.h: $(LIBS)
	@mkdir -p include/stp
	@cp c_interface/c_interface.h $@

.PHONY: clean
clean:
	rm -rf *~
	rm -rf *.a
	rm -rf lib/*.a
	rm -rf bin/*~
	rm -rf bin/stp
	rm -rf *.log
	rm -f TAGS
	$(MAKE) clean -C AST
	$(MAKE) clean -C sat
	$(MAKE) clean -C simplifier
	$(MAKE) clean -C bitvec
	$(MAKE) clean -C parser
	$(MAKE) clean -C c_interface
	$(MAKE) clean -C constantbv

