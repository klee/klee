//===-- ExprSMTLIBPrinter.h ------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_EXPRSMTLIBPRINTER_H
#define KLEE_EXPRSMTLIBPRINTER_H

#include <ostream>
#include <string>
#include <set>
#include <map>
#include <klee/Constraints.h>
#include <klee/Expr.h>
#include <klee/util/PrintContext.h>
#include <klee/Solver.h>

namespace klee {

	///Base Class for SMTLIBv2 printer for Expr trees. It uses the QF_ABV logic. Note however the logic can be
	///set to QF_AUFBV because some solvers (e.g. STP) complain if this logic is set to QF_ABV.
	///
	///This printer does not abbreviate expressions. The printer ExprSMTLIBLetPrinter does though.
	///
	/// It is intended to be used as follows
	/// -# Create instance of this class
	/// -# Set output ( setOutput() )
	/// -# Set query to print ( setQuery() )
	/// -# Set options using the methods prefixed with the word "set".
	/// -# Call generateOutput()
	///
	/// The class can then be used again on another query ( setQuery() ).
	/// The options set are persistent across queries (apart from setArrayValuesToGet() and PRODUCE_MODELS)
	class ExprSMTLIBPrinter
	{
		public:

			///Different SMTLIBv2 logics supported by this class
			/// \sa setLogic()
			enum SMTLIBv2Logic
			{
				QF_ABV,  ///< Logic using Theory of Arrays and Theory of Bitvectors
				QF_AUFBV ///< Logic using Theory of Arrays and Theory of Bitvectors and has uninterpreted functions
			};

			///Different SMTLIBv2 options that have a boolean value that can be set
			/// \sa setSMTLIBboolOption
			enum SMTLIBboolOptions
			{
				PRINT_SUCCESS, ///< print-success SMTLIBv2 option
				PRODUCE_MODELS,///< produce-models SMTLIBv2 option
				INTERACTIVE_MODE ///< interactive-mode SMTLIBv2 option
			};

			///Different SMTLIBv2 bool option values
			/// \sa setSMTLIBboolOption
			enum SMTLIBboolValues
			{
				OPTION_TRUE, ///< Set option to true
				OPTION_FALSE, ///< Set option to false
				OPTION_DEFAULT ///< Use solver's defaults (the option will not be set in output)
			};

			enum ConstantDisplayMode
			{
				BINARY,///< Display bit vector constants in binary e.g. #b00101101
				HEX, ///< Display bit vector constants in Hexidecimal e.g.#x2D
				DECIMAL ///< Display bit vector constants in Decimal e.g. (_ bv45 8)
			};

			///Different supported SMTLIBv2 sorts (a.k.a type) in QF_AUFBV
			enum SMTLIB_SORT
			{
				SORT_BITVECTOR,
				SORT_BOOL
			};



			///Allows the way Constant bitvectors are printed to be changed.
			///This setting is persistent across queries.
			/// \return true if setting the mode was successful
			bool setConstantDisplayMode(ConstantDisplayMode cdm);

			ConstantDisplayMode getConstantDisplayMode() { return cdm;}

			///Create a new printer that will print a query in the SMTLIBv2 language.
			ExprSMTLIBPrinter();

			///Set the output stream that will be printed to.
			///This call is persistent across queries.
			void setOutput(std::ostream& output);

			///Set the query to print. This will setArrayValuesToGet()
			///to none (i.e. no array values will be requested using
			///the SMTLIBv2 (get-value ()) command).
			void setQuery(const Query& q);

			virtual ~ExprSMTLIBPrinter();

			/// Print the query to the std::ostream
			/// setOutput() and setQuery() must be called before calling this.
			///
			/// All options should be set before calling this.
			/// \sa setConstantDisplayMode
			/// \sa setLogic()
			/// \sa setHumanReadable
			/// \sa setSMTLIBboolOption
			/// \sa setArrayValuesToGet
			///
			/// Mostly it does not matter what order the options are set in. However calling
			/// setArrayValuesToGet() implies PRODUCE_MODELS is set so, if a call to setSMTLIBboolOption()
			/// is made that uses the PRODUCE_MODELS before calling setArrayValuesToGet() then the setSMTLIBboolOption()
			/// call will be ineffective.
			virtual void generateOutput();

			///Set which SMTLIBv2 logic to use.
			///This only affects what logic is used in the (set-logic <logic>) command.
			///The rest of the printed SMTLIBv2 commands are the same regardless of the logic used.
			///
			/// \return true if setting logic was successful.
			bool setLogic(SMTLIBv2Logic l);

			///Sets how readable the printed SMTLIBv2 commands are.
			/// \param hr If set to true the printed commands are made more human readable.
			///
			/// The printed commands are made human readable by...
			/// - Indenting and line breaking.
			/// - Adding comments
			void setHumanReadable(bool hr);

			///Set SMTLIB options.
			/// These options will be printed when generateOutput() is called via
			/// the SMTLIBv2 command (set-option :option-name <value>)
			///
			/// By default no options will be printed so the SMTLIBv2 solver will use
			/// its default values for all options.
			///
			/// \return true if option was successfully set.
			///
			/// The options set are persistent across calls to setQuery() apart from the
			/// PRODUCE_MODELS option which this printer may automatically set/unset.
			bool setSMTLIBboolOption(SMTLIBboolOptions option, SMTLIBboolValues value);

