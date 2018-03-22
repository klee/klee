########
# KLEE #
########
KLEE=/home/oren/GIT/klee/vanilla.klee/build/bin/klee

################
# PROGRAM NAME #
################
PROGRAM_PATH=mupdf-1.12.0-source/build/debug
PROGRAM_NAME=mutool

############################
# DEFAULT TARGET FIRST ... #
############################
.PHONY: compile_and_run
compile_and_run:
	@clear
	@setterm -term linux -fore blue
	@echo "*****************************************"
	@echo "*                                       *"
	@echo "* [0] Copy from my source files to gzip *"
	@echo "*                                       *"
	@echo "*****************************************"
	@setterm -term linux -fore white
	#cp mySourceFiles/dnstracer.c dnstracer-1.6
	@setterm -term linux -fore blue
	@echo "*************************"
	@echo "*                       *"
	@echo "* [1] Bulid gzip-1.2.4a *"
	@echo "*                       *"
	@echo "*************************"
	@setterm -term linux -fore white
	@setterm -term linux -fore blue
	@echo "**********************************"
	@echo "*                                *"
	@echo "* [2] extract ${PROGRAM_NAME}.bc *"
	@echo "*                                *"
	@echo "**********************************"
	@setterm -term linux -fore white
	#extract-bc ${PROGRAM_PATH}/${PROGRAM_NAME}
	@setterm -term linux -fore blue
	@echo "*******************************************"
	@echo "*                                         *"
	@echo "* [3] Remove debug info from bitcode file *"
	@echo "*                                         *"
	@echo "*******************************************"
	@setterm -term linux -fore white
	opt -strip-debug -o       \
	${PROGRAM_PATH}/${PROGRAM_NAME}_no_dbg.bc \
	${PROGRAM_PATH}/${PROGRAM_NAME}.bc
	@setterm -term linux -fore blue
	@echo "************************************************"
	@echo "*                                              *"
	@echo "* [4] Add names to insructions in bitcode file *"
	@echo "*                                              *"
	@echo "************************************************"
	@setterm -term linux -fore white
	opt -instnamer -o                                    \
	${PROGRAM_PATH}/${PROGRAM_NAME}_no_dbg_with_names.bc \
	${PROGRAM_PATH}/${PROGRAM_NAME}_no_dbg.bc
	@setterm -term linux -fore blue
	@echo "*********************************************"
	@echo "*                                           *"
	@echo "* [5] Run KLEE with its BAZILLION flags ... *"
	@echo "*                                           *"
	@echo "*********************************************"
	@setterm -term linux -fore white
	#gdb -x ./myGdbScripts/gdbCommands.txt --args        \
	${KLEE}                                              \
	--libc=uclibc                                        \
	--posix-runtime                                      \
	-solver-backend=z3                                   \
	-output-module                                       \
	-output-source                                       \
	-use-cex-cache=0                                     \
	-use-independent-solver=0                            \
	-use-query-log=solver:smt2                           \
	-debug-z3-dump-queries=Z3queries.txt                 \
	-debug-z3-log-api-interaction=Z3LogAPI.txt           \
	${PROGRAM_PATH}/${PROGRAM_NAME}_no_dbg_with_names.bc \
	--sym-args 0 1 1050
	@setterm -term linux -fore blue
	@echo "******************************************"
	@echo "*                                        *"
	@echo "* [6] Make a human readable bitcode file *"
	@echo "*                                        *"
	@echo "******************************************"
	@setterm -term linux -fore white
	llvm-dis klee-last/final.bc
#################################
# BUILD EVERYTHING FMOR SCRATCH #
#################################
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
	./myBuildingScripts/build-step0-mupdf
