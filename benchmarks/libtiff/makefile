#########
# INPUT #
#########

##################
# DEFAULT TARGET #
##################
.PHONY: compile_and_run
compile_and_run:
	@clear
	@setterm -term linux -fore blue
	@echo "******************************************************************"
	@echo "*                                                                *"
	@echo "* Run tiffsplit with specially crafted input that causes failure *"
	@echo "*                                                                *"
	@echo "******************************************************************"
	@setterm -term linux -fore white
	./build/tools/tiffsplit myInput/Input.tif `perl -e 'print "a"x40000;'`
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
	./myBuildingScripts/build-step0-libtiff
