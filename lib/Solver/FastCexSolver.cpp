//===-- FastCexSolver.cpp -------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Solver.h"

#include "klee/Constraints.h"
#include "klee/Expr.h"
#include "klee/IncompleteSolver.h"
#include "klee/util/ExprEvaluator.h"
#include "klee/util/ExprRangeEvaluator.h"
#include "klee/util/ExprVisitor.h"
// FIXME: Use APInt.
#include "klee/Internal/Support/IntEvaluation.h"

#include <iostream>
#include <sstream>
#include <cassert>
#include <map>
#include <vector>

using namespace klee;

/***/

//#define LOG
#ifdef LOG
std::ostream *theLog;
#endif

      // Hacker's Delight, pgs 58-63
static uint64_t minOR(uint64_t a, uint64_t b,
                      uint64_t c, uint64_t d) {
  uint64_t temp, m = ((uint64_t) 1)<<63;
  while (m) {
    if (~a & c & m) {
      temp = (a | m) & -m;
      if (temp <= b) { a = temp; break; }
    } else if (a & ~c & m) {
      temp = (c | m) & -m;
      if (temp <= d) { c = temp; break; }
    }
    m >>= 1;
  }
  
  return a | c;
}
static uint64_t maxOR(uint64_t a, uint64_t b,
                      uint64_t c, uint64_t d) {
  uint64_t temp, m = ((uint64_t) 1)<<63;

  while (m) {
    if (b & d & m) {
      temp = (b - m) | (m - 1);
      if (temp >= a) { b = temp; break; }
      temp = (d - m) | (m -1);
      if (temp >= c) { d = temp; break; }
    }
    m >>= 1;
  }

  return b | d;
}
static uint64_t minAND(uint64_t a, uint64_t b,
                       uint64_t c, uint64_t d) {
  uint64_t temp, m = ((uint64_t) 1)<<63;
  while (m) {
    if (~a & ~c & m) {
      temp = (a | m) & -m;
      if (temp <= b) { a = temp; break; }
      temp = (c | m) & -m;
      if (temp <= d) { c = temp; break; }
    }
    m >>= 1;
  }
  
  return a & c;
}
static uint64_t maxAND(uint64_t a, uint64_t b,
                       uint64_t c, uint64_t d) {
  uint64_t temp, m = ((uint64_t) 1)<<63;
  while (m) {
    if (b & ~d & m) {
      temp = (b & ~m) | (m - 1);
      if (temp >= a) { b = temp; break; }
    } else if (~b & d & m) {
      temp = (d & ~m) | (m - 1);
      if (temp >= c) { d = temp; break; }
    }
    m >>= 1;
  }
  
  return b & d;
}

///

class ValueRange {
private:
  uint64_t m_min, m_max;

public:
  ValueRange() : m_min(1),m_max(0) {}
  ValueRange(uint64_t value) : m_min(value), m_max(value) {}
  ValueRange(uint64_t _min, uint64_t _max) : m_min(_min), m_max(_max) {}
  ValueRange(const ValueRange &b) : m_min(b.m_min), m_max(b.m_max) {}

  void print(std::ostream &os) const {
    if (isFixed()) {
      os << m_min;
    } else {
      os << "[" << m_min << "," << m_max << "]";
    }
  }

  bool isEmpty() const { 
    return m_min>m_max; 
  }
  bool contains(uint64_t value) const { 
    return this->intersects(ValueRange(value)); 
  }
  bool intersects(const ValueRange &b) const { 
    return !this->set_intersection(b).isEmpty(); 
  }

  bool isFullRange(unsigned bits) {
    return m_min==0 && m_max==bits64::maxValueOfNBits(bits);
  }

