//===-- main.cpp ------------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Common.h"
#include "klee/Config/Version.h"
#include "klee/Expr/Constraints.h"
#include "klee/Expr/Expr.h"
#include "klee/Expr/ExprBuilder.h"
#include "klee/Expr/ExprPPrinter.h"
#include "klee/Expr/ExprSMTLIBPrinter.h"
#include "klee/Expr/ExprVisitor.h"
#include "klee/Expr/Parser/Lexer.h"
#include "klee/Expr/Parser/Parser.h"
#include "klee/Internal/Support/PrintVersion.h"
#include "klee/OptionCategories.h"
#include "klee/Solver/Solver.h"
#include "klee/Solver/SolverCmdLine.h"
#include "klee/Solver/SolverImpl.h"
#include "klee/Statistics.h"

#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"

#include <sys/stat.h>
#include <unistd.h>


#include "llvm/Support/Signals.h"

using namespace llvm;
using namespace klee;
using namespace klee::expr;

namespace {
llvm::cl::opt<std::string> InputFile(llvm::cl::desc("<input query log>"),
                                     llvm::cl::Positional, llvm::cl::init("-"),
                                     llvm::cl::cat(klee::ExprCat));

enum ToolActions { PrintTokens, PrintAST, PrintSMTLIBv2, Evaluate };

static llvm::cl::opt<ToolActions> ToolAction(
    llvm::cl::desc("Tool actions:"), llvm::cl::init(Evaluate),
    llvm::cl::values(clEnumValN(PrintTokens, "print-tokens",
                                "Print tokens from the input file."),
                     clEnumValN(PrintSMTLIBv2, "print-smtlib",
                                "Print parsed input file as SMT-LIBv2 query."),
                     clEnumValN(PrintAST, "print-ast",
                                "Print parsed AST nodes from the input file."),
                     clEnumValN(Evaluate, "evaluate",
                                "Evaluate parsed AST nodes from the input file.")
                         KLEE_LLVM_CL_VAL_END),
    llvm::cl::cat(klee::SolvingCat));

enum BuilderKinds {
  DefaultBuilder,
  ConstantFoldingBuilder,
  SimplifyingBuilder
};

static llvm::cl::opt<BuilderKinds> BuilderKind(
    "builder", llvm::cl::desc("Expression builder:"),
    llvm::cl::init(DefaultBuilder),
    llvm::cl::values(clEnumValN(DefaultBuilder, "default",
                                "Default expression construction."),
                     clEnumValN(ConstantFoldingBuilder, "constant-folding",
                                "Fold constant expressions."),
                     clEnumValN(SimplifyingBuilder, "simplify",
                                "Fold constants and simplify expressions.")
                         KLEE_LLVM_CL_VAL_END),
    llvm::cl::cat(klee::ExprCat));

llvm::cl::opt<std::string> DirectoryToWriteQueryLogs(
    "query-log-dir",
    llvm::cl::desc(
        "The folder to write query logs to (default=current directory)"),
    llvm::cl::init("."), llvm::cl::cat(klee::ExprCat));

llvm::cl::opt<bool> ClearArrayAfterQuery(
    "clear-array-decls-after-query",
    llvm::cl::desc("Discard the previous array declarations after a query "
                   "is performed (default=false)"),
    llvm::cl::init(false), llvm::cl::cat(klee::ExprCat));
} // namespace

static std::string getQueryLogPath(const char filename[])
{
	//check directoryToWriteLogs exists
	struct stat s;
	if( !(stat(DirectoryToWriteQueryLogs.c_str(),&s) == 0 && S_ISDIR(s.st_mode)) )
	{
          llvm::errs() << "Directory to log queries \""
                       << DirectoryToWriteQueryLogs << "\" does not exist!"
                       << "\n";
          exit(1);
        }

	//check permissions okay
	if( !( (s.st_mode & S_IWUSR) && getuid() == s.st_uid) &&
	    !( (s.st_mode & S_IWGRP) && getgid() == s.st_gid) &&
	    !( s.st_mode & S_IWOTH)
	)
	{
          llvm::errs() << "Directory to log queries \""
                       << DirectoryToWriteQueryLogs << "\" is not writable!"
                       << "\n";
          exit(1);
        }

	std::string path = DirectoryToWriteQueryLogs;
	path += "/";
	path += filename;
	return path;
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
    llvm::outs() << "(Token \"" << T.getKindName() << "\" "
                 << "\"" << escapedString(T.start, T.length) << "\" "
                 << T.length << " " << T.line << " " << T.column << ")\n";
  } while (T.kind != Token::EndOfFile);
}

