//===-- SMTLIBLoggingSolver.cpp -----------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Solver.h"

#include "klee/Expr.h"
#include "klee/SolverImpl.h"
#include "klee/Statistics.h"

#include "klee/util/ExprSMTLIBLetPrinter.h"

#include "klee/Internal/Support/QueryLog.h"
#include "klee/Internal/System/Time.h"

#include "llvm/Support/CommandLine.h"

#include <fstream>

using namespace klee;
using namespace llvm;
using namespace klee::util;

///This SolverImpl will log queries to a file in the SMTLIBv2 format
///and pass the query down to the underlying solver.
///
///This is basically just a "copy" of PCLoggingSolver with a few modifications.
class SMTLIBLoggingSolver : public SolverImpl
{
	Solver *solver;
	std::ofstream os;
	ExprSMTLIBPrinter* printer;
	unsigned queryCount;
	double startTime;

	void startQuery(const Query& query, const char *typeName, const std::vector<const Array*>* objects=NULL)
	{
		Statistic *S = theStatisticManager->getStatisticByName("Instructions");
		uint64_t instructions = S ? S->getValue() : 0;
		os << ";SMTLIBv2 Query " << queryCount++ << " -- "
		   << "Type: " << typeName << ", "
		   << "Instructions: " << instructions << "\n";
		printer->setQuery(query);

		if(objects!=NULL)
			printer->setArrayValuesToGet(*objects);

		printer->generateOutput();
		os << "\n";

		startTime = getWallTime();
	}

	void finishQuery(bool success)
	{
		double delta = getWallTime() - startTime;
		os << ";   " << (success ? "OK" : "FAIL") << " -- "
		   << "Elapsed: " << delta << "\n";
	}

	public:
		SMTLIBLoggingSolver(Solver *_solver, std::string path)
		: solver(_solver),
		os(path.c_str(), std::ios::trunc),
		printer(),
		queryCount(0),
		startTime(0)
		{
		  //Setup the printer
		  printer = createSMTLIBPrinter();
		  printer->setOutput(os);
		}

		~SMTLIBLoggingSolver()
		{
			delete printer;
			delete solver;
		}

		bool computeTruth(const Query& query, bool &isValid)
		{
			startQuery(query, "Truth");
			bool success = solver->impl->computeTruth(query, isValid);
			finishQuery(success);
			if (success)
			  os << ";   Is Valid: " << (isValid ? "true" : "false") << "\n";
			os << "\n";
			return success;
		}

		bool computeValidity(const Query& query, Solver::Validity &result)
		{
			startQuery(query, "Validity");
			bool success = solver->impl->computeValidity(query, result);
			finishQuery(success);
			if (success)
			  os << ";   Validity: " << result << "\n";
			os << "\n";
			return success;
		}

		bool computeValue(const Query& query, ref<Expr> &result)
		{
			startQuery(query.withFalse(), "Value");

			bool success = solver->impl->computeValue(query, result);
			finishQuery(success);
			if (success)
			  os << ";   Result: " << result << "\n";
			os << "\n";
			return success;
		}

		bool computeInitialValues(const Query& query,
								const std::vector<const Array*> &objects,
								std::vector< std::vector<unsigned char> > &values,
								bool &hasSolution)
		{

		  startQuery(query, "InitialValues", &objects);


			bool success = solver->impl->computeInitialValues(query, objects,
															  values, hasSolution);
			finishQuery(success);
			if (success)
			{
				os << ";   Solvable: " << (hasSolution ? "true" : "false") << "\n";

				if (hasSolution)
				{
					std::vector< std::vector<unsigned char> >::iterator
					values_it = values.begin();
					for (std::vector<const Array*>::const_iterator i = objects.begin(),
					   e = objects.end(); i != e; ++i, ++values_it)
					{
						const Array *array = *i;
						std::vector<unsigned char> &data = *values_it;
						os << ";     " << array->name << " = [";

						for (unsigned j = 0; j < array->size; j++)
						{
							os << (int) data[j];
							if (j+1 < array->size)
							  os << ",";
						}

						os << "]\n";
					}
				}
			}

			os << "\n";
			return success;
		}
};


Solver* klee::createSMTLIBLoggingSolver(Solver *_solver, std::string path)
{
  return new Solver(new SMTLIBLoggingSolver(_solver, path));
}
