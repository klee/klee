#include <iostream>

#include "expr/Lexer.h"
#include "expr/Parser.h"

#include "klee/Config/Version.h"
#include "klee/Constraints.h"
#include "klee/Expr.h"
#include "klee/ExprBuilder.h"
#include "klee/Solver.h"
#include "klee/Statistics.h"
#include "klee/util/ExprPPrinter.h"
#include "klee/util/ExprVisitor.h"

#include "klee/util/ExprSMTLIBLetPrinter.h"

#include "llvm/ADT/OwningPtr.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"

// FIXME: Ugh, this is gross. But otherwise our config.h conflicts with LLVMs.
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION

#if LLVM_VERSION_CODE < LLVM_VERSION(2, 9)
#include "llvm/System/Signals.h"
#else
#include "llvm/Support/Signals.h"
#include "llvm/Support/system_error.h"
#endif

using namespace llvm;
using namespace klee;
using namespace klee::expr;

namespace {
  llvm::cl::opt<std::string>
  InputFile(llvm::cl::desc("<input query log>"), llvm::cl::Positional,
            llvm::cl::init("-"));

  enum ToolActions {
    PrintTokens,
    PrintAST,
    PrintSMTLIBv2,
    Evaluate
  };

  static llvm::cl::opt<ToolActions> 
  ToolAction(llvm::cl::desc("Tool actions:"),
             llvm::cl::init(Evaluate),
             llvm::cl::values(
             clEnumValN(PrintTokens, "print-tokens",
                        "Print tokens from the input file."),
			clEnumValN(PrintSMTLIBv2, "print-smtlib",
					   "Print parsed input file as SMT-LIBv2 query."),
             clEnumValN(PrintAST, "print-ast",
                        "Print parsed AST nodes from the input file."),
             clEnumValN(Evaluate, "evaluate",
                        "Print parsed AST nodes from the input file."),
             clEnumValEnd));


  enum BuilderKinds {
    DefaultBuilder,
    ConstantFoldingBuilder,
    SimplifyingBuilder
  };

  static llvm::cl::opt<BuilderKinds> 
  BuilderKind("builder",
              llvm::cl::desc("Expression builder:"),
              llvm::cl::init(DefaultBuilder),
              llvm::cl::values(
              clEnumValN(DefaultBuilder, "default",
                         "Default expression construction."),
              clEnumValN(ConstantFoldingBuilder, "constant-folding",
                         "Fold constant expressions."),
              clEnumValN(SimplifyingBuilder, "simplify",
                         "Fold constants and simplify expressions."),
              clEnumValEnd));

  cl::opt<bool>
  UseDummySolver("use-dummy-solver",
		   cl::init(false));

  cl::opt<bool>
  UseFastCexSolver("use-fast-cex-solver",
		   cl::init(false));
  
  cl::opt<bool>
  UseSTPQueryPCLog("use-stp-query-pc-log",
                   cl::init(false));
}

static std::string escapedString(const char *start, unsigned length) {
  std::string Str;
  llvm::raw_string_ostream s(Str);
  for (unsigned i=0; i<length; ++i) {
    char c = start[i];
    if (isprint(c)) {
      s << c;
    } else if (c == '\n') {
      s << "\\n";
    } else {
      s << "\\x" 
        << hexdigit(((unsigned char) c >> 4) & 0xF) 
        << hexdigit((unsigned char) c & 0xF);
    }
  }
  return s.str();
}

static void PrintInputTokens(const MemoryBuffer *MB) {
  Lexer L(MB);
  Token T;
  do {
    L.Lex(T);
    std::cout << "(Token \"" << T.getKindName() << "\" "
               << "\"" << escapedString(T.start, T.length) << "\" "
               << T.length << " "
               << T.line << " " << T.column << ")\n";
  } while (T.kind != Token::EndOfFile);
}

static bool PrintInputAST(const char *Filename,
                          const MemoryBuffer *MB,
                          ExprBuilder *Builder) {
  std::vector<Decl*> Decls;
  Parser *P = Parser::Create(Filename, MB, Builder);
  P->SetMaxErrors(20);

  unsigned NumQueries = 0;
  while (Decl *D = P->ParseTopLevelDecl()) {
    if (!P->GetNumErrors()) {
      if (isa<QueryCommand>(D))
        std::cout << "# Query " << ++NumQueries << "\n";

      D->dump();
    }
    Decls.push_back(D);
  }

  bool success = true;
  if (unsigned N = P->GetNumErrors()) {
    std::cerr << Filename << ": parse failure: "
               << N << " errors.\n";
    success = false;
  }

  for (std::vector<Decl*>::iterator it = Decls.begin(),
         ie = Decls.end(); it != ie; ++it)
    delete *it;

  delete P;

  return success;
}

