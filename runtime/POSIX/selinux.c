//===-- selinux.c ---------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

/* Very basic SELinux support */

#include "klee/Config/config.h"

#ifdef HAVE_SELINUX_SELINUX_H

#include "klee/klee.h"

#include <selinux/selinux.h>
#include <stdlib.h>
#include <errno.h>

/* for now, assume we run on an SELinux machine */
int exe_selinux = 1;

/* NULL is the default policy behavior */
security_context_t create_con = NULL;


int is_selinux_enabled() {
  return exe_selinux;
}


/***/

int getfscreatecon(security_context_t *context) {
  *context = create_con;
  return 0;
}


int setfscreatecon(security_context_t context) {
  if (context == NULL) {
    create_con = context;
    return 0;
  }

  /* on my machine, setfscreatecon seems to incorrectly accept one
     char strings.. Also, make sure mcstrans > 0.2.8 for replay 
     (important bug fixed) */
  if (context[0] != '\0' && context[1] == '\0')
    klee_silent_exit(1);

  return -1;
}

/***/

int setfilecon(const char *path, security_context_t con) {
  if (con)
    return 0;
  
  errno = ENOSPC;
  return -1;  
}

int lsetfilecon(const char *path, security_context_t con) {
  return setfilecon(path, con);
}

int fsetfilecon(int fd, security_context_t con) {
  return setfilecon("", con);
}

/***/

void freecon(security_context_t con) {}
void freeconary(security_context_t *con) {}

#endif
