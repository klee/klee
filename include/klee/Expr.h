//===-- Expr.h --------------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_EXPR_H
#define KLEE_EXPR_H

#include "klee/util/Bits.h"
#include "klee/util/Ref.h"

#include "llvm/ADT/APInt.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SmallVector.h"

#include <set>
#include <vector>
#include <iosfwd> // FIXME: Remove this!!!

namespace llvm {
  class Type;
}

namespace klee {

class Array;
class ConstantExpr;
class ObjectState;

template<class T> class ref;


/// Class representing symbolic expressions.
/**

<b>Expression canonicalization</b>: we define certain rules for
canonicalization rules for Exprs in order to simplify code that
pattern matches Exprs (since the number of forms are reduced), to open
up further chances for optimization, and to increase similarity for
caching and other purposes.

The general rules are:
<ol>
<li> No Expr has all constant arguments.</li>

<li> Booleans:
    <ol type="a">
     <li> \c Ne, \c Ugt, \c Uge, \c Sgt, \c Sge are not used </li>
     <li> The only acceptable operations with boolean arguments are 
          \c Not \c And, \c Or, \c Xor, \c Eq, 
	  as well as \c SExt, \c ZExt,
          \c Select and \c NotOptimized. </li>
     <li> The only boolean operation which may involve a constant is boolean not (<tt>== false</tt>). </li>
     </ol>
</li>

<li> Linear Formulas: 
   <ol type="a">
   <li> For any subtree representing a linear formula, a constant
   term must be on the LHS of the root node of the subtree.  In particular, 
   in a BinaryExpr a constant must always be on the LHS.  For example, subtraction 
   by a constant c is written as <tt>add(-c, ?)</tt>.  </li>
    </ol>
</li>


<li> Chains are unbalanced to the right </li>

</ol>


<b>Steps required for adding an expr</b>:
   -# Add case to printKind
   -# Add to ExprVisitor
   -# Add to IVC (implied value concretization) if possible

Todo: Shouldn't bool \c Xor just be written as not equal?

*/

class Expr {
public:
  static unsigned count;
  static const unsigned MAGIC_HASH_CONSTANT = 39;

  /// The type of an expression is simply its width, in bits. 
  typedef unsigned Width; 
  
  static const Width InvalidWidth = 0;
  static const Width Bool = 1;
  static const Width Int8 = 8;
  static const Width Int16 = 16;
  static const Width Int32 = 32;
  static const Width Int64 = 64;
  static const Width Fl80 = 80;
  

  enum Kind {
    InvalidKind = -1,

    // Primitive

    Constant = 0,

    // Special

    /// Prevents optimization below the given expression.  Used for
    /// testing: make equality constraints that KLEE will not use to
    /// optimize to concretes.
    NotOptimized,

    //// Skip old varexpr, just for deserialization, purge at some point
    Read=NotOptimized+2, 
    Select,
    Concat,
    Extract,

    // Casting,
    ZExt,
    SExt,

    // All subsequent kinds are binary.

    // Arithmetic
    Add,
    Sub,
    Mul,
    UDiv,
    SDiv,
    URem,
    SRem,

    // Bit
    Not,
    And,
    Or,
    Xor,
    Shl,
    LShr,
    AShr,
    
    // Compare
    Eq,
    Ne,  ///< Not used in canonical form
    Ult,
    Ule,
    Ugt, ///< Not used in canonical form
    Uge, ///< Not used in canonical form
    Slt,
    Sle,
    Sgt, ///< Not used in canonical form
    Sge, ///< Not used in canonical form

    LastKind=Sge,

    CastKindFirst=ZExt,
    CastKindLast=SExt,
    BinaryKindFirst=Add,
    BinaryKindLast=Sge,
    CmpKindFirst=Eq,
    CmpKindLast=Sge
  };

  unsigned refCount;

protected:  
  unsigned hashValue;
  
public:
  Expr() : refCount(0) { Expr::count++; }
  virtual ~Expr() { Expr::count--; } 

  virtual Kind getKind() const = 0;
  virtual Width getWidth() const = 0;
  
