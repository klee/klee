include ../Makefile.common

SRCS = AST.cpp ASTKind.cpp  ASTUtil.cpp BitBlast.cpp SimpBool.cpp  ToCNF.cpp ToSAT.cpp Transform.cpp
OBJS = $(SRCS:.cpp=.o)

#Make the ast library for use by other modules
libast.a:	$(OBJS)
	-rm -rf $@
	$(AR) rc libast.a $(OBJS)
	$(RANLIB) libast.a

ASTKind.o: ASTKind.h ASTKind.cpp
	$(CXX) $(CXXFLAGS)   -c -o ASTKind.o ASTKind.cpp

# ASTKind.h and ASTKind.cpp are automatically generated
ASTKind.h ASTKind.cpp:	ASTKind.kinds genkinds.pl
	./genkinds.pl

# cnftest:	cnftest.o ToCNF.o AST.o ASTUtil.o ASTKind.o BitBlast.o AST.h
#	$(CC) $(LDFLAGS) ToCNF.o BitBlast.o ASTKind.o  ASTUtil.o AST.o  cnftest.o  -o cnftest 

# bbtest: $(OBJS)
#	$(CC) $(LDFLAGS) BitBlast.o ASTKind.o  ASTUtil.o AST.o  bbtest.o -o bbtest 

# asttest: $(OBJS)
#	$(CC) $(LDFLAGS) ASTKind.o  ASTUtil.o AST.o asttest.o  -lstdc++ -o asttest

clean:
	rm -rf *.o *~ bbtest asttest cnftest *.a  ASTKind.h ASTKind.cpp .#*

depend:
	makedepend -Y -- $(CFLAGS) -- $(SRCS)
# DO NOT DELETE

AST.o: AST.h ASTUtil.h ASTKind.h ../sat/Solver.h ../sat/SolverTypes.h
AST.o: ../sat/Global.h ../sat/VarOrder.h ../sat/Solver.h ../sat/Heap.h
AST.o: ../AST/ASTUtil.h ../sat/SolverTypes.h ../constantbv/constantbv.h
ASTUtil.o: ASTUtil.h
BitBlast.o: AST.h ASTUtil.h ASTKind.h ../sat/Solver.h ../sat/SolverTypes.h
BitBlast.o: ../sat/Global.h ../sat/VarOrder.h ../sat/Solver.h ../sat/Heap.h
BitBlast.o: ../AST/ASTUtil.h ../sat/SolverTypes.h ../constantbv/constantbv.h
SimpBool.o: AST.h ASTUtil.h ASTKind.h ../sat/Solver.h ../sat/SolverTypes.h
SimpBool.o: ../sat/Global.h ../sat/VarOrder.h ../sat/Solver.h ../sat/Heap.h
SimpBool.o: ../AST/ASTUtil.h ../sat/SolverTypes.h ../constantbv/constantbv.h
ToCNF.o: AST.h ASTUtil.h ASTKind.h ../sat/Solver.h ../sat/SolverTypes.h
ToCNF.o: ../sat/Global.h ../sat/VarOrder.h ../sat/Solver.h ../sat/Heap.h
ToCNF.o: ../AST/ASTUtil.h ../sat/SolverTypes.h ../constantbv/constantbv.h
ToSAT.o: AST.h ASTUtil.h ASTKind.h ../sat/Solver.h ../sat/SolverTypes.h
ToSAT.o: ../sat/Global.h ../sat/VarOrder.h ../sat/Solver.h ../sat/Heap.h
ToSAT.o: ../AST/ASTUtil.h ../sat/SolverTypes.h ../constantbv/constantbv.h
ToSAT.o: ../simplifier/bvsolver.h ../AST/AST.h
Transform.o: AST.h ASTUtil.h ASTKind.h ../sat/Solver.h ../sat/SolverTypes.h
Transform.o: ../sat/Global.h ../sat/VarOrder.h ../sat/Solver.h ../sat/Heap.h
Transform.o: ../AST/ASTUtil.h ../sat/SolverTypes.h ../constantbv/constantbv.h
