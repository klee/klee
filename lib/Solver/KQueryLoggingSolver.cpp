//===-- KQueryLoggingSolver.cpp -----------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "QueryLoggingSolver.h"

#include "klee/Expr/Expr.h"
#include "klee/Expr/ExprPPrinter.h"
#include "klee/System/Time.h"

#include <utility>

using namespace klee;

class KQueryLoggingSolver : public QueryLoggingSolver {

private :
    ExprPPrinter *printer;

    virtual void printQuery(const Query& query,
                            const Query* falseQuery = 0,
                            const std::vector<const Array*>* objects = 0) {

        const ref<Expr>* evalExprsBegin = 0;
        const ref<Expr>* evalExprsEnd = 0;

        if (0 != falseQuery) {
            evalExprsBegin = &query.expr;
            evalExprsEnd = &query.expr + 1;
        }

        const Array* const *evalArraysBegin = 0;
        const Array* const *evalArraysEnd = 0;

        if ((0 != objects) && (false == objects->empty())) {
            evalArraysBegin = &((*objects)[0]);
            evalArraysEnd = &((*objects)[0]) + objects->size();
        }

        const Query* q = (0 == falseQuery) ? &query : falseQuery;

        printer->printQuery(logBuffer, q->constraints, q->expr,
                            evalExprsBegin, evalExprsEnd,
                            evalArraysBegin, evalArraysEnd);
    }

public:
  KQueryLoggingSolver(std::unique_ptr<Solver> solver, std::string path,
                      time::Span queryTimeToLog, bool logTimedOut)
      : QueryLoggingSolver(std::move(solver), std::move(path), "#",
                           queryTimeToLog, logTimedOut),
        printer(ExprPPrinter::create(logBuffer)) {}

    virtual ~KQueryLoggingSolver() {
        delete printer;
    }
};

///

std::unique_ptr<Solver>
klee::createKQueryLoggingSolver(std::unique_ptr<Solver> solver,
                                std::string path, time::Span minQueryTimeToLog,
                                bool logTimedOut) {
  return std::make_unique<Solver>(std::make_unique<KQueryLoggingSolver>(
      std::move(solver), std::move(path), minQueryTimeToLog, logTimedOut));
}