static bool PrintInputAST(const char *Filename,
                          const MemoryBuffer *MB,
                          ExprBuilder *Builder) {
  std::vector<Decl*> Decls;
  Parser *P = Parser::Create(Filename, MB, Builder, ClearArrayAfterQuery);
  P->SetMaxErrors(20);

  unsigned NumQueries = 0;
  while (Decl *D = P->ParseTopLevelDecl()) {
    if (!P->GetNumErrors()) {
      if (isa<QueryCommand>(D))
        llvm::outs() << "# Query " << ++NumQueries << "\n";

      D->dump();
    }
    Decls.push_back(D);
  }

  bool success = true;
  if (unsigned N = P->GetNumErrors()) {
    llvm::errs() << Filename << ": parse failure: " << N << " errors.\n";
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
  Parser *P = Parser::Create(Filename, MB, Builder, ClearArrayAfterQuery);
  P->SetMaxErrors(20);
  while (Decl *D = P->ParseTopLevelDecl()) {
    Decls.push_back(D);
  }

  bool success = true;
  if (unsigned N = P->GetNumErrors()) {
    llvm::errs() << Filename << ": parse failure: " << N << " errors.\n";
    success = false;
  }  

  if (!success)
    return false;

  Solver *coreSolver = klee::createCoreSolver(CoreSolverToUse);

  if (CoreSolverToUse != DUMMY_SOLVER) {
    const time::Span maxCoreSolverTime(MaxCoreSolverTime);
    if (maxCoreSolverTime) {
      coreSolver->setCoreSolverTimeout(maxCoreSolverTime);
    }
  }

  Solver *S = constructSolverChain(coreSolver,
                                   getQueryLogPath(ALL_QUERIES_SMT2_FILE_NAME),
                                   getQueryLogPath(SOLVER_QUERIES_SMT2_FILE_NAME),
                                   getQueryLogPath(ALL_QUERIES_KQUERY_FILE_NAME),
                                   getQueryLogPath(SOLVER_QUERIES_KQUERY_FILE_NAME));

  unsigned Index = 0;
  for (std::vector<Decl*>::iterator it = Decls.begin(),
         ie = Decls.end(); it != ie; ++it) {
    Decl *D = *it;
    if (QueryCommand *QC = dyn_cast<QueryCommand>(D)) {
      llvm::outs() << "Query " << Index << ":\t";

      assert("FIXME: Support counterexample query commands!");
      if (QC->Values.empty() && QC->Objects.empty()) {
        bool result;
        if (S->mustBeTrue(Query(ConstraintManager(QC->Constraints), QC->Query),
                          result)) {
          llvm::outs() << (result ? "VALID" : "INVALID");
        } else {
          llvm::outs() << "FAIL (reason: "
                    << SolverImpl::getOperationStatusString(S->impl->getOperationStatusCode())
                    << ")";
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
          llvm::outs() << "INVALID\n";
          llvm::outs() << "\tExpr 0:\t" << result;
        } else {
          llvm::outs() << "FAIL (reason: "
                    << SolverImpl::getOperationStatusString(S->impl->getOperationStatusCode())
                    << ")";
        }
      } else {
        std::vector< std::vector<unsigned char> > result;
        
        if (S->getInitialValues(Query(ConstraintManager(QC->Constraints), 
                                      QC->Query),
                                QC->Objects, result)) {
          llvm::outs() << "INVALID\n";

          for (unsigned i = 0, e = result.size(); i != e; ++i) {
            llvm::outs() << "\tArray " << i << ":\t"
                       << QC->Objects[i]->name
                       << "[";
            for (unsigned j = 0; j != QC->Objects[i]->size; ++j) {
              llvm::outs() << (unsigned) result[i][j];
              if (j + 1 != QC->Objects[i]->size)
                llvm::outs() << ", ";
            }
            llvm::outs() << "]";
            if (i + 1 != e)
              llvm::outs() << "\n";
          }
        } else {
          SolverImpl::SolverRunStatus retCode = S->impl->getOperationStatusCode();
          if (SolverImpl::SOLVER_RUN_STATUS_TIMEOUT == retCode) {
            llvm::outs() << " FAIL (reason: "
                      << SolverImpl::getOperationStatusString(retCode)
                      << ")";
          }           
          else {
            llvm::outs() << "VALID (counterexample request ignored)";
          }
        }
      }

      llvm::outs() << "\n";
      ++Index;
    }
  }

  for (std::vector<Decl*>::iterator it = Decls.begin(),
         ie = Decls.end(); it != ie; ++it)
    delete *it;
  delete P;

  delete S;

  if (uint64_t queries = *theStatisticManager->getStatisticByName("Queries")) {
    llvm::outs()
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
        Parser *P = Parser::Create(Filename, MB, Builder, ClearArrayAfterQuery);
        P->SetMaxErrors(20);
	while (Decl *D = P->ParseTopLevelDecl())
	{
		Decls.push_back(D);
	}

	bool success = true;
	if (unsigned N = P->GetNumErrors())
	{
		llvm::errs() << Filename << ": parse failure: "
				   << N << " errors.\n";
		success = false;
	}

	if (!success)
	return false;

	ExprSMTLIBPrinter printer;
	printer.setOutput(llvm::outs());

	unsigned int queryNumber = 0;
	//Loop over the declarations
	for (std::vector<Decl*>::iterator it = Decls.begin(), ie = Decls.end(); it != ie; ++it)
	{
		Decl *D = *it;
		if (QueryCommand *QC = dyn_cast<QueryCommand>(D))
		{
			//print line break to separate from previous query
			if(queryNumber!=0) 	llvm::outs() << "\n";

			//Output header for this query as a SMT-LIBv2 comment
			llvm::outs() << ";SMTLIBv2 Query " << queryNumber << "\n";

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
			printer.setQuery(query);

			if(!QC->Objects.empty())
				printer.setArrayValuesToGet(QC->Objects);

			printer.generateOutput();


			queryNumber++;
		}
	}

	//Clean up
	for (std::vector<Decl*>::iterator it = Decls.begin(),
			ie = Decls.end(); it != ie; ++it)
		delete *it;
	delete P;

	return true;
}

int main(int argc, char **argv) {

  KCommandLine::HideOptions(llvm::cl::GeneralCategory);

  bool success = true;

#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 9)
  llvm::sys::PrintStackTraceOnErrorSignal(argv[0]);
#else
  llvm::sys::PrintStackTraceOnErrorSignal();
#endif
  llvm::cl::SetVersionPrinter(klee::printVersion);
  llvm::cl::ParseCommandLineOptions(argc, argv);

  std::string ErrorStr;
  
  auto MBResult = MemoryBuffer::getFileOrSTDIN(InputFile.c_str());
  if (!MBResult) {
    llvm::errs() << argv[0] << ": error: " << MBResult.getError().message()
                 << "\n";
    return 1;
  }
  std::unique_ptr<MemoryBuffer> &MB = *MBResult;
  
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
    PrintInputTokens(MB.get());
    break;
  case PrintAST:
    success = PrintInputAST(InputFile=="-" ? "<stdin>" : InputFile.c_str(), MB.get(),
                            Builder);
    break;
  case Evaluate:
    success = EvaluateInputAST(InputFile=="-" ? "<stdin>" : InputFile.c_str(),
                               MB.get(), Builder);
    break;
  case PrintSMTLIBv2:
    success = printInputAsSMTLIBv2(InputFile=="-"? "<stdin>" : InputFile.c_str(), MB.get(),Builder);
    break;
  default:
    llvm::errs() << argv[0] << ": error: Unknown program action!\n";
  }

  delete Builder;
  llvm::llvm_shutdown();
  return success ? 0 : 1;
}
