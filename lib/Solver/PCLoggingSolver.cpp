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

using namespace klee;

///

class PCLoggingSolver : public QueryLoggingSolver {

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
    PCLoggingSolver(Solver *_solver, std::string path, int queryTimeToLog) 
    : QueryLoggingSolver(_solver, path, "#", queryTimeToLog),    
    printer(ExprPPrinter::create(logBuffer)) {
    }                                                      
    
    virtual ~PCLoggingSolver() {    
        delete printer;
    }    
};

///

Solver *klee::createPCLoggingSolver(Solver *_solver, std::string path,
                                    int minQueryTimeToLog) {
  return new Solver(new PCLoggingSolver(_solver, path, minQueryTimeToLog));
}