  virtual unsigned getNumKids() const = 0;
  virtual ref<Expr> getKid(unsigned i) const = 0;
    
  virtual void print(std::ostream &os) const;

  /// dump - Print the expression to stderr.
  void dump() const;

  /// Returns the pre-computed hash of the current expression
  virtual unsigned hash() const { return hashValue; }

  /// (Re)computes the hash of the current expression.
  /// Returns the hash value. 
  virtual unsigned computeHash();
  
  /// Returns 0 iff b is structuraly equivalent to *this
  typedef llvm::DenseSet<std::pair<const Expr *, const Expr *> > ExprEquivSet;
  int compare(const Expr &b, ExprEquivSet &equivs) const;
  int compare(const Expr &b) const {
    ExprEquivSet equivs;
    return compare(b, equivs);
  }
  virtual int compareContents(const Expr &b) const { return 0; }

  // Given an array of new kids return a copy of the expression
  // but using those children. 
  virtual ref<Expr> rebuild(ref<Expr> kids[/* getNumKids() */]) const = 0;

  //

  /// isZero - Is this a constant zero.
  bool isZero() const;
  
  /// isTrue - Is this the true expression.
  bool isTrue() const;

  /// isFalse - Is this the false expression.
  bool isFalse() const;

  /* Static utility methods */

  static void printKind(std::ostream &os, Kind k);
  static void printWidth(std::ostream &os, Expr::Width w);

  /// returns the smallest number of bytes in which the given width fits
  static inline unsigned getMinBytesForWidth(Width w) {
      return (w + 7) / 8;
  }

  /* Kind utilities */

  /* Utility creation functions */
  static ref<Expr> createSExtToPointerWidth(ref<Expr> e);
  static ref<Expr> createZExtToPointerWidth(ref<Expr> e);
  static ref<Expr> createImplies(ref<Expr> hyp, ref<Expr> conc);
  static ref<Expr> createIsZero(ref<Expr> e);

  /// Create a little endian read of the given type at offset 0 of the
  /// given object.
  static ref<Expr> createTempRead(const Array *array, Expr::Width w);
  
  static ref<ConstantExpr> createPointer(uint64_t v);

  struct CreateArg;
  static ref<Expr> createFromKind(Kind k, std::vector<CreateArg> args);

  static bool isValidKidWidth(unsigned kid, Width w) { return true; }
  static bool needsResultType() { return false; }

  static bool classof(const Expr *) { return true; }
};

struct Expr::CreateArg {
  ref<Expr> expr;
  Width width;
  
  CreateArg(Width w = Bool) : expr(0), width(w) {}
  CreateArg(ref<Expr> e) : expr(e), width(Expr::InvalidWidth) {}
  
  bool isExpr() { return !isWidth(); }
  bool isWidth() { return width != Expr::InvalidWidth; }
};

// Comparison operators

inline bool operator==(const Expr &lhs, const Expr &rhs) {
  return lhs.compare(rhs) == 0;
}

inline bool operator<(const Expr &lhs, const Expr &rhs) {
  return lhs.compare(rhs) < 0;
}

inline bool operator!=(const Expr &lhs, const Expr &rhs) {
  return !(lhs == rhs);
}

inline bool operator>(const Expr &lhs, const Expr &rhs) {
  return rhs < lhs;
}

inline bool operator<=(const Expr &lhs, const Expr &rhs) {
  return !(lhs > rhs);
}

inline bool operator>=(const Expr &lhs, const Expr &rhs) {
  return !(lhs < rhs);
}

// Printing operators

inline std::ostream &operator<<(std::ostream &os, const Expr &e) {
  e.print(os);
  return os;
}

inline std::ostream &operator<<(std::ostream &os, const Expr::Kind kind) {
  Expr::printKind(os, kind);
  return os;
}

// Terminal Exprs

class ConstantExpr : public Expr {
public:
  static const Kind kind = Constant;
  static const unsigned numKids = 0;

private:
  llvm::APInt value;

  ConstantExpr(const llvm::APInt &v) : value(v) {}

public:
  ~ConstantExpr() {}
  
