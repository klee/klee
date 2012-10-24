/* -*- mode: c++; c-basic-offset: 2; -*- */

// FIXME: This does not belong here.
#include "../lib/Core/Common.h"

#include "klee/ExecutionState.h"
#include "klee/Expr.h"
#include "klee/Interpreter.h"
#include "klee/Statistics.h"
#include "klee/Config/Version.h"
#include "klee/Internal/ADT/KTest.h"
#include "klee/Internal/ADT/TreeStream.h"
#include "klee/Internal/Support/ModuleUtil.h"
#include "klee/Internal/System/Time.h"

#include "llvm/Constants.h"
#include "llvm/Module.h"
#if LLVM_VERSION_CODE < LLVM_VERSION(2, 7)
#include "llvm/ModuleProvider.h"
#endif
#include "llvm/Type.h"
#include "llvm/InstrTypes.h"
#include "llvm/Instruction.h"
#include "llvm/Instructions.h"
#if LLVM_VERSION_CODE >= LLVM_VERSION(2, 7)
#include "llvm/LLVMContext.h"
#endif
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/MemoryBuffer.h"

// FIXME: Ugh, this is gross. But otherwise our config.h conflicts with LLVMs.
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION
#if LLVM_VERSION_CODE < LLVM_VERSION(3, 0)
#include "llvm/Target/TargetSelect.h"
#else
#include "llvm/Support/TargetSelect.h"
#endif
#if LLVM_VERSION_CODE < LLVM_VERSION(2, 9)
#include "llvm/System/Signals.h"
#else
#include "llvm/Support/Signals.h"
#include "llvm/Support/system_error.h"
#endif
#include <iostream>
#include <fstream>
#include <cerrno>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>

#include <iostream>
#include <iterator>
#include <fstream>
#include <sstream>

using namespace llvm;
using namespace klee;

namespace {
  cl::opt<std::string>
  InputFile(cl::desc("<input bytecode>"), cl::Positional, cl::init("-"));

  cl::opt<std::string>
  RunInDir("run-in", cl::desc("Change to the given directory prior to executing"));

  cl::opt<std::string>
  Environ("environ", cl::desc("Parse environ from given file (in \"env\" format)"));

  cl::list<std::string>
  InputArgv(cl::ConsumeAfter, 
            cl::desc("<program arguments>..."));

  cl::opt<bool>
  NoOutput("no-output", 
           cl::desc("Don't generate test files"));
    
  cl::opt<bool>
  WarnAllExternals("warn-all-externals", 
                   cl::desc("Give initial warning for all externals."));
    
  cl::opt<bool>
  WriteCVCs("write-cvcs", 
            cl::desc("Write .cvc files for each test case"));

  cl::opt<bool>
  WritePCs("write-pcs", 
            cl::desc("Write .pc files for each test case"));
  
  cl::opt<bool>
  WriteSMT2s("write-smt2s",
            cl::desc("Write .smt2 (SMT-LIBv2) files for each test case"));

  cl::opt<bool>
  WriteCov("write-cov", 
           cl::desc("Write coverage information for each test case"));
  
  cl::opt<bool>
  WriteTestInfo("write-test-info", 
                cl::desc("Write additional test case information"));
  
  cl::opt<bool>
  WritePaths("write-paths", 
                cl::desc("Write .path files for each test case"));
    
  cl::opt<bool>
  WriteSymPaths("write-sym-paths", 
                cl::desc("Write .sym.path files for each test case"));
    
  cl::opt<bool>
  ExitOnError("exit-on-error", 
              cl::desc("Exit if errors occur"));
    

  enum LibcType {
    NoLibc, KleeLibc, UcLibc
  };

  cl::opt<LibcType>
  Libc("libc", 
       cl::desc("Choose libc version (none by default)."),
       cl::values(clEnumValN(NoLibc, "none", "Don't link in a libc"),
                  clEnumValN(KleeLibc, "klee", "Link in klee libc"),
		  clEnumValN(UcLibc, "uclibc", "Link in uclibc (adapted for klee)"),
		  clEnumValEnd),
       cl::init(NoLibc));

    
  cl::opt<bool>
  WithPOSIXRuntime("posix-runtime", 
		cl::desc("Link with POSIX runtime.  Options that can be passed as arguments to the programs are: --sym-argv <max-len>  --sym-argvs <min-argvs> <max-argvs> <max-len> + file model options"),
		cl::init(false));
    
  cl::opt<bool>
  OptimizeModule("optimize", 
                 cl::desc("Optimize before execution"),
		 cl::init(false));

  cl::opt<bool>
  CheckDivZero("check-div-zero", 
               cl::desc("Inject checks for division-by-zero"),
               cl::init(true));
    
  cl::opt<std::string>
  OutputDir("output-dir", 
            cl::desc("Directory to write results in (defaults to klee-out-N)"),
            cl::init(""));

  // this is a fake entry, its automagically handled
  cl::list<std::string>
  ReadArgsFilesFake("read-args", 
                    cl::desc("File to read arguments from (one arg per line)"));
    
  cl::opt<bool>
  ReplayKeepSymbolic("replay-keep-symbolic", 
                     cl::desc("Replay the test cases only by asserting"
                              "the bytes, not necessarily making them concrete."));
    
  cl::list<std::string>
  ReplayOutFile("replay-out",
                cl::desc("Specify an out file to replay"),
                cl::value_desc("out file"));

  cl::list<std::string>
  ReplayOutDir("replay-out-dir",
	       cl::desc("Specify a directory to replay .out files from"),
	       cl::value_desc("output directory"));

  cl::opt<std::string>
  ReplayPathFile("replay-path",
                 cl::desc("Specify a path file to replay"),
                 cl::value_desc("path file"));

  cl::list<std::string>
  SeedOutFile("seed-out");
  
  cl::list<std::string>
  SeedOutDir("seed-out-dir");
  
  cl::opt<unsigned>
  MakeConcreteSymbolic("make-concrete-symbolic",
                       cl::desc("Rate at which to make concrete reads symbolic (0=off)"),
                       cl::init(0));
 
  cl::opt<unsigned>
  StopAfterNTests("stop-after-n-tests",
	     cl::desc("Stop execution after generating the given number of tests.  Extra tests corresponding to partially explored paths will also be dumped."),
	     cl::init(0));

