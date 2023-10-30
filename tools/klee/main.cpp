/* -*- mode: c++; c-basic-offset: 2; -*- */

//===-- main.cpp ------------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/ADT/KTest.h"
#include "klee/ADT/TreeStream.h"
#include "klee/Config/Version.h"
#include "klee/Core/Interpreter.h"
#include "klee/Expr/Expr.h"
#include "klee/Solver/SolverCmdLine.h"
#include "klee/Statistics/Statistics.h"
#include "klee/Support/Debug.h"
#include "klee/Support/ErrorHandling.h"
#include "klee/Support/FileHandling.h"
#include "klee/Support/ModuleUtil.h"
#include "klee/Support/OptionCategories.h"
#include "klee/Support/PrintVersion.h"
#include "klee/System/Time.h"

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/Bitcode/BitcodeReader.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Errno.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/Support/Signals.h"
#include "llvm/Support/TargetSelect.h"
DISABLE_WARNING_POP

#include <dirent.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <sstream>

using namespace llvm;
using namespace klee;

namespace {
  cl::opt<std::string>
  InputFile(cl::desc("<input bytecode>"), cl::Positional, cl::init("-"));

  cl::list<std::string>
  InputArgv(cl::ConsumeAfter,
            cl::desc("<program arguments>..."));


  /*** Test case options ***/

  cl::OptionCategory TestCaseCat("Test case options",
                                 "These options select the files to generate for each test case.");

  cl::opt<bool>
  WriteNone("write-no-tests",
            cl::init(false),
            cl::desc("Do not generate any test files (default=false)"),
            cl::cat(TestCaseCat));

  cl::opt<bool>
  WriteCVCs("write-cvcs",
            cl::desc("Write .cvc files for each test case (default=false)"),
            cl::cat(TestCaseCat));

  cl::opt<bool>
  WriteKQueries("write-kqueries",
                cl::desc("Write .kquery files for each test case (default=false)"),
                cl::cat(TestCaseCat));

  cl::opt<bool>
  WriteSMT2s("write-smt2s",
             cl::desc("Write .smt2 (SMT-LIBv2) files for each test case (default=false)"),
             cl::cat(TestCaseCat));

  cl::opt<bool>
  WriteCov("write-cov",
           cl::desc("Write coverage information for each test case (default=false)"),
           cl::cat(TestCaseCat));

  cl::opt<bool>
  WriteTestInfo("write-test-info",
                cl::desc("Write additional test case information (default=false)"),
                cl::cat(TestCaseCat));

  cl::opt<bool>
  WritePaths("write-paths",
             cl::desc("Write .path files for each test case (default=false)"),
             cl::cat(TestCaseCat));

  cl::opt<bool>
  WriteSymPaths("write-sym-paths",
                cl::desc("Write .sym.path files for each test case (default=false)"),
                cl::cat(TestCaseCat));


  /*** Startup options ***/

  cl::OptionCategory StartCat("Startup options",
                              "These options affect how execution is started.");

  cl::opt<std::string>
  EntryPoint("entry-point",
             cl::desc("Function in which to start execution (default=main)"),
             cl::init("main"),
             cl::cat(StartCat));

  cl::opt<std::string>
  RunInDir("run-in-dir",
           cl::desc("Change to the given directory before starting execution (default=location of tested file)."),
           cl::cat(StartCat));
  
  cl::opt<std::string>
  OutputDir("output-dir",
            cl::desc("Directory in which to write results (default=klee-out-<N>)"),
            cl::init(""),
            cl::cat(StartCat));

  cl::opt<std::string>
  Environ("env-file",
          cl::desc("Parse environment from the given file (in \"env\" format)"),
          cl::cat(StartCat));

  cl::opt<bool>
  OptimizeModule("optimize",
                 cl::desc("Optimize the code before execution (default=false)."),
		 cl::init(false),
                 cl::cat(StartCat));

  cl::opt<bool>
  WarnAllExternals("warn-all-external-symbols",
                   cl::desc("Issue a warning on startup for all external symbols (default=false)."),
                   cl::cat(StartCat));
  

  /*** Linking options ***/

  cl::OptionCategory LinkCat("Linking options",
                             "These options control the libraries being linked.");

  enum class LibcType { FreestandingLibc, KleeLibc, UcLibc };

  cl::opt<LibcType> Libc(
      "libc", cl::desc("Choose libc version (none by default)."),
      cl::values(
          clEnumValN(
              LibcType::FreestandingLibc, "none",
              "Don't link in a libc (only provide freestanding environment)"),
          clEnumValN(LibcType::KleeLibc, "klee", "Link in KLEE's libc"),
          clEnumValN(LibcType::UcLibc, "uclibc",
                     "Link in uclibc (adapted for KLEE)")),
      cl::init(LibcType::FreestandingLibc), cl::cat(LinkCat));

  cl::list<std::string>
      LinkLibraries("link-llvm-lib",
                    cl::desc("Link the given bitcode library before execution, "
                             "e.g. .bca, .bc, .a. Can be used multiple times."),
                    cl::value_desc("bitcode library file"), cl::cat(LinkCat));

  cl::opt<bool>
  WithPOSIXRuntime("posix-runtime",
                   cl::desc("Link with POSIX runtime. Options that can be passed as arguments to the programs are: --sym-arg <max-len>  --sym-args <min-argvs> <max-argvs> <max-len> + file model options (default=false)."),
                   cl::init(false),
                   cl::cat(LinkCat));

  cl::opt<bool> WithUBSanRuntime("ubsan-runtime",
                                 cl::desc("Link with UBSan runtime."),
                                 cl::init(false), cl::cat(LinkCat));

  cl::opt<std::string> RuntimeBuild(
      "runtime-build",
      cl::desc("Link with versions of the runtime library that were built with "
               "the provided configuration (default=" RUNTIME_CONFIGURATION
               ")."),
      cl::init(RUNTIME_CONFIGURATION), cl::cat(LinkCat));

  /*** Checks options ***/