  ValueRange set_intersection(const ValueRange &b) const {
    return ValueRange(std::max(m_min,b.m_min), std::min(m_max,b.m_max));
  }
  ValueRange set_union(const ValueRange &b) const {
    return ValueRange(std::min(m_min,b.m_min), std::max(m_max,b.m_max));
  }
  ValueRange set_difference(const ValueRange &b) const {
    if (b.isEmpty() || b.m_min > m_max || b.m_max < m_min) { // no intersection
      return *this;
    } else if (b.m_min <= m_min && b.m_max >= m_max) { // empty
      return ValueRange(1,0); 
    } else if (b.m_min <= m_min) { // one range out
      // cannot overflow because b.m_max < m_max
      return ValueRange(b.m_max+1, m_max);
    } else if (b.m_max >= m_max) {
      // cannot overflow because b.min > m_min
      return ValueRange(m_min, b.m_min-1);
    } else {
      // two ranges, take bottom
      return ValueRange(m_min, b.m_min-1);
    }
  }
  ValueRange binaryAnd(const ValueRange &b) const {
    // XXX
    assert(!isEmpty() && !b.isEmpty() && "XXX");
    if (isFixed() && b.isFixed()) {
      return ValueRange(m_min & b.m_min);
    } else {
      return ValueRange(minAND(m_min, m_max, b.m_min, b.m_max),
                        maxAND(m_min, m_max, b.m_min, b.m_max));
    }
  }
  ValueRange binaryAnd(uint64_t b) const { return binaryAnd(ValueRange(b)); }
  ValueRange binaryOr(ValueRange b) const {
    // XXX
    assert(!isEmpty() && !b.isEmpty() && "XXX");
    if (isFixed() && b.isFixed()) {
      return ValueRange(m_min | b.m_min);
    } else {
      return ValueRange(minOR(m_min, m_max, b.m_min, b.m_max),
                        maxOR(m_min, m_max, b.m_min, b.m_max));
    }
  }
  ValueRange binaryOr(uint64_t b) const { return binaryOr(ValueRange(b)); }
  ValueRange binaryXor(ValueRange b) const {
    if (isFixed() && b.isFixed()) {
      return ValueRange(m_min ^ b.m_min);
    } else {
      uint64_t t = m_max | b.m_max;
      while (!bits64::isPowerOfTwo(t))
        t = bits64::withoutRightmostBit(t);
      return ValueRange(0, (t<<1)-1);
    }
  }

  ValueRange binaryShiftLeft(unsigned bits) const {
    return ValueRange(m_min<<bits, m_max<<bits);
  }
  ValueRange binaryShiftRight(unsigned bits) const {
    return ValueRange(m_min>>bits, m_max>>bits);
  }

  ValueRange concat(const ValueRange &b, unsigned bits) const {
    return binaryShiftLeft(bits).binaryOr(b);
  }
  ValueRange extract(uint64_t lowBit, uint64_t maxBit) const {
    return binaryShiftRight(lowBit).binaryAnd(bits64::maxValueOfNBits(maxBit-lowBit));
  }

  ValueRange add(const ValueRange &b, unsigned width) const {
    return ValueRange(0, bits64::maxValueOfNBits(width));
  }
  ValueRange sub(const ValueRange &b, unsigned width) const {
    return ValueRange(0, bits64::maxValueOfNBits(width));
  }
  ValueRange mul(const ValueRange &b, unsigned width) const {
    return ValueRange(0, bits64::maxValueOfNBits(width));
  }
  ValueRange udiv(const ValueRange &b, unsigned width) const {
    return ValueRange(0, bits64::maxValueOfNBits(width));
  }
  ValueRange sdiv(const ValueRange &b, unsigned width) const {
    return ValueRange(0, bits64::maxValueOfNBits(width));
  }
  ValueRange urem(const ValueRange &b, unsigned width) const {
    return ValueRange(0, bits64::maxValueOfNBits(width));
  }
  ValueRange srem(const ValueRange &b, unsigned width) const {
    return ValueRange(0, bits64::maxValueOfNBits(width));
  }

  // use min() to get value if true (XXX should we add a method to
  // make code clearer?)
  bool isFixed() const { return m_min==m_max; }

