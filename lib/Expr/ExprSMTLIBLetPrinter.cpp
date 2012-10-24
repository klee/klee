//===-- ExprSMTLIBLetPrinter.cpp ------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <iostream>
#include "llvm/Support/CommandLine.h"
#include "klee/util/ExprSMTLIBLetPrinter.h"

using namespace std;

namespace ExprSMTLIBOptions
{
	llvm::cl::opt<bool> useLetExpressions("smtlib-use-let-expressions",
			llvm::cl::desc("Enables generated SMT-LIBv2 files to use (let) expressions (default=on)"),
			llvm::cl::init(true)
	);
}

namespace klee
{
	const char ExprSMTLIBLetPrinter::BINDING_PREFIX[] = "?B";


	ExprSMTLIBLetPrinter::ExprSMTLIBLetPrinter() :
	bindings(), firstEO(), twoOrMoreEO(),
	disablePrintedAbbreviations(false)
	{
	}

	void ExprSMTLIBLetPrinter::generateOutput()
	{
		if(p==NULL || query == NULL || o ==NULL)
		{
			std::cerr << "Can't print SMTLIBv2. Output or query bad!" << std::endl;
			return;
		}

		generateBindings();

		if(isHumanReadable()) printNotice();
		printOptions();
		printSetLogic();
		printArrayDeclarations();
		printLetExpression();
		printAction();
		printExit();
	}

	void ExprSMTLIBLetPrinter::scan(const ref<Expr>& e)
	{
		if(isa<ConstantExpr>(e))
			return; //we don't need to scan simple constants

		if(firstEO.insert(e).second)
		{
			//We've not seen this expression before

			if(const ReadExpr* re = dyn_cast<ReadExpr>(e))
			{

				//Attempt to insert array and if array wasn't present before do more things
				if(usedArrays.insert(re->updates.root).second)
				{

					//check if the array is constant
					if( re->updates.root->isConstantArray())
						haveConstantArray=true;

					//scan the update list
					scanUpdates(re->updates.head);

				}

			}

			//recurse into the children
			Expr* ep = e.get();
			for(unsigned int i=0; i < ep->getNumKids(); i++)
				scan(ep->getKid(i));
		}
		else
		{
			/* We must of seen the expression before. Add it to
			 * the set of twoOrMoreOccurances. We don't need to
			 * check if the insertion fails.
			 */
			twoOrMoreEO.insert(e);
		}
	}

	void ExprSMTLIBLetPrinter::generateBindings()
	{
		//Assign a number to each binding that will be used
		unsigned int counter=0;
		for(set<ref<Expr> >::const_iterator i=twoOrMoreEO.begin();
				i!= twoOrMoreEO.end(); ++i)
		{
			bindings.insert(std::make_pair(*i,counter));
			++counter;
		}
	}

	void ExprSMTLIBLetPrinter::printExpression(const ref<Expr>& e, ExprSMTLIBPrinter::SMTLIB_SORT expectedSort)
	{
		map<const ref<Expr>,unsigned int>::const_iterator i= bindings.find(e);

		if(disablePrintedAbbreviations || i == bindings.end())
		{
			/*There is no abbreviation for this expression so print it normally.
			 * Do this by using the parent method.
			 */
			ExprSMTLIBPrinter::printExpression(e,expectedSort);
		}
		else
		{
			//Use binding name e.g. "?B1"

			/* Handle the corner case where the expectedSort
			 * doesn't match the sort of the abbreviation. Not really very efficient (calls bindings.find() twice),
			 * we'll cast and call ourself again but with the correct expectedSort.
			 */
			if(getSort(e) != expectedSort)
			{
				printCastToSort(e,expectedSort);
				return;
			}

			// No casting is needed in this depth of recursion, just print the abbreviation
			*p << BINDING_PREFIX << i->second;
		}
	}

	void ExprSMTLIBLetPrinter::reset()
	{
		//Let parent clean up first
		ExprSMTLIBPrinter::reset();

		firstEO.clear();
		twoOrMoreEO.clear();
		bindings.clear();
	}

	void ExprSMTLIBLetPrinter::printLetExpression()
	{
		*p << "(assert"; p->pushIndent(); printSeperator();

		if(bindings.size() !=0 )
		{
			//Only print let expression if we have bindings to use.
			*p << "(let"; p->pushIndent(); printSeperator();
			*p << "( "; p->pushIndent();

			//Print each binding
			for(map<const ref<Expr>, unsigned int>::const_iterator i= bindings.begin();
					i!=bindings.end(); ++i)
			{
				printSeperator();
				*p << "(" << BINDING_PREFIX << i->second << " ";
				p->pushIndent();

				//Disable abbreviations so none are used here.
				disablePrintedAbbreviations=true;

				//We can abbreviate SORT_BOOL or SORT_BITVECTOR in let expressions
				printExpression(i->first,getSort(i->first));

				p->popIndent();
				printSeperator();
				*p << ")";
			}


			p->popIndent(); printSeperator();
			*p << ")";
			printSeperator();

			//Re-enable printing abbreviations.
			disablePrintedAbbreviations=false;

		}

		//print out Expressions with abbreviations.
		unsigned int numberOfItems= query->constraints.size() +1; //+1 for query
		unsigned int itemsLeft=numberOfItems;
		vector<ref<Expr> >::const_iterator constraint=query->constraints.begin();

		/* Produce nested (and () () statements. If the constraint set
		 * is empty then we will only print the "queryAssert".
		 */
		for(; itemsLeft !=0; --itemsLeft)
		{
			if(itemsLeft >=2)
			{
				*p << "(and"; p->pushIndent(); printSeperator();
				printExpression(*constraint,SORT_BOOL); //We must and together bool expressions
				printSeperator();
				++constraint;
				continue;
			}
			else
			{
				// must have 1 item left (i.e. the "queryAssert")
				if(isHumanReadable()) { *p << "; query"; p->breakLineI();}
				printExpression(queryAssert,SORT_BOOL); //The query must be a bool expression

			}
		}

		/* Produce closing brackets for nested "and statements".
		 * Number of "nested ands" = numberOfItems -1
		 */
		itemsLeft= numberOfItems -1;
		for(; itemsLeft!=0; --itemsLeft)
		{
			p->popIndent(); printSeperator();
			*p << ")";
		}



		if(bindings.size() !=0)
		{
			//end let expression
			p->popIndent(); printSeperator();
			*p << ")";  printSeperator();
		}

		//end assert
		p->popIndent(); printSeperator();
		*p << ")";
		p->breakLineI();
	}

	ExprSMTLIBPrinter* createSMTLIBPrinter()
	{
		if(ExprSMTLIBOptions::useLetExpressions)
			return new ExprSMTLIBLetPrinter();
		else
			return new ExprSMTLIBPrinter();
	}


}

