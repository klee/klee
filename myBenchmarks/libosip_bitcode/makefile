###############
# DIRECTORIES #
###############
BASEDIR       = $(shell pwd)
BUILD_DIR     = ${BASEDIR}/build
BITCODE_DIR   = ${BUILD_DIR}/src/test
OUTPUT_DIR    = ${BITCODE_DIR}
LIBOSIP_DIR   = ${BASEDIR}/libosip2-4.1.0
MY_SOURCE_DIR = ${BASEDIR}/mySourceFiles

######################
# WHICH KLEE TO USE? #
######################
MY_KLEE_DIR      = ../../myStrKlee 
VANILLA_KLEE_DIR = ../../vanillaKlee

######################
# WHICH KLEE TO USE? #
######################
#KLEE_DIR = ${MY_KLEE_DIR}
KLEE_DIR = ${VANILLA_KLEE_DIR}

#########
# FILES #
#########
KLEE                        = ${KLEE_DIR}/build/bin/klee
BITCODE_FILE                = ${BITCODE_DIR}/torture_test
HUMAN_READABLE_BITCODE_FILE = ${BITCODE_DIR}/input.ll

###########
# TARGETS #
###########
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
	cp ${MY_SOURCE_DIR}/torture.c   ${LIBOSIP_DIR}/src/test
	cp ${MY_SOURCE_DIR}/osip_body.c ${LIBOSIP_DIR}/src/osipparser2
	cp ${MY_SOURCE_DIR}/osip_port.c ${LIBOSIP_DIR}/src/osipparser2
	@setterm -term linux -fore blue
	@echo "************************************************"
	@echo "*                                              *"
	@echo "* [1] Bulid libosip2-4.1.0 and torture_test.bc *"
	@echo "*                                              *"
	@echo "************************************************"
	@setterm -term linux -fore white
	cd ${BUILD_DIR} && (LLVM_COMPILER=clang make)	
	@setterm -term linux -fore blue
	@echo "*******************************"
	@echo "*                             *"
	@echo "* [2] extract torture_test.bc *"
	@echo "*                             *"
	@echo "*******************************"
	@setterm -term linux -fore white
	extract-bc ${BITCODE_DIR}/torture_test
	@setterm -term linux -fore blue
	@echo "*******************************************"
	@echo "*                                         *"
	@echo "* [3] Remove debug info from bitcode file *"
	@echo "*                                         *"
	@echo "*******************************************"
	@setterm -term linux -fore white
	opt -strip-debug -o       \
	${BITCODE_FILE}_no_dbg.bc \
	${BITCODE_FILE}.bc
	@setterm -term linux -fore blue
	@echo "************************************************"
	@echo "*                                              *"
	@echo "* [4] Add names to insructions in bitcode file *"
	@echo "*                                              *"
	@echo "************************************************"
	@setterm -term linux -fore white
	opt -instnamer -o                    \
	${BITCODE_FILE}_no_dbg_with_names.bc \
	${BITCODE_FILE}_no_dbg.bc
	@setterm -term linux -fore blue
	@echo "******************************************"
	@echo "*                                        *"
	@echo "* [5] Make a human readable bitcode file *"
	@echo "*                                        *"
	@echo "******************************************"
	@setterm -term linux -fore white
	llvm-dis -o=${HUMAN_READABLE_BITCODE_FILE} ${OUTPUT_DIR}/klee-last/final.bc
	@setterm -term linux -fore blue
	@echo "*********************************************"
	@echo "*                                           *"
	@echo "* [6] Run KLEE with its BAZILLION flags ... *"
	@echo "*                                           *"
	@echo "*********************************************"
	@setterm -term linux -fore white
	#gdb -x ./myGdbScripts/gdbCommands.txt --args                                 \
	${KLEE}                                                                       \
	--libc=uclibc                                                                 \
	--posix-runtime                                                               \
	-solver-backend=z3                                                            \
	-output-module                                                                \
	-output-source                                                                \
	-use-cex-cache=0                                                              \
	-use-independent-solver=0                                                     \
	-use-query-log=solver:smt2                                                    \
	-debug-z3-dump-queries=/home/oren/GIT/LatestKlee/myZ3Logs/Z3queries.txt       \
	-debug-z3-log-api-interaction=/home/oren/GIT/LatestKlee/myZ3Logs/Z3LogAPI.txt \
	${BITCODE_FILE}_no_dbg_with_names.bc 
build_everything_from_scratch:
	@clear
	@setterm -term linux -fore red
	@echo "************************************************************"
	@echo "*                                                          *"
	@echo "* ======================================================== *"
	@echo "*                                                          *"
	@echo "*                          CAUTION                         *"
	@echo "*                                                          *"
	@echo "*           make is about to re-build everything           *"
	@echo "*                                                          *"
	@echo "* ======================================================== *"
	@echo "*                                                          *"
	@echo "*                                                          *"
	@echo "*                                                          *"
	@echo "* [1] make will rm -rf everything (build, libosip)         *"
	@echo "*                                                          *"
	@echo "* [2] make will wget everything (libosip)                  *"
	@echo "*                                                          *"
	@echo "* [3] make will re-build everything (libosip)              *"
	@echo "*                                                          *"
	@echo "* [4] You have 10 secs to abort (ctrl c)                   *"
	@echo "*                                                          *"
	@echo "************************************************************"
	@setterm -term linux -fore white
	@sleep 10
	rm -rf ./build
	rm -rf ./libosip2-4.1.0
	rm -rf ./libosip2-4.1.0.zip
	./myBuildingScripts/build-step0-libosip