  bool operator==(const ValueRange &b) const { return m_min==b.m_min && m_max==b.m_max; }
  bool operator!=(const ValueRange &b) const { return !(*this==b); }

  bool mustEqual(const uint64_t b) const { return m_min==m_max && m_min==b; }
  bool mayEqual(const uint64_t b) const { return m_min<=b && m_max>=b; }
  
  bool mustEqual(const ValueRange &b) const { return isFixed() && b.isFixed() && m_min==b.m_min; }
  bool mayEqual(const ValueRange &b) const { return this->intersects(b); }

  uint64_t min() const { 
    assert(!isEmpty() && "cannot get minimum of empty range");
    return m_min; 
  }

  uint64_t max() const { 
    assert(!isEmpty() && "cannot get maximum of empty range");
    return m_max; 
  }
  
  int64_t minSigned(unsigned bits) const {
    assert((m_min>>bits)==0 && (m_max>>bits)==0 &&
           "range is outside given number of bits");

    // if max allows sign bit to be set then it can be smallest value,
    // otherwise since the range is not empty, min cannot have a sign
    // bit

    uint64_t smallest = ((uint64_t) 1 << (bits-1));
    if (m_max >= smallest) {
      return ints::sext(smallest, 64, bits);
    } else {
      return m_min;
    }
  }

  int64_t maxSigned(unsigned bits) const {
    assert((m_min>>bits)==0 && (m_max>>bits)==0 &&
           "range is outside given number of bits");

    uint64_t smallest = ((uint64_t) 1 << (bits-1));

    // if max and min have sign bit then max is max, otherwise if only
    // max has sign bit then max is largest signed integer, otherwise
    // max is max

    if (m_min < smallest && m_max >= smallest) {
      return smallest - 1;
    } else {
      return ints::sext(m_max, 64, bits);
    }
  }
};

inline std::ostream &operator<<(std::ostream &os, const ValueRange &vr) {
  vr.print(os);
  return os;
}

// used to find all memory object ids and the maximum size of any
// object state that references them (for symbolic size).
class ObjectFinder : public ExprVisitor {
protected:
  Action visitRead(const ReadExpr &re) {
    addUpdates(re.updates);
    return Action::doChildren();
  }

  // XXX nice if this information was cached somewhere, used by
  // independence as well right?
  void addUpdates(const UpdateList &ul) {
    for (const UpdateNode *un=ul.head; un; un=un->next) {
      visit(un->index);
      visit(un->value);
    }

    addObject(*ul.root);
  }

public:
  void addObject(const Array& array) {
    unsigned id = array.id;
    std::map<unsigned,unsigned>::iterator it = results.find(id);

    // FIXME: Not 64-bit size clean.
    if (it == results.end()) {
      results[id] = (unsigned) array.size;      
    } else {
      it->second = std::max(it->second, (unsigned) array.size);
    }
  }

public:
  std::map<unsigned, unsigned> results;
};

// XXX waste of space, rather have ByteValueRange
typedef ValueRange CexValueData;

class CexObjectData {
public:
  unsigned size;
  CexValueData *values;

public:
  CexObjectData(unsigned _size) : size(_size), values(new CexValueData[size]) {
    for (unsigned i=0; i<size; i++)
      values[i] = ValueRange(0, 255);
  }
};

class CexRangeEvaluator : public ExprRangeEvaluator<ValueRange> {
public:
  std::map<unsigned, CexObjectData> &objectValues;
  CexRangeEvaluator(std::map<unsigned, CexObjectData> &_objectValues) 
    : objectValues(_objectValues) {}

  ValueRange getInitialReadRange(const Array &os, ValueRange index) {
    return ValueRange(0, 255);
  }
};

