//===-- misc.c ------------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/klee.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

#if 0
#define MAX_SYM_ENV_SIZE 32
typedef struct {
  char name[MAX_SYM_ENV_SIZE];
  char *value;
} sym_env_var;

static sym_env_var *__klee_sym_env = 0;
static unsigned __klee_sym_env_count = 0;
static unsigned __klee_sym_env_nvars = 0;
static unsigned __klee_sym_env_var_size = 0;
void __klee_init_environ(unsigned nvars, 
                         unsigned var_size) {
  assert(var_size);
  __klee_sym_env = malloc(sizeof(*__klee_sym_env) * nvars);
  assert(__klee_sym_env);

  __klee_sym_env_nvars = nvars;
  __klee_sym_env_var_size = var_size;  
}

static size_t __strlen(const char *s) {
  const char *s2 = s;
  while (*s2) ++s2;
  return s2-s;
}

extern char *__getenv(const char *name);
char *getenv(const char *name) {
  char *res = __getenv(name);

  if (!__klee_sym_env_nvars)
    return res;

  /* If it exists in the system environment fork and return the actual
     result or 0. */
  if (res) {
    return klee_range(0, 2, name) ? res : 0;
  } else {
    size_t i, len = __strlen(name);

    if (len>=MAX_SYM_ENV_SIZE) {
      /* Don't deal with strings to large to fit in our name. */
      return 0;
    } else {
      /* Check for existing entry */
      for (i=0; i<__klee_sym_env_count; ++i)
        if (memcmp(__klee_sym_env[i].name, name, len+1)==0)
          return __klee_sym_env[i].value;
      
      /* Otherwise create if room and we choose to */
      if (__klee_sym_env_count < __klee_sym_env_nvars) {
        if (klee_range(0, 2, name)) {
          char *s = malloc(__klee_sym_env_var_size+1);
          klee_make_symbolic(s, __klee_sym_env_var_size+1. "env");
          s[__klee_sym_env_var_size] = '\0';
          
          memcpy(__klee_sym_env[__klee_sym_env_count].name, name, len+1);
          __klee_sym_env[__klee_sym_env_count].value = s;
          ++__klee_sym_env_count;
          
          return s;
        }
      }
      
      return 0;
    }
  }
}
#endif
