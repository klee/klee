// REQUIRES: not-darwin
// RUN: %llvmgcc %s -emit-llvm -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: printf "KLEE_TEST_FLE=VA0\nKLEE_TEST_FLE=VA1" > %t_env.file
// RUN: env "KLEE_TEST_SYS=VAL" %klee -libc=uclibc -env-system -env-file=%t_env.file -env-variable="KLEE_TEST_VAR=VA0" -env-variable="KLEE_TEST_VAR=VA1" --output-dir=%t.klee-out --exit-on-error %t1.bc
// RUN: rm -rf env.file

#include <assert.h>
#include <string.h>

unsigned file, system, variable;

int main(int argc, char **argv, char **envp) {
  for (char **env = envp; *env != 0; ++env) {
    if (!strncmp(*env, "KLEE_TEST_FLE=VA0", 18)) ++file;
    if (!strncmp(*env, "KLEE_TEST_FLE=VA1", 18)) ++file;
    if (!strncmp(*env, "KLEE_TEST_SYS=VAL", 18)) ++system;
    if (!strncmp(*env, "KLEE_TEST_VAR=VA0", 18)) ++variable;
    if (!strncmp(*env, "KLEE_TEST_VAR=VA1", 18)) ++variable;
  }

  assert(file == 2 && system == 1 && variable == 2);
  return 0;
}