class CexConstifier : public ExprEvaluator {
protected:
  ref<Expr> getInitialValue(const Array& array, unsigned index) {
    std::map<unsigned, CexObjectData>::iterator it = 
      objectValues.find(array.id);
    assert(it != objectValues.end() && "missing object?");
    CexObjectData &cod = it->second;
    
    if (index >= cod.size) {
      return ReadExpr::create(UpdateList(&array, true, 0), 
                              ConstantExpr::alloc(index, Expr::Int32));
    } else {
      CexValueData &cvd = cod.values[index];
      assert(cvd.min() == cvd.max() && "value is not fixed");
      return ConstantExpr::alloc(cvd.min(), Expr::Int8);
    }
  }

public:
  std::map<unsigned, CexObjectData> &objectValues;
  CexConstifier(std::map<unsigned, CexObjectData> &_objectValues) 
    : objectValues(_objectValues) {}
};

class CexData {
public:
  std::map<unsigned, CexObjectData> objectValues;

public:
  CexData(ObjectFinder &finder) {
    for (std::map<unsigned,unsigned>::iterator it = finder.results.begin(),
           ie = finder.results.end(); it != ie; ++it) {
      objectValues.insert(std::pair<unsigned, CexObjectData>(it->first, 
                                                             CexObjectData(it->second)));
    }
  }  
  ~CexData() {
    for (std::map<unsigned, CexObjectData>::iterator it = objectValues.begin(),
           ie = objectValues.end(); it != ie; ++it)
      delete[] it->second.values;
  }

  void forceExprToValue(ref<Expr> e, uint64_t value) {
    forceExprToRange(e, CexValueData(value,value));
  }

