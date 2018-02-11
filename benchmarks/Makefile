OPT:=opt-3.4
CLANG:=clang-3.4
#SED_SCRIPT:=../scripts/sedScript
SED_SCRIPT:=cp

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


PATH:=$(shell realpath ../../build-klee/bin):$(shell realpath ../scripts):$(shell realpath ../scripts/z3QueriesParserFolder/FOLDER_4_PARSER):$(PATH)
export PATH

.PHONY: phony_explicit
phony_explicit:

#run target, for example to run strcmp_example.bc use: make strcmp_example.run
%.run: %.bc phony_explicit
	klee -solver-backend=z3 -search=dfs -use-cex-cache=0 -use-independent-solver=0 -use-query-log=solver:smt2 \
	-debug-z3-dump-queries=/tmp/Z3queries.txt $<

.PHONY: clean
clean: 
	rm -fr klee-* $(BENCHMARKS:.c=.bc)