  cl::opt<bool>
  Watchdog("watchdog",
           cl::desc("Use a watchdog process to enforce --max-time."),
           cl::init(0));
}

extern cl::opt<double> MaxTime;

/***/

class KleeHandler : public InterpreterHandler {
private:
  Interpreter *m_interpreter;
  TreeStreamWriter *m_pathWriter, *m_symPathWriter;
  std::ostream *m_infoFile;

  char m_outputDirectory[1024];
  unsigned m_testIndex;  // number of tests written so far
  unsigned m_pathsExplored; // number of paths explored so far

  // used for writing .ktest files
  int m_argc;
  char **m_argv;

public:
  KleeHandler(int argc, char **argv);
  ~KleeHandler();

  std::ostream &getInfoStream() const { return *m_infoFile; }
  unsigned getNumTestCases() { return m_testIndex; }
  unsigned getNumPathsExplored() { return m_pathsExplored; }
  void incPathsExplored() { m_pathsExplored++; }

  void setInterpreter(Interpreter *i);

  void processTestCase(const ExecutionState  &state,
                       const char *errorMessage, 
                       const char *errorSuffix);

  std::string getOutputFilename(const std::string &filename);
  std::ostream *openOutputFile(const std::string &filename);
  std::string getTestFilename(const std::string &suffix, unsigned id);
  std::ostream *openTestFile(const std::string &suffix, unsigned id);

  // load a .out file
  static void loadOutFile(std::string name, 
                          std::vector<unsigned char> &buffer);

  // load a .path file
  static void loadPathFile(std::string name, 
                           std::vector<bool> &buffer);

  static void getOutFiles(std::string path,
			  std::vector<std::string> &results);
};

KleeHandler::KleeHandler(int argc, char **argv) 
  : m_interpreter(0),
    m_pathWriter(0),
    m_symPathWriter(0),
    m_infoFile(0),
    m_testIndex(0),
    m_pathsExplored(0),
    m_argc(argc),
    m_argv(argv) {
  std::string theDir;

  if (OutputDir=="") {
    llvm::sys::Path directory(InputFile);
    std::string dirname = "";
    directory.eraseComponent();
    
    if (directory.isEmpty())
      directory.set(".");
    
    for (int i = 0; ; i++) {
      char buf[256], tmp[64];
      sprintf(tmp, "klee-out-%d", i);
      dirname = tmp;
      sprintf(buf, "%s/%s", directory.c_str(), tmp);
      theDir = buf;
      
      if (DIR *dir = opendir(theDir.c_str())) {
        closedir(dir);
      } else {
        break;
      }
    }    

    std::cerr << "KLEE: output directory = \"" << dirname << "\"\n";

    llvm::sys::Path klee_last(directory);
    klee_last.appendComponent("klee-last");
      
    if ((unlink(klee_last.c_str()) < 0) && (errno != ENOENT)) {
      perror("Cannot unlink klee-last");
      assert(0 && "exiting.");
    }
    
    if (symlink(dirname.c_str(), klee_last.c_str()) < 0) {
      perror("Cannot make symlink");
      assert(0 && "exiting.");
    }
  } else {
    theDir = OutputDir;
  }
  
  sys::Path p(theDir);
  if (!p.isAbsolute()) {
    sys::Path cwd = sys::Path::GetCurrentDirectory();
    cwd.appendComponent(theDir);
    p = cwd;
  }
  strcpy(m_outputDirectory, p.c_str());

  if (mkdir(m_outputDirectory, 0775) < 0) {
    std::cerr << "KLEE: ERROR: Unable to make output directory: \"" 
               << m_outputDirectory 
               << "\", refusing to overwrite.\n";
    exit(1);
  }

  char fname[1024];
  snprintf(fname, sizeof(fname), "%s/warnings.txt", m_outputDirectory);
  klee_warning_file = fopen(fname, "w");
  assert(klee_warning_file);

  snprintf(fname, sizeof(fname), "%s/messages.txt", m_outputDirectory);
  klee_message_file = fopen(fname, "w");
  assert(klee_message_file);

  m_infoFile = openOutputFile("info");
}

KleeHandler::~KleeHandler() {
  if (m_pathWriter) delete m_pathWriter;
  if (m_symPathWriter) delete m_symPathWriter;
  delete m_infoFile;
}

void KleeHandler::setInterpreter(Interpreter *i) {
  m_interpreter = i;

  if (WritePaths) {
    m_pathWriter = new TreeStreamWriter(getOutputFilename("paths.ts"));
    assert(m_pathWriter->good());
    m_interpreter->setPathWriter(m_pathWriter);
  }

  if (WriteSymPaths) {
    m_symPathWriter = new TreeStreamWriter(getOutputFilename("symPaths.ts"));
    assert(m_symPathWriter->good());
    m_interpreter->setSymbolicPathWriter(m_symPathWriter);
  }
}

std::string KleeHandler::getOutputFilename(const std::string &filename) {
  char outfile[1024];
  sprintf(outfile, "%s/%s", m_outputDirectory, filename.c_str());
  return outfile;
}

std::ostream *KleeHandler::openOutputFile(const std::string &filename) {
  std::ios::openmode io_mode = std::ios::out | std::ios::trunc 
                             | std::ios::binary;
  std::ostream *f;
  std::string path = getOutputFilename(filename);
  f = new std::ofstream(path.c_str(), io_mode);
  if (!f) {
    klee_error("error opening file \"%s\" (out of memory)", filename.c_str());
  } else if (!f->good()) {
    klee_error("error opening file \"%s\".  KLEE may have run out of file descriptors: try to increase the maximum number of open file descriptors by using ulimit.", filename.c_str());
    delete f;
    f = NULL;
  }

  return f;
}

std::string KleeHandler::getTestFilename(const std::string &suffix, unsigned id) {
  char filename[1024];
  sprintf(filename, "test%06d.%s", id, suffix.c_str());
  return getOutputFilename(filename);
}

std::ostream *KleeHandler::openTestFile(const std::string &suffix, unsigned id) {
  char filename[1024];
  sprintf(filename, "test%06d.%s", id, suffix.c_str());
  return openOutputFile(filename);
}


