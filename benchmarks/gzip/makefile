.PHONY: compile_and_run
compile_and_run:
	@clear
	@setterm -term linux -fore blue
	@echo "**************************************************************"
	@echo "*                                                            *"
	@echo "* Run LIBOSIP with tailored torture test that causes failure *"
	@echo "*                                                            *"
	@echo "**************************************************************"
	@setterm -term linux -fore white
	gdb -x ./myGdbScripts/gdbCommands.txt --args \
	./build/src/test/torture_test                \
	./myInput/Input                              \
	0                                            \
	-c
.PHONY: print_all_c_stmts
print_all_c_stmts:
	@clear
	@setterm -term linux -fore blue
	@echo "**********************************************************"
	@echo "*                                                        *"
	@echo "* Print all c statements until critical point is reached *"
	@echo "*                                                        *"
	@echo "**********************************************************"
	@setterm -term linux -fore white
	gdb -x ./myGdbScripts/gdbCommandsSingleStep.txt --args \
	./build/src/test/torture_test                          \
	./myInput/Input                                        \
	0                                                      \
	-c
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
