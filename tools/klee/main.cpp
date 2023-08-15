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
#include "klee/Core/Context.h"
#include "klee/Core/Interpreter.h"
#include "klee/Core/TargetedExecutionReporter.h"
#include "klee/Module/LocationInfo.h"
#include "klee/Module/SarifReport.h"
#include "klee/Module/TargetForest.h"
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
#include "llvm/IR/Instruction.h"
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

#include <csignal>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

#include <cerrno>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <llvm/IR/InstIterator.h>
#include <sstream>

using json = nlohmann::json;

using namespace llvm;
using namespace klee;

namespace {
cl::opt<std::string> InputFile(cl::desc("<input bytecode>"), cl::Positional,
                               cl::init("-"));

cl::list<std::string> InputArgv(cl::ConsumeAfter,
                                cl::desc("<program arguments>..."));

/*** Test case options ***/

cl::OptionCategory TestCaseCat(
    "Test case options",
    "These options select the files to generate for each test case.");

cl::opt<bool>
    WriteNone("write-no-tests", cl::init(false),
              cl::desc("Do not generate any test files (default=false)"),
              cl::cat(TestCaseCat));

cl::opt<bool> WriteKTests(
    "write-ktests", cl::init(true),
    cl::desc("Write .ktest files for each test case (default=true)"),
    cl::cat(TestCaseCat));

cl::opt<bool>
    WriteCVCs("write-cvcs",
              cl::desc("Write .cvc files for each test case (default=false)"),
              cl::cat(TestCaseCat));

cl::opt<bool> WriteKQueries(
    "write-kqueries",
    cl::desc("Write .kquery files for each test case (default=false)"),
    cl::cat(TestCaseCat));

cl::opt<bool> WriteSMT2s(
    "write-smt2s",
    cl::desc(
        "Write .smt2 (SMT-LIBv2) files for each test case (default=false)"),
    cl::cat(TestCaseCat));

cl::opt<bool> WriteCov(
    "write-cov",
    cl::desc("Write coverage information for each test case (default=false)"),
    cl::cat(TestCaseCat));

cl::opt<bool> WriteTestInfo(
    "write-test-info",
    cl::desc("Write additional test case information (default=false)"),
    cl::cat(TestCaseCat));

cl::opt<bool>
    WritePaths("write-paths",
               cl::desc("Write .path files for each test case (default=false)"),
               cl::cat(TestCaseCat));

cl::opt<bool> WriteSymPaths(
    "write-sym-paths",
    cl::desc("Write .sym.path files for each test case (default=false)"),
    cl::cat(TestCaseCat));

cl::opt<bool> WriteKPaths(
    "write-kpaths",
    cl::desc("Write .kpath files for each test case (default=false)"),
    cl::cat(TestCaseCat));

cl::opt<bool>
    WriteStates("write-states", cl::init(false),
                cl::desc("Write state info for debug (default=false)"),
                cl::cat(TestCaseCat));

/*** Startup options ***/

cl::OptionCategory StartCat("Startup options",
                            "These options affect how execution is started.");

cl::opt<std::string>
    EntryPoint("entry-point",
               cl::desc("Function in which to start execution (default=main)"),
               cl::init("main"), cl::cat(StartCat));

cl::opt<bool> UTBotMode("utbot", cl::desc("Klee was launched by utbot"),
                        cl::init(false), cl::cat(StartCat));

cl::opt<bool> InteractiveMode("interactive",
                              cl::desc("Launch klee in interactive mode."),
                              cl::init(false), cl::cat(StartCat));

cl::opt<int> TimeoutPerFunction("timeout-per-function",
                                cl::desc("Timeout per function in klee."),
                                cl::init(0), cl::cat(StartCat));

cl::opt<std::string>
    EntryPointsFile("entrypoints-file",
                    cl::desc("Path to file with entrypoints name."),
                    cl::init("entrypoints.txt"), cl::cat(StartCat));

const int MAX_PROCESS_NUMBER = 50;
cl::opt<int> ProcessNumber("process-number",
                           cl::desc("Number of parallel process in klee, must "
                                    "lie in [1, 50] (default = 1)."),
                           cl::init(1), cl::cat(StartCat));

cl::opt<std::string>
    AnalysisReproduce("analysis-reproduce",
                      cl::desc("Path of JSON file containing static analysis "
                               "paths to be reproduced"),
                      cl::init(""), cl::cat(StartCat));

cl::opt<std::string>
    RunInDir("run-in-dir",
             cl::desc("Change to the given directory before starting execution "
                      "(default=location of tested file)."),
             cl::cat(StartCat));

cl::opt<std::string> OutputDir(
    "output-dir",
    cl::desc("Directory in which to write results (default=klee-out-<N>)"),
    cl::init(""), cl::cat(StartCat));

cl::opt<std::string> Environ(
    "env-file",
    cl::desc("Parse environment from the given file (in \"env\" format)"),
    cl::cat(StartCat));

cl::opt<bool> OptimizeModule(
    "optimize", cl::desc("Optimize the code before execution (default=false)."),
    cl::init(false), cl::cat(StartCat));

cl::opt<bool> SimplifyModule(
    "simplify", cl::desc("Simplify the code before execution (default=true)."),
    cl::init(true), cl::cat(StartCat));

cl::opt<bool> WarnAllExternals(
    "warn-all-external-symbols",
    cl::desc(
        "Issue a warning on startup for all external symbols (default=false)."),
    cl::cat(StartCat));

cl::opt<Interpreter::GuidanceKind> UseGuidedSearch(
    "use-guided-search",
    cl::values(clEnumValN(Interpreter::GuidanceKind::NoGuidance, "none",
                          "Use basic klee symbolic execution"),
               clEnumValN(Interpreter::GuidanceKind::CoverageGuidance,
                          "coverage",
                          "Takes place in two steps. First, all acyclic "
                          "paths are executed, "
                          "then the execution is guided to sections of the "
                          "program not yet covered. "
                          "These steps are repeated until all blocks of the "
                          "program are covered"),
               clEnumValN(Interpreter::GuidanceKind::ErrorGuidance, "error",
                          "The execution is guided to sections of the "
                          "program errors not yet covered")),
    cl::init(Interpreter::GuidanceKind::CoverageGuidance),
    cl::desc("Kind of execution mode"), cl::cat(StartCat));

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

cl::opt<bool> WithPOSIXRuntime(
    "posix-runtime",
    cl::desc("Link with POSIX runtime. Options that can be passed as arguments "
             "to the programs are: --sym-arg <max-len>  --sym-args <min-argvs> "
             "<max-argvs> <max-len> + file model options (default=false)."),
    cl::init(false), cl::cat(LinkCat));

cl::opt<bool> WithFPRuntime("fp-runtime",
                            cl::desc("Link with floating-point KLEE library."),
                            cl::init(false), cl::cat(LinkCat));

cl::opt<bool> WithUBSanRuntime("ubsan-runtime",
                               cl::desc("Link with UBSan runtime."),
                               cl::init(false), cl::cat(LinkCat));

cl::opt<std::string> RuntimeBuild(
    "runtime-build",
    cl::desc("Link with versions of the runtime library that were built with "
             "the provided configuration (default=" RUNTIME_CONFIGURATION ")."),
    cl::init(RUNTIME_CONFIGURATION), cl::cat(LinkCat));

/*** Checks options ***/

cl::OptionCategory
    ChecksCat("Checks options",
              "These options control some of the checks being done by KLEE.");

cl::opt<bool>
    CheckDivZero("check-div-zero",
                 cl::desc("Inject checks for division-by-zero (default=true)"),
                 cl::init(true), cl::cat(ChecksCat));

cl::opt<bool>
    CheckOvershift("check-overshift",
                   cl::desc("Inject checks for overshift (default=true)"),
                   cl::init(true), cl::cat(ChecksCat));

cl::opt<bool>
    OptExitOnError("exit-on-error",
                   cl::desc("Exit KLEE if an error in the tested application "
                            "has been found (default=false)"),
                   cl::init(false), cl::cat(TerminationCat));

/*** Replaying options ***/

cl::OptionCategory ReplayCat("Replaying options",
                             "These options impact replaying of test cases.");

cl::list<std::string>
    ReplayKTestFile("replay-ktest-file",
                    cl::desc("Specify a ktest file to use for replay"),
                    cl::value_desc("ktest file"), cl::cat(ReplayCat));

cl::list<std::string>
    ReplayKTestDir("replay-ktest-dir",
                   cl::desc("Specify a directory to replay ktest files from"),
                   cl::value_desc("output directory"), cl::cat(ReplayCat));

cl::opt<std::string> ReplayPathFile("replay-path",
                                    cl::desc("Specify a path file to replay"),
                                    cl::value_desc("path file"),
                                    cl::cat(ReplayCat));

cl::list<std::string> SeedOutFile("seed-file",
                                  cl::desc(".ktest file to be used as seed"),
                                  cl::cat(SeedingCat));

cl::list<std::string>
    SeedOutDir("seed-dir",
               cl::desc("Directory with .ktest files to be used as seeds"),
               cl::cat(SeedingCat));

cl::opt<unsigned> MakeConcreteSymbolic(
    "make-concrete-symbolic",
    cl::desc("Probabilistic rate at which to make concrete reads symbolic, "
             "i.e. approximately 1 in n concrete reads will be made symbolic "
             "(0=off, 1=all).  "
             "Used for testing (default=0)"),
    cl::init(0), cl::cat(DebugCat));

cl::opt<unsigned> MaxTests(
    "max-tests",
    cl::desc("Stop execution after generating the given number of tests. Extra "
             "tests corresponding to partially explored paths will also be "
             "dumped.  Set to 0 to disable (default=0)"),
    cl::init(0), cl::cat(TerminationCat));

cl::opt<bool> Watchdog(
    "watchdog",
    cl::desc("Use a watchdog process to enforce --max-time (default=false)"),
    cl::init(false), cl::cat(TerminationCat));

cl::opt<bool> Libcxx(
    "libcxx",
    cl::desc("Link the llvm libc++ library into the bitcode (default=false)"),
    cl::init(false), cl::cat(LinkCat));

cl::OptionCategory
    TestCompCat("Options specific to Test-Comp",
                "These are options specific to the Test-Comp competition.");

cl::opt<bool> WriteXMLTests("write-xml-tests",
                            cl::desc("Write XML-formated tests"),
                            cl::init(false), cl::cat(TestCompCat));

cl::opt<std::string>
    XMLMetadataProgramFile("xml-metadata-programfile",
                           cl::desc("Original file name for xml metadata"),
                           cl::cat(TestCompCat));

cl::opt<std::string> XMLMetadataProgramHash(
    "xml-metadata-programhash",
    llvm::cl::desc("Test-Comp hash sum of original file for xml metadata"),
    llvm::cl::cat(TestCompCat));

} // namespace

