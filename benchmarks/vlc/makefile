###########
# PROGRAM #
###########
PROGRAM = vlc
PROGRAM_PATH = ./build

#################
# PROGRAM INPUT #
#################
PROGRAM_INPUT = $(shell python -c 'print "A"*1025')

#############
# GDB STUFF #
#############
GDB_SCRIPT = ./myGdbScripts/gdbCommands.txt

##################
# DEFAULT TARGET #
##################
.PHONY: compile_and_run
compile_and_run:
	@clear
	@setterm -term linux -fore blue
	@echo "****************************************************************"
	@echo "*                                                              *"
	@echo "* Run program with specially crafted input that causes failure *"
	@echo "*                                                              *"
	@echo "****************************************************************"
	@setterm -term linux -fore white
	gdb -x ${GDB_SCRIPT} --args \
	${PROGRAM_PATH}/${PROGRAM} ${PROGRAM_INPUT}
PHONY: build_everything_from_scratch
build_everything_from_scratch:
	@clear
	@setterm -term linux -fore blue
	@echo "*************************************"
	@echo "*                                   *"
	@echo "* Build everything from scratch ... *"
	@echo "*                                   *"
	@echo "*************************************"
	@setterm -term linux -fore white
	./myBuildingScripts/build-step0-${PROGRAM}