/* Outputs all files (.ktest, .pc, .cov etc.) describing a test case */
void KleeHandler::processTestCase(const ExecutionState &state,
                                  const char *errorMessage, 
                                  const char *errorSuffix) {
  if (errorMessage && ExitOnError) {
    std::cerr << "EXITING ON ERROR:\n" << errorMessage << "\n";
    exit(1);
  }

  if (!NoOutput) {
    std::vector< std::pair<std::string, std::vector<unsigned char> > > out;
    bool success = m_interpreter->getSymbolicSolution(state, out);

    if (!success)
      klee_warning("unable to get symbolic solution, losing test case");

    double start_time = util::getWallTime();

    unsigned id = ++m_testIndex;

    if (success) {
      KTest b;      
      b.numArgs = m_argc;
      b.args = m_argv;
      b.symArgvs = 0;
      b.symArgvLen = 0;
      b.numObjects = out.size();
      b.objects = new KTestObject[b.numObjects];
      assert(b.objects);
      for (unsigned i=0; i<b.numObjects; i++) {
        KTestObject *o = &b.objects[i];
        o->name = const_cast<char*>(out[i].first.c_str());
        o->numBytes = out[i].second.size();
        o->bytes = new unsigned char[o->numBytes];
        assert(o->bytes);
        std::copy(out[i].second.begin(), out[i].second.end(), o->bytes);
      }
      
      if (!kTest_toFile(&b, getTestFilename("ktest", id).c_str())) {
        klee_warning("unable to write output test case, losing it");
      }
      
      for (unsigned i=0; i<b.numObjects; i++)
        delete[] b.objects[i].bytes;
      delete[] b.objects;
    }

    if (errorMessage) {
      std::ostream *f = openTestFile(errorSuffix, id);
      *f << errorMessage;
      delete f;
    }
    
    if (m_pathWriter) {
      std::vector<unsigned char> concreteBranches;
      m_pathWriter->readStream(m_interpreter->getPathStreamID(state),
                               concreteBranches);
      std::ostream *f = openTestFile("path", id);
      std::copy(concreteBranches.begin(), concreteBranches.end(), 
                std::ostream_iterator<unsigned char>(*f, "\n"));
      delete f;
    }
   
    if (errorMessage || WritePCs) {
      std::string constraints;
      m_interpreter->getConstraintLog(state, constraints,Interpreter::KQUERY);
      std::ostream *f = openTestFile("pc", id);
      *f << constraints;
      delete f;
    }

    if (WriteCVCs) {
      std::string constraints;
      m_interpreter->getConstraintLog(state, constraints, Interpreter::STP);
      std::ostream *f = openTestFile("cvc", id);
      *f << constraints;
      delete f;
    }
    
    if(WriteSMT2s) {
    	std::string constraints;
        m_interpreter->getConstraintLog(state, constraints, Interpreter::SMTLIB2);
        std::ostream *f = openTestFile("smt2", id);
        *f << constraints;
        delete f;
    }

    if (m_symPathWriter) {
      std::vector<unsigned char> symbolicBranches;
      m_symPathWriter->readStream(m_interpreter->getSymbolicPathStreamID(state),
                                  symbolicBranches);
      std::ostream *f = openTestFile("sym.path", id);
      std::copy(symbolicBranches.begin(), symbolicBranches.end(), 
                std::ostream_iterator<unsigned char>(*f, "\n"));
      delete f;
    }

    if (WriteCov) {
      std::map<const std::string*, std::set<unsigned> > cov;
      m_interpreter->getCoveredLines(state, cov);
      std::ostream *f = openTestFile("cov", id);
      for (std::map<const std::string*, std::set<unsigned> >::iterator
             it = cov.begin(), ie = cov.end();
           it != ie; ++it) {
        for (std::set<unsigned>::iterator
               it2 = it->second.begin(), ie = it->second.end();
             it2 != ie; ++it2)
          *f << *it->first << ":" << *it2 << "\n";
      }
      delete f;
    }

    if (m_testIndex == StopAfterNTests)
      m_interpreter->setHaltExecution(true);

    if (WriteTestInfo) {
      double elapsed_time = util::getWallTime() - start_time;
      std::ostream *f = openTestFile("info", id);
      *f << "Time to generate test case: " 
         << elapsed_time << "s\n";
      delete f;
    }
  }
}

  // load a .path file
void KleeHandler::loadPathFile(std::string name, 
                                     std::vector<bool> &buffer) {
  std::ifstream f(name.c_str(), std::ios::in | std::ios::binary);

  if (!f.good())
    assert(0 && "unable to open path file");

  while (f.good()) {
    unsigned value;
    f >> value;
    buffer.push_back(!!value);
    f.get();
  }
}

void KleeHandler::getOutFiles(std::string path,
			      std::vector<std::string> &results) {
  llvm::sys::Path p(path);
  std::set<llvm::sys::Path> contents;
  std::string error;
  if (p.getDirectoryContents(contents, &error)) {
    std::cerr << "ERROR: unable to read output directory: " << path 
               << ": " << error << "\n";
    exit(1);
  }
  for (std::set<llvm::sys::Path>::iterator it = contents.begin(),
         ie = contents.end(); it != ie; ++it) {
#if LLVM_VERSION_CODE != LLVM_VERSION(2, 6)
    std::string f = it->str();
#else
    std::string f = it->toString();
#endif
    if (f.substr(f.size()-6,f.size()) == ".ktest") {
      results.push_back(f);
    }
  }
}

//===----------------------------------------------------------------------===//
// main Driver function
//
#if ENABLE_STPLOG == 1
extern "C" void STPLOG_init(const char *);
#endif

static std::string strip(std::string &in) {
  unsigned len = in.size();
  unsigned lead = 0, trail = len;
  while (lead<len && isspace(in[lead]))
    ++lead;
  while (trail>lead && isspace(in[trail-1]))
    --trail;
  return in.substr(lead, trail-lead);
}

static void readArgumentsFromFile(char *file, std::vector<std::string> &results) {
  std::ifstream f(file);
  assert(f.is_open() && "unable to open input for reading arguments");
  while (!f.eof()) {
    std::string line;
    std::getline(f, line);
    line = strip(line);
    if (!line.empty())
      results.push_back(line);
  }
  f.close();
}

