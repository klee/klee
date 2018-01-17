.PHONY: compile_and_run
compile_and_run:
	@clear
	@setterm -term linux -fore blue
	@echo "***************************************************************************"
	@echo "*                                                                         *"
	@echo "* [0] Copy modified torture.c file with msg in memory and NOT from a file *"
	@echo "*                                                                         *"
	@echo "***************************************************************************"
	@setterm -term linux -fore white
	cp ./mySourceFiles/torture.c ./libosip2-4.1.0/src/test/
	@setterm -term linux -fore blue
	@echo "***********************************************"
	@echo "*                                             *"
	@echo "* [1] build libosip package with new trture.c *"
	@echo "*                                             *"
	@echo "***********************************************"
	@setterm -term linux -fore white
	cd ./build && make -j
	@setterm -term linux -fore blue
	@echo "******************************************************************"
	@echo "*                                                                *"
	@echo "* [2] Run LIBOSIP with tailored torture test that causes failure *"
	@echo "*                                                                *"
	@echo "******************************************************************"
	@setterm -term linux -fore white
	gdb -x ./myGdbScripts/gdbCommands.txt --args \
	./build/src/test/torture_test                \
	./myInput/Input                              \
	0                                            \
	-c