namespace klee {
extern cl::opt<std::string> MaxTime;
extern cl::opt<std::string> FunctionCallReproduce;
class ExecutionState;
} // namespace klee

/***/

class KleeHandler : public InterpreterHandler {
private:
  Interpreter *m_interpreter;
  TreeStreamWriter *m_pathWriter, *m_symPathWriter;
  std::unique_ptr<llvm::raw_ostream> m_infoFile;

  SmallString<128> m_outputDirectory;

  unsigned m_numTotalTests;     // Number of tests received from the interpreter
  unsigned m_numGeneratedTests; // Number of tests successfully generated
  unsigned m_pathsCompleted;    // number of completed paths
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
  void incPathsExplored(std::uint32_t num = 1) { m_pathsExplored += num; }

  void setInterpreter(Interpreter *i);

  void processTestCase(const ExecutionState &state, const char *message,
                       const char *suffix, bool isError = false);

  void writeTestCaseXML(bool isError, const KTest &out, unsigned id);

  std::string getOutputFilename(const std::string &filename);
  std::unique_ptr<llvm::raw_fd_ostream>
  openOutputFile(const std::string &filename);
  std::string getTestFilename(const std::string &suffix, unsigned id);
  std::unique_ptr<llvm::raw_fd_ostream> openTestFile(const std::string &suffix,
                                                     unsigned id);

  // load a .path file
  static void loadPathFile(std::string name, std::vector<bool> &buffer);

  static void getKTestFilesInDir(std::string directoryPath,
                                 std::vector<std::string> &results);

  static std::string getRunTimeLibraryPath(const char *argv0);

  void setOutputDirectory(const std::string &directory);

  SmallString<128> getOutputDirectory() const;
};

