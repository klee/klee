#########
# INPUT #
#########
STRING_200_A=$(shell python -c 'print "A"*200')
INPUT=myInput/B/${STRING_200_A}/${STRING_200_A}/${STRING_200_A}/${STRING_200_A}/${STRING_200_A}/${STRING_200_A}/Input.gz

############################
# DEFAULT TARGET FIRST ... #
############################
.PHONY: compile_and_run
compile_and_run:
	@clear
	@setterm -term linux -fore blue
	@echo "********************************************************"
	@echo "*                                                      *"
	@echo "* Run gzip with with long filename that causes failure *"
	@echo "*                                                      *"
	@echo "********************************************************"
	@setterm -term linux -fore white
	#gdb -x ./myGdbScripts/gdbCommands.txt --args \
	gdb --args \
	build/gzip \
	${INPUT}
############################
# PRINT C TRACE OF CVE ... #
############################
.PHONY: print_CVE_C_trace
print_CVE_C_trace:
	@clear
	@setterm -term linux -fore blue
	@echo "**********************************************************"
	@echo "*                                                        *"
	@echo "* Print all c statements until critical point is reached *"
	@echo "*                                                        *"
	@echo "**********************************************************"
	@setterm -term linux -fore white
	gdb -x ./myGdbScripts/gdbCommandsSingleStep.txt --args \
	build/gzip                                           \
	myInput/B/${STRING_200_A}/${STRING_200_A}/${STRING_200_A}/${STRING_200_A}/${STRING_200_A}/Input.gz
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
	./myBuildingScripts/build-step0-gzip