  void forceExprToRange(ref<Expr> e, CexValueData range) {
#ifdef LOG
    //    *theLog << "force: " << e << " to " << range << "\n";
#endif
    switch (e->getKind()) {
    case Expr::Constant: {
      // rather a pity if the constant isn't in the range, but how can
      // we use this?
      break;
    }

      // Special

    case Expr::NotOptimized: break;

    case Expr::Read: {
      ReadExpr *re = cast<ReadExpr>(e);
      const Array *array = re->updates.root;
      CexObjectData &cod = objectValues.find(array->id)->second;

      // XXX we need to respect the version here and object state chain

      if (ConstantExpr *CE = dyn_cast<ConstantExpr>(re->index)) {
        if (CE->getConstantValue() < array->size) {
          CexValueData &cvd = cod.values[CE->getConstantValue()];
          CexValueData tmp = cvd.set_intersection(range);
        
          if (tmp.isEmpty()) {
            if (range.isFixed()) // ranges conflict, if new one is fixed use that
              cvd = range;
          } else {
            cvd = tmp;
          }
        }
      } else {
        // XXX        fatal("XXX not implemented");
      }
      
      break;
    }

    case Expr::Select: {
      SelectExpr *se = cast<SelectExpr>(e);
      ValueRange cond = evalRangeForExpr(se->cond);
      if (cond.isFixed()) {
        if (cond.min()) {
          forceExprToRange(se->trueExpr, range);
        } else {
          forceExprToRange(se->falseExpr, range);
        }
      } else {
        // XXX imprecise... we have a choice here. One method is to
        // simply force both sides into the specified range (since the
        // condition is indetermined). This may lose in two ways, the
        // first is that the condition chosen may limit further
        // restrict the range in each of the children, however this is
        // less of a problem as the range will be a superset of legal
        // values. The other is if the condition ends up being forced
        // by some other constraints, then we needlessly forced one
        // side into the given range.
        //
        // The other method would be to force the condition to one
        // side and force that side into the given range. This loses
        // when we force the condition to an unsatisfiable value
        // (either because the condition cannot be that, or the
        // resulting range given that condition is not in the required
        // range).
        // 
        // Currently we just force both into the range. A hybrid would
        // be to evaluate the ranges for each of the children... if
        // one of the ranges happens to already be a subset of the
        // required range then it may be preferable to force the
        // condition to that side.
        forceExprToRange(se->trueExpr, range);
        forceExprToRange(se->falseExpr, range);
      }
      break;
    }

      // XXX imprecise... the problem here is that extracting bits
      // loses information about what bits are connected across the
      // bytes. if a value can be 1 or 256 then either the top or
      // lower byte is 0, but just extraction loses this information
      // and will allow neither,one,or both to be 1.
      //
      // we can protect against this in a limited fashion by writing
      // the extraction a byte at a time, then checking the evaluated
      // value, isolating for that range, and continuing.
    case Expr::Concat: {
      ConcatExpr *ce = cast<ConcatExpr>(e);
      if (ce->is2ByteConcat()) {
	forceExprToRange(ce->getKid(0), range.extract( 8, 16));
	forceExprToRange(ce->getKid(1), range.extract( 0,  8));
      }
      else if (ce->is4ByteConcat()) {
	forceExprToRange(ce->getKid(0), range.extract(24, 32));
	forceExprToRange(ce->getKid(1), range.extract(16, 24));
	forceExprToRange(ce->getKid(2), range.extract( 8, 16));
	forceExprToRange(ce->getKid(3), range.extract( 0,  8));
      }
      else if (ce->is8ByteConcat()) {
	forceExprToRange(ce->getKid(0), range.extract(56, 64));
	forceExprToRange(ce->getKid(1), range.extract(48, 56));
	forceExprToRange(ce->getKid(2), range.extract(40, 48));
	forceExprToRange(ce->getKid(3), range.extract(32, 40));
	forceExprToRange(ce->getKid(4), range.extract(24, 32));
	forceExprToRange(ce->getKid(5), range.extract(16, 24));
	forceExprToRange(ce->getKid(6), range.extract( 8, 16));
	forceExprToRange(ce->getKid(7), range.extract( 0,  8));
      }
      
      break;
    }

    case Expr::Extract: {
      // XXX
      break;
    }

      // Casting

      // Simply intersect the output range with the range of all
      // possible outputs and then truncate to the desired number of
      // bits. 

      // For ZExt this simplifies to just intersection with the
      // possible input range.
    case Expr::ZExt: {
      CastExpr *ce = cast<CastExpr>(e);
      unsigned inBits = ce->src->getWidth();
      ValueRange input = range.set_intersection(ValueRange(0, bits64::maxValueOfNBits(inBits)));
      forceExprToRange(ce->src, input);
      break;
    }
      // For SExt instead of doing the intersection we just take the output range
      // minus the impossible values. This is nicer since it is a single interval.
    case Expr::SExt: {
      CastExpr *ce = cast<CastExpr>(e);
      unsigned inBits = ce->src->getWidth();
      unsigned outBits = ce->width;
      ValueRange output = range.set_difference(ValueRange(1<<(inBits-1),
                                                          (bits64::maxValueOfNBits(outBits)-
                                                           bits64::maxValueOfNBits(inBits-1)-1)));
      ValueRange input = output.binaryAnd(bits64::maxValueOfNBits(inBits));
      forceExprToRange(ce->src, input);
      break;
    }

      // Binary

    case Expr::And: {
      BinaryExpr *be = cast<BinaryExpr>(e);
      if (be->getWidth()==Expr::Bool) {
        if (range.isFixed()) {
          ValueRange left = evalRangeForExpr(be->left);
          ValueRange right = evalRangeForExpr(be->right);

          if (!range.min()) {
            if (left.mustEqual(0) || right.mustEqual(0)) {
              // all is well
            } else {
              // XXX heuristic, which order

              forceExprToValue(be->left, 0);
              left = evalRangeForExpr(be->left);

              // see if that worked
              if (!left.mustEqual(1))
                forceExprToValue(be->right, 0);
            }
          } else {
            if (!left.mustEqual(1)) forceExprToValue(be->left, 1);
            if (!right.mustEqual(1)) forceExprToValue(be->right, 1);
          }
        }
      } else {
        // XXX
      }
      break;
    }

    case Expr::Or: {
      BinaryExpr *be = cast<BinaryExpr>(e);
      if (be->getWidth()==Expr::Bool) {
        if (range.isFixed()) {
          ValueRange left = evalRangeForExpr(be->left);
          ValueRange right = evalRangeForExpr(be->right);

          if (range.min()) {
            if (left.mustEqual(1) || right.mustEqual(1)) {
              // all is well
            } else {
              // XXX heuristic, which order?
              
              // force left to value we need
              forceExprToValue(be->left, 1);
              left = evalRangeForExpr(be->left);

              // see if that worked
              if (!left.mustEqual(1))
                forceExprToValue(be->right, 1);
            }
          } else {
            if (!left.mustEqual(0)) forceExprToValue(be->left, 0);
            if (!right.mustEqual(0)) forceExprToValue(be->right, 0);
          }
        }
      } else {
        // XXX
      }
      break;
    }

    case Expr::Xor: break;

      // Comparison

    case Expr::Eq: {
      BinaryExpr *be = cast<BinaryExpr>(e);
      if (range.isFixed()) {
        if (ConstantExpr *CE = dyn_cast<ConstantExpr>(be->left)) {
          uint64_t value = CE->getConstantValue();
          if (range.min()) {
            forceExprToValue(be->right, value);
          } else {
            if (value==0) {
              forceExprToRange(be->right, 
                               CexValueData(1,
                                            ints::sext(1, 
                                                       be->right->getWidth(),
                                                       1)));
            } else {
              // XXX heuristic / lossy, could be better to pick larger range?
              forceExprToRange(be->right, CexValueData(0, value-1));
            }
          }
        } else {
          // XXX what now
        }
      }
      break;
    }

    case Expr::Ult: {
      BinaryExpr *be = cast<BinaryExpr>(e);
      
      // XXX heuristic / lossy, what order if conflict

      if (range.isFixed()) {
        ValueRange left = evalRangeForExpr(be->left);
        ValueRange right = evalRangeForExpr(be->right);

        uint64_t maxValue = bits64::maxValueOfNBits(be->right->getWidth());

        // XXX should deal with overflow (can lead to empty range)

        if (left.isFixed()) {
          if (range.min()) {
            forceExprToRange(be->right, CexValueData(left.min()+1, maxValue));
          } else {
            forceExprToRange(be->right, CexValueData(0, left.min()));
          }
        } else if (right.isFixed()) {
          if (range.min()) {
            forceExprToRange(be->left, CexValueData(0, right.min()-1));
          } else {
            forceExprToRange(be->left, CexValueData(right.min(), maxValue));
          }
        } else {
          // XXX ???
        }
      }
      break;
    }
    case Expr::Ule: {
      BinaryExpr *be = cast<BinaryExpr>(e);
      
      // XXX heuristic / lossy, what order if conflict

      if (range.isFixed()) {
        ValueRange left = evalRangeForExpr(be->left);
        ValueRange right = evalRangeForExpr(be->right);

        // XXX should deal with overflow (can lead to empty range)

        uint64_t maxValue = bits64::maxValueOfNBits(be->right->getWidth());
        if (left.isFixed()) {
          if (range.min()) {
            forceExprToRange(be->right, CexValueData(left.min(), maxValue));
          } else {
            forceExprToRange(be->right, CexValueData(0, left.min()-1));
          }
        } else if (right.isFixed()) {
          if (range.min()) {
            forceExprToRange(be->left, CexValueData(0, right.min()));
          } else {
            forceExprToRange(be->left, CexValueData(right.min()+1, maxValue));
          }
        } else {
          // XXX ???
        }
      }
      break;
    }

    case Expr::Ne:
    case Expr::Ugt:
    case Expr::Uge:
    case Expr::Sgt:
    case Expr::Sge:
      assert(0 && "invalid expressions (uncanonicalized");

    default:
      break;
    }
  }