  cl::OptionCategory ChecksCat("Checks options",
                               "These options control some of the checks being done by KLEE.");

  cl::opt<bool>
  CheckDivZero("check-div-zero",
               cl::desc("Inject checks for division-by-zero (default=true)"),
               cl::init(true),
               cl::cat(ChecksCat));

  cl::opt<bool>
  CheckOvershift("check-overshift",
                 cl::desc("Inject checks for overshift (default=true)"),
                 cl::init(true),
                 cl::cat(ChecksCat));



  cl::opt<bool>
  OptExitOnError("exit-on-error",
                 cl::desc("Exit KLEE if an error in the tested application has been found (default=false)"),
                 cl::init(false),
                 cl::cat(TerminationCat));


  /*** Replaying options ***/
  
  cl::OptionCategory ReplayCat("Replaying options",
                               "These options impact replaying of test cases.");
  
  cl::list<std::string>
  ReplayKTestFile("replay-ktest-file",
                  cl::desc("Specify a .ktest file to use for replay"),
                  cl::value_desc(".ktest file"),
                  cl::cat(ReplayCat));

  cl::list<std::string>
  ReplayKTestDir("replay-ktest-dir",
                 cl::desc("Specify a directory to replay .ktest files from"),
                 cl::value_desc("output directory"),
                 cl::cat(ReplayCat));

  cl::opt<std::string>
  ReplayPathFile("replay-path",
                 cl::desc("Specify a path file to replay"),
                 cl::value_desc("path file"),
                 cl::cat(ReplayCat));



  cl::list<std::string>
  SeedOutFile("seed-file",
              cl::desc(".ktest file to be used as seed"),
              cl::cat(SeedingCat));

  cl::list<std::string>
  SeedOutDir("seed-dir",
             cl::desc("Directory with .ktest files to be used as seeds"),
             cl::cat(SeedingCat));

  cl::opt<unsigned>
  MakeConcreteSymbolic("make-concrete-symbolic",
                       cl::desc("Probabilistic rate at which to make concrete reads symbolic, "
				"i.e. approximately 1 in n concrete reads will be made symbolic (0=off, 1=all).  "
				"Used for testing (default=0)"),
                       cl::init(0),
                       cl::cat(DebugCat));

  cl::opt<unsigned>
  MaxTests("max-tests",
           cl::desc("Stop execution after generating the given number of tests. Extra tests corresponding to partially explored paths will also be dumped.  Set to 0 to disable (default=0)"),
           cl::init(0),
           cl::cat(TerminationCat));

  cl::opt<bool>
  Watchdog("watchdog",
           cl::desc("Use a watchdog process to enforce --max-time (default=false)"),
           cl::init(false),
           cl::cat(TerminationCat));

  cl::opt<bool>
  Libcxx("libcxx",
           cl::desc("Link the llvm libc++ library into the bitcode (default=false)"),
           cl::init(false),
           cl::cat(LinkCat));
}

namespace klee {
extern cl::opt<std::string> MaxTime;
class ExecutionState;
}

/***/

class KleeHandler : public InterpreterHandler {
private:
  Interpreter *m_interpreter;
  TreeStreamWriter *m_pathWriter, *m_symPathWriter;
  std::unique_ptr<llvm::raw_ostream> m_infoFile;

  SmallString<128> m_outputDirectory;

  unsigned m_numTotalTests;     // Number of tests received from the interpreter
  unsigned m_numGeneratedTests; // Number of tests successfully generated
  unsigned m_pathsCompleted; // number of completed paths
  unsigned m_pathsExplored; // number of partially explored and completed paths

  // used for writing .ktest files
  int m_argc;
  char **m_argv;

public:
  KleeHandler(int argc, char **argv);
  ~KleeHandler();

  llvm::raw_ostream &getInfoStream() const { return *m_infoFile; }
  /// Returns the number of test cases successfully generated so far
  unsigned getNumTestCases() { return m_numGeneratedTests; }
  unsigned getNumPathsCompleted() { return m_pathsCompleted; }
  unsigned getNumPathsExplored() { return m_pathsExplored; }
  void incPathsCompleted() { ++m_pathsCompleted; }
  void incPathsExplored(std::uint32_t num = 1) {
    m_pathsExplored += num; }

  void setInterpreter(Interpreter *i);

  void processTestCase(const ExecutionState  &state,
                       const char *errorMessage,
                       const char *errorSuffix);

  std::string getOutputFilename(const std::string &filename);
  std::unique_ptr<llvm::raw_fd_ostream> openOutputFile(const std::string &filename);
  std::string getTestFilename(const std::string &suffix, unsigned id);
  std::unique_ptr<llvm::raw_fd_ostream> openTestFile(const std::string &suffix, unsigned id);

  // load a .path file
  static void loadPathFile(std::string name,
                           std::vector<bool> &buffer);

  static void getKTestFilesInDir(std::string directoryPath,
                                 std::vector<std::string> &results);

  static std::string getRunTimeLibraryPath(const char *argv0);
};