static void parseArguments(int argc, char **argv) {
  std::vector<std::string> arguments;

  for (int i=1; i<argc; i++) {
    if (!strcmp(argv[i],"--read-args") && i+1<argc) {
      readArgumentsFromFile(argv[++i], arguments);
    } else {
      arguments.push_back(argv[i]);
    }
  }
    
  int numArgs = arguments.size() + 1;
  const char **argArray = new const char*[numArgs+1];
  argArray[0] = argv[0];
  argArray[numArgs] = 0;
  for (int i=1; i<numArgs; i++) {
    argArray[i] = arguments[i-1].c_str();
  }

  cl::ParseCommandLineOptions(numArgs, (char**) argArray, " klee\n");
  delete[] argArray;
}



static int initEnv(Module *mainModule) {

  /*
    nArgcP = alloc oldArgc->getType()
    nArgvV = alloc oldArgv->getType()
    store oldArgc nArgcP
    store oldArgv nArgvP
    klee_init_environment(nArgcP, nArgvP)
    nArgc = load nArgcP
    nArgv = load nArgvP
    oldArgc->replaceAllUsesWith(nArgc)
    oldArgv->replaceAllUsesWith(nArgv)
  */

  Function *mainFn = mainModule->getFunction("main");
    
  if (mainFn->arg_size() < 2) {
    klee_error("Cannot handle ""--posix-runtime"" when main() has less than two arguments.\n");
  }

  Instruction* firstInst = mainFn->begin()->begin();

  Value* oldArgc = mainFn->arg_begin();
  Value* oldArgv = ++mainFn->arg_begin();
  
  AllocaInst* argcPtr = 
    new AllocaInst(oldArgc->getType(), "argcPtr", firstInst);
  AllocaInst* argvPtr = 
    new AllocaInst(oldArgv->getType(), "argvPtr", firstInst);

  /* Insert void klee_init_env(int* argc, char*** argv) */
  std::vector<const Type*> params;
  params.push_back(Type::getInt32Ty(getGlobalContext()));
  params.push_back(Type::getInt32Ty(getGlobalContext()));
  Function* initEnvFn = 
    cast<Function>(mainModule->getOrInsertFunction("klee_init_env",
                                                   Type::getVoidTy(getGlobalContext()),
                                                   argcPtr->getType(),
                                                   argvPtr->getType(),
                                                   NULL));
  assert(initEnvFn);    
  std::vector<Value*> args;
  args.push_back(argcPtr);
  args.push_back(argvPtr);
#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 0)
  Instruction* initEnvCall = CallInst::Create(initEnvFn, args,
					      "", firstInst);
#else
  Instruction* initEnvCall = CallInst::Create(initEnvFn, args.begin(), args.end(), 
					      "", firstInst);
#endif
  Value *argc = new LoadInst(argcPtr, "newArgc", firstInst);
  Value *argv = new LoadInst(argvPtr, "newArgv", firstInst);
  
  oldArgc->replaceAllUsesWith(argc);
  oldArgv->replaceAllUsesWith(argv);

  new StoreInst(oldArgc, argcPtr, initEnvCall);
  new StoreInst(oldArgv, argvPtr, initEnvCall);

  return 0;
}


// This is a terrible hack until we get some real modelling of the
// system. All we do is check the undefined symbols and m and warn about
// any "unrecognized" externals and about any obviously unsafe ones.

// Symbols we explicitly support
static const char *modelledExternals[] = {
  "_ZTVN10__cxxabiv117__class_type_infoE",
  "_ZTVN10__cxxabiv120__si_class_type_infoE",
  "_ZTVN10__cxxabiv121__vmi_class_type_infoE",

  // special functions
  "_assert", 
  "__assert_fail", 
  "__assert_rtn", 
  "calloc", 
  "_exit", 
  "exit", 
  "free", 
  "abort", 
  "klee_abort", 
  "klee_assume", 
  "klee_check_memory_access",
  "klee_define_fixed_object",
  "klee_get_errno", 
  "klee_get_value",
  "klee_get_obj_size", 
  "klee_is_symbolic", 
  "klee_make_symbolic", 
  "klee_mark_global", 
  "klee_merge", 
  "klee_prefer_cex",
  "klee_print_expr", 
  "klee_print_range", 
  "klee_report_error", 
  "klee_set_forking",
  "klee_silent_exit", 
  "klee_warning", 
  "klee_warning_once", 
  "klee_alias_function",
  "klee_stack_trace",
  "llvm.dbg.stoppoint", 
  "llvm.va_start", 
  "llvm.va_end", 
  "malloc", 
  "realloc", 
  "_ZdaPv", 
  "_ZdlPv", 
  "_Znaj", 
  "_Znwj", 
  "_Znam", 
  "_Znwm", 
};
// Symbols we aren't going to warn about
static const char *dontCareExternals[] = {
#if 0
  // stdio
  "fprintf",
  "fflush",
  "fopen",
  "fclose",
  "fputs_unlocked",
  "putchar_unlocked",
  "vfprintf",
  "fwrite",
  "puts",
  "printf",
  "stdin",
  "stdout",
  "stderr",
  "_stdio_term",
  "__errno_location",
  "fstat",
#endif

  // static information, pretty ok to return
  "getegid",
  "geteuid",
  "getgid",
  "getuid",
  "getpid",
  "gethostname",
  "getpgrp",
  "getppid",
  "getpagesize",
  "getpriority",
  "getgroups",
  "getdtablesize",
  "getrlimit",
  "getrlimit64",
  "getcwd",
  "getwd",
  "gettimeofday",
  "uname",

  // fp stuff we just don't worry about yet
  "frexp",  
  "ldexp",
  "__isnan",
  "__signbit",
};
// Extra symbols we aren't going to warn about with klee-libc
static const char *dontCareKlee[] = {
  "__ctype_b_loc",
  "__ctype_get_mb_cur_max",

  // io system calls
  "open",
  "write",
  "read",
  "close",
};
// Extra symbols we aren't going to warn about with uclibc
static const char *dontCareUclibc[] = {
  "__dso_handle",

  // Don't warn about these since we explicitly commented them out of
  // uclibc.
  "printf",
  "vprintf"
};
// Symbols we consider unsafe
static const char *unsafeExternals[] = {
  "fork", // oh lord
  "exec", // heaven help us
  "error", // calls _exit
  "raise", // yeah
  "kill", // mmmhmmm
};
#define NELEMS(array) (sizeof(array)/sizeof(array[0]))
void externalsAndGlobalsCheck(const Module *m) {
  std::map<std::string, bool> externals;
  std::set<std::string> modelled(modelledExternals, 
                                 modelledExternals+NELEMS(modelledExternals));
  std::set<std::string> dontCare(dontCareExternals, 
                                 dontCareExternals+NELEMS(dontCareExternals));
  std::set<std::string> unsafe(unsafeExternals, 
                               unsafeExternals+NELEMS(unsafeExternals));

  switch (Libc) {
  case KleeLibc: 
    dontCare.insert(dontCareKlee, dontCareKlee+NELEMS(dontCareKlee));
    break;
  case UcLibc:
    dontCare.insert(dontCareUclibc,
                    dontCareUclibc+NELEMS(dontCareUclibc));
    break;
  case NoLibc: /* silence compiler warning */
    break;
  }
  

  if (WithPOSIXRuntime)
    dontCare.insert("syscall");

  for (Module::const_iterator fnIt = m->begin(), fn_ie = m->end(); 
       fnIt != fn_ie; ++fnIt) {
    if (fnIt->isDeclaration() && !fnIt->use_empty())
      externals.insert(std::make_pair(fnIt->getName(), false));
    for (Function::const_iterator bbIt = fnIt->begin(), bb_ie = fnIt->end(); 
         bbIt != bb_ie; ++bbIt) {
      for (BasicBlock::const_iterator it = bbIt->begin(), ie = bbIt->end(); 
           it != ie; ++it) {
        if (const CallInst *ci = dyn_cast<CallInst>(it)) {
          if (isa<InlineAsm>(ci->getCalledValue())) {
            klee_warning_once(&*fnIt,
                              "function \"%s\" has inline asm", 
                              fnIt->getName().data());
          }
        }
      }
    }
  }
  for (Module::const_global_iterator 
         it = m->global_begin(), ie = m->global_end(); 
       it != ie; ++it)
    if (it->isDeclaration() && !it->use_empty())
      externals.insert(std::make_pair(it->getName(), true));
  // and remove aliases (they define the symbol after global
  // initialization)
  for (Module::const_alias_iterator 
         it = m->alias_begin(), ie = m->alias_end(); 
       it != ie; ++it) {
    std::map<std::string, bool>::iterator it2 = 
      externals.find(it->getName());
    if (it2!=externals.end())
      externals.erase(it2);
  }

  std::map<std::string, bool> foundUnsafe;
  for (std::map<std::string, bool>::iterator
         it = externals.begin(), ie = externals.end();
       it != ie; ++it) {
    const std::string &ext = it->first;
    if (!modelled.count(ext) && (WarnAllExternals || 
                                 !dontCare.count(ext))) {
      if (unsafe.count(ext)) {
        foundUnsafe.insert(*it);
      } else {
        klee_warning("undefined reference to %s: %s",
                     it->second ? "variable" : "function",
                     ext.c_str());
      }
    }
  }

  for (std::map<std::string, bool>::iterator
         it = foundUnsafe.begin(), ie = foundUnsafe.end();
       it != ie; ++it) {
    const std::string &ext = it->first;
    klee_warning("undefined reference to %s: %s (UNSAFE)!",
                 it->second ? "variable" : "function",
                 ext.c_str());
  }
}