  void fixValues() {
    for (std::map<unsigned, CexObjectData>::iterator it = objectValues.begin(),
           ie = objectValues.end(); it != ie; ++it) {
      CexObjectData &cod = it->second;
      for (unsigned i=0; i<cod.size; i++) {
        CexValueData &cvd = cod.values[i];
        cvd = CexValueData(cvd.min() + (cvd.max()-cvd.min())/2);
      }
    }
  }

  ValueRange evalRangeForExpr(ref<Expr> &e) {
    CexRangeEvaluator ce(objectValues);
    return ce.evaluate(e);
  }

  bool exprMustBeValue(ref<Expr> e, uint64_t value) {
    CexConstifier cc(objectValues);
    ref<Expr> v = cc.visit(e);
    if (!isa<ConstantExpr>(v)) return false;
    // XXX reenable once all reads and vars are fixed
    //    assert(v.isConstant() && "not all values have been fixed");
    return v->getConstantValue() == value;
  }
};

/* *** */


class FastCexSolver : public IncompleteSolver {
public:
  FastCexSolver();
  ~FastCexSolver();

  IncompleteSolver::PartialValidity computeTruth(const Query&);  
  bool computeValue(const Query&, ref<Expr> &result);
  bool computeInitialValues(const Query&,
                            const std::vector<const Array*> &objects,
                            std::vector< std::vector<unsigned char> > &values,
                            bool &hasSolution);
};

