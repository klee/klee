#CC=llvm-gcc --emit-llvm
CC=clang -emit-llvm

all: direct controlflow regions

direct: taint_direct.o
		echo 
		echo Testing...
		echo There should be no assertion fails!
		klee -taint=direct taint_direct.o
		echo 

controlflow: taint_controlflow.o
		echo 
		echo Testing...
		echo There should be no assertion fails!
		klee -taint=control-flow taint_controlflow.o
		echo 

regions: taint_regions.o
		echo 
		echo Testing...
		echo There should be no assertion fails!
		klee -taint=regions taint_regions.o
		echo 

taint_direct.o: taint.c
		$(CC) -O0 -DDIRECT -c taint.c -I ../../include/ -o taint_direct.o

taint_controlflow.o: taint.c
		$(CC) -O0 -DCONTROLFLOW -c taint.c -I ../../include/ -o taint_controlflow.o

taint_regions.o: taint.c
		$(CC) -O0 -DREGIONS -c taint.c -I ../../include/ -o taint_regions.o

clean:
		rm -Rf klee-*
		rm -f taint.o

