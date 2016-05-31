//===-- PCLoggingSolver.cpp -----------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "QueryLoggingSolver.h"

#include "klee/Expr.h"
#include "klee/util/ExprPPrinter.h"
#include "klee/Internal/Support/QueryLog.h"

#include "klee/Internal/Module/InstructionInfoTable.h"

using namespace klee;

///

class PCLoggingSolver : public QueryLoggingSolver {

private :
    ExprPPrinter *printer;
    const InstructionInfoProvider *iip;

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

        if (iip) {
          const InstructionInfo *ii = iip->GetCurrentInstruction();
          if (ii && ii->file != "")
            logBuffer << "# Code location: " << ii->file << ":" << ii->line
                      << "\n";
          else if (ii)
            logBuffer << "# Code location: [no debug info]\n";
          else
            logBuffer << "# Code location: [unknown location]\n";
        } else {
          logBuffer << "# Code location: [unknown location]\n";
        }

        printer->printQuery(logBuffer, q->constraints, q->expr,
                            evalExprsBegin, evalExprsEnd,
                            evalArraysBegin, evalArraysEnd);
    }
  
public:
  PCLoggingSolver(Solver *_solver, std::string path, int queryTimeToLog,
                  const InstructionInfoProvider *_iip)
      : QueryLoggingSolver(_solver, path, "#", queryTimeToLog),
        printer(ExprPPrinter::create(logBuffer)), iip(_iip) {}

    virtual ~PCLoggingSolver() {    
        delete printer;
    }
};

///

Solver *klee::createPCLoggingSolver(Solver *_solver, std::string path,
                                    int minQueryTimeToLog,
                                    const InstructionInfoProvider *iip) {
  return new Solver(new PCLoggingSolver(_solver, path, minQueryTimeToLog, iip));
}
