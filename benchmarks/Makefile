OPT:=opt-3.4
CLANG:=clang-3.4
SED_SCRIPT:=../scripts/sedScript

BENCHMARKS:=$(wildcard *.c)

KLEE_INCLUDES:=$(shell realpath ../include)

.PHONY: all
all: $(BENCHMARKS:.c=.bc)

%.preproc.c: %.c
	$(SED_SCRIPT) $< $@
	

%.unopt.bc: %.preproc.c
	$(CLANG) -c -emit-llvm -I$(KLEE_INCLUDES) -o $@ $<

%.bc: %.unopt.bc
	$(OPT) -instnamer -o $@ $<

.PHONY: clean
clean: 
	rm $(BENCHMARKS:.c=.bc)