static bool EvaluateInputAST(const char *Filename,
                             const MemoryBuffer *MB,
                             ExprBuilder *Builder) {
  std::vector<Decl*> Decls;
  Parser *P = Parser::Create(Filename, MB, Builder);
  P->SetMaxErrors(20);
  while (Decl *D = P->ParseTopLevelDecl()) {
    Decls.push_back(D);
  }

  bool success = true;
  if (unsigned N = P->GetNumErrors()) {
    std::cerr << Filename << ": parse failure: "
               << N << " errors.\n";
    success = false;
  }  

  if (!success)
    return false;

  // FIXME: Support choice of solver.
  Solver *S, *STP = S = 
    UseDummySolver ? createDummySolver() : new STPSolver(true);
  if (UseSTPQueryPCLog)
    S = createPCLoggingSolver(S, "stp-queries.pc");
  if (UseFastCexSolver)
    S = createFastCexSolver(S);
  S = createCexCachingSolver(S);
  S = createCachingSolver(S);
  S = createIndependentSolver(S);
  if (0)
    S = createValidatingSolver(S, STP);

  unsigned Index = 0;
  for (std::vector<Decl*>::iterator it = Decls.begin(),
         ie = Decls.end(); it != ie; ++it) {
    Decl *D = *it;
    if (QueryCommand *QC = dyn_cast<QueryCommand>(D)) {
      std::cout << "Query " << Index << ":\t";

      assert("FIXME: Support counterexample query commands!");
      if (QC->Values.empty() && QC->Objects.empty()) {
        bool result;
        if (S->mustBeTrue(Query(ConstraintManager(QC->Constraints), QC->Query),
                          result)) {
          std::cout << (result ? "VALID" : "INVALID");
        } else {
          std::cout << "FAIL";
        }
      } else if (!QC->Values.empty()) {
        assert(QC->Objects.empty() && 
               "FIXME: Support counterexamples for values and objects!");
        assert(QC->Values.size() == 1 &&
               "FIXME: Support counterexamples for multiple values!");
        assert(QC->Query->isFalse() &&
               "FIXME: Support counterexamples with non-trivial query!");
        ref<ConstantExpr> result;
        if (S->getValue(Query(ConstraintManager(QC->Constraints), 
                              QC->Values[0]),
                        result)) {
          std::cout << "INVALID\n";
          std::cout << "\tExpr 0:\t" << result;
        } else {
          std::cout << "FAIL";
        }
      } else {
        std::vector< std::vector<unsigned char> > result;
        
        if (S->getInitialValues(Query(ConstraintManager(QC->Constraints), 
                                      QC->Query),
                                QC->Objects, result)) {
          std::cout << "INVALID\n";

          for (unsigned i = 0, e = result.size(); i != e; ++i) {
            std::cout << "\tArray " << i << ":\t"
                       << QC->Objects[i]->name
                       << "[";
            for (unsigned j = 0; j != QC->Objects[i]->size; ++j) {
              std::cout << (unsigned) result[i][j];
              if (j + 1 != QC->Objects[i]->size)
                std::cout << ", ";
            }
            std::cout << "]";
            if (i + 1 != e)
              std::cout << "\n";
          }
        } else {
          std::cout << "FAIL";
        }
      }

      std::cout << "\n";
      ++Index;
    }
  }

  for (std::vector<Decl*>::iterator it = Decls.begin(),
         ie = Decls.end(); it != ie; ++it)
    delete *it;
  delete P;

  delete S;

  if (uint64_t queries = *theStatisticManager->getStatisticByName("Queries")) {
    std::cout 
      << "--\n"
      << "total queries = " << queries << "\n"
      << "total queries constructs = " 
      << *theStatisticManager->getStatisticByName("QueriesConstructs") << "\n"
      << "valid queries = " 
      << *theStatisticManager->getStatisticByName("QueriesValid") << "\n"
      << "invalid queries = " 
      << *theStatisticManager->getStatisticByName("QueriesInvalid") << "\n"
      << "query cex = " 
      << *theStatisticManager->getStatisticByName("QueriesCEX") << "\n";
  }

  return success;
}

