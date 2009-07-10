//===-- Solver.h ------------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_SOLVER_H
#define KLEE_SOLVER_H

#include "klee/Expr.h"

#include <vector>

namespace klee {
  class ConstraintManager;
  class Expr;
  class SolverImpl;

  struct Query {
  public:
    const ConstraintManager &constraints;
    ref<Expr> expr;

    Query(const ConstraintManager& _constraints, ref<Expr> _expr)
      : constraints(_constraints), expr(_expr) {
    }

    /// withExpr - Return a copy of the query with the given expression.
    Query withExpr(ref<Expr> _expr) const {
      return Query(constraints, _expr);
    }

    /// withFalse - Return a copy of the query with a false expression.
    Query withFalse() const {
      return Query(constraints, ConstantExpr::alloc(0, Expr::Bool));
    }

    /// negateExpr - Return a copy of the query with the expression negated.
    Query negateExpr() const {
      return withExpr(Expr::createIsZero(expr));
    }
  };

  class Solver {
    // DO NOT IMPLEMENT.
    Solver(const Solver&);
    void operator=(const Solver&);

  public:
    enum Validity {
      True = 1,
      False = -1,
      Unknown = 0
    };
  
  public:
    /// validity_to_str - Return the name of given Validity enum value.
    static const char *validity_to_str(Validity v);

  public:
    SolverImpl *impl;

  public:
    Solver(SolverImpl *_impl) : impl(_impl) {};
    virtual ~Solver();

    /// evaluate - Determine the full validity of an expression in particular
    /// state.
    ////
    /// \param [out] result - The validity of the given expression (provably
    /// true, provably false, or neither).
    ///
    /// \return True on success.
    bool evaluate(const Query&, Validity &result);
  
    /// mustBeTrue - Determine if the expression is provably true.
    ///
    /// \param [out] result - On success, true iff the expresssion is provably
    /// false.
    ///
    /// \return True on success.
    bool mustBeTrue(const Query&, bool &result);

    /// mustBeFalse - Determine if the expression is provably false.
    ///
    /// \param [out] result - On success, true iff the expresssion is provably
    /// false.
    ///
    /// \return True on success.
    bool mustBeFalse(const Query&, bool &result);

    /// mayBeTrue - Determine if there is a valid assignment for the given state
    /// in which the expression evaluates to false.
    ///
    /// \param [out] result - On success, true iff the expresssion is true for
    /// some satisfying assignment.
    ///
    /// \return True on success.
    bool mayBeTrue(const Query&, bool &result);

    /// mayBeFalse - Determine if there is a valid assignment for the given
    /// state in which the expression evaluates to false.
    ///
    /// \param [out] result - On success, true iff the expresssion is false for
    /// some satisfying assignment.
    ///
    /// \return True on success.
    bool mayBeFalse(const Query&, bool &result);

    /// getValue - Compute one possible value for the given expression.
    ///
    /// \param [out] result - On success, a value for the expression in some
    /// satisying assignment.
    ///
    /// \return True on success.
    bool getValue(const Query&, ref<ConstantExpr> &result);

    /// getInitialValues - Compute the initial values for a list of objects.
    ///
    /// \param [out] result - On success, this vector will be filled in with an
    /// array of bytes for each given object (with length matching the object
    /// size). The bytes correspond to the initial values for the objects for
    /// some satisying assignment.
    ///
    /// \return True on success.
    ///
    /// NOTE: This function returns failure if there is no satisfying
    /// assignment.
    //
    // FIXME: This API is lame. We should probably just provide an API which
    // returns an Assignment object, then clients can get out whatever values
    // they want. This also allows us to optimize the representation.
    bool getInitialValues(const Query&, 
                          const std::vector<const Array*> &objects,
                          std::vector< std::vector<unsigned char> > &result);

    /// getRange - Compute a tight range of possible values for a given
    /// expression.
    ///
    /// \return - A pair with (min, max) values for the expression.
    ///
    /// \post(mustBeTrue(min <= e <= max) && 
    ///       mayBeTrue(min == e) &&
    ///       mayBeTrue(max == e))
    //
    // FIXME: This should go into a helper class, and should handle failure.
    virtual std::pair< ref<Expr>, ref<Expr> > getRange(const Query&);
  };

  /// STPSolver - A complete solver based on STP.
  class STPSolver : public Solver {
  public:
    /// STPSolver - Construct a new STPSolver.
    ///
    /// \param useForkedSTP - Whether STP should be run in a separate process
    /// (required for using timeouts).
    STPSolver(bool useForkedSTP);

    
    
    /// getConstraintLog - Return the constraint log for the given state in CVC
    /// format.
    char *getConstraintLog(const Query&);
    
    /// setTimeout - Set constraint solver timeout delay to the given value; 0
    /// is off.
    void setTimeout(double timeout);
  };

  /* *** */

  /// createValidatingSolver - Create a solver which will validate all query
  /// results against an oracle, used for testing that an optimized solver has
  /// the same results as an unoptimized one. This solver will assert on any
  /// mismatches.
  ///
  /// \param s - The primary underlying solver to use.
  /// \param oracle - The solver to check query results against.
  Solver *createValidatingSolver(Solver *s, Solver *oracle);

  /// createCachingSolver - Create a solver which will cache the queries in
  /// memory (without eviction).
  ///
  /// \param s - The underlying solver to use.
  Solver *createCachingSolver(Solver *s);

  /// createCexCachingSolver - Create a counterexample caching solver. This is a
  /// more sophisticated cache which records counterexamples for a constraint
  /// set and uses subset/superset relations among constraints to try and
  /// quickly find satisfying assignments.
  ///
  /// \param s - The underlying solver to use.
  Solver *createCexCachingSolver(Solver *s);

  /// createFastCexSolver - Create a "fast counterexample solver", which tries
  /// to quickly compute a satisfying assignment for a constraint set using
  /// value propogation and range analysis.
  ///
  /// \param s - The underlying solver to use.
  Solver *createFastCexSolver(Solver *s);

  /// createIndependentSolver - Create a solver which will eliminate any
  /// unnecessary constraints before propogating the query to the underlying
  /// solver.
  ///
  /// \param s - The underlying solver to use.
  Solver *createIndependentSolver(Solver *s);
  
  /// createPCLoggingSolver - Create a solver which will forward all queries
  /// after writing them to the given path in .pc format.
  Solver *createPCLoggingSolver(Solver *s, std::string path);

  /// createDummySolver - Create a dummy solver implementation which always
  /// fails.
  Solver *createDummySolver();
}

#endif