			/// Set the array names that the SMTLIBv2 solver will be asked to determine.
			/// Calling this method implies the PRODUCE_MODELS option is true and will change
			/// any previously set value.
			///
			/// If no call is made to this function before ExprSMTLIBPrinter::generateOutput() then
			/// the solver will only be asked to check satisfiability.
			///
			/// If the passed vector is not empty then the values of those arrays will be requested
			/// via (get-value ()) SMTLIBv2 command in the output stream in the same order as vector.
			void setArrayValuesToGet(const std::vector<const Array*>& a);

			/// \return True if human readable mode is switched on
			bool isHumanReadable();


		protected:
			///Contains the arrays found during scans
			std::set<const Array*> usedArrays;

			///Output stream to write to
			std::ostream* o;

			///The query to print
			const Query* query;

			///Determine the SMTLIBv2 sort of the expression
			SMTLIB_SORT getSort(const ref<Expr>& e);

			///Print an expression but cast it to a particular SMTLIBv2 sort first.
			void printCastToSort(const ref<Expr>& e, ExprSMTLIBPrinter::SMTLIB_SORT sort);

			//Resets various internal objects for a new query
			virtual void reset();

			//Scan all constraints and the query
			virtual void scanAll();

			//Print an initial SMTLIBv2 comment before anything else is printed
			virtual void printNotice();

			//Print SMTLIBv2 options e.g. (set-option :option-name <value>) command
			virtual void printOptions();

			//Print SMTLIBv2 logic to use e.g. (set-logic QF_ABV)
			virtual void printSetLogic();

			//Print SMTLIBv2 assertions for constant arrays
			virtual void printArrayDeclarations();

			//Print SMTLIBv2 for all constraints in the query
			virtual void printConstraints();

			//Print SMTLIBv2 assert statement for the "mangled" query
			virtual void printQuery();

			///Print the SMTLIBv2 command to check satisfiability and also optionally request for values
			/// \sa setArrayValuesToGet()
			virtual void printAction();

			///Print the SMTLIBv2 command to exit
			virtual void printExit();

			///Print a Constant in the format specified by the current "Constant Display Mode"
			void printConstant(const ref<ConstantExpr>& e);

			///Recursively print expression
			/// \param e is the expression to print
			/// \param expectedSort is the sort we want. If "e" is not of the right type a cast will be performed.
			virtual void printExpression(const ref<Expr>& e, ExprSMTLIBPrinter::SMTLIB_SORT expectedSort);

			///Scan Expression recursively for Arrays in expressions. Found arrays are added to
			/// the usedArrays vector.
			virtual void scan(const ref<Expr>& e);

			/* Rules of recursion for "Special Expression handlers" and printSortArgsExpr()
			 *
			 * 1. The parent of the recursion will have created an indent level for you so you don't need to add another initially.
			 * 2. You do not need to add a line break (via printSeperator() ) at the end, the parent caller will handle that.
			 * 3. The effect of a single recursive call should not affect the depth of the indent stack (nor the contents
			 *    of the indent stack prior to the call). I.e. After executing a single recursive call the indent stack
			 *    should have the same size and contents as before executing the recursive call.
			 */

			//Special Expression handlers
			virtual void printReadExpr(const ref<ReadExpr>& e);
			virtual void printExtractExpr(const ref<ExtractExpr>& e);
			virtual void printCastExpr(const ref<CastExpr>& e);
			virtual void printNotEqualExpr(const ref<NeExpr>& e);
			virtual void printSelectExpr(const ref<SelectExpr>& e, ExprSMTLIBPrinter::SMTLIB_SORT s);

			//For the set of operators that take sort "s" arguments
			virtual void printSortArgsExpr(const ref<Expr>& e, ExprSMTLIBPrinter::SMTLIB_SORT s);

			///For the set of operators that come in two sorts (e.g. (and () ()) (bvand () ()) )
			///These are and,xor,or,not
			/// \param e the Expression to print
			/// \param s the sort of the expression we want
			virtual void printLogicalOrBitVectorExpr(const ref<Expr>& e, ExprSMTLIBPrinter::SMTLIB_SORT s);

			///Recursively prints updatesNodes
			virtual void printUpdatesAndArray(const UpdateNode* un, const Array* root);

			///This method does the translation between Expr classes and SMTLIBv2 keywords
			/// \return A C-string of the SMTLIBv2 keyword
			virtual const char* getSMTLIBKeyword(const ref<Expr>& e);

			virtual void printSeperator();

			///Helper function for scan() that scans the expressions of an update node
			virtual void scanUpdates(const UpdateNode* un);

			///Helper printer class
			PrintContext* p;

			///This contains the query from the solver but negated for our purposes.
			/// \sa mangleQuery()
			ref<Expr> queryAssert;

			///Indicates if there were any constant arrays founds during a scan()
			bool haveConstantArray;


		private:
			SMTLIBv2Logic logicToUse;

			volatile bool humanReadable;

			//Map of enabled SMTLIB Options
			std::map<SMTLIBboolOptions,bool> smtlibBoolOptions;

			///This sets queryAssert to be the boolean negation of the original Query
			void mangleQuery();

			//Print a SMTLIBv2 option as a C-string
			const char* getSMTLIBOptionString(ExprSMTLIBPrinter::SMTLIBboolOptions option);

			//Pointer to a vector of Arrays. These will be used for the (get-value () ) call.
			const std::vector<const Array*> * arraysToCallGetValueOn;

			ConstantDisplayMode cdm;

	};



}

#endif
