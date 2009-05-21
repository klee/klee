LEVEL = ../..

# hard-coding bad. will get fixed.
LCCFLAGS  += -O0 -Wall
LCXXFLAGS += -O0 -Wall
LLCFLAGS =

test:
	./ConcreteTest.py

clean::
	-rm -rf Output klee-last klee-out* test.log
	-rm -rf *.bc

# We do not want to make .d files for tests! 
DISABLE_AUTO_DEPENDENCIES=1

include ${LEVEL}/Makefile.common

# Compile from X.c to Output/X.ll
Output/%.ll: %.c $(LCC1) Output/.dir $(INCLUDES)
	$(LLVMGCCWITHPATH) --emit-llvm $(CPPFLAGS) $(LCCFLAGS) -S $< -o $@

# Compile from X.cpp to Output/X.ll
Output/%.ll: %.cpp $(LCC1XX) Output/.dir $(INCLUDES)
	$(LLVMGXXWITHPATH) --emit-llvm $(CPPFLAGS) $(LCXXFLAGS) -S $< -o $@

# Compile from X.cc to Output/X.ll
Output/%.ll: %.cc $(LCC1XX) Output/.dir $(INCLUDES)
	$(LLVMGXXWITHPATH) --emit-llvm $(CPPFLAGS) $(LCXXFLAGS) -S $< -o $@

# LLVM Assemble from Output/X.ll to Output/X.bc.  Output/X.ll must have come
# from GCC output, so use GCCAS.
#
Output/%.bc: Output/%.ll $(LLVMAS)
	$(LLVMAS) -f $< -o $@

# LLVM Assemble from X.ll to Output/X.bc.  Because we are coming directly from
# LLVM source, use the non-transforming assembler.
#
Output/%.bc: %.ll $(LLVMAS) Output/.dir
	$(LLVMAS) -f $< -o $@

Output/linked_%.bc: Output/%.bc Output/_testingUtils.bc
	$(LLVMLD) -disable-opt -link-as-library Output/_testingUtils.bc $< -o $@

.PRECIOUS: Output/.dir

## Cancel built-in implicit rules that override above rules
%: %.s

%: %.c

%.o: %.c

