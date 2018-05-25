###############
# DIRECTORIES #
###############
BASEDIR        = $(shell pwd)
KLEE_ROOT_DIR  = /data/str.klee
KLEE_SRC_DIR   = ${KLEE_ROOT_DIR}/klee
KLEE_BUILD_DIR = ${KLEE_ROOT_DIR}/santized-build-klee


################
# PROGRAM NAME #
################
PROGRAM_PATH=build/src/test
PROGRAM_NAME=torture_test

#########
# FILES #
#########
KLEE = ${KLEE_BUILD_DIR}/bin/klee
OPT := opt-3.4

.PHONY: all
all: run

libosip2-4.1.0/src/test/torture.c: mySourceFiles/torture.c
	cp mySourceFiles/torture.c            libosip2-4.1.0/src/test

libosip2-4.1.0/src/osipparser2/%.c: mySourceFiles/%.c
	cp $< $@

MODIFIED_FILES := test/torture.c osipparser2/osip_parser_cfg.c osipparser2/osip_message_parse.c


build/src/test/torture_test_no_dbg_with_names.bc: $(addprefix libosip2-4.1.0/src/, $(MODIFIED_FILES))
	cd build && (LLVM_COMPILER=clang C_INCLUDE_PATH=${KLEE_SRC_DIR}/include make)
	extract-bc build/src/test/torture_test
	$(OPT) -instnamer -o                                \
	build/src/test/torture_test_no_dbg_with_names.bc \
	build/src/test/torture_test.bc

.PHONY: run
run: build/src/test/torture_test_no_dbg_with_names.bc
	@setterm -term linux -fore blue
	@echo "*********************************************"
	@echo "*                                           *"
	@echo "* [5] Run KLEE with its BAZILLION flags ... *"
	@echo "*                                           *"
	@echo "*********************************************"
	@setterm -term linux -fore white
	#--posix-runtime                                 \
	#-constants-as-ABs                               \
	#-use-query-log=solver:smt2                      \
	# gdb -x ./myGdbScripts/gdbCommands.txt --args    \
	${KLEE}                                         \
	-solver-backend=z3                              \
	-output-module                                  \
	-output-source                                  \
	--libc=uclibc                                   \
	-use-query-log=all:kquery                                  \
	-use-independent-solver=1                       \
	-use-cex-cache=1                                \
	-debug-z3-dump-queries=Z3queries.txt            \
	-debug-z3-log-api-interaction=Z3LogAPI.txt      \
	${PROGRAM_PATH}/${PROGRAM_NAME}_no_dbg_with_names.bc 
	@setterm -term linux -fore blue
	@echo "******************************************"
	@echo "*                                        *"
	@echo "* [6] Make a human readable bitcode file *"
	@echo "*                                        *"
	@echo "******************************************"
	@setterm -term linux -fore white
	llvm-dis klee-last/final.bc
.PHONY: build_everything_from_scratch
build_everything_from_scratch:
	@clear
	@setterm -term linux -fore blue
	@echo "*************************************"
	@echo "*                                   *"
	@echo "* Build everything from scratch ... *"
	@echo "*                                   *"
	@echo "*************************************"
	@setterm -term linux -fore white
	./myBuildingScripts/build-step0-libosip