KleeHandler::KleeHandler(int argc, char **argv)
    : m_interpreter(0), m_pathWriter(0), m_symPathWriter(0),
      m_outputDirectory(), m_numTotalTests(0), m_numGeneratedTests(0),
      m_pathsCompleted(0), m_pathsExplored(0), m_argc(argc), m_argv(argv) {

  // create output directory (OutputDir or "klee-out-<i>")
  bool dir_given = OutputDir != "";
  SmallString<128> directory(dir_given ? OutputDir : InputFile);

  if (!dir_given) sys::path::remove_filename(directory);
  if (auto ec = sys::fs::make_absolute(directory)) {
    klee_error("unable to determine absolute path: %s", ec.message().c_str());
  }

  if (dir_given) {
    // OutputDir
    if (mkdir(directory.c_str(), 0775) < 0)
      klee_error("cannot create \"%s\": %s", directory.c_str(), strerror(errno));

    m_outputDirectory = directory;
  } else {
    // "klee-out-<i>"
    int i = 0;
    for (; i < INT_MAX; ++i) {
      SmallString<128> d(directory);
      llvm::sys::path::append(d, "klee-out-");
      raw_svector_ostream ds(d);
      ds << i;
      // SmallString is always up-to-date, no need to flush. See Support/raw_ostream.h

      // create directory and try to link klee-last
      if (mkdir(d.c_str(), 0775) == 0) {
        m_outputDirectory = d;

        SmallString<128> klee_last(directory);
        llvm::sys::path::append(klee_last, "klee-last");

        if ((unlink(klee_last.c_str()) < 0) && (errno != ENOENT)) {
          klee_warning("cannot remove existing klee-last symlink: %s",
                       strerror(errno));
        }

        size_t offset = m_outputDirectory.size() -
                        llvm::sys::path::filename(m_outputDirectory).size();
        if (symlink(m_outputDirectory.c_str() + offset, klee_last.c_str()) <
            0) {
          klee_warning("cannot create klee-last symlink: %s", strerror(errno));
        }

        break;
      }

      // otherwise try again or exit on error
      if (errno != EEXIST)
        klee_error("cannot create \"%s\": %s", m_outputDirectory.c_str(), strerror(errno));
    }
    if (i == INT_MAX && m_outputDirectory.str().equals(""))
        klee_error("cannot create output directory: index out of range");
  }

  klee_message("output directory is \"%s\"", m_outputDirectory.c_str());

  // open warnings.txt
  std::string file_path = getOutputFilename("warnings.txt");
  if ((klee_warning_file = fopen(file_path.c_str(), "w")) == NULL)
    klee_error("cannot open file \"%s\": %s", file_path.c_str(), strerror(errno));

  // open messages.txt
  file_path = getOutputFilename("messages.txt");
  if ((klee_message_file = fopen(file_path.c_str(), "w")) == NULL)
    klee_error("cannot open file \"%s\": %s", file_path.c_str(), strerror(errno));

  // open info
  m_infoFile = openOutputFile("info");
}

KleeHandler::~KleeHandler() {
  delete m_pathWriter;
  delete m_symPathWriter;
  fclose(klee_warning_file);
  fclose(klee_message_file);
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
  SmallString<128> path = m_outputDirectory;
  sys::path::append(path,filename);
  return path.c_str();
}

std::unique_ptr<llvm::raw_fd_ostream>
KleeHandler::openOutputFile(const std::string &filename) {
  std::string Error;
  std::string path = getOutputFilename(filename);
  auto f = klee_open_output_file(path, Error);
  if (!f) {
    klee_warning("error opening file \"%s\".  KLEE may have run out of file "
                 "descriptors: try to increase the maximum number of open file "
                 "descriptors by using ulimit (%s).",
                 path.c_str(), Error.c_str());
    return nullptr;
  }
  return f;
}

std::string KleeHandler::getTestFilename(const std::string &suffix, unsigned id) {
  std::stringstream filename;
  filename << "test" << std::setfill('0') << std::setw(6) << id << '.' << suffix;
  return filename.str();
}

std::unique_ptr<llvm::raw_fd_ostream>
KleeHandler::openTestFile(const std::string &suffix, unsigned id) {
  return openOutputFile(getTestFilename(suffix, id));
}


/* Outputs all files (.ktest, .kquery, .cov etc.) describing a test case */
void KleeHandler::processTestCase(const ExecutionState &state,
                                  const char *errorMessage,
                                  const char *errorSuffix) {
  if (!WriteNone) {
    std::vector< std::pair<std::string, std::vector<unsigned char> > > out;
    bool success = m_interpreter->getSymbolicSolution(state, out);

    if (!success)
      klee_warning("unable to get symbolic solution, losing test case");

    const auto start_time = time::getWallTime();

    unsigned id = ++m_numTotalTests;

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

      if (!kTest_toFile(&b, getOutputFilename(getTestFilename("ktest", id)).c_str())) {
        klee_warning("unable to write output test case, losing it");
      } else {
        ++m_numGeneratedTests;
      }

      for (unsigned i=0; i<b.numObjects; i++)
        delete[] b.objects[i].bytes;
      delete[] b.objects;
    }

    if (errorMessage) {
      auto f = openTestFile(errorSuffix, id);
      if (f)
        *f << errorMessage;
    }

    if (m_pathWriter) {
      std::vector<unsigned char> concreteBranches;
      m_pathWriter->readStream(m_interpreter->getPathStreamID(state),
                               concreteBranches);
      auto f = openTestFile("path", id);
      if (f) {
        for (const auto &branch : concreteBranches) {
          *f << branch << '\n';
        }
      }
    }

    if (errorMessage || WriteKQueries) {
      std::string constraints;
      m_interpreter->getConstraintLog(state, constraints,Interpreter::KQUERY);
      auto f = openTestFile("kquery", id);
      if (f)
        *f << constraints;
    }

    if (WriteCVCs) {
      // FIXME: If using Z3 as the core solver the emitted file is actually
      // SMT-LIBv2 not CVC which is a bit confusing
      std::string constraints;
      m_interpreter->getConstraintLog(state, constraints, Interpreter::STP);
      auto f = openTestFile("cvc", id);
      if (f)
        *f << constraints;
    }

    if (WriteSMT2s) {
      std::string constraints;
        m_interpreter->getConstraintLog(state, constraints, Interpreter::SMTLIB2);
        auto f = openTestFile("smt2", id);
        if (f)
          *f << constraints;
    }

    if (m_symPathWriter) {
      std::vector<unsigned char> symbolicBranches;
      m_symPathWriter->readStream(m_interpreter->getSymbolicPathStreamID(state),
                                  symbolicBranches);
      auto f = openTestFile("sym.path", id);
      if (f) {
        for (const auto &branch : symbolicBranches) {
          *f << branch << '\n';
        }
      }
    }

    if (WriteCov) {
      std::map<const std::string*, std::set<unsigned> > cov;
      m_interpreter->getCoveredLines(state, cov);
      auto f = openTestFile("cov", id);
      if (f) {
        for (const auto &entry : cov) {
          for (const auto &line : entry.second) {
            *f << *entry.first << ':' << line << '\n';
          }
        }
      }
    }

    if (m_numGeneratedTests == MaxTests)
      m_interpreter->setHaltExecution(true);

    if (WriteTestInfo) {
      time::Span elapsed_time(time::getWallTime() - start_time);
      auto f = openTestFile("info", id);
      if (f)
        *f << "Time to generate test case: " << elapsed_time << '\n';
    }
  } // if (!WriteNone)

  if (errorMessage && OptExitOnError) {
    m_interpreter->prepareForEarlyExit();
    klee_error("EXITING ON ERROR:\n%s\n", errorMessage);
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
    if (f.good())
      buffer.push_back(!!value);
    f.get();
  }
}

