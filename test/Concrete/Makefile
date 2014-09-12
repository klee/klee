LEVEL = ../..

LCCFLAGS  += -O0 -Wall
LCXXFLAGS += -O0 -Wall
LLCFLAGS =

clean::
	-rm -rf Output

# We do not want to make .d files for tests! 
DISABLE_AUTO_DEPENDENCIES=1

include ${LEVEL}/Makefile.common

Output/%.bc: %.c Output/.dir $(INCLUDES)
	$(LLVMCC) -emit-llvm $(CPPFLAGS) $(LCCFLAGS) -c $< -o $@

Output/%.bc: %.cpp Output/.dir $(INCLUDES)
	$(LLVMCXX) -emit-llvm $(CPPFLAGS) $(LCXXFLAGS) -c $< -o $@

Output/%.bc: %.cc $(LCC1XX) Output/.dir $(INCLUDES)
	$(LLVMCXX) -emit-llvm $(CPPFLAGS) $(LCXXFLAGS) -c $< -o $@

Output/%.bc: %.ll $(LLVMAS) Output/.dir
	$(LLVMAS) -f $< -o $@

# We build a separate testingUtils bitcode for each test, to make sure parallel
# tests don't interact with one another.
Output/%_testingUtils.bc: _testingUtils.c $(LLVMAS) Output/.dir
	$(LLVMCC) -emit-llvm $(CPPFLAGS) $(LCCFLAGS) -c $< -o $@

Output/linked_%.bc: Output/%.bc Output/%_testingUtils.bc
	$(LLVMLD) $< Output/$*_testingUtils.bc -o $@

.PRECIOUS: Output/.dir

## Cancel built-in implicit rules that override above rules
%: %.s

%: %.c

%.o: %.c

