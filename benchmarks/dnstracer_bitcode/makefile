###############
# DIRECTORIES #
###############
KLEE_ROOT_DIR  = /home/oren/GIT/klee/vanilla.klee
KLEE_SRC_DIR   = ${KLEE_ROOT_DIR}/klee
KLEE_BUILD_DIR = ${KLEE_ROOT_DIR}/build

#########
# FILES #
#########
KLEE = ${KLEE_BUILD_DIR}/bin/klee

.PHONY: compile_and_run
compile_and_run:
	@clear
	@setterm -term linux -fore blue
	@echo "**********************************************"
	@echo "*                                            *"
	@echo "* [0] Copy from my source files to dnstracer *"
	@echo "*                                            *"
	@echo "**********************************************"
	@setterm -term linux -fore white
	#cp mySourceFiles/torture.c   libosip2-4.1.0/src/test
	@setterm -term linux -fore blue
	@echo "***************************"
	@echo "*                         *"
	@echo "* [1] Bulid dnstracer-1.6 *"
	@echo "*                         *"
	@echo "***************************"
	@setterm -term linux -fore white
	cd build && (LLVM_COMPILER=clang make)	
	@setterm -term linux -fore blue
	@echo "****************************"
	@echo "*                          *"
	@echo "* [2] extract dnstracer.bc *"
	@echo "*                          *"
	@echo "****************************"
	@setterm -term linux -fore white
	extract-bc build/dnstracer
	@setterm -term linux -fore blue
	@echo "*******************************************"
	@echo "*                                         *"
	@echo "* [3] Remove debug info from bitcode file *"
	@echo "*                                         *"
	@echo "*******************************************"
	@setterm -term linux -fore white
	opt -strip-debug -o       \
	build/dnstracer_no_dbg.bc \
	build/dnstracer.bc
	@setterm -term linux -fore blue
	@echo "************************************************"
	@echo "*                                              *"
	@echo "* [4] Add names to insructions in bitcode file *"
	@echo "*                                              *"
	@echo "************************************************"
	@setterm -term linux -fore white
	opt -instnamer -o                    \
	build/dnstracer_no_dbg_with_names.bc \
	build/dnstracer_no_dbg.bc
	@setterm -term linux -fore blue
	@echo "*********************************************"
	@echo "*                                           *"
	@echo "* [5] Run KLEE with its BAZILLION flags ... *"
	@echo "*                                           *"
	@echo "*********************************************"
	@setterm -term linux -fore white
	#gdb -x ./myGdbScripts/gdbCommands.txt --args   \
	${KLEE}                                         \
	--libc=uclibc                                   \
	--posix-runtime                                 \
	-solver-backend=z3                              \
	-output-module                                  \
	-output-source                                  \
	-use-cex-cache=0                                \
	-use-independent-solver=0                       \
	-use-query-log=solver:smt2                      \
	-debug-z3-dump-queries=Z3queries.txt            \
	-debug-z3-log-api-interaction=Z3LogAPI.txt      \
	build/dnstracer_no_dbg_with_names.bc 
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
	./myBuildingScripts/build-step0-dnstracer