void KleeHandler::getKTestFilesInDir(std::string directoryPath,
                                     std::vector<std::string> &results) {
  std::error_code ec;
  llvm::sys::fs::directory_iterator i(directoryPath, ec), e;
  for (; i != e && !ec; i.increment(ec)) {
    auto f = i->path();
    if (f.size() >= 6 && f.substr(f.size()-6,f.size()) == ".ktest") {
      results.push_back(f);
    }
  }

  if (ec) {
    llvm::errs() << "ERROR: unable to read output directory: " << directoryPath
                 << ": " << ec.message() << "\n";
    exit(1);
  }
}

std::string KleeHandler::getRunTimeLibraryPath(const char *argv0) {
  // allow specifying the path to the runtime library
  const char *env = getenv("KLEE_RUNTIME_LIBRARY_PATH");
  if (env)
    return std::string(env);

  // Take any function from the execution binary but not main (as not allowed by
  // C++ standard)
  void *MainExecAddr = (void *)(intptr_t)getRunTimeLibraryPath;
  SmallString<128> toolRoot(
      llvm::sys::fs::getMainExecutable(argv0, MainExecAddr)
      );

  // Strip off executable so we have a directory path
  llvm::sys::path::remove_filename(toolRoot);

  SmallString<128> libDir;

  if (strlen( KLEE_INSTALL_BIN_DIR ) != 0 &&
      strlen( KLEE_INSTALL_RUNTIME_DIR ) != 0 &&
      toolRoot.str().endswith( KLEE_INSTALL_BIN_DIR ))
  {
    KLEE_DEBUG_WITH_TYPE("klee_runtime", llvm::dbgs() <<
                         "Using installed KLEE library runtime: ");
    libDir = toolRoot.str().substr(0,
               toolRoot.str().size() - strlen( KLEE_INSTALL_BIN_DIR ));
    llvm::sys::path::append(libDir, KLEE_INSTALL_RUNTIME_DIR);
  }
  else
  {
    KLEE_DEBUG_WITH_TYPE("klee_runtime", llvm::dbgs() <<
                         "Using build directory KLEE library runtime :");
    libDir = KLEE_DIR;
    llvm::sys::path::append(libDir, "runtime/lib");
  }

  KLEE_DEBUG_WITH_TYPE("klee_runtime", llvm::dbgs() <<
                       libDir.c_str() << "\n");
  return libDir.c_str();
}

//===----------------------------------------------------------------------===//
// main Driver function
//

static Function *mainFn = nullptr;
static Function *entryFn = nullptr;

static std::string strip(std::string &in) {
  unsigned len = in.size();
  unsigned lead = 0, trail = len;
  while (lead<len && isspace(in[lead]))
    ++lead;
  while (trail>lead && isspace(in[trail-1]))
    --trail;
  return in.substr(lead, trail-lead);
}

static void parseArguments(int argc, char **argv) {
  cl::SetVersionPrinter(klee::printVersion);
  // This version always reads response files
  cl::ParseCommandLineOptions(argc, argv, " klee\n");
}

static void
preparePOSIX(std::vector<std::unique_ptr<llvm::Module>> &loadedModules,
             llvm::StringRef libCPrefix) {
  mainFn->setName("__klee_posix_wrapped_main");

  // Add a definition of the main function if needed. This is the case if we
  // link against a libc implementation. Preparing for libc linking (i.e.
  // linking with uClibc will expect a main function and rename it to
  // _user_main. We just provide the definition here.
  if (!libCPrefix.empty() && !mainFn->getParent()->getFunction("main"))
    llvm::Function::Create(mainFn->getFunctionType(),
                           llvm::Function::ExternalLinkage, "main",
                           mainFn->getParent());

  llvm::Function *wrapper = nullptr;
  for (auto &module : loadedModules) {
    wrapper = module->getFunction("__klee_posix_wrapper");
    if (wrapper)
      break;
  }
  assert(wrapper && "klee_posix_wrapper not found");

  // Rename the POSIX wrapper to prefixed entrypoint, e.g. _user_main as uClibc
  // would expect it or main otherwise
  wrapper->setName(libCPrefix + "main");
}


// This is a terrible hack until we get some real modeling of the
// system. All we do is check the undefined symbols and warn about
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
  "__errno_location",
  "__error",
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
  "klee_get_valuef",
  "klee_get_valued",
  "klee_get_valuel",
  "klee_get_valuell",
  "klee_get_value_i32",
  "klee_get_value_i64",
  "klee_get_obj_size",
  "klee_is_symbolic",
  "klee_make_symbolic",
  "klee_mark_global",
  "klee_open_merge",
  "klee_close_merge",
  "klee_prefer_cex",
  "klee_posix_prefer_cex",
  "klee_print_expr",
  "klee_print_range",
  "klee_report_error",
  "klee_set_forking",
  "klee_silent_exit",
  "klee_warning",
  "klee_warning_once",
  "klee_stack_trace",
#ifdef SUPPORT_KLEE_EH_CXX
  "_klee_eh_Unwind_RaiseException_impl",
  "klee_eh_typeid_for",