KleeHandler::KleeHandler(int argc, char **argv)
    : m_interpreter(0), m_pathWriter(0), m_symPathWriter(0),
      m_outputDirectory(), m_numTotalTests(0), m_numGeneratedTests(0),
      m_pathsCompleted(0), m_pathsExplored(0), m_argc(argc), m_argv(argv) {

  // create output directory (OutputDir or "klee-out-<i>")
  bool dir_given = OutputDir != "";
  SmallString<128> directory(dir_given ? OutputDir : InputFile);

  if (!dir_given)
    sys::path::remove_filename(directory);
  if (auto ec = sys::fs::make_absolute(directory)) {
    klee_error("unable to determine absolute path: %s", ec.message().c_str());
  }

  if (dir_given) {
    // OutputDir
    if (mkdir(directory.c_str(), 0775) < 0)
      klee_error("cannot create \"%s\": %s", directory.c_str(),
                 strerror(errno));

    m_outputDirectory = directory;
  } else {
    // "klee-out-<i>"
    int i = 0;
    for (; i < INT_MAX; ++i) {
      SmallString<128> d(directory);
      llvm::sys::path::append(d, "klee-out-");
      raw_svector_ostream ds(d);
      ds << i;
      // SmallString is always up-to-date, no need to flush. See
      // Support/raw_ostream.h

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
        klee_error("cannot create \"%s\": %s", m_outputDirectory.c_str(),
                   strerror(errno));
    }
    if (i == INT_MAX && m_outputDirectory.str().equals(""))
      klee_error("cannot create output directory: index out of range");
  }

  klee_message("output directory is \"%s\"", m_outputDirectory.c_str());

  // open warnings.txt
  std::string file_path = getOutputFilename("warnings.txt");
  if ((klee_warning_file = fopen(file_path.c_str(), "w")) == NULL)
    klee_error("cannot open file \"%s\": %s", file_path.c_str(),
               strerror(errno));

  // open messages.txt
  file_path = getOutputFilename("messages.txt");
  if ((klee_message_file = fopen(file_path.c_str(), "w")) == NULL)
    klee_error("cannot open file \"%s\": %s", file_path.c_str(),
               strerror(errno));

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
  sys::path::append(path, filename);
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

std::string KleeHandler::getTestFilename(const std::string &suffix,
                                         unsigned id) {
  std::stringstream filename;
  filename << "test" << std::setfill('0') << std::setw(6) << id << '.'
           << suffix;
  return filename.str();
}

SmallString<128> KleeHandler::getOutputDirectory() const {
  return m_outputDirectory;
}

std::unique_ptr<llvm::raw_fd_ostream>
KleeHandler::openTestFile(const std::string &suffix, unsigned id) {
  return openOutputFile(getTestFilename(suffix, id));
}

/* Outputs all files (.ktest, .kquery, .cov etc.) describing a test case */
void KleeHandler::processTestCase(const ExecutionState &state,
                                  const char *message, const char *suffix,
                                  bool isError) {
  unsigned id = ++m_numTotalTests;
  if (!WriteNone &&
      (FunctionCallReproduce == "" || strcmp(suffix, "assert.err") == 0 ||
       strcmp(suffix, "reachable.err") == 0)) {
    KTest ktest;
    ktest.numArgs = m_argc;
    ktest.args = m_argv;
    ktest.symArgvs = 0;
    ktest.symArgvLen = 0;

    bool success = m_interpreter->getSymbolicSolution(state, ktest);

    if (!success)
      klee_warning("unable to get symbolic solution, losing test case");

    const auto start_time = time::getWallTime();

    if (WriteKTests) {

      if (success) {
        if (!kTest_toFile(
                &ktest,
                getOutputFilename(getTestFilename("ktest", id)).c_str())) {
          klee_warning("unable to write output test case, losing it");
        } else {
          ++m_numGeneratedTests;
        }

        if (WriteStates) {
          auto f = openTestFile("state", id);
          m_interpreter->logState(state, id, f);
        }
      }

      if (message) {
        auto f = openTestFile(suffix, id);
        if (f)
          *f << message;
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
    }

    if (message || WriteKQueries) {
      std::string constraints;
      m_interpreter->getConstraintLog(state, constraints, Interpreter::KQUERY);
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

    if (WriteKPaths) {
      std::string blockPath;
      m_interpreter->getBlockPath(state, blockPath);
      auto f = openTestFile("kpath", id);
      if (f)
        *f << blockPath;
    }

    if (WriteCov) {
      std::map<std::string, std::set<unsigned>> cov;
      m_interpreter->getCoveredLines(state, cov);
      auto f = openTestFile("cov", id);
      if (f) {
        for (const auto &entry : cov) {
          for (const auto &line : entry.second) {
            *f << entry.first << ':' << line << '\n';
          }
        }
      }
    }

    if (WriteXMLTests) {
      writeTestCaseXML(message != nullptr, ktest, id);
      ++m_numGeneratedTests;
    }

    for (unsigned i = 0; i < ktest.numObjects; i++) {
      delete[] ktest.objects[i].bytes;
      delete[] ktest.objects[i].pointers;
    }
    delete[] ktest.objects;

    if (m_numGeneratedTests == MaxTests)
      m_interpreter->setHaltExecution(HaltExecution::MaxTests);

    if (!WriteXMLTests && WriteTestInfo) {
      time::Span elapsed_time(time::getWallTime() - start_time);
      auto f = openTestFile("info", id);
      if (f)
        *f << "Time to generate test case: " << elapsed_time << '\n';
    }
  } // if (!WriteNone)

  if (isError && OptExitOnError) {
    m_interpreter->prepareForEarlyExit();
    klee_error("EXITING ON ERROR:\n%s\n", message);
  }
}

void KleeHandler::writeTestCaseXML(bool isError, const KTest &assignments,
                                   unsigned id) {

  // TODO: This is super specific to test-comp and assumes that the name is the
  // type information
  auto file = openTestFile("xml", id);
  if (!file)
    return;

  *file << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n";
  *file << "<!DOCTYPE testcase PUBLIC \"+//IDN sosy-lab.org//DTD test-format "
           "testcase 1.0//EN\" "
           "\"https://sosy-lab.org/test-format/testcase-1.0.dtd\">\n";
  *file << "<testcase";
  if (isError)
    *file << " coversError=\"true\"";
  *file << ">\n";
  for (unsigned i = 0; i < assignments.numObjects; i++) {
    auto item = assignments.objects[i];
    std::string name(item.name);

    *file << "\t<input variable=\"" << name << "\" ";
    *file << "type =\"";
    // print type of the input
    *file << item.name;
    *file << "\">";
    // Ignore the type
    auto type_size_bytes = item.numBytes * 8;
    llvm::APInt v(type_size_bytes, 0, false);
    for (int i = item.numBytes - 1; i >= 0; i--) {
      v <<= 8;
      v |= item.bytes[i];
    }
    // print value

    // Check if this is an unsigned type
    if (name.find("u") == 0) {
      v.print(*file, false);
    } else if (name.rfind("*") != std::string::npos) {
      // Pointer types
      v.print(*file, false);
    } else if (name.find("float") == 0) {
      llvm::APFloat(APFloatBase::IEEEhalf(), v).print(*file);
    } else if (name.find("double") == 0) {
      llvm::APFloat(APFloatBase::IEEEdouble(), v).print(*file);
    } else if (name.rfind("_t") != std::string::npos) {
      // arbitrary type, e.g. sector_t
      v.print(*file, false);
    } else if (name.find("_") == 0) {
      // _Bool
      v.print(*file, false);
    } else {
      // the rest must be signed
      v.print(*file, true);
    }
    *file << "</input>\n";
  }
  *file << "</testcase>\n";
}

// load a .path file
void KleeHandler::loadPathFile(std::string name, std::vector<bool> &buffer) {
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
    if (f.size() >= 6 && f.substr(f.size() - 6, f.size()) == ".ktest") {
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
      llvm::sys::fs::getMainExecutable(argv0, MainExecAddr));

  // Strip off executable so we have a directory path
  llvm::sys::path::remove_filename(toolRoot);

  SmallString<128> libDir;

  if (strlen(KLEE_INSTALL_BIN_DIR) != 0 &&
      strlen(KLEE_INSTALL_RUNTIME_DIR) != 0 &&
      toolRoot.str().endswith(KLEE_INSTALL_BIN_DIR)) {
    KLEE_DEBUG_WITH_TYPE("klee_runtime",
                         llvm::dbgs()
                             << "Using installed KLEE library runtime: ");
    libDir = toolRoot.str().substr(0, toolRoot.str().size() -
                                          strlen(KLEE_INSTALL_BIN_DIR));
    llvm::sys::path::append(libDir, KLEE_INSTALL_RUNTIME_DIR);
  } else {
    KLEE_DEBUG_WITH_TYPE("klee_runtime",
                         llvm::dbgs()
                             << "Using build directory KLEE library runtime :");
    libDir = KLEE_DIR;
    llvm::sys::path::append(libDir, "runtime/lib");
  }

  KLEE_DEBUG_WITH_TYPE("klee_runtime", llvm::dbgs() << libDir.c_str() << "\n");
  return libDir.c_str();
}

void KleeHandler::setOutputDirectory(const std::string &directoryName) {
  // create output directory
  if (directoryName == "") {
    klee_error("Empty name of new directory");
  }
  SmallString<128> directory(directoryName);

  if (auto ec = sys::fs::make_absolute(directory)) {
    klee_error("unable to determine absolute path: %s", ec.message().c_str());
  }

  // OutputDir
  if (mkdir(directory.c_str(), 0775) < 0) {
    klee_error("cannot create \"%s\": %s", directory.c_str(), strerror(errno));
  }

  m_outputDirectory = directory;

  klee_message("output directory is \"%s\"", m_outputDirectory.c_str());

  // open warnings.txt
  std::string file_path = getOutputFilename("warnings.txt");
  if ((klee_warning_file = fopen(file_path.c_str(), "w")) == NULL) {
    klee_error("cannot open file \"%s\": %s", file_path.c_str(),
               strerror(errno));
  }

  // open messages.txt
  file_path = getOutputFilename("messages.txt");
  if ((klee_message_file = fopen(file_path.c_str(), "w")) == NULL) {
    klee_error("cannot open file \"%s\": %s", file_path.c_str(),
               strerror(errno));
  }
  // open info
  m_infoFile = openOutputFile("info");
}

//===----------------------------------------------------------------------===//
// main Driver function
//

static Function *mainFn = nullptr;
static Function *entryFn = nullptr;

static std::string strip(std::string &in) {
  unsigned len = in.size();
  unsigned lead = 0, trail = len;
  while (lead < len && isspace(in[lead]))
    ++lead;
  while (trail > lead && isspace(in[trail - 1]))
    --trail;
  return in.substr(lead, trail - lead);
}

static void parseArguments(int argc, char **argv) {
  cl::SetVersionPrinter(klee::printVersion);
  // This version always reads response files
  cl::ParseCommandLineOptions(argc, argv, " klee\n");
}

static void
preparePOSIX(std::vector<std::unique_ptr<llvm::Module>> &loadedModules) {
  // Get the main function from the main module and rename it such that it can
  // be called after the POSIX setup
  Function *mainFn = nullptr;
  for (auto &module : loadedModules) {
    mainFn = module->getFunction(EntryPoint);
    if (mainFn && !mainFn->empty())
      break;
  }

  if (!mainFn || mainFn->empty()) {
    klee_error("Entry function '%s' not found in module.", EntryPoint.c_str());
  }
  mainFn->setName("__klee_posix_wrapped_main");

  llvm::Function *wrapper = nullptr;
  for (auto &module : loadedModules) {
    wrapper = module->getFunction("__klee_posix_wrapper");
    if (wrapper)
      break;
  }
  assert(wrapper && "klee_posix_wrapper not found");

  // Rename the POSIX wrapper to prefixed entrypoint, e.g. _user_main as uClibc
  // would expect it or main otherwise
  wrapper->setName(EntryPoint);
  loadedModules.front()->getOrInsertFunction(wrapper->getName(),
                                             wrapper->getFunctionType());
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
    "_assert", "__assert_fail", "__assert_rtn", "__errno_location", "__error",
    "calloc", "_exit", "exit", "free", "abort", "klee_abort", "klee_assume",
    "klee_check_memory_access", "klee_define_fixed_object", "klee_get_errno",
    "klee_get_valuef", "klee_get_valued", "klee_get_valuel", "klee_get_valuell",
    "klee_get_value_i32", "klee_get_value_i64", "klee_get_obj_size",
    "klee_is_symbolic", "klee_make_symbolic", "klee_mark_global",
    "klee_prefer_cex", "klee_posix_prefer_cex", "klee_print_expr",
    "klee_print_range", "klee_report_error", "klee_set_forking",
    "klee_silent_exit", "klee_warning", "klee_warning_once", "klee_stack_trace",
#ifdef SUPPORT_KLEE_EH_CXX
    "_klee_eh_Unwind_RaiseException_impl", "klee_eh_typeid_for",
#endif
    "llvm.dbg.declare", "llvm.dbg.value", "llvm.va_start", "llvm.va_end",
    "malloc", "realloc", "memalign", "_ZdaPv", "_ZdlPv", "_Znaj", "_Znwj",
    "_Znam", "_Znwm", "__ubsan_handle_type_mismatch_v1",
    "__ubsan_handle_type_mismatch_v1_abort",
    "__ubsan_handle_alignment_assumption",
    "__ubsan_handle_alignment_assumption_abort", "__ubsan_handle_add_overflow",
    "__ubsan_handle_add_overflow_abort", "__ubsan_handle_sub_overflow",
    "__ubsan_handle_sub_overflow_abort", "__ubsan_handle_mul_overflow",
    "__ubsan_handle_mul_overflow_abort", "__ubsan_handle_negate_overflow",
    "__ubsan_handle_negate_overflow_abort", "__ubsan_handle_divrem_overflow",
    // Floating point intrinstics
    "klee_rintf", "klee_rint", "klee_rintl", "klee_is_nan_float",
    "klee_is_nan_double", "klee_is_nan_long_double", "klee_is_infinite_float",
    "klee_is_infinite_double", "klee_is_infinite_long_double",
    "klee_is_normal_float", "klee_is_normal_double",
    "klee_is_normal_long_double", "klee_is_subnormal_float",
    "klee_is_subnormal_double", "klee_is_subnormal_long_double",
    "klee_get_rounding_mode", "klee_set_rounding_mode_internal",
    "klee_sqrt_float", "klee_sqrt_double", "klee_sqrt_long_double",
    "klee_abs_float", "klee_abs_double", "klee_abs_long_double",
    "__ubsan_handle_divrem_overflow_abort",
    "__ubsan_handle_shift_out_of_bounds",
    "__ubsan_handle_shift_out_of_bounds_abort", "__ubsan_handle_out_of_bounds",
    "__ubsan_handle_out_of_bounds_abort", "__ubsan_handle_builtin_unreachable",
    "__ubsan_handle_missing_return", "__ubsan_handle_vla_bound_not_positive",
    "__ubsan_handle_vla_bound_not_positive_abort",
    "__ubsan_handle_float_cast_overflow",
    "__ubsan_handle_float_cast_overflow_abort",
    "__ubsan_handle_load_invalid_value",
    "__ubsan_handle_load_invalid_value_abort",
    "__ubsan_handle_implicit_conversion",
    "__ubsan_handle_implicit_conversion_abort",
    "__ubsan_handle_invalid_builtin", "__ubsan_handle_invalid_builtin_abort",
    "__ubsan_handle_nonnull_return_v1",
    "__ubsan_handle_nonnull_return_v1_abort",
    "__ubsan_handle_nullability_return_v1",
    "__ubsan_handle_nullability_return_v1_abort", "__ubsan_handle_nonnull_arg",
    "__ubsan_handle_nonnull_arg_abort", "__ubsan_handle_nullability_arg",
    "__ubsan_handle_nullability_arg_abort", "__ubsan_handle_pointer_overflow",
    "__ubsan_handle_pointer_overflow_abort",

    // also there are some handlers in
    // llvm-project/compiler-rt/lib/ubsan/ubsan_handlers_cxx.cpp that
    // could not reasonably be implemented
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
    "printf", "vprintf"};

// Symbols we consider unsafe
static const char *unsafeExternals[] = {
    "fork",  // oh lord
    "exec",  // heaven help us
    "error", // calls _exit
    "raise", // yeah
    "kill",  // mmmhmmm
};

#define NELEMS(array) (sizeof(array) / sizeof(array[0]))
void externalsAndGlobalsCheck(const llvm::Module *m) {
  std::map<std::string, bool> externals;
  std::set<std::string> modelled(modelledExternals,
                                 modelledExternals + NELEMS(modelledExternals));
  std::set<std::string> dontCare(dontCareExternals,
                                 dontCareExternals + NELEMS(dontCareExternals));
  std::set<std::string> unsafe(unsafeExternals,
                               unsafeExternals + NELEMS(unsafeExternals));

  switch (Libc) {
  case LibcType::KleeLibc:
    dontCare.insert(dontCareKlee, dontCareKlee + NELEMS(dontCareKlee));
    break;
  case LibcType::UcLibc:
    dontCare.insert(dontCareUclibc, dontCareUclibc + NELEMS(dontCareUclibc));
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

  for (Module::const_global_iterator it = m->global_begin(),
                                     ie = m->global_end();
       it != ie; ++it)
    if (it->isDeclaration() && !it->use_empty())
      externals.insert(std::make_pair(it->getName(), true));
  // and remove aliases (they define the symbol after global
  // initialization)
  for (Module::const_alias_iterator it = m->alias_begin(), ie = m->alias_end();
       it != ie; ++it) {
    std::map<std::string, bool>::iterator it2 =
        externals.find(it->getName().str());
    if (it2 != externals.end())
      externals.erase(it2);
  }

  std::map<std::string, bool> foundUnsafe;
  for (std::map<std::string, bool>::iterator it = externals.begin(),
                                             ie = externals.end();
       it != ie; ++it) {
    const std::string &ext = it->first;
    if (!modelled.count(ext) && (WarnAllExternals || !dontCare.count(ext))) {
      if (ext.compare(0, 5, "llvm.") != 0) { // not an LLVM reserved name
        if (unsafe.count(ext)) {
          foundUnsafe.insert(*it);
        } else if (WarnAllExternals) {
          klee_warning("undefined reference to %s: %s",
                       it->second ? "variable" : "function", ext.c_str());
        }
      }
    }
  }

  for (std::map<std::string, bool>::iterator it = foundUnsafe.begin(),
                                             ie = foundUnsafe.end();
       it != ie; ++it) {
    const std::string &ext = it->first;
    klee_warning("undefined reference to %s: %s (UNSAFE)!",
                 it->second ? "variable" : "function", ext.c_str());
  }
}

static Interpreter *theInterpreter = 0;
static nonstd::optional<SarifReport> paths = nonstd::nullopt;

static std::atomic_bool interrupted{false};

// Pulled out so it can be easily called from a debugger.
extern "C" void halt_execution() {
  theInterpreter->setHaltExecution(HaltExecution::Interrupt);
}

extern "C" void stop_forking() { theInterpreter->setInhibitForking(true); }

static void interrupt_handle() {
  if (!interrupted && theInterpreter) {
    llvm::errs() << "KLEE: ctrl-c detected, requesting interpreter to halt.\n";
    halt_execution();
    sys::SetInterruptFunction(interrupt_handle);
  } else {
    if (paths && (!theInterpreter || !theInterpreter->hasTargetForest())) {
      for (const auto &res : paths->results) {
        reportFalsePositive(confidence::MinConfidence, res.errors, res.id,
                            "max-time");
      }
    }
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
  if (system(buffer) == -1)
    perror("system");
}

static void replaceOrRenameFunction(llvm::Module *module, const char *old_name,
                                    const char *new_name) {
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
createLibCWrapper(std::vector<std::unique_ptr<llvm::Module>> &userModules,
                  std::vector<std::unique_ptr<llvm::Module>> &libsModules,
                  llvm::StringRef intendedFunction,
                  llvm::StringRef libcMainFunction) {
  // We now need to swap things so that libcMainFunction is the entry
  // point, in such a way that the arguments are passed to
  // libcMainFunction correctly. We do this by renaming the user main
  // and generating a stub function to call intendedFunction. There is
  // also an implicit cooperation in that runFunctionAsMain sets up
  // the environment arguments to what a libc expects (following
  // argv), since it does not explicitly take an envp argument.
  auto &ctx = userModules[0]->getContext();
  Function *userMainFn = nullptr;
  for (auto &module : userModules) {
    Function *func = module->getFunction(intendedFunction);
    if (func) {
      // Rename entry point using a prefix
      func->setName("__user_" + intendedFunction);
      userMainFn = func;
    }
  }

  if (!userMainFn) {
    klee_error("Entry function '%s' not found in module.",
               intendedFunction.str().c_str());
  }

  // force import of libcMainFunction
  llvm::Function *libcMainFn = nullptr;
  for (auto &module : libsModules) {
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

  userModules.front()->getOrInsertFunction(stub->getName(),
                                           stub->getFunctionType());
}

static void
linkWithUclibc(StringRef libDir, std::string bit_suffix, std::string opt_suffix,
               std::vector<std::unique_ptr<llvm::Module>> &userModules,
               std::vector<std::unique_ptr<llvm::Module>> &libsModules) {
  LLVMContext &ctx = userModules[0]->getContext();
  std::string errorMsg;

  // Link the fortified library
  SmallString<128> FortifyPath(libDir);
  llvm::sys::path::append(FortifyPath,
                          "libkleeRuntimeFortify" + opt_suffix + ".bca");
  if (!klee::loadFileAsOneModule(FortifyPath.c_str(), ctx, libsModules,
                                 errorMsg))
    klee_error("error loading the fortify library '%s': %s",
               FortifyPath.c_str(), errorMsg.c_str());

  // Ensure that klee-uclibc exists
  SmallString<128> uclibcBCA(libDir);
  // Hack to find out bitness of .bc file

  if (bit_suffix == "64") {
    llvm::sys::path::append(uclibcBCA, KLEE_UCLIBC_BCA_64_NAME);
  } else if (bit_suffix == "32") {
    llvm::sys::path::append(uclibcBCA, KLEE_UCLIBC_BCA_32_NAME);
  } else {
    klee_error("Cannot determine bitness of source file from the name %s",
               uclibcBCA.c_str());
  }

  if (!klee::loadFileAsOneModule(uclibcBCA.c_str(), ctx, libsModules, errorMsg))
    klee_error("Cannot find klee-uclibc '%s': %s", uclibcBCA.c_str(),
               errorMsg.c_str());

  replaceOrRenameFunction(libsModules.back().get(), "__libc_open", "open");
  replaceOrRenameFunction(libsModules.back().get(), "__libc_fcntl", "fcntl");

  createLibCWrapper(userModules, libsModules, EntryPoint, "__uClibc_main");

  klee_message("NOTE: Using klee-uclibc : %s", uclibcBCA.c_str());
}

static SarifReport parseInputPathTree(const std::string &inputPathTreePath) {
  std::ifstream file(inputPathTreePath);
  if (file.fail())
    klee_error("Cannot read file %s", inputPathTreePath.c_str());
  json reportJson = json::parse(file);
  SarifReportJson sarifJson = reportJson.get<SarifReportJson>();
  klee_warning("we are parsing file %s", inputPathTreePath.c_str());
  return klee::convertAndFilterSarifJson(sarifJson);
}

static nonstd::optional<SarifReport> parseStaticAnalysisInput() {
  if (AnalysisReproduce != "")
    return parseInputPathTree(AnalysisReproduce);
  return nonstd::nullopt;
}

static int run_klee_on_function(int pArgc, char **pArgv, char **pEnvp,
                                std::unique_ptr<KleeHandler> &handler,
                                std::unique_ptr<Interpreter> &interpreter,
                                llvm::Module *finalModule,
                                std::vector<bool> &replayPath) {
  Function *mainFn = finalModule->getFunction(EntryPoint);
  if ((UseGuidedSearch != Interpreter::GuidanceKind::ErrorGuidance) &&
      !mainFn) { // in error guided mode we do not need main function
    klee_error("Entry function '%s' not found in module.", EntryPoint.c_str());
  }

  externalsAndGlobalsCheck(finalModule);

  if (ReplayPathFile != "") {
    interpreter->setReplayPath(&replayPath);
  }

  auto startTime = std::time(nullptr);

  if (WriteXMLTests) {
    // Write metadata.xml
    auto meta_file = handler->openOutputFile("metadata.xml");
    if (!meta_file)
      klee_error("Could not write metadata.xml");

    *meta_file
        << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n";
    *meta_file
        << "<!DOCTYPE test-metadata PUBLIC \"+//IDN sosy-lab.org//DTD "
           "test-format test-metadata 1.0//EN\" "
           "\"https://sosy-lab.org/test-format/test-metadata-1.0.dtd\">\n";
    *meta_file << "<test-metadata>\n";
    *meta_file << "\t<sourcecodelang>C</sourcecodelang>\n";
    *meta_file << "\t<producer>" << PACKAGE_STRING << "</producer>\n";

    // Assume with early exit a bug finding mode and otherwise coverage
    if (OptExitOnError)
      *meta_file << "\t<specification>COVER( init(main()), FQL(COVER "
                    "EDGES(@CALL(__VERIFIER_error))) )</specification>\n";
    else
      *meta_file << "\t<specification>COVER( init(main()), FQL(COVER "
                    "EDGES(@DECISIONEDGE)) )</specification>\n";

    // Assume the input file resembles the original source file; just exchange
    // extension
    *meta_file << "\t<programfile>" << XMLMetadataProgramFile
               << ".c</programfile>\n";
    *meta_file << "\t<programhash>" << XMLMetadataProgramHash
               << "</programhash>\n";
    *meta_file << "\t<entryfunction>" << EntryPoint << "</entryfunction>\n";
    *meta_file << "\t<architecture>"
               << finalModule->getDataLayout().getPointerSizeInBits()
               << "bit</architecture>\n";
    std::stringstream t;
    t << std::put_time(std::localtime(&startTime), "%Y-%m-%dT%H:%M:%SZ");
    *meta_file << "\t<creationtime>" << t.str() << "</creationtime>\n";
    *meta_file << "</test-metadata>\n";
  }

  { // output clock info and start time
    std::stringstream startInfo;
    startInfo << time::getClockInfo() << "Started: "
              << std::put_time(std::localtime(&startTime), "%Y-%m-%d %H:%M:%S")
              << '\n';
    handler->getInfoStream() << startInfo.str();
    handler->getInfoStream().flush();
  }

  if (!ReplayKTestDir.empty() || !ReplayKTestFile.empty()) {
    assert(SeedOutFile.empty());
    assert(SeedOutDir.empty());

    std::vector<std::string> kTestFiles = ReplayKTestFile;
    for (std::vector<std::string>::iterator it = ReplayKTestDir.begin(),
                                            ie = ReplayKTestDir.end();
         it != ie; ++it)
      KleeHandler::getKTestFilesInDir(*it, kTestFiles);
    std::vector<KTest *> kTests;
    for (std::vector<std::string>::iterator it = kTestFiles.begin(),
                                            ie = kTestFiles.end();
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

    unsigned i = 0;
    for (std::vector<KTest *>::iterator it = kTests.begin(), ie = kTests.end();
         it != ie; ++it) {
      KTest *out = *it;
      interpreter->setReplayKTest(out);
      llvm::errs() << "KLEE: replaying: " << *it << " (" << kTest_numBytes(out)
                   << " bytes)"
                   << " (" << ++i << "/" << kTestFiles.size() << ")\n";
      // XXX should put envp in .ktest ?
      interpreter->runFunctionAsMain(mainFn, out->numArgs, out->args, pEnvp);
      if (interrupted)
        break;
    }
    interpreter->setReplayKTest(0);
    while (!kTests.empty()) {
      kTest_free(kTests.back());
      kTests.pop_back();
    }
  } else {
    std::vector<KTest *> seeds;
    for (std::vector<std::string>::iterator it = SeedOutFile.begin(),
                                            ie = SeedOutFile.end();
         it != ie; ++it) {
      KTest *out = kTest_fromFile(it->c_str());
      if (!out) {
        klee_error("unable to open: %s\n", (*it).c_str());
      }
      seeds.push_back(out);
    }
    for (std::vector<std::string>::iterator it = SeedOutDir.begin(),
                                            ie = SeedOutDir.end();
         it != ie; ++it) {
      std::vector<std::string> kTestFiles;
      KleeHandler::getKTestFilesInDir(*it, kTestFiles);
      for (std::vector<std::string>::iterator it2 = kTestFiles.begin(),
                                              ie = kTestFiles.end();
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

    interpreter->runFunctionAsMain(mainFn, pArgc, pArgv, pEnvp);

    while (!seeds.empty()) {
      kTest_free(seeds.back());
      seeds.pop_back();
    }
  }

  auto endTime = std::time(nullptr);
  { // output end and elapsed time
    std::uint32_t h;
    std::uint8_t m, s;
    std::tie(h, m, s) = time::seconds(endTime - startTime).toHMS();
    std::stringstream endInfo;
    endInfo << "Finished: "
            << std::put_time(std::localtime(&endTime), "%Y-%m-%d %H:%M:%S")
            << '\n'
            << "Elapsed: " << std::setfill('0') << std::setw(2) << h << ':'
            << std::setfill('0') << std::setw(2) << +m << ':'
            << std::setfill('0') << std::setw(2) << +s << '\n';
    handler->getInfoStream() << endInfo.str();
    handler->getInfoStream().flush();
  }
  return 0;
}

void kill_child(pid_t child_pid) {
  if (kill(child_pid, 0) != 0) {
    return;
  }
  [[maybe_unused]] int statusSIGTERM = kill(child_pid, SIGTERM);
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  if (kill(child_pid, 0) == 0) {
    int statusSIGKILL = kill(child_pid, SIGKILL);
    if (statusSIGKILL != 0) {
      klee_error("Kill with signal SIGKILL return nonzero code.");
    }
  }
}

void wait_until_any_child_dies(
    const std::vector<
        std::pair<pid_t, std::chrono::time_point<std::chrono::steady_clock>>>
        &child_processes) {
  while (true) {
    bool something_dead = false;
    for (const auto &child_process : child_processes) {
      if (kill(child_process.first, 0) != 0) {
        something_dead = true;
      }
      auto current_time = std::chrono::steady_clock::now();
      auto duration_function = std::chrono::duration_cast<std::chrono::seconds>(
                                   current_time - child_process.second)
                                   .count();
      if (duration_function > TimeoutPerFunction) {
        kill_child(child_process.first);
        something_dead = true;
      }
    }
    if (something_dead) {
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

int main(int argc, char **argv, char **envp) {
  if (theInterpreter) {
    theInterpreter = nullptr;
  }
  interrupted = false;

  klee::klee_warning_file = nullptr;
  klee::klee_message_file = nullptr;
  klee::ContextInitialized = false;

  atexit(llvm_shutdown); // Call llvm_shutdown() on exit.

#if LLVM_VERSION_CODE >= LLVM_VERSION(13, 0)
  KCommandLine::HideOptions(llvm::cl::getGeneralCategory());
#else
  KCommandLine::HideOptions(llvm::cl::GeneralCategory);
#endif

  llvm::InitializeNativeTarget();

  parseArguments(argc, argv);
  sys::PrintStackTraceOnErrorSignal(argv[0]);

  if (Watchdog) {
    if (MaxTime.empty()) {
      klee_error("--watchdog used without --max-time");
    }

    int pid = fork();
    if (pid < 0) {
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

            if (level == 1) {
              klee_warning(
                  "KLEE: WATCHDOG: time expired, attempting halt via INT\n");
              kill(pid, SIGINT);
            } else if (level == 2) {
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
  std::vector<std::unique_ptr<llvm::Module>> loadedUserModules;
  std::vector<std::unique_ptr<llvm::Module>> loadedLibsModules;
  if (!klee::loadFileAsOneModule(InputFile, ctx, loadedUserModules, errorMsg)) {
    klee_error("error loading program '%s': %s", InputFile.c_str(),
               errorMsg.c_str());
  }
  // Load and link the whole files content. The assumption is that this is the
  // application under test.
  // Nothing gets removed in the first place.

  if (!loadedUserModules.front()) {
    klee_error("error loading program '%s': %s", InputFile.c_str(),
               errorMsg.c_str());
  }

  llvm::Module *mainModule = loadedUserModules.front().get();
  FLCtoOpcode origInstructions;

  if (UseGuidedSearch == Interpreter::GuidanceKind::ErrorGuidance) {
    for (const auto &Func : *mainModule) {
      for (const auto &instr : llvm::instructions(Func)) {
        auto locationInfo = getLocationInfo(&instr);
        origInstructions[locationInfo.file][locationInfo.line]
                        [locationInfo.column]
                            .insert(instr.getOpcode());
      }
    }

    std::vector<llvm::Type *> args;
    args.push_back(llvm::Type::getInt32Ty(ctx)); // argc
    args.push_back(llvm::PointerType::get(
        Type::getInt8PtrTy(ctx),
        mainModule->getDataLayout().getAllocaAddrSpace())); // argv
    args.push_back(llvm::PointerType::get(
        Type::getInt8PtrTy(ctx),
        mainModule->getDataLayout().getAllocaAddrSpace())); // envp
    std::string stubEntryPoint = "__klee_entry_point_main";
    Function *stub = Function::Create(
        llvm::FunctionType::get(llvm::Type::getInt32Ty(ctx), args, false),
        GlobalVariable::ExternalLinkage, stubEntryPoint, mainModule);
    BasicBlock *bb = BasicBlock::Create(ctx, "entry", stub);

    llvm::IRBuilder<> Builder(bb);
    Builder.CreateRet(ConstantInt::get(Type::getInt32Ty(ctx), 0));
    EntryPoint = stubEntryPoint;
  }

  std::set<std::string> mainModuleFunctions;
  for (auto &Function : *mainModule) {
    if (!Function.isDeclaration()) {
      mainModuleFunctions.insert(Function.getName().str());
    }
  }
  std::set<std::string> mainModuleGlobals;
  for (const auto &gv : mainModule->globals()) {
    mainModuleGlobals.insert(gv.getName().str());
  }

  const std::string &module_triple = mainModule->getTargetTriple();
  std::string host_triple = llvm::sys::getDefaultTargetTriple();

  if (module_triple != host_triple)
    klee_warning("Module and host target triples do not match: '%s' != '%s'\n"
                 "This may cause unexpected crashes or assertion violations.",
                 module_triple.c_str(), host_triple.c_str());

  // Detect architecture
  std::string bit_suffix = "64"; // Fall back to 64bit
  if (module_triple.find("i686") != std::string::npos ||
      module_triple.find("i586") != std::string::npos ||
      module_triple.find("i486") != std::string::npos ||
      module_triple.find("i386") != std::string::npos ||
      module_triple.find("arm") != std::string::npos)
    bit_suffix = "32";

  // Add additional user-selected suffix
  std::string opt_suffix = bit_suffix + "_" + RuntimeBuild.getValue();

  if (UseGuidedSearch == Interpreter::GuidanceKind::ErrorGuidance) {
    SimplifyModule = false;
  }

  std::string LibraryDir = KleeHandler::getRunTimeLibraryPath(argv[0]);
  Interpreter::ModuleOptions Opts(LibraryDir.c_str(), EntryPoint, opt_suffix,
                                  /*Optimize=*/OptimizeModule,
                                  /*Simplify*/ SimplifyModule,
                                  /*CheckDivZero=*/CheckDivZero,
                                  /*CheckOvershift=*/CheckOvershift,
                                  /*WithFPRuntime=*/WithFPRuntime,
                                  /*WithPOSIXRuntime=*/WithPOSIXRuntime);

  // Get the main function
  for (auto &module : loadedUserModules) {
    mainFn = module->getFunction("main");
    if (mainFn)
      break;
  }

  // Get the entry point function
  if (EntryPoint.empty())
    klee_error("entry-point cannot be empty");

  for (auto &module : loadedUserModules) {
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
    if (!klee::loadFileAsOneModule(Path.c_str(), mainModule->getContext(),
                                   loadedUserModules, errorMsg))
      klee_error("error loading POSIX support '%s': %s", Path.c_str(),
                 errorMsg.c_str());

    if (Libc != LibcType::UcLibc) {
      SmallString<128> Path_io(Opts.LibraryDir);
      llvm::sys::path::append(Path_io,
                              "libkleeRuntimeIO_C" + opt_suffix + ".bca");
      klee_message("NOTE: using klee versions of input/output functions: %s",
                   Path_io.c_str());
      if (!klee::loadFileAsOneModule(Path_io.c_str(), mainModule->getContext(),
                                     loadedUserModules, errorMsg))
        klee_error("error loading POSIX_IO support '%s': %s", Path_io.c_str(),
                   errorMsg.c_str());
    }

    if (!UTBotMode) {
      preparePOSIX(loadedUserModules);
    }
  }

  if (WithFPRuntime) {
#if ENABLE_FP
    SmallString<128> Path(Opts.LibraryDir);
    llvm::sys::path::append(Path, "libkleeRuntimeFp" + opt_suffix + ".bca");
    if (!klee::loadFileAsOneModule(Path.c_str(), mainModule->getContext(),
                                   loadedUserModules, errorMsg))
      klee_error("error loading klee FP runtime '%s': %s", Path.c_str(),
                 errorMsg.c_str());
#else
    klee_error("unable to link with klee FP runtime without "
               "-DENABLE_FLOATING_POINT=ON");
#endif
  }

  if (WithUBSanRuntime) {
    SmallString<128> Path(Opts.LibraryDir);
    llvm::sys::path::append(Path, "libkleeUBSan" + opt_suffix + ".bca");
    if (!klee::loadFileAsOneModule(Path.c_str(), mainModule->getContext(),
                                   loadedUserModules, errorMsg))
      klee_error("error loading UBSan support '%s': %s", Path.c_str(),
                 errorMsg.c_str());
  }

  if (Libcxx) {
#ifndef SUPPORT_KLEE_LIBCXX
    klee_error("KLEE was not compiled with libc++ support");
#else
    SmallString<128> LibcxxBC(Opts.LibraryDir);
    llvm::sys::path::append(LibcxxBC, KLEE_LIBCXX_BC_NAME);
    if (!klee::loadFileAsOneModule(LibcxxBC.c_str(), mainModule->getContext(),
                                   loadedLibsModules, errorMsg))
      klee_error("error loading libc++ '%s': %s", LibcxxBC.c_str(),
                 errorMsg.c_str());
    klee_message("NOTE: Using libc++ : %s", LibcxxBC.c_str());
#ifdef SUPPORT_KLEE_EH_CXX
    SmallString<128> EhCxxPath(Opts.LibraryDir);
    llvm::sys::path::append(EhCxxPath, "libkleeeh-cxx" + opt_suffix + ".bca");
    if (!klee::loadFileAsOneModule(EhCxxPath.c_str(), mainModule->getContext(),
                                   loadedLibsModules, errorMsg))
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
    if (!klee::loadFileAsOneModule(Path.c_str(), mainModule->getContext(),
                                   loadedLibsModules, errorMsg))
      klee_error("error loading klee libc '%s': %s", Path.c_str(),
                 errorMsg.c_str());
  }
  /* Falls through. */
  case LibcType::FreestandingLibc: {
    SmallString<128> Path(Opts.LibraryDir);
    llvm::sys::path::append(Path,
                            "libkleeRuntimeFreestanding" + opt_suffix + ".bca");
    if (!klee::loadFileAsOneModule(Path.c_str(), mainModule->getContext(),
                                   loadedLibsModules, errorMsg))
      klee_error("error loading freestanding support '%s': %s", Path.c_str(),
                 errorMsg.c_str());
    break;
  }
  case LibcType::UcLibc:
    linkWithUclibc(LibraryDir, bit_suffix, opt_suffix, loadedUserModules,
                   loadedLibsModules);
    break;
  }

  for (const auto &library : LinkLibraries) {
    if (!klee::loadFileAsOneModule(library, mainModule->getContext(),
                                   loadedLibsModules, errorMsg))
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
    pEnvp = new char *[items.size() + 1];
    unsigned i = 0;
    for (; i != items.size(); ++i)
      pEnvp[i] = strdup(items[i].c_str());
    pEnvp[i] = 0;
  } else {
    pEnvp = envp;
  }

  pArgc = InputArgv.size() + 1;
  pArgv = new char *[pArgc];
  for (unsigned i = 0; i < InputArgv.size() + 1; i++) {
    std::string &arg = (i == 0 ? InputFile : InputArgv[i - 1]);
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

  std::unique_ptr<KleeHandler> handler =
      std::make_unique<KleeHandler>(pArgc, pArgv);

  if (UseGuidedSearch == Interpreter::GuidanceKind::ErrorGuidance) {
    paths = parseStaticAnalysisInput();
  } else if (FunctionCallReproduce != "") {
    klee_warning("Turns on error-guided mode to cover %s function",
                 FunctionCallReproduce.c_str());
    UseGuidedSearch = Interpreter::GuidanceKind::ErrorGuidance;
  }

  Interpreter::InterpreterOptions IOpts(paths);
  IOpts.MakeConcreteSymbolic = MakeConcreteSymbolic;
  IOpts.Guidance = UseGuidedSearch;
  std::unique_ptr<Interpreter> interpreter(
      Interpreter::create(ctx, IOpts, handler.get()));
  theInterpreter = interpreter.get();
  assert(interpreter);
  handler->setInterpreter(interpreter.get());

  for (int i = 0; i < argc; i++) {
    handler->getInfoStream() << argv[i] << (i + 1 < argc ? " " : "\n");
  }
  handler->getInfoStream() << "PID: " << getpid() << "\n";

  // Get the desired main function.  klee_main initializes uClibc
  // locale and other data and then calls main.

  auto finalModule = interpreter->setModule(
      loadedUserModules, loadedLibsModules, Opts,
      std::move(mainModuleFunctions), std::move(mainModuleGlobals),
      std::move(origInstructions));

  externalsAndGlobalsCheck(finalModule);
  if (InteractiveMode) {
    klee_message("KLEE finish preprocessing.");
    std::ifstream entrypoints(EntryPointsFile);
    if (!entrypoints.good()) {
      klee_error("Opening %s failed.", EntryPointsFile.c_str());
    }

    if (ProcessNumber < 1 || ProcessNumber > MAX_PROCESS_NUMBER) {
      klee_error("Incorrect number of process, your value is %d, but it must "
                 "lie in [1, %d].",
                 ProcessNumber.getValue(), MAX_PROCESS_NUMBER);
    }
    klee_message("Start %d processes.", ProcessNumber.getValue());

    SmallString<128> outputDirectory = handler->getOutputDirectory();
    const size_t PROCESS = ProcessNumber;
    using time_point = std::chrono::time_point<std::chrono::steady_clock>;
    std::vector<std::pair<pid_t, time_point>> child_processes;
    signal(SIGCHLD, SIG_IGN);
    while (true) {
      std::string entrypoint;
      if (!(entrypoints >> entrypoint)) {
        break;
      }

      if (child_processes.size() == PROCESS) {
        if (TimeoutPerFunction != 0) {
          wait_until_any_child_dies(child_processes);
        } else {
          wait(NULL);
        }
      }

      std::vector<std::pair<pid_t, time_point>> alive_child;
      for (const auto &child_process : child_processes) {
        if (kill(child_process.first, 0) == 0) {
          alive_child.push_back(child_process);
        }
      }
      child_processes = alive_child;

      pid_t pid = fork();
      if (pid < 0) {
        klee_error("%s", "Cannot create child process.");
      } else if (pid == 0) {
        signal(SIGCHLD, SIG_DFL);
        EntryPoint = entrypoint;
        SmallString<128> newOutputDirectory = outputDirectory;
        sys::path::append(newOutputDirectory, entrypoint);
        handler->setOutputDirectory(newOutputDirectory.c_str());
        run_klee_on_function(pArgc, pArgv, pEnvp, handler, interpreter,
                             finalModule, replayPath);
        exit(0);
      } else {
        child_processes.emplace_back(pid, std::chrono::steady_clock::now());
      }
    }
    if (TimeoutPerFunction != 0) {
      while (!child_processes.empty()) {
        wait_until_any_child_dies(child_processes);
        std::vector<std::pair<pid_t, time_point>> alive_child;
        for (const auto &child_process : child_processes) {
          if (kill(child_process.first, 0) == 0) {
            alive_child.push_back(child_process);
          }
        }
        child_processes = alive_child;
      }
    } else {
      while (wait(NULL) > 0)
        ;
    }
  } else {
    run_klee_on_function(pArgc, pArgv, pEnvp, handler, interpreter, finalModule,
                         replayPath);
  }

  paths.reset();

  // Free all the args.
  for (unsigned i = 0; i < InputArgv.size() + 1; i++)
    delete[] pArgv[i];
  delete[] pArgv;

  uint64_t queries = *theStatisticManager->getStatisticByName("SolverQueries");
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
  uint64_t forks = *theStatisticManager->getStatisticByName("Forks");

  handler->getInfoStream() << "KLEE: done: explored paths = " << 1 + forks
                           << "\n";

  // Write some extra information in the info file which users won't
  // necessarily care about or understand.
  if (queries)
    handler->getInfoStream() << "KLEE: done: avg. constructs per query = "
                             << queryConstructs / queries << "\n";
  handler->getInfoStream() << "KLEE: done: total queries = " << queries << "\n"
                           << "KLEE: done: valid queries = " << queriesValid
                           << "\n"
                           << "KLEE: done: invalid queries = " << queriesInvalid
                           << "\n"
                           << "KLEE: done: query cex = " << queryCounterexamples
                           << "\n";

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
  return 0;
}
