#define _LARGEFILE64_SOURCE
#include "../../runtime/POSIX/klee_init_env.c"

int __klee_posix_wrapped_main(__attribute__((unused)) int argc,
                              __attribute__((unused)) char **argv,
                              __attribute__((unused)) char **envp) {
  return 0;
}