FastCexSolver::FastCexSolver() { }

FastCexSolver::~FastCexSolver() { }

IncompleteSolver::PartialValidity 
FastCexSolver::computeTruth(const Query& query) {
#ifdef LOG
  std::ostringstream log;
  theLog = &log;
  //  log << "------ start FastCexSolver::mustBeTrue ------\n";
  log << "-- QUERY --\n";
  unsigned i=0;
  for (ConstraintManager::const_iterator it = query.constraints.begin(), 
         ie = query.constraints.end(); it != ie; ++it)
    log << "    C" << i++ << ": " << *it << ", \n";
  log << "    Q : " << query.expr << "\n";
#endif

  ObjectFinder of;
  for (ConstraintManager::const_iterator it = query.constraints.begin(), 
         ie = query.constraints.end(); it != ie; ++it)
    of.visit(*it);
  of.visit(query.expr);
  CexData cd(of);

  for (ConstraintManager::const_iterator it = query.constraints.begin(), 
         ie = query.constraints.end(); it != ie; ++it)
    cd.forceExprToValue(*it, 1);
  cd.forceExprToValue(query.expr, 0);

#ifdef LOG
  log << " -- ranges --\n";
  for (std::map<unsigned, CexObjectData>::iterator it = objectValues.begin(),
         ie = objectValues.end(); it != ie; ++it) {
    CexObjectData &cod = it->second;
    log << "    arr" << it->first << "[" << cod.size << "] = [";
    unsigned continueFrom=cod.size-1;
    for (; continueFrom>0; continueFrom--)
      if (cod.values[continueFrom-1]!=cod.values[continueFrom])
        break;
    for (unsigned i=0; i<cod.size; i++) {
      log << cod.values[i];
      if (i<cod.size-1) {
        log << ", ";
        if (i==continueFrom) {
          log << "...";
          break;
        }
      }
    }
    log << "]\n";
  }
#endif

  // this could be done lazily of course
  cd.fixValues();

#ifdef LOG
  log << " -- fixed values --\n";
  for (std::map<unsigned, CexObjectData>::iterator it = objectValues.begin(),
         ie = objectValues.end(); it != ie; ++it) {
    CexObjectData &cod = it->second;
    log << "    arr" << it->first << "[" << cod.size << "] = [";
    unsigned continueFrom=cod.size-1;
    for (; continueFrom>0; continueFrom--)
      if (cod.values[continueFrom-1]!=cod.values[continueFrom])
        break;
    for (unsigned i=0; i<cod.size; i++) {
      log << cod.values[i];
      if (i<cod.size-1) {
        log << ", ";
        if (i==continueFrom) {
          log << "...";
          break;
        }
      }
    }
    log << "]\n";
  }
#endif

  // check the result

  bool isGood = true;

  if (!cd.exprMustBeValue(query.expr, 0)) isGood = false;

  for (ConstraintManager::const_iterator it = query.constraints.begin(), 
         ie = query.constraints.end(); it != ie; ++it)
    if (!cd.exprMustBeValue(*it, 1)) 
      isGood = false;

#ifdef LOG
  log << " -- evaluating result --\n";
  
  i=0;
  for (ConstraintManager::const_iterator it = query.constraints.begin(), 
         ie = query.constraints.end(); it != ie; ++it) {
    bool res = cd.exprMustBeValue(*it, 1);
    log << "    C" << i++ << ": " << (res?"true":"false") << "\n";
  }
  log << "    Q : " 
      << (cd.exprMustBeValue(query.expr, 0)?"true":"false") << "\n";
  
  log << "\n\n";
#endif

  return isGood ? IncompleteSolver::MayBeFalse : IncompleteSolver::None;
}