  Width getWidth() const { return value.getBitWidth(); }
  Kind getKind() const { return Constant; }

  unsigned getNumKids() const { return 0; }
  ref<Expr> getKid(unsigned i) const { return 0; }

  /// getAPValue - Return the arbitrary precision value directly.
  ///
  /// Clients should generally not use the APInt value directly and instead use
  /// native ConstantExpr APIs.
  const llvm::APInt &getAPValue() const { return value; }

  /// getZExtValue - Return the constant value for a limited number of bits.
  ///
  /// This routine should be used in situations where the width of the constant
  /// is known to be limited to a certain number of bits.
  uint64_t getZExtValue(unsigned bits = 64) const {
    assert(getWidth() <= bits && "Value may be out of range!");
    return value.getZExtValue();
  }

  /// getLimitedValue - If this value is smaller than the specified limit,
  /// return it, otherwise return the limit value.
  uint64_t getLimitedValue(uint64_t Limit = ~0ULL) const {
    return value.getLimitedValue(Limit);
  }

  /// toString - Return the constant value as a string
  /// \param Res specifies the string for the result to be placed in
  /// \param radix specifies the base (e.g. 2,10,16). The default is base 10
  void toString(std::string &Res, unsigned radix=10) const;


 
  int compareContents(const Expr &b) const { 
    const ConstantExpr &cb = static_cast<const ConstantExpr&>(b);
    if (getWidth() != cb.getWidth()) 
      return getWidth() < cb.getWidth() ? -1 : 1;
    if (value == cb.value)
      return 0;
    return value.ult(cb.value) ? -1 : 1;
  }

  virtual ref<Expr> rebuild(ref<Expr> kids[]) const { 
    assert(0 && "rebuild() on ConstantExpr"); 
    return (Expr*) this;
  }

  virtual unsigned computeHash();
  
  static ref<Expr> fromMemory(void *address, Width w);
  void toMemory(void *address);

  static ref<ConstantExpr> alloc(const llvm::APInt &v) {
    ref<ConstantExpr> r(new ConstantExpr(v));
    r->computeHash();
    return r;
  }

  static ref<ConstantExpr> alloc(const llvm::APFloat &f) {
    return alloc(f.bitcastToAPInt());
  }

  static ref<ConstantExpr> alloc(uint64_t v, Width w) {
    return alloc(llvm::APInt(w, v));
  }
  
  static ref<ConstantExpr> create(uint64_t v, Width w) {
    assert(v == bits64::truncateToNBits(v, w) &&
           "invalid constant");
    return alloc(v, w);
  }

  static bool classof(const Expr *E) {
    return E->getKind() == Expr::Constant;
  }
  static bool classof(const ConstantExpr *) { return true; }

  /* Utility Functions */

  /// isZero - Is this a constant zero.
  bool isZero() const { return getAPValue().isMinValue(); }

  /// isOne - Is this a constant one.
  bool isOne() const { return getLimitedValue() == 1; }
  
  /// isTrue - Is this the true expression.
  bool isTrue() const { 
    return (getWidth() == Expr::Bool && value.getBoolValue()==true);
  }

  /// isFalse - Is this the false expression.
  bool isFalse() const {
    return (getWidth() == Expr::Bool && value.getBoolValue()==false);
  }

  /// isAllOnes - Is this constant all ones.
  bool isAllOnes() const { return getAPValue().isAllOnesValue(); }

  /* Constant Operations */

