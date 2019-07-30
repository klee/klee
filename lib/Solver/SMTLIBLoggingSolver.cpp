//===-- SMTLIBLoggingSolver.cpp -------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "QueryLoggingSolver.h"

#include "klee/Expr/ExprSMTLIBPrinter.h"

using namespace klee;

/// This QueryLoggingSolver will log queries to a file in the SMTLIBv2 format
/// and pass the query down to the underlying solver.
class SMTLIBLoggingSolver : public QueryLoggingSolver
{
        private:
    
                ExprSMTLIBPrinter printer;

                virtual void printQuery(const Query& query,
                                        const Query* falseQuery = 0,
                                        const std::vector<const Array*>* objects = 0) 
                {
                        if (0 == falseQuery) 
                        {
                                printer.setQuery(query);
                        }
                        else
                        {
                                printer.setQuery(*falseQuery);
                        }

                        if (0 != objects)
                        {
                                printer.setArrayValuesToGet(*objects);
                        }

                        printer.generateOutput();
                }    
        
	public:
		SMTLIBLoggingSolver(Solver *_solver,
                        std::string path,
                        time::Span queryTimeToLog,
                        bool logTimedOut)
		: QueryLoggingSolver(_solver, path, ";", queryTimeToLog, logTimedOut)
		{
		  //Setup the printer
		  printer.setOutput(logBuffer);
		}
};


Solver* klee::createSMTLIBLoggingSolver(Solver *_solver, std::string path,
                                        time::Span minQueryTimeToLog, bool logTimedOut)
{
  return new Solver(new SMTLIBLoggingSolver(_solver, path, minQueryTimeToLog, logTimedOut));
}