static bool printInputAsSMTLIBv2(const char *Filename,
                             const MemoryBuffer *MB,
                             ExprBuilder *Builder)
{
	//Parse the input file
	std::vector<Decl*> Decls;
	Parser *P = Parser::Create(Filename, MB, Builder);
	P->SetMaxErrors(20);
	while (Decl *D = P->ParseTopLevelDecl())
	{
		Decls.push_back(D);
	}

	bool success = true;
	if (unsigned N = P->GetNumErrors())
	{
		std::cerr << Filename << ": parse failure: "
				   << N << " errors.\n";
		success = false;
	}

	if (!success)
	return false;

	ExprSMTLIBPrinter* printer = createSMTLIBPrinter();
	printer->setOutput(std::cout);

	unsigned int queryNumber = 0;
	//Loop over the declarations
	for (std::vector<Decl*>::iterator it = Decls.begin(), ie = Decls.end(); it != ie; ++it)
	{
		Decl *D = *it;
		if (QueryCommand *QC = dyn_cast<QueryCommand>(D))
		{
			//print line break to separate from previous query
			if(queryNumber!=0) 	std::cout << std::endl;

			//Output header for this query as a SMT-LIBv2 comment
			std::cout << ";SMTLIBv2 Query " << queryNumber << std::endl;

			/* Can't pass ConstraintManager constructor directly
			 * as argument to Query object. Like...
			 * query(ConstraintManager(QC->Constraints),QC->Query);
			 *
			 * For some reason if constructed this way the first
			 * constraint in the constraint set is set to NULL and
			 * will later cause a NULL pointer dereference.
			 */
			ConstraintManager constraintM(QC->Constraints);
			Query query(constraintM,QC->Query);
			printer->setQuery(query);

			if(!QC->Objects.empty())
				printer->setArrayValuesToGet(QC->Objects);

			printer->generateOutput();


			queryNumber++;
		}
	}

	//Clean up
	for (std::vector<Decl*>::iterator it = Decls.begin(),
			ie = Decls.end(); it != ie; ++it)
		delete *it;
	delete P;

	delete printer;

	return true;
}

int main(int argc, char **argv) {
  bool success = true;

  llvm::sys::PrintStackTraceOnErrorSignal();
  llvm::cl::ParseCommandLineOptions(argc, argv);

  std::string ErrorStr;
  
#if LLVM_VERSION_CODE < LLVM_VERSION(2, 9)
  MemoryBuffer *MB = MemoryBuffer::getFileOrSTDIN(InputFile.c_str(), &ErrorStr);
  if (!MB) {
    std::cerr << argv[0] << ": error: " << ErrorStr << "\n";
    return 1;
  }
#else
  OwningPtr<MemoryBuffer> MB;
  error_code ec=MemoryBuffer::getFileOrSTDIN(InputFile.c_str(), MB);
  if (ec) {
    std::cerr << argv[0] << ": error: " << ec.message() << "\n";
    return 1;
  }
#endif
  
  ExprBuilder *Builder = 0;
  switch (BuilderKind) {
  case DefaultBuilder:
    Builder = createDefaultExprBuilder();
    break;
  case ConstantFoldingBuilder:
    Builder = createDefaultExprBuilder();
    Builder = createConstantFoldingExprBuilder(Builder);
    break;
  case SimplifyingBuilder:
    Builder = createDefaultExprBuilder();
    Builder = createConstantFoldingExprBuilder(Builder);
    Builder = createSimplifyingExprBuilder(Builder);
    break;
  }

  switch (ToolAction) {
  case PrintTokens:
#if LLVM_VERSION_CODE < LLVM_VERSION(2, 9)
    PrintInputTokens(MB);
#else
    PrintInputTokens(MB.get());
#endif
    break;
  case PrintAST:
#if LLVM_VERSION_CODE < LLVM_VERSION(2, 9)
    success = PrintInputAST(InputFile=="-" ? "<stdin>" : InputFile.c_str(), MB,
                            Builder);
#else
    success = PrintInputAST(InputFile=="-" ? "<stdin>" : InputFile.c_str(), MB.get(),
                            Builder);
#endif
    break;
  case Evaluate:
#if LLVM_VERSION_CODE < LLVM_VERSION(2, 9)
    success = EvaluateInputAST(InputFile=="-" ? "<stdin>" : InputFile.c_str(),
                               MB, Builder);
#else
    success = EvaluateInputAST(InputFile=="-" ? "<stdin>" : InputFile.c_str(),
                               MB.get(), Builder);
#endif
    break;
  case PrintSMTLIBv2:
#if LLVM_VERSION_CODE < LLVM_VERSION(2, 9)
    success = printInputAsSMTLIBv2(InputFile=="-"? "<stdin>" : InputFile.c_str(), MB,Builder);
#else
    success = printInputAsSMTLIBv2(InputFile=="-"? "<stdin>" : InputFile.c_str(), MB.get(),Builder);
#endif
    break;
  default:
    std::cerr << argv[0] << ": error: Unknown program action!\n";
  }

  delete Builder;
#if LLVM_VERSION_CODE < LLVM_VERSION(2, 9)
  delete MB;
#endif
  llvm::llvm_shutdown();
  return success ? 0 : 1;
}