  ref<ConstantExpr> Concat(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> Extract(unsigned offset, Width W);
  ref<ConstantExpr> ZExt(Width W);
  ref<ConstantExpr> SExt(Width W);
  ref<ConstantExpr> Add(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> Sub(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> Mul(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> UDiv(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> SDiv(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> URem(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> SRem(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> And(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> Or(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> Xor(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> Shl(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> LShr(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> AShr(const ref<ConstantExpr> &RHS);

  // Comparisons return a constant expression of width 1.

  ref<ConstantExpr> Eq(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> Ne(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> Ult(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> Ule(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> Ugt(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> Uge(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> Slt(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> Sle(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> Sgt(const ref<ConstantExpr> &RHS);
  ref<ConstantExpr> Sge(const ref<ConstantExpr> &RHS);

  ref<ConstantExpr> Neg();
  ref<ConstantExpr> Not();
};

  
// Utility classes

class NonConstantExpr : public Expr {
public:
  static bool classof(const Expr *E) {
    return E->getKind() != Expr::Constant;
  }
  static bool classof(const NonConstantExpr *) { return true; }
};

class BinaryExpr : public NonConstantExpr {
public:
  ref<Expr> left, right;

public:
  unsigned getNumKids() const { return 2; }
  ref<Expr> getKid(unsigned i) const { 
    if(i == 0)
      return left;
    if(i == 1)
      return right;
    return 0;
  }
 
protected:
  BinaryExpr(const ref<Expr> &l, const ref<Expr> &r) : left(l), right(r) {}

public:
  static bool classof(const Expr *E) {
    Kind k = E->getKind();
    return Expr::BinaryKindFirst <= k && k <= Expr::BinaryKindLast;
  }
  static bool classof(const BinaryExpr *) { return true; }
};


class CmpExpr : public BinaryExpr {

protected:
  CmpExpr(ref<Expr> l, ref<Expr> r) : BinaryExpr(l,r) {}
  
public:                                                       
  Width getWidth() const { return Bool; }

  static bool classof(const Expr *E) {
    Kind k = E->getKind();
    return Expr::CmpKindFirst <= k && k <= Expr::CmpKindLast;
  }
  static bool classof(const CmpExpr *) { return true; }
};

// Special

class NotOptimizedExpr : public NonConstantExpr {
public:
  static const Kind kind = NotOptimized;
  static const unsigned numKids = 1;
  ref<Expr> src;

  static ref<Expr> alloc(const ref<Expr> &src) {
    ref<Expr> r(new NotOptimizedExpr(src));
    r->computeHash();
    return r;
  }
  
  static ref<Expr> create(ref<Expr> src);
  
  Width getWidth() const { return src->getWidth(); }
  Kind getKind() const { return NotOptimized; }

  unsigned getNumKids() const { return 1; }
  ref<Expr> getKid(unsigned i) const { return src; }

  virtual ref<Expr> rebuild(ref<Expr> kids[]) const { return create(kids[0]); }

private:
  NotOptimizedExpr(const ref<Expr> &_src) : src(_src) {}

public:
  static bool classof(const Expr *E) {
    return E->getKind() == Expr::NotOptimized;
  }
  static bool classof(const NotOptimizedExpr *) { return true; }
};


/// Class representing a byte update of an array.
class UpdateNode {
  friend class UpdateList;  

  mutable unsigned refCount;
  // cache instead of recalc
  unsigned hashValue;

public:
  const UpdateNode *next;
  ref<Expr> index, value;
  
private:
  /// size of this update sequence, including this update
  unsigned size;
  
public:
  UpdateNode(const UpdateNode *_next, 
             const ref<Expr> &_index, 
             const ref<Expr> &_value);

  unsigned getSize() const { return size; }

  int compare(const UpdateNode &b) const;  
  unsigned hash() const { return hashValue; }

private:
  UpdateNode() : refCount(0) {}
  ~UpdateNode();

  unsigned computeHash();
};

class Array {
public:
  const std::string name;
  // FIXME: Not 64-bit clean.
  unsigned size;  

  /// constantValues - The constant initial values for this array, or empty for
  /// a symbolic array. If non-empty, this size of this array is equivalent to
  /// the array size.
  const std::vector< ref<ConstantExpr> > constantValues;
  
public:
  /// Array - Construct a new array object.
  ///
  /// \param _name - The name for this array. Names should generally be unique
  /// across an application, but this is not necessary for correctness, except
  /// when printing expressions. When expressions are printed the output will
  /// not parse correctly since two arrays with the same name cannot be
  /// distinguished once printed.
  Array(const std::string &_name, uint64_t _size, 
        const ref<ConstantExpr> *constantValuesBegin = 0,
        const ref<ConstantExpr> *constantValuesEnd = 0)
    : name(_name), size(_size), 
      constantValues(constantValuesBegin, constantValuesEnd) {      
    assert((isSymbolicArray() || constantValues.size() == size) &&
           "Invalid size for constant array!");
    computeHash();
#ifndef NDEBUG
    for (const ref<ConstantExpr> *it = constantValuesBegin;
         it != constantValuesEnd; ++it)
      assert((*it)->getWidth() == getRange() &&
             "Invalid initial constant value!");
#endif
  }
  ~Array();

  bool isSymbolicArray() const { return constantValues.empty(); }
  bool isConstantArray() const { return !isSymbolicArray(); }

  Expr::Width getDomain() const { return Expr::Int32; }
  Expr::Width getRange() const { return Expr::Int8; }
  
  unsigned computeHash();
  unsigned hash() const { return hashValue; }
   
private:
  unsigned hashValue;
};

/// Class representing a complete list of updates into an array.
class UpdateList { 
  friend class ReadExpr; // for default constructor

public:
  const Array *root;
  
  /// pointer to the most recent update node
  const UpdateNode *head;
  
public:
  UpdateList(const Array *_root, const UpdateNode *_head);
  UpdateList(const UpdateList &b);
  ~UpdateList();
  
  UpdateList &operator=(const UpdateList &b);

  /// size of this update list
  unsigned getSize() const { return (head ? head->getSize() : 0); }
  
  void extend(const ref<Expr> &index, const ref<Expr> &value);

  int compare(const UpdateList &b) const;
  unsigned hash() const;
};

/// Class representing a one byte read from an array. 
class ReadExpr : public NonConstantExpr {
public:
  static const Kind kind = Read;
  static const unsigned numKids = 1;
  
public:
  UpdateList updates;
  ref<Expr> index;

public:
  static ref<Expr> alloc(const UpdateList &updates, const ref<Expr> &index) {
    ref<Expr> r(new ReadExpr(updates, index));
    r->computeHash();
    return r;
  }
  
  static ref<Expr> create(const UpdateList &updates, ref<Expr> i);
  
  Width getWidth() const { return Expr::Int8; }
  Kind getKind() const { return Read; }
  
  unsigned getNumKids() const { return numKids; }
  ref<Expr> getKid(unsigned i) const { return !i ? index : 0; }  
  
  int compareContents(const Expr &b) const;

  virtual ref<Expr> rebuild(ref<Expr> kids[]) const { 
    return create(updates, kids[0]);
  }

  virtual unsigned computeHash();

private:
  ReadExpr(const UpdateList &_updates, const ref<Expr> &_index) : 
    updates(_updates), index(_index) {}

public:
  static bool classof(const Expr *E) {
    return E->getKind() == Expr::Read;
  }
  static bool classof(const ReadExpr *) { return true; }
};


/// Class representing an if-then-else expression.
class SelectExpr : public NonConstantExpr {
public:
  static const Kind kind = Select;
  static const unsigned numKids = 3;
  
public:
  ref<Expr> cond, trueExpr, falseExpr;

public:
  static ref<Expr> alloc(const ref<Expr> &c, const ref<Expr> &t, 
                         const ref<Expr> &f) {
    ref<Expr> r(new SelectExpr(c, t, f));
    r->computeHash();
    return r;
  }
  
  static ref<Expr> create(ref<Expr> c, ref<Expr> t, ref<Expr> f);

  Width getWidth() const { return trueExpr->getWidth(); }
  Kind getKind() const { return Select; }

  unsigned getNumKids() const { return numKids; }
  ref<Expr> getKid(unsigned i) const { 
        switch(i) {
        case 0: return cond;
        case 1: return trueExpr;
        case 2: return falseExpr;
        default: return 0;
        }
   }

  static bool isValidKidWidth(unsigned kid, Width w) {
    if (kid == 0)
      return w == Bool;
    else
      return true;
  }
    
  virtual ref<Expr> rebuild(ref<Expr> kids[]) const { 
    return create(kids[0], kids[1], kids[2]);
  }

private:
  SelectExpr(const ref<Expr> &c, const ref<Expr> &t, const ref<Expr> &f) 
    : cond(c), trueExpr(t), falseExpr(f) {}

public:
  static bool classof(const Expr *E) {
    return E->getKind() == Expr::Select;
  }
  static bool classof(const SelectExpr *) { return true; }
};


/** Children of a concat expression can have arbitrary widths.  
    Kid 0 is the left kid, kid 1 is the right kid.
*/
class ConcatExpr : public NonConstantExpr { 
public: 
  static const Kind kind = Concat;
  static const unsigned numKids = 2;

private:
  Width width;
  ref<Expr> left, right;  

public:
  static ref<Expr> alloc(const ref<Expr> &l, const ref<Expr> &r) {
    ref<Expr> c(new ConcatExpr(l, r));
    c->computeHash();
    return c;
  }
  
  static ref<Expr> create(const ref<Expr> &l, const ref<Expr> &r);

  Width getWidth() const { return width; }
  Kind getKind() const { return kind; }
  ref<Expr> getLeft() const { return left; }
  ref<Expr> getRight() const { return right; }

  unsigned getNumKids() const { return numKids; }
  ref<Expr> getKid(unsigned i) const { 
    if (i == 0) return left; 
    else if (i == 1) return right;
    else return NULL;
  }

  /// Shortcuts to create larger concats.  The chain returned is unbalanced to the right
  static ref<Expr> createN(unsigned nKids, const ref<Expr> kids[]);
  static ref<Expr> create4(const ref<Expr> &kid1, const ref<Expr> &kid2,
			   const ref<Expr> &kid3, const ref<Expr> &kid4);
  static ref<Expr> create8(const ref<Expr> &kid1, const ref<Expr> &kid2,
			   const ref<Expr> &kid3, const ref<Expr> &kid4,
			   const ref<Expr> &kid5, const ref<Expr> &kid6,
			   const ref<Expr> &kid7, const ref<Expr> &kid8);
  
  virtual ref<Expr> rebuild(ref<Expr> kids[]) const { return create(kids[0], kids[1]); }
  
private:
  ConcatExpr(const ref<Expr> &l, const ref<Expr> &r) : left(l), right(r) {
    width = l->getWidth() + r->getWidth();
  }

public:
  static bool classof(const Expr *E) {
    return E->getKind() == Expr::Concat;
  }
  static bool classof(const ConcatExpr *) { return true; }
};


/** This class represents an extract from expression {\tt expr}, at
    bit offset {\tt offset} of width {\tt width}.  Bit 0 is the right most 
    bit of the expression.
 */
class ExtractExpr : public NonConstantExpr { 
public:
  static const Kind kind = Extract;
  static const unsigned numKids = 1;
  
public:
  ref<Expr> expr;
  unsigned offset;
  Width width;

public:  
  static ref<Expr> alloc(const ref<Expr> &e, unsigned o, Width w) {
    ref<Expr> r(new ExtractExpr(e, o, w));
    r->computeHash();
    return r;
  }
  
  /// Creates an ExtractExpr with the given bit offset and width
  static ref<Expr> create(ref<Expr> e, unsigned bitOff, Width w);

  Width getWidth() const { return width; }
  Kind getKind() const { return Extract; }

  unsigned getNumKids() const { return numKids; }
  ref<Expr> getKid(unsigned i) const { return expr; }

  int compareContents(const Expr &b) const {
    const ExtractExpr &eb = static_cast<const ExtractExpr&>(b);
    if (offset != eb.offset) return offset < eb.offset ? -1 : 1;
    if (width != eb.width) return width < eb.width ? -1 : 1;
    return 0;
  }

  virtual ref<Expr> rebuild(ref<Expr> kids[]) const { 
    return create(kids[0], offset, width);
  }

  virtual unsigned computeHash();

private:
  ExtractExpr(const ref<Expr> &e, unsigned b, Width w) 
    : expr(e),offset(b),width(w) {}

public:
  static bool classof(const Expr *E) {
    return E->getKind() == Expr::Extract;
  }
  static bool classof(const ExtractExpr *) { return true; }
};


/** 
    Bitwise Not 
*/
class NotExpr : public NonConstantExpr { 
public:
  static const Kind kind = Not;
  static const unsigned numKids = 1;
  
  ref<Expr> expr;

public:  
  static ref<Expr> alloc(const ref<Expr> &e) {
    ref<Expr> r(new NotExpr(e));
    r->computeHash();
    return r;
  }
  
  static ref<Expr> create(const ref<Expr> &e);

  Width getWidth() const { return expr->getWidth(); }
  Kind getKind() const { return Not; }

  unsigned getNumKids() const { return numKids; }
  ref<Expr> getKid(unsigned i) const { return expr; }

  int compareContents(const Expr &b) const {
    const NotExpr &eb = static_cast<const NotExpr&>(b);
    if (expr != eb.expr) return expr < eb.expr ? -1 : 1;
    return 0;
  }

  virtual ref<Expr> rebuild(ref<Expr> kids[]) const { 
    return create(kids[0]);
  }

  virtual unsigned computeHash();

public:
  static bool classof(const Expr *E) {
    return E->getKind() == Expr::Not;
  }
  static bool classof(const NotExpr *) { return true; }

private:
  NotExpr(const ref<Expr> &e) : expr(e) {}
};



// Casting

class CastExpr : public NonConstantExpr {
public:
  ref<Expr> src;
  Width width;

public:
  CastExpr(const ref<Expr> &e, Width w) : src(e), width(w) {}

  Width getWidth() const { return width; }

  unsigned getNumKids() const { return 1; }
  ref<Expr> getKid(unsigned i) const { return (i==0) ? src : 0; }
  
  static bool needsResultType() { return true; }
  
  int compareContents(const Expr &b) const {
    const CastExpr &eb = static_cast<const CastExpr&>(b);
    if (width != eb.width) return width < eb.width ? -1 : 1;
    return 0;
  }

  virtual unsigned computeHash();

  static bool classof(const Expr *E) {
    Expr::Kind k = E->getKind();
    return Expr::CastKindFirst <= k && k <= Expr::CastKindLast;
  }
  static bool classof(const CastExpr *) { return true; }
};

#define CAST_EXPR_CLASS(_class_kind)                             \
class _class_kind ## Expr : public CastExpr {                    \
public:                                                          \
  static const Kind kind = _class_kind;                          \
  static const unsigned numKids = 1;                             \
public:                                                          \
    _class_kind ## Expr(ref<Expr> e, Width w) : CastExpr(e,w) {} \
    static ref<Expr> alloc(const ref<Expr> &e, Width w) {        \
      ref<Expr> r(new _class_kind ## Expr(e, w));                \
      r->computeHash();                                          \
      return r;                                                  \
    }                                                            \
    static ref<Expr> create(const ref<Expr> &e, Width w);        \
    Kind getKind() const { return _class_kind; }                 \
    virtual ref<Expr> rebuild(ref<Expr> kids[]) const {          \
      return create(kids[0], width);                             \
    }                                                            \
                                                                 \
    static bool classof(const Expr *E) {                         \
      return E->getKind() == Expr::_class_kind;                  \
    }                                                            \
    static bool classof(const  _class_kind ## Expr *) {          \
      return true;                                               \
    }                                                            \
};                                                               \

CAST_EXPR_CLASS(SExt)
CAST_EXPR_CLASS(ZExt)

// Arithmetic/Bit Exprs

#define ARITHMETIC_EXPR_CLASS(_class_kind)                           \
class _class_kind ## Expr : public BinaryExpr {                      \
public:                                                              \
  static const Kind kind = _class_kind;                              \
  static const unsigned numKids = 2;                                 \
public:                                                              \
    _class_kind ## Expr(const ref<Expr> &l,                          \
                        const ref<Expr> &r) : BinaryExpr(l,r) {}     \
    static ref<Expr> alloc(const ref<Expr> &l, const ref<Expr> &r) { \
      ref<Expr> res(new _class_kind ## Expr (l, r));                 \
      res->computeHash();                                            \
      return res;                                                    \
    }                                                                \
    static ref<Expr> create(const ref<Expr> &l, const ref<Expr> &r); \
    Width getWidth() const { return left->getWidth(); }              \
    Kind getKind() const { return _class_kind; }                     \
    virtual ref<Expr> rebuild(ref<Expr> kids[]) const {              \
      return create(kids[0], kids[1]);                               \
    }                                                                \
                                                                     \
    static bool classof(const Expr *E) {                             \
      return E->getKind() == Expr::_class_kind;                      \
    }                                                                \
    static bool classof(const  _class_kind ## Expr *) {              \
      return true;                                                   \
    }                                                                \
};                                                                   \

ARITHMETIC_EXPR_CLASS(Add)
ARITHMETIC_EXPR_CLASS(Sub)
ARITHMETIC_EXPR_CLASS(Mul)
ARITHMETIC_EXPR_CLASS(UDiv)
ARITHMETIC_EXPR_CLASS(SDiv)
ARITHMETIC_EXPR_CLASS(URem)
ARITHMETIC_EXPR_CLASS(SRem)
ARITHMETIC_EXPR_CLASS(And)
ARITHMETIC_EXPR_CLASS(Or)
ARITHMETIC_EXPR_CLASS(Xor)
ARITHMETIC_EXPR_CLASS(Shl)
ARITHMETIC_EXPR_CLASS(LShr)
ARITHMETIC_EXPR_CLASS(AShr)

// Comparison Exprs

#define COMPARISON_EXPR_CLASS(_class_kind)                           \
class _class_kind ## Expr : public CmpExpr {                         \
public:                                                              \
  static const Kind kind = _class_kind;                              \
  static const unsigned numKids = 2;                                 \
public:                                                              \
    _class_kind ## Expr(const ref<Expr> &l,                          \
                        const ref<Expr> &r) : CmpExpr(l,r) {}        \
    static ref<Expr> alloc(const ref<Expr> &l, const ref<Expr> &r) { \
      ref<Expr> res(new _class_kind ## Expr (l, r));                 \
      res->computeHash();                                            \
      return res;                                                    \
    }                                                                \
    static ref<Expr> create(const ref<Expr> &l, const ref<Expr> &r); \
    Kind getKind() const { return _class_kind; }                     \
    virtual ref<Expr> rebuild(ref<Expr> kids[]) const {              \
      return create(kids[0], kids[1]);                               \
    }                                                                \
                                                                     \
    static bool classof(const Expr *E) {                             \
      return E->getKind() == Expr::_class_kind;                      \
    }                                                                \
    static bool classof(const  _class_kind ## Expr *) {              \
      return true;                                                   \
    }                                                                \
};                                                                   \

COMPARISON_EXPR_CLASS(Eq)
COMPARISON_EXPR_CLASS(Ne)
COMPARISON_EXPR_CLASS(Ult)
COMPARISON_EXPR_CLASS(Ule)
COMPARISON_EXPR_CLASS(Ugt)
COMPARISON_EXPR_CLASS(Uge)
COMPARISON_EXPR_CLASS(Slt)
COMPARISON_EXPR_CLASS(Sle)
COMPARISON_EXPR_CLASS(Sgt)
COMPARISON_EXPR_CLASS(Sge)

// Implementations

inline bool Expr::isZero() const {
  if (const ConstantExpr *CE = dyn_cast<ConstantExpr>(this))
    return CE->isZero();
  return false;
}
  
inline bool Expr::isTrue() const {
  assert(getWidth() == Expr::Bool && "Invalid isTrue() call!");
  if (const ConstantExpr *CE = dyn_cast<ConstantExpr>(this))
    return CE->isTrue();
  return false;
}
  
inline bool Expr::isFalse() const {
  assert(getWidth() == Expr::Bool && "Invalid isFalse() call!");
  if (const ConstantExpr *CE = dyn_cast<ConstantExpr>(this))
    return CE->isFalse();
  return false;
}

} // End klee namespace

#endif
