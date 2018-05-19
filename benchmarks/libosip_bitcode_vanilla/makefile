###############
# DIRECTORIES #
###############
BASEDIR        = $(shell pwd)
KLEE_ROOT_DIR  = /data
KLEE_SRC_DIR   = ${KLEE_ROOT_DIR}/klee
KLEE_BUILD_DIR = ${KLEE_ROOT_DIR}/klee/build

################
# PROGRAM NAME #
################
PROGRAM_PATH=build/src/test
PROGRAM_NAME=torture_test

#########
# FILES #
#########
KLEE = ${KLEE_BUILD_DIR}/bin/klee

.PHONY: compile_and_run
compile_and_run:
	@clear
	@setterm -term linux -fore blue
	@echo "********************************************"
	@echo "*                                          *"
	@echo "* [0] Copy from my source files to libosip *"
	@echo "*                                          *"
	@echo "********************************************"
	@setterm -term linux -fore white
	cp mySourceFiles/torture.c            libosip2-4.1.0/src/test
	#cp mySourceFiles/osip_body.c          libosip2-4.1.0/src/osipparser2
	#cp mySourceFiles/osip_port.c          libosip2-4.1.0/src/osipparser2
	cp mySourceFiles/osip_parser_cfg.c    libosip2-4.1.0/src/osipparser2
	cp mySourceFiles/osip_message_parse.c libosip2-4.1.0/src/osipparser2
	@setterm -term linux -fore blue
	@echo "************************************************"
	@echo "*                                              *"
	@echo "* [1] Bulid libosip2-4.1.0 and torture_test.bc *"
	@echo "*                                              *"
	@echo "************************************************"
	@setterm -term linux -fore white
	cd build && (LLVM_COMPILER=clang C_INCLUDE_PATH=${KLEE_SRC_DIR}/include make)	
	@setterm -term linux -fore blue
	@echo "*******************************"
	@echo "*                             *"
	@echo "* [2] extract torture_test.bc *"
	@echo "*                             *"
	@echo "*******************************"
	@setterm -term linux -fore white
	extract-bc ${PROGRAM_PATH}/${PROGRAM_NAME}
	@setterm -term linux -fore blue
	@echo "*******************************************"
	@echo "*                                         *"
	@echo "* [3] Remove debug info from bitcode file *"
	@echo "*                                         *"
	@echo "*******************************************"
	@setterm -term linux -fore white
	opt-3.4 -strip-debug -o                   \
	build/src/test/torture_test_no_dbg.bc \
	build/src/test/torture_test.bc
	@setterm -term linux -fore blue
	@echo "************************************************"
	@echo "*                                              *"
	@echo "* [4] Add names to insructions in bitcode file *"
	@echo "*                                              *"
	@echo "************************************************"
	@setterm -term linux -fore white
	opt-3.4 -instnamer -o                                \
	build/src/test/torture_test_no_dbg_with_names.bc \
	build/src/test/torture_test_no_dbg.bc
	@setterm -term linux -fore blue
	@echo "*********************************************"
	@echo "*                                           *"
	@echo "* [5] Run KLEE with its BAZILLION flags ... *"
	@echo "*                                           *"
	@echo "*********************************************"
	@setterm -term linux -fore white
	#--posix-runtime                                 \
	#-constants-as-ABs                               \
	gdb -x ./myGdbScripts/gdbCommands.txt --args    \
	${KLEE}                                         \
	--libc=uclibc                                   \
	-solver-backend=z3                              \
	-output-module                                  \
	-output-source                                  \
	-use-cex-cache=0                                \
	-use-independent-solver=0                       \
	-use-query-log=solver:smt2                      \
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