static Interpreter *theInterpreter = 0;

static bool interrupted = false;

// Pulled out so it can be easily called from a debugger.
extern "C"
void halt_execution() {
  theInterpreter->setHaltExecution(true);
}

extern "C"
void stop_forking() {
  theInterpreter->setInhibitForking(true);
}

static void interrupt_handle() {
  if (!interrupted && theInterpreter) {
    std::cerr << "KLEE: ctrl-c detected, requesting interpreter to halt.\n";
    halt_execution();
    sys::SetInterruptFunction(interrupt_handle);
  } else {
    std::cerr << "KLEE: ctrl-c detected, exiting.\n";
    exit(1);
  }
  interrupted = true;
}

// This is a temporary hack. If the running process has access to
// externals then it can disable interrupts, which screws up the
// normal "nice" watchdog termination process. We try to request the
// interpreter to halt using this mechanism as a last resort to save
// the state data before going ahead and killing it.
static void halt_via_gdb(int pid) {
  char buffer[256];
  sprintf(buffer, 
          "gdb --batch --eval-command=\"p halt_execution()\" "
          "--eval-command=detach --pid=%d &> /dev/null",
          pid);
  //  fprintf(stderr, "KLEE: WATCHDOG: running: %s\n", buffer);
  if (system(buffer)==-1) 
    perror("system");
}

// returns the end of the string put in buf
static char *format_tdiff(char *buf, long seconds)
{
  assert(seconds >= 0);

  long minutes = seconds / 60;  seconds %= 60;
  long hours   = minutes / 60;  minutes %= 60;
  long days    = hours   / 24;  hours   %= 24;

  buf = strrchr(buf, '\0');
  if (days > 0) buf += sprintf(buf, "%ld days, ", days);
  buf += sprintf(buf, "%02ld:%02ld:%02ld", hours, minutes, seconds);
  return buf;
}

