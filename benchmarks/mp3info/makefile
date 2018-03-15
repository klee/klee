##################
# DEFAULT TARGET #
##################
.PHONY: compile_and_run
compile_and_run:
	@clear
	@setterm -term linux -fore blue
	@echo "******************************************************************"
	@echo "*                                                                *"
	@echo "* Run dnstracer with specially crafted input that causes failure *"
	@echo "*                                                                *"
	@echo "******************************************************************"
	@setterm -term linux -fore white
	gdb -x myGdbScripts/gdbCommands.txt --args mp3info-0.8.5/mp3info $(shell python -c 'print "A"*1025')
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
	./myBuildingScripts/build-step0-mp3info
