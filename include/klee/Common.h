//===-- Common.h ------------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

/*
 * This file groups declarations that are common to both KLEE and Kleaver.
 */

#ifndef KLEE_COMMON_H
#define KLEE_COMMON_H

namespace klee {

#define ALL_QUERIES_SMT2_FILE_NAME      "all-queries.smt2"
#define SOLVER_QUERIES_SMT2_FILE_NAME   "solver-queries.smt2"
#define ALL_QUERIES_PC_FILE_NAME        "all-queries.pc"
#define SOLVER_QUERIES_PC_FILE_NAME     "solver-queries.pc"

}

#endif /* KLEE_COMMON_H */