#ifndef KLEE_UCLIBC
static llvm::Module *linkWithUclibc(llvm::Module *mainModule) {
  fprintf(stderr, "error: invalid libc, no uclibc support!\n");
  exit(1);
  return 0;
}
#else
static llvm::Module *linkWithUclibc(llvm::Module *mainModule) {
  Function *f;
  // force import of __uClibc_main
  mainModule->getOrInsertFunction("__uClibc_main",
                                  FunctionType::get(Type::getVoidTy(getGlobalContext()),
                                               std::vector<LLVM_TYPE_Q Type*>(),
                                                    true));
  
  // force various imports
  if (WithPOSIXRuntime) {
    LLVM_TYPE_Q llvm::Type *i8Ty = Type::getInt8Ty(getGlobalContext());
    mainModule->getOrInsertFunction("realpath",
                                    PointerType::getUnqual(i8Ty),
                                    PointerType::getUnqual(i8Ty),
                                    PointerType::getUnqual(i8Ty),
                                    NULL);
    mainModule->getOrInsertFunction("getutent",
                                    PointerType::getUnqual(i8Ty),
                                    NULL);
    mainModule->getOrInsertFunction("__fgetc_unlocked",
                                    Type::getInt32Ty(getGlobalContext()),
                                    PointerType::getUnqual(i8Ty),
                                    NULL);
    mainModule->getOrInsertFunction("__fputc_unlocked",
                                    Type::getInt32Ty(getGlobalContext()),
                                    Type::getInt32Ty(getGlobalContext()),
                                    PointerType::getUnqual(i8Ty),
                                    NULL);
  }

  f = mainModule->getFunction("__ctype_get_mb_cur_max");
  if (f) f->setName("_stdlib_mb_cur_max");

  // Strip of asm prefixes for 64 bit versions because they are not
  // present in uclibc and we want to make sure stuff will get
  // linked. In the off chance that both prefixed and unprefixed
  // versions are present in the module, make sure we don't create a
  // naming conflict.
  for (Module::iterator fi = mainModule->begin(), fe = mainModule->end();
       fi != fe;) {
    Function *f = fi;
    ++fi;
    const std::string &name = f->getName();
    if (name[0]=='\01') {
      unsigned size = name.size();
      if (name[size-2]=='6' && name[size-1]=='4') {
        std::string unprefixed = name.substr(1);

        // See if the unprefixed version exists.
        if (Function *f2 = mainModule->getFunction(unprefixed)) {
          f->replaceAllUsesWith(f2);
          f->eraseFromParent();
        } else {
          f->setName(unprefixed);
        }
      }
    }
  }
  
  mainModule = klee::linkWithLibrary(mainModule, 
                                     KLEE_UCLIBC "/lib/libc.a");
  assert(mainModule && "unable to link with uclibc");

  // more sighs, this is horrible but just a temp hack
  //    f = mainModule->getFunction("__fputc_unlocked");
  //    if (f) f->setName("fputc_unlocked");
  //    f = mainModule->getFunction("__fgetc_unlocked");
  //    if (f) f->setName("fgetc_unlocked");
  
  Function *f2;
  f = mainModule->getFunction("open");
  f2 = mainModule->getFunction("__libc_open");
  if (f2) {
    if (f) {
      f2->replaceAllUsesWith(f);
      f2->eraseFromParent();
    } else {
      f2->setName("open");
      assert(f2->getName() == "open");
    }
  }

  f = mainModule->getFunction("fcntl");
  f2 = mainModule->getFunction("__libc_fcntl");
  if (f2) {
    if (f) {
      f2->replaceAllUsesWith(f);
      f2->eraseFromParent();
    } else {
      f2->setName("fcntl");
      assert(f2->getName() == "fcntl");
    }
  }

  // XXX we need to rearchitect so this can also be used with
  // programs externally linked with uclibc.

  // We now need to swap things so that __uClibc_main is the entry
  // point, in such a way that the arguments are passed to
  // __uClibc_main correctly. We do this by renaming the user main
  // and generating a stub function to call __uClibc_main. There is
  // also an implicit cooperation in that runFunctionAsMain sets up
  // the environment arguments to what uclibc expects (following
  // argv), since it does not explicitly take an envp argument.
  Function *userMainFn = mainModule->getFunction("main");
  assert(userMainFn && "unable to get user main");    
  Function *uclibcMainFn = mainModule->getFunction("__uClibc_main");
  assert(uclibcMainFn && "unable to get uclibc main");    
  userMainFn->setName("__user_main");

  const FunctionType *ft = uclibcMainFn->getFunctionType();
  assert(ft->getNumParams() == 7);

  std::vector<LLVM_TYPE_Q Type*> fArgs;
  fArgs.push_back(ft->getParamType(1)); // argc
  fArgs.push_back(ft->getParamType(2)); // argv
  Function *stub = Function::Create(FunctionType::get(Type::getInt32Ty(getGlobalContext()), fArgs, false),
      			      GlobalVariable::ExternalLinkage,
      			      "main",
      			      mainModule);
  BasicBlock *bb = BasicBlock::Create(getGlobalContext(), "entry", stub);

  std::vector<llvm::Value*> args;
  args.push_back(llvm::ConstantExpr::getBitCast(userMainFn, 
                                                ft->getParamType(0)));
  args.push_back(stub->arg_begin()); // argc
  args.push_back(++stub->arg_begin()); // argv    
  args.push_back(Constant::getNullValue(ft->getParamType(3))); // app_init
  args.push_back(Constant::getNullValue(ft->getParamType(4))); // app_fini
  args.push_back(Constant::getNullValue(ft->getParamType(5))); // rtld_fini
  args.push_back(Constant::getNullValue(ft->getParamType(6))); // stack_end
#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 0)
  CallInst::Create(uclibcMainFn, args, "", bb);
#else
  CallInst::Create(uclibcMainFn, args.begin(), args.end(), "", bb);
#endif
  
  new UnreachableInst(getGlobalContext(), bb);

  return mainModule;
}
#endif

