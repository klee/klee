################
# PROGRAM NAME #
################
PROGRAM_PATH=build
PROGRAM_NAME=opj_mj2_extract

######################
# INPUT REPRODUCTION #
######################
s=$(printf "%-757s" "0")
t=$(printf "%-4924s" "0")

##################
# DEFAULT TARGET #
##################
.PHONY: compile_and_run
compile_and_run:
	@clear
	@setterm -term linux -fore blue
	@echo "****************************************************************"
	@echo "*                                                              *"
	@echo "* Run xmllint with specially crafted input that causes failure *"
	@echo "*                                                              *"
	@echo "****************************************************************"
	@setterm -term linux -fore white
	echo '<!DOCTYPEa[<!ELEMENT a (F'"${s// /0}:${t// /0}"')><!ATTLIST a><!ELEMENT b EMPTY><!ATTLIST b s CDATA #IMPLIED>]><a/>' > bug1.xml
	gdb -x myGdbScripts/gdbCommands.txt --args ${PROGRAM_PATH}/${PROGRAM_NAME} ${INPUT}
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