#endif
  "llvm.dbg.declare",
  "llvm.dbg.value",
  "llvm.va_start",
  "llvm.va_end",
  "malloc",
  "realloc",
  "memalign",
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

  // I/O system calls
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
void externalsAndGlobalsCheck(const llvm::Module *m) {
  std::map<std::string, bool> externals;
  std::set<std::string> modelled(modelledExternals,
                                 modelledExternals+NELEMS(modelledExternals));
  std::set<std::string> dontCare(dontCareExternals,
                                 dontCareExternals+NELEMS(dontCareExternals));
  std::set<std::string> unsafe(unsafeExternals,
                               unsafeExternals+NELEMS(unsafeExternals));

  switch (Libc) {
  case LibcType::KleeLibc:
    dontCare.insert(dontCareKlee, dontCareKlee+NELEMS(dontCareKlee));
    break;
  case LibcType::UcLibc:
    dontCare.insert(dontCareUclibc,
                    dontCareUclibc+NELEMS(dontCareUclibc));
    break;
  case LibcType::FreestandingLibc: /* silence compiler warning */
    break;
  }

  if (WithPOSIXRuntime)
    dontCare.insert("syscall");

  for (Module::const_iterator fnIt = m->begin(), fn_ie = m->end();
       fnIt != fn_ie; ++fnIt) {
    if (fnIt->isDeclaration() && !fnIt->use_empty())
      externals.insert(std::make_pair(fnIt->getName(), false));
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
        externals.find(it->getName().str());
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
      if (ext.compare(0, 5, "llvm.") != 0) { // not an LLVM reserved name
        if (unsafe.count(ext)) {
          foundUnsafe.insert(*it);
        } else {
          klee_warning("undefined reference to %s: %s",
                       it->second ? "variable" : "function",
                       ext.c_str());
        }
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
    llvm::errs() << "KLEE: ctrl-c detected, requesting interpreter to halt.\n";
    halt_execution();
    sys::SetInterruptFunction(interrupt_handle);
  } else {
    llvm::errs() << "KLEE: ctrl-c detected, exiting.\n";
    exit(1);
  }
  interrupted = true;
}

static void interrupt_handle_watchdog() {
  // just wait for the child to finish
}

// This is a temporary hack. If the running process has access to
// externals then it can disable interrupts, which screws up the
// normal "nice" watchdog termination process. We try to request the
// interpreter to halt using this mechanism as a last resort to save
// the state data before going ahead and killing it.
static void halt_via_gdb(int pid) {
  char buffer[256];
  snprintf(buffer, sizeof(buffer),
          "gdb --batch --eval-command=\"p halt_execution()\" "
          "--eval-command=detach --pid=%d &> /dev/null",
          pid);
  //  fprintf(stderr, "KLEE: WATCHDOG: running: %s\n", buffer);
  if (system(buffer)==-1)
    perror("system");
}

static void replaceOrRenameFunction(llvm::Module *module,
		const char *old_name, const char *new_name)
{
  Function *new_function, *old_function;
  new_function = module->getFunction(new_name);
  old_function = module->getFunction(old_name);
  if (old_function) {
    if (new_function) {
      old_function->replaceAllUsesWith(new_function);
      old_function->eraseFromParent();
    } else {
      old_function->setName(new_name);
      assert(old_function->getName() == new_name);
    }
  }
}

static void
createLibCWrapper(std::vector<std::unique_ptr<llvm::Module>> &modules,
                  llvm::StringRef intendedFunction,
                  llvm::StringRef libcMainFunction) {
  // We now need to swap things so that libcMainFunction is the entry
  // point, in such a way that the arguments are passed to
  // libcMainFunction correctly. We do this by renaming the user main
  // and generating a stub function to call intendedFunction. There is
  // also an implicit cooperation in that runFunctionAsMain sets up
  // the environment arguments to what a libc expects (following
  // argv), since it does not explicitly take an envp argument.
  auto &ctx = modules[0]->getContext();
  Function *userMainFn = modules[0]->getFunction(intendedFunction);
  assert(userMainFn && "unable to get user main");
  // Rename entry point using a prefix
  userMainFn->setName("__user_" + intendedFunction);

  // force import of libcMainFunction
  llvm::Function *libcMainFn = nullptr;
  for (auto &module : modules) {
    if ((libcMainFn = module->getFunction(libcMainFunction)))
      break;
  }
  if (!libcMainFn)
    klee_error("Could not add %s wrapper", libcMainFunction.str().c_str());

  auto inModuleReference = libcMainFn->getParent()->getOrInsertFunction(
      userMainFn->getName(), userMainFn->getFunctionType());

  const auto ft = libcMainFn->getFunctionType();

  if (ft->getNumParams() != 7)
    klee_error("Imported %s wrapper does not have the correct "
               "number of arguments",
               libcMainFunction.str().c_str());

  std::vector<Type *> fArgs;
  fArgs.push_back(ft->getParamType(1)); // argc
  fArgs.push_back(ft->getParamType(2)); // argv
  Function *stub =
      Function::Create(FunctionType::get(Type::getInt32Ty(ctx), fArgs, false),
                       GlobalVariable::ExternalLinkage, intendedFunction,
                       libcMainFn->getParent());
  BasicBlock *bb = BasicBlock::Create(ctx, "entry", stub);
  llvm::IRBuilder<> Builder(bb);

  std::vector<llvm::Value *> args;
  args.push_back(llvm::ConstantExpr::getBitCast(
      cast<llvm::Constant>(inModuleReference.getCallee()),
      ft->getParamType(0)));
  args.push_back(&*(stub->arg_begin())); // argc
  auto arg_it = stub->arg_begin();
  args.push_back(&*(++arg_it));                                // argv
  args.push_back(Constant::getNullValue(ft->getParamType(3))); // app_init
  args.push_back(Constant::getNullValue(ft->getParamType(4))); // app_fini
  args.push_back(Constant::getNullValue(ft->getParamType(5))); // rtld_fini
  args.push_back(Constant::getNullValue(ft->getParamType(6))); // stack_end
  Builder.CreateCall(libcMainFn, args);
  Builder.CreateUnreachable();
}

static void
linkWithUclibc(StringRef libDir, std::string opt_suffix,
               std::vector<std::unique_ptr<llvm::Module>> &modules) {
  LLVMContext &ctx = modules[0]->getContext();

  size_t newModules = modules.size();

  // Ensure that klee-uclibc exists
  SmallString<128> uclibcBCA(libDir);
  std::string errorMsg;
  llvm::sys::path::append(uclibcBCA, KLEE_UCLIBC_BCA_NAME);
  if (!klee::loadFile(uclibcBCA.c_str(), ctx, modules, errorMsg))
    klee_error("Cannot find klee-uclibc '%s': %s", uclibcBCA.c_str(),
               errorMsg.c_str());

  for (auto i = newModules, j = modules.size(); i < j; ++i) {
    replaceOrRenameFunction(modules[i].get(), "__libc_open", "open");
    replaceOrRenameFunction(modules[i].get(), "__libc_fcntl", "fcntl");
  }

  if (mainFn)
    createLibCWrapper(modules, "main", "__uClibc_main");
  klee_message("NOTE: Using klee-uclibc : %s", uclibcBCA.c_str());

  // Link the fortified library
  SmallString<128> FortifyPath(libDir);
  llvm::sys::path::append(FortifyPath,
                          "libkleeRuntimeFortify" + opt_suffix + ".bca");
  if (!klee::loadFile(FortifyPath.c_str(), ctx, modules, errorMsg))
    klee_error("error loading the fortify library '%s': %s",
               FortifyPath.c_str(), errorMsg.c_str());
}

int main(int argc, char **argv, char **envp) {
  atexit(llvm_shutdown); // Call llvm_shutdown() on exit

  KCommandLine::KeepOnlyCategories(
     {&ChecksCat,      &DebugCat,    &ExtCallsCat, &ExprCat,     &LinkCat,
      &MemoryCat,      &MergeCat,    &MiscCat,     &ModuleCat,   &ReplayCat,
      &SearchCat,      &SeedingCat,  &SolvingCat,  &StartCat,    &StatsCat,
      &TerminationCat, &TestCaseCat, &TestGenCat,  &ExecTreeCat, &ExecTreeCat});
  llvm::InitializeNativeTarget();

  parseArguments(argc, argv);
  sys::PrintStackTraceOnErrorSignal(argv[0]);

  if (Watchdog) {
    if (MaxTime.empty()) {
      klee_error("--watchdog used without --max-time");
    }

    int pid = fork();
    if (pid<0) {
      klee_error("unable to fork watchdog");
    } else if (pid) {
      klee_message("KLEE: WATCHDOG: watching %d\n", pid);
      fflush(stderr);
      sys::SetInterruptFunction(interrupt_handle_watchdog);

      const time::Span maxTime(MaxTime);
      auto nextStep = time::getWallTime() + maxTime + (maxTime / 10);
      int level = 0;

      while (1) {
        sleep(1);

        int status, res = waitpid(pid, &status, WNOHANG);

        if (res < 0) {
          if (errno == ECHILD) {
            // No child, no need to watch but return error since
            // we didn't catch the exit.
            klee_warning("KLEE: watchdog exiting (no child)\n");
            return 1;
          } else if (errno != EINTR) {
            perror("watchdog waitpid");
            exit(1);
          }
        } else if (res == pid && WIFEXITED(status)) {
          return WEXITSTATUS(status);
        } else {
          auto time = time::getWallTime();

          if (time > nextStep) {
            ++level;

            if (level==1) {
              klee_warning(
                  "KLEE: WATCHDOG: time expired, attempting halt via INT\n");
              kill(pid, SIGINT);
            } else if (level==2) {
              klee_warning(
                  "KLEE: WATCHDOG: time expired, attempting halt via gdb\n");
              halt_via_gdb(pid);
            } else {
              klee_warning(
                  "KLEE: WATCHDOG: kill(9)ing child (I tried to be nice)\n");
              kill(pid, SIGKILL);
              return 1; // what more can we do?
            }

            // Ideally this triggers a dump, which may take a while,
            // so try and give the process extra time to clean up.
            auto max = std::max(time::seconds(15), maxTime / 10);
            nextStep = time::getWallTime() + max;
          }
        }
      }

      return 0;
    }
  }

  sys::SetInterruptFunction(interrupt_handle);

  // Load the bytecode...
  std::string errorMsg;
  LLVMContext ctx;
#if LLVM_VERSION_CODE == LLVM_VERSION(15, 0)
  // We have to force the upgrade to opaque pointer explicitly for LLVM 15.
  ctx.setOpaquePointers(true);
#endif
  std::vector<std::unique_ptr<llvm::Module>> loadedModules;
  if (!klee::loadFile(InputFile, ctx, loadedModules, errorMsg)) {
    klee_error("error loading program '%s': %s", InputFile.c_str(),
               errorMsg.c_str());
  }
  // Load and link the whole files content. The assumption is that this is the
  // application under test.
  // Nothing gets removed in the first place.
  std::unique_ptr<llvm::Module> M(klee::linkModules(
      loadedModules, "" /* link all modules together */, errorMsg));
  if (!M) {
    klee_error("error loading program '%s': %s", InputFile.c_str(),
               errorMsg.c_str());
  }

  llvm::Module *mainModule = M.get();

  const std::string &module_triple = mainModule->getTargetTriple();
  std::string host_triple = llvm::sys::getDefaultTargetTriple();

  if (module_triple != host_triple)
    klee_warning("Module and host target triples do not match: '%s' != '%s'\n"
                 "This may cause unexpected crashes or assertion violations.",
                 module_triple.c_str(), host_triple.c_str());

  // Detect architecture
  std::string opt_suffix = "64"; // Fall back to 64bit
  if (module_triple.find("i686") != std::string::npos ||
      module_triple.find("i586") != std::string::npos ||
      module_triple.find("i486") != std::string::npos ||
      module_triple.find("i386") != std::string::npos)
    opt_suffix = "32";

  // Add additional user-selected suffix
  opt_suffix += "_" + RuntimeBuild.getValue();

  // Push the module as the first entry
  loadedModules.emplace_back(std::move(M));

  std::string LibraryDir = KleeHandler::getRunTimeLibraryPath(argv[0]);
  Interpreter::ModuleOptions Opts(LibraryDir.c_str(), EntryPoint, opt_suffix,
                                  /*Optimize=*/OptimizeModule,
                                  /*CheckDivZero=*/CheckDivZero,
                                  /*CheckOvershift=*/CheckOvershift);

  // Get the main function
  for (auto &module : loadedModules) {
    mainFn = module->getFunction("main");
    if (mainFn)
      break;
  }

  // Get the entry point function
  if (EntryPoint.empty())
    klee_error("entry-point cannot be empty");

  for (auto &module : loadedModules) {
    entryFn = module->getFunction(EntryPoint);
    if (entryFn)
      break;
  }

  if (!entryFn)
    klee_error("Entry function '%s' not found in module.", EntryPoint.c_str());


  if (WithPOSIXRuntime) {
    SmallString<128> Path(Opts.LibraryDir);
    llvm::sys::path::append(Path, "libkleeRuntimePOSIX" + opt_suffix + ".bca");
    klee_message("NOTE: Using POSIX model: %s", Path.c_str());
    if (!klee::loadFile(Path.c_str(), mainModule->getContext(), loadedModules,
                        errorMsg))
      klee_error("error loading POSIX support '%s': %s", Path.c_str(),
                 errorMsg.c_str());

    std::string libcPrefix = (Libc == LibcType::UcLibc ? "__user_" : "");
    if (mainFn)
      preparePOSIX(loadedModules, libcPrefix);
  }

  if (WithUBSanRuntime) {
    SmallString<128> Path(Opts.LibraryDir);
    llvm::sys::path::append(Path, "libkleeUBSan" + opt_suffix + ".bca");
    if (!klee::loadFile(Path.c_str(), mainModule->getContext(), loadedModules,
                        errorMsg))
      klee_error("error loading UBSan support '%s': %s", Path.c_str(),
                 errorMsg.c_str());
  }

  if (Libcxx) {
#ifndef SUPPORT_KLEE_LIBCXX
    klee_error("KLEE was not compiled with libc++ support");
#else
    SmallString<128> LibcxxBC(Opts.LibraryDir);
    llvm::sys::path::append(LibcxxBC, KLEE_LIBCXX_BC_NAME);
    if (!klee::loadFile(LibcxxBC.c_str(), mainModule->getContext(), loadedModules,
                        errorMsg))
      klee_error("error loading libc++ '%s': %s", LibcxxBC.c_str(),
                 errorMsg.c_str());
    klee_message("NOTE: Using libc++ : %s", LibcxxBC.c_str());
#ifdef SUPPORT_KLEE_EH_CXX
    SmallString<128> EhCxxPath(Opts.LibraryDir);
    llvm::sys::path::append(EhCxxPath, "libkleeeh-cxx" + opt_suffix + ".bca");
    if (!klee::loadFile(EhCxxPath.c_str(), mainModule->getContext(),
                        loadedModules, errorMsg))
      klee_error("error loading libklee-eh-cxx '%s': %s", EhCxxPath.c_str(),
                 errorMsg.c_str());
    klee_message("NOTE: Enabled runtime support for C++ exceptions");
#else
    klee_message("NOTE: KLEE was not compiled with support for C++ exceptions");
#endif
#endif
  }

  switch (Libc) {
  case LibcType::KleeLibc: {
    // FIXME: Find a reasonable solution for this.
    SmallString<128> Path(Opts.LibraryDir);
    llvm::sys::path::append(Path,
                            "libkleeRuntimeKLEELibc" + opt_suffix + ".bca");
    if (!klee::loadFile(Path.c_str(), mainModule->getContext(), loadedModules,
                        errorMsg))
      klee_error("error loading klee libc '%s': %s", Path.c_str(),
                 errorMsg.c_str());
  }
  /* Falls through. */
  case LibcType::FreestandingLibc: {
    SmallString<128> Path(Opts.LibraryDir);
    llvm::sys::path::append(Path,
                            "libkleeRuntimeFreestanding" + opt_suffix + ".bca");
    if (!klee::loadFile(Path.c_str(), mainModule->getContext(), loadedModules,
                        errorMsg))
      klee_error("error loading freestanding support '%s': %s", Path.c_str(),
                 errorMsg.c_str());
    break;
  }
  case LibcType::UcLibc:
    linkWithUclibc(LibraryDir, opt_suffix, loadedModules);
    break;
  }

  for (const auto &library : LinkLibraries) {
    if (!klee::loadFile(library, mainModule->getContext(), loadedModules,
                        errorMsg))
      klee_error("error loading bitcode library '%s': %s", library.c_str(),
                 errorMsg.c_str());
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

  Interpreter::InterpreterOptions IOpts;
  IOpts.MakeConcreteSymbolic = MakeConcreteSymbolic;
  KleeHandler *handler = new KleeHandler(pArgc, pArgv);
  Interpreter *interpreter =
    theInterpreter = Interpreter::create(ctx, IOpts, handler);
  assert(interpreter);
  handler->setInterpreter(interpreter);

  for (int i = 0; i < argc; i++)
    handler->getInfoStream() << argv[i] << (i + 1 < argc ? " " : "\n");
  handler->getInfoStream() << "PID: " << getpid() << "\n";

  // Get the desired main function.  klee_main initializes uClibc
  // locale and other data and then calls main.

  auto finalModule = interpreter->setModule(loadedModules, Opts);
  entryFn = finalModule->getFunction(EntryPoint);
  if (!entryFn)
    klee_error("Entry function '%s' not found in module.", EntryPoint.c_str());

  externalsAndGlobalsCheck(finalModule);

  std::vector<bool> replayPath;
  if (!ReplayPathFile.empty()) {
    KleeHandler::loadPathFile(ReplayPathFile, replayPath);
    interpreter->setReplayPath(&replayPath);
  }

  auto startTime = std::time(nullptr);
  { // output clock info and start time
    std::stringstream startInfo;
    startInfo << time::getClockInfo()
              << "Started: "
              << std::put_time(std::localtime(&startTime), "%Y-%m-%d %H:%M:%S") << '\n';
    handler->getInfoStream() << startInfo.str();
    handler->getInfoStream().flush();
  }

  if (!ReplayKTestDir.empty() || !ReplayKTestFile.empty()) {
    assert(SeedOutFile.empty());
    assert(SeedOutDir.empty());

    std::vector<std::string> kTestFiles = ReplayKTestFile;
    for (std::vector<std::string>::iterator
           it = ReplayKTestDir.begin(), ie = ReplayKTestDir.end();
         it != ie; ++it)
      KleeHandler::getKTestFilesInDir(*it, kTestFiles);
    std::vector<KTest*> kTests;
    for (std::vector<std::string>::iterator
           it = kTestFiles.begin(), ie = kTestFiles.end();
         it != ie; ++it) {
      KTest *out = kTest_fromFile(it->c_str());
      if (out) {
        kTests.push_back(out);
      } else {
        klee_warning("unable to open: %s\n", (*it).c_str());
      }
    }

    if (RunInDir != "") {
      int res = chdir(RunInDir.c_str());
      if (res < 0) {
        klee_error("Unable to change directory to: %s - %s", RunInDir.c_str(),
                   sys::StrError(errno).c_str());
      }
    }

    unsigned i=0;
    for (std::vector<KTest*>::iterator
           it = kTests.begin(), ie = kTests.end();
         it != ie; ++it) {
      KTest *out = *it;
      interpreter->setReplayKTest(out);
      llvm::errs() << "KLEE: replaying: " << *it << " (" << kTest_numBytes(out)
                   << " bytes)"
                   << " (" << ++i << "/" << kTestFiles.size() << ")\n";
      // XXX should put envp in .ktest ?
      interpreter->runFunctionAsMain(entryFn, out->numArgs, out->args, pEnvp);
      if (interrupted) break;
    }
    interpreter->setReplayKTest(0);
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
        klee_error("unable to open: %s\n", (*it).c_str());
      }
      seeds.push_back(out);
    }
    for (std::vector<std::string>::iterator
           it = SeedOutDir.begin(), ie = SeedOutDir.end();
         it != ie; ++it) {
      std::vector<std::string> kTestFiles;
      KleeHandler::getKTestFilesInDir(*it, kTestFiles);
      for (std::vector<std::string>::iterator
             it2 = kTestFiles.begin(), ie = kTestFiles.end();
           it2 != ie; ++it2) {
        KTest *out = kTest_fromFile(it2->c_str());
        if (!out) {
          klee_error("unable to open: %s\n", (*it2).c_str());
        }
        seeds.push_back(out);
      }
      if (kTestFiles.empty()) {
        klee_error("seeds directory is empty: %s\n", (*it).c_str());
      }
    }

    if (!seeds.empty()) {
      klee_message("KLEE: using %lu seeds\n", seeds.size());
      interpreter->useSeeds(&seeds);
    }
    if (RunInDir != "") {
      int res = chdir(RunInDir.c_str());
      if (res < 0) {
        klee_error("Unable to change directory to: %s - %s", RunInDir.c_str(),
                   sys::StrError(errno).c_str());
      }
    }

    interpreter->runFunctionAsMain(entryFn, pArgc, pArgv, pEnvp);

    while (!seeds.empty()) {
      kTest_free(seeds.back());
      seeds.pop_back();
    }
  }

  auto endTime = std::time(nullptr);
  { // output end and elapsed time
    std::uint32_t h;
    std::uint8_t m, s;
    std::tie(h,m,s) = time::seconds(endTime - startTime).toHMS();
    std::stringstream endInfo;
    endInfo << "Finished: "
            << std::put_time(std::localtime(&endTime), "%Y-%m-%d %H:%M:%S") << '\n'
            << "Elapsed: "
            << std::setfill('0') << std::setw(2) << h
            << ':'
            << std::setfill('0') << std::setw(2) << +m
            << ':'
            << std::setfill('0') << std::setw(2) << +s
            << '\n';
            handler->getInfoStream() << endInfo.str();
    handler->getInfoStream().flush();
  }

  // Free all the args.
  for (unsigned i=0; i<InputArgv.size()+1; i++)
    delete[] pArgv[i];
  delete[] pArgv;

  delete interpreter;

  uint64_t queries =
    *theStatisticManager->getStatisticByName("SolverQueries");
  uint64_t queriesValid =
    *theStatisticManager->getStatisticByName("QueriesValid");
  uint64_t queriesInvalid =
    *theStatisticManager->getStatisticByName("QueriesInvalid");
  uint64_t queryCounterexamples =
    *theStatisticManager->getStatisticByName("QueriesCEX");
  uint64_t queryConstructs =
    *theStatisticManager->getStatisticByName("QueryConstructs");
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
  stats << '\n'
        << "KLEE: done: total instructions = " << instructions << '\n'
        << "KLEE: done: completed paths = " << handler->getNumPathsCompleted()
        << '\n'
        << "KLEE: done: partially completed paths = "
        << handler->getNumPathsExplored() - handler->getNumPathsCompleted()
        << '\n'
        << "KLEE: done: generated tests = " << handler->getNumTestCases()
        << '\n';

  bool useColors = llvm::errs().is_displayed();
  if (useColors)
    llvm::errs().changeColor(llvm::raw_ostream::GREEN,
                             /*bold=*/true,
                             /*bg=*/false);

  llvm::errs() << stats.str();

  if (useColors)
    llvm::errs().resetColor();

  handler->getInfoStream() << stats.str();

  delete handler;

  return 0;
}