int main(int argc, char **argv, char **envp) {  
#if ENABLE_STPLOG == 1
  STPLOG_init("stplog.c");
#endif

  atexit(llvm_shutdown);  // Call llvm_shutdown() on exit.

  llvm::InitializeNativeTarget();

  parseArguments(argc, argv);
  sys::PrintStackTraceOnErrorSignal();

  if (Watchdog) {
    if (MaxTime==0) {
      klee_error("--watchdog used without --max-time");
    }

    int pid = fork();
    if (pid<0) {
      klee_error("unable to fork watchdog");
    } else if (pid) {
      fprintf(stderr, "KLEE: WATCHDOG: watching %d\n", pid);
      fflush(stderr);

      double nextStep = util::getWallTime() + MaxTime*1.1;
      int level = 0;

      // Simple stupid code...
      while (1) {
        sleep(1);

        int status, res = waitpid(pid, &status, WNOHANG);

        if (res < 0) {
          if (errno==ECHILD) { // No child, no need to watch but
                               // return error since we didn't catch
                               // the exit.
            fprintf(stderr, "KLEE: watchdog exiting (no child)\n");
            return 1;
          } else if (errno!=EINTR) {
            perror("watchdog waitpid");
            exit(1);
          }
        } else if (res==pid && WIFEXITED(status)) {
          return WEXITSTATUS(status);
        } else {
          double time = util::getWallTime();

          if (time > nextStep) {
            ++level;
            
            if (level==1) {
              fprintf(stderr, "KLEE: WATCHDOG: time expired, attempting halt via INT\n");
              kill(pid, SIGINT);
            } else if (level==2) {
              fprintf(stderr, "KLEE: WATCHDOG: time expired, attempting halt via gdb\n");
              halt_via_gdb(pid);
            } else {
              fprintf(stderr, "KLEE: WATCHDOG: kill(9)ing child (I tried to be nice)\n");
              kill(pid, SIGKILL);
              return 1; // what more can we do
            }

            // Ideally this triggers a dump, which may take a while,
            // so try and give the process extra time to clean up.
            nextStep = util::getWallTime() + std::max(15., MaxTime*.1);
          }
        }
      }

      return 0;
    }
  }

  sys::SetInterruptFunction(interrupt_handle);

  // Load the bytecode...
  std::string ErrorMsg;
#if LLVM_VERSION_CODE < LLVM_VERSION(2, 7)
  ModuleProvider *MP = 0;
  if (MemoryBuffer *Buffer = MemoryBuffer::getFileOrSTDIN(InputFile, &ErrorMsg)) {
    MP = getBitcodeModuleProvider(Buffer, getGlobalContext(), &ErrorMsg);
    if (!MP) delete Buffer;
  }
  
  if (!MP)
    klee_error("error loading program '%s': %s", InputFile.c_str(), ErrorMsg.c_str());

  Module *mainModule = MP->materializeModule();
  MP->releaseModule();
  delete MP;
#else
  Module *mainModule = 0;
#if LLVM_VERSION_CODE < LLVM_VERSION(2, 9)
  MemoryBuffer *Buffer = MemoryBuffer::getFileOrSTDIN(InputFile, &ErrorMsg);
  if (Buffer) {
    mainModule = getLazyBitcodeModule(Buffer, getGlobalContext(), &ErrorMsg);
    if (!mainModule) delete Buffer;
  }
#else
  OwningPtr<MemoryBuffer> BufferPtr;
  error_code ec=MemoryBuffer::getFileOrSTDIN(InputFile.c_str(), BufferPtr);
  if (ec) {
    klee_error("error loading program '%s': %s", InputFile.c_str(),
               ec.message().c_str());
  }
  mainModule = getLazyBitcodeModule(BufferPtr.get(), getGlobalContext(), &ErrorMsg);

#endif
  if (mainModule) {
    if (mainModule->MaterializeAllPermanently(&ErrorMsg)) {
      delete mainModule;
      mainModule = 0;
    }
  }
  if (!mainModule)
    klee_error("error loading program '%s': %s", InputFile.c_str(),
               ErrorMsg.c_str());
#endif

  if (WithPOSIXRuntime) {
    int r = initEnv(mainModule);
    if (r != 0)
      return r;
  }

#if defined(KLEE_LIB_DIR) && defined(USE_KLEE_LIB_DIR)
  /* KLEE_LIB_DIR is the lib dir of installed files as opposed to 
   * where libs in the klee source tree are generated.
   */
  llvm::sys::Path LibraryDir(KLEE_LIB_DIR);
#else
  llvm::sys::Path LibraryDir(KLEE_DIR "/" RUNTIME_CONFIGURATION "/lib");
#endif
  Interpreter::ModuleOptions Opts(LibraryDir.c_str(),
                                  /*Optimize=*/OptimizeModule, 
                                  /*CheckDivZero=*/CheckDivZero);
  
  switch (Libc) {
  case NoLibc: /* silence compiler warning */
    break;

  case KleeLibc: {
    // FIXME: Find a reasonable solution for this.
    llvm::sys::Path Path(Opts.LibraryDir);
    Path.appendComponent("libklee-libc.bca");
    mainModule = klee::linkWithLibrary(mainModule, Path.c_str());
    assert(mainModule && "unable to link with klee-libc");
    break;
  }

  case UcLibc:
    mainModule = linkWithUclibc(mainModule);
    break;
  }

  if (WithPOSIXRuntime) {
    llvm::sys::Path Path(Opts.LibraryDir);
    Path.appendComponent("libkleeRuntimePOSIX.bca");
    klee_message("NOTE: Using model: %s", Path.c_str());
    mainModule = klee::linkWithLibrary(mainModule, Path.c_str());
    assert(mainModule && "unable to link with simple model");
  }  

  // Get the desired main function.  klee_main initializes uClibc
  // locale and other data and then calls main.
  Function *mainFn = mainModule->getFunction("main");
  if (!mainFn) {
    std::cerr << "'main' function not found in module.\n";
    return -1;
  }

  // FIXME: Change me to std types.
  int pArgc;
  char **pArgv;
  char **pEnvp;
  if (Environ != "") {
    std::vector<std::string> items;
    std::ifstream f(Environ.c_str());
    if (!f.good())
      klee_error("unable to open --environ file: %s", Environ.c_str());
    while (!f.eof()) {
      std::string line;
      std::getline(f, line);
      line = strip(line);
      if (!line.empty())
        items.push_back(line);
    }
    f.close();
    pEnvp = new char *[items.size()+1];
    unsigned i=0;
    for (; i != items.size(); ++i)
      pEnvp[i] = strdup(items[i].c_str());
    pEnvp[i] = 0;
  } else {
    pEnvp = envp;
  }

  pArgc = InputArgv.size() + 1; 
  pArgv = new char *[pArgc];
  for (unsigned i=0; i<InputArgv.size()+1; i++) {
    std::string &arg = (i==0 ? InputFile : InputArgv[i-1]);
    unsigned size = arg.size() + 1;
    char *pArg = new char[size];
    
    std::copy(arg.begin(), arg.end(), pArg);
    pArg[size - 1] = 0;
    
    pArgv[i] = pArg;
  }

  std::vector<bool> replayPath;

  if (ReplayPathFile != "") {
    KleeHandler::loadPathFile(ReplayPathFile, replayPath);
  }

  Interpreter::InterpreterOptions IOpts;
  IOpts.MakeConcreteSymbolic = MakeConcreteSymbolic;
  KleeHandler *handler = new KleeHandler(pArgc, pArgv);
  Interpreter *interpreter = 
    theInterpreter = Interpreter::create(IOpts, handler);
  handler->setInterpreter(interpreter);
  
  std::ostream &infoFile = handler->getInfoStream();
  for (int i=0; i<argc; i++) {
    infoFile << argv[i] << (i+1<argc ? " ":"\n");
  }
  infoFile << "PID: " << getpid() << "\n";

  const Module *finalModule = 
    interpreter->setModule(mainModule, Opts);
  externalsAndGlobalsCheck(finalModule);

  if (ReplayPathFile != "") {
    interpreter->setReplayPath(&replayPath);
  }

  char buf[256];
  time_t t[2];
  t[0] = time(NULL);
  strftime(buf, sizeof(buf), "Started: %Y-%m-%d %H:%M:%S\n", localtime(&t[0]));
  infoFile << buf;
  infoFile.flush();

  if (!ReplayOutDir.empty() || !ReplayOutFile.empty()) {
    assert(SeedOutFile.empty());
    assert(SeedOutDir.empty());

    std::vector<std::string> outFiles = ReplayOutFile;
    for (std::vector<std::string>::iterator
           it = ReplayOutDir.begin(), ie = ReplayOutDir.end();
         it != ie; ++it)
      KleeHandler::getOutFiles(*it, outFiles);    
    std::vector<KTest*> kTests;
    for (std::vector<std::string>::iterator
           it = outFiles.begin(), ie = outFiles.end();
         it != ie; ++it) {
      KTest *out = kTest_fromFile(it->c_str());
      if (out) {
        kTests.push_back(out);
      } else {
        std::cerr << "KLEE: unable to open: " << *it << "\n";
      }
    }

    if (RunInDir != "") {
      int res = chdir(RunInDir.c_str());
      if (res < 0) {
        klee_error("Unable to change directory to: %s", RunInDir.c_str());
      }
    }

    unsigned i=0;
    for (std::vector<KTest*>::iterator
           it = kTests.begin(), ie = kTests.end();
         it != ie; ++it) {
      KTest *out = *it;
      interpreter->setReplayOut(out);
      std::cerr << "KLEE: replaying: " << *it << " (" << kTest_numBytes(out) << " bytes)"
                 << " (" << ++i << "/" << outFiles.size() << ")\n";
      // XXX should put envp in .ktest ?
      interpreter->runFunctionAsMain(mainFn, out->numArgs, out->args, pEnvp);
      if (interrupted) break;
    }
    interpreter->setReplayOut(0);
    while (!kTests.empty()) {
      kTest_free(kTests.back());
      kTests.pop_back();
    }
  } else {
    std::vector<KTest *> seeds;
    for (std::vector<std::string>::iterator
           it = SeedOutFile.begin(), ie = SeedOutFile.end();
         it != ie; ++it) {
      KTest *out = kTest_fromFile(it->c_str());
      if (!out) {
        std::cerr << "KLEE: unable to open: " << *it << "\n";
        exit(1);
      }
      seeds.push_back(out);
    } 
    for (std::vector<std::string>::iterator
           it = SeedOutDir.begin(), ie = SeedOutDir.end();
         it != ie; ++it) {
      std::vector<std::string> outFiles;
      KleeHandler::getOutFiles(*it, outFiles);
      for (std::vector<std::string>::iterator
             it2 = outFiles.begin(), ie = outFiles.end();
           it2 != ie; ++it2) {
        KTest *out = kTest_fromFile(it2->c_str());
        if (!out) {
          std::cerr << "KLEE: unable to open: " << *it2 << "\n";
          exit(1);
        }
        seeds.push_back(out);
      }
      if (outFiles.empty()) {
        std::cerr << "KLEE: seeds directory is empty: " << *it << "\n";
        exit(1);
      }
    }
       
    if (!seeds.empty()) {
      std::cerr << "KLEE: using " << seeds.size() << " seeds\n";
      interpreter->useSeeds(&seeds);
    }
    if (RunInDir != "") {
      int res = chdir(RunInDir.c_str());
      if (res < 0) {
        klee_error("Unable to change directory to: %s", RunInDir.c_str());
      }
    }
    interpreter->runFunctionAsMain(mainFn, pArgc, pArgv, pEnvp);

    while (!seeds.empty()) {
      kTest_free(seeds.back());
      seeds.pop_back();
    }
  }
      
  t[1] = time(NULL);
  strftime(buf, sizeof(buf), "Finished: %Y-%m-%d %H:%M:%S\n", localtime(&t[1]));
  infoFile << buf;

  strcpy(buf, "Elapsed: ");
  strcpy(format_tdiff(buf, t[1] - t[0]), "\n");
  infoFile << buf;

  // Free all the args.
  for (unsigned i=0; i<InputArgv.size()+1; i++)
    delete[] pArgv[i];
  delete[] pArgv;

  delete interpreter;

  uint64_t queries = 
    *theStatisticManager->getStatisticByName("Queries");
  uint64_t queriesValid = 
    *theStatisticManager->getStatisticByName("QueriesValid");
  uint64_t queriesInvalid = 
    *theStatisticManager->getStatisticByName("QueriesInvalid");
  uint64_t queryCounterexamples = 
    *theStatisticManager->getStatisticByName("QueriesCEX");
  uint64_t queryConstructs = 
    *theStatisticManager->getStatisticByName("QueriesConstructs");
  uint64_t instructions = 
    *theStatisticManager->getStatisticByName("Instructions");
  uint64_t forks = 
    *theStatisticManager->getStatisticByName("Forks");

  handler->getInfoStream() 
    << "KLEE: done: explored paths = " << 1 + forks << "\n";

  // Write some extra information in the info file which users won't
  // necessarily care about or understand.
  if (queries)
    handler->getInfoStream() 
      << "KLEE: done: avg. constructs per query = " 
                             << queryConstructs / queries << "\n";  
  handler->getInfoStream() 
    << "KLEE: done: total queries = " << queries << "\n"
    << "KLEE: done: valid queries = " << queriesValid << "\n"
    << "KLEE: done: invalid queries = " << queriesInvalid << "\n"
    << "KLEE: done: query cex = " << queryCounterexamples << "\n";

  std::stringstream stats;
  stats << "\n";
  stats << "KLEE: done: total instructions = " 
        << instructions << "\n";
  stats << "KLEE: done: completed paths = " 
        << handler->getNumPathsExplored() << "\n";
  stats << "KLEE: done: generated tests = " 
        << handler->getNumTestCases() << "\n";
  std::cerr << stats.str();
  handler->getInfoStream() << stats.str();

#if LLVM_VERSION_CODE >= LLVM_VERSION(2, 9)
  BufferPtr.take();
#endif
  delete handler;

  return 0;
}
