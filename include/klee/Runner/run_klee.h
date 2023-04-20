#ifndef KLEE_RUN_KLEE_H
#define KLEE_RUN_KLEE_H

#include "klee/Module/SarifReport.h"

klee::SarifReport parseInputPathTree(const std::string &inputPathTreePath);

int run_klee(int argc, char **argv, char **envp);

#endif // KLEE_RUN_KLEE_H