bool FastCexSolver::computeValue(const Query& query, ref<Expr> &result) {
  ObjectFinder of;
  for (ConstraintManager::const_iterator it = query.constraints.begin(), 
         ie = query.constraints.end(); it != ie; ++it)
    of.visit(*it);
  of.visit(query.expr);
  CexData cd(of);

  for (ConstraintManager::const_iterator it = query.constraints.begin(), 
         ie = query.constraints.end(); it != ie; ++it)
    cd.forceExprToValue(*it, 1);

  // this could be done lazily of course
  cd.fixValues();

  // check the result
  for (ConstraintManager::const_iterator it = query.constraints.begin(), 
         ie = query.constraints.end(); it != ie; ++it)
    if (!cd.exprMustBeValue(*it, 1))
      return false;

  CexConstifier cc(cd.objectValues);
  ref<Expr> value = cc.visit(query.expr);

  if (isa<ConstantExpr>(value)) {
    result = value;
    return true;
  } else {
    return false;
  }
}

bool
FastCexSolver::computeInitialValues(const Query& query,
                                    const std::vector<const Array*>
                                      &objects,
                                    std::vector< std::vector<unsigned char> >
                                      &values,
                                    bool &hasSolution) {
  ObjectFinder of;
  for (ConstraintManager::const_iterator it = query.constraints.begin(), 
         ie = query.constraints.end(); it != ie; ++it)
    of.visit(*it);
  of.visit(query.expr);
  for (unsigned i = 0; i != objects.size(); ++i)
    of.addObject(*objects[i]);
  CexData cd(of);

  for (ConstraintManager::const_iterator it = query.constraints.begin(), 
         ie = query.constraints.end(); it != ie; ++it)
    cd.forceExprToValue(*it, 1);
  cd.forceExprToValue(query.expr, 0);

  // this could be done lazily of course
  cd.fixValues();

  // check the result
  for (ConstraintManager::const_iterator it = query.constraints.begin(), 
         ie = query.constraints.end(); it != ie; ++it)
    if (!cd.exprMustBeValue(*it, 1))
      return false;
  if (!cd.exprMustBeValue(query.expr, 0))
    return false;

  hasSolution = true;
  CexConstifier cc(cd.objectValues);
  for (unsigned i = 0; i != objects.size(); ++i) {
    const Array *array = objects[i];
    std::vector<unsigned char> data;
    data.reserve(array->size);

    for (unsigned i=0; i < array->size; i++) {
      ref<Expr> value =
        cc.visit(ReadExpr::create(UpdateList(array, true, 0),
                                  ConstantExpr::create(i,
                                                       kMachinePointerType)));
      
      if (ConstantExpr *CE = dyn_cast<ConstantExpr>(value)) {
        data.push_back(CE->getConstantValue());
      } else {
        // FIXME: When does this happen?
        return false;
      }
    }

    values.push_back(data);
  }

  return true;
}


Solver *klee::createFastCexSolver(Solver *s) {
  return new Solver(new StagedSolverImpl(new FastCexSolver(), s));
}
