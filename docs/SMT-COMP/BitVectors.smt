(theory Fixed_Size_BitVectors

:written_by {Silvio Ranise, Cesare Tinelli, and Clark Barrett}

:date {May 7, 2007}

:notes
   "Against the requirements of the current SMT-LIB standard this theory does
    not provide a value for the formal attributes :sorts, :funs, and :preds.
    The reason is that the theory has an infinite number of sort, function, and
    predicate symbols, and so they cannot be specified formally in the current
    SMT-LIB language.  While extending SMT-LIB's type system with dependent
    types would allow a finitary formal specification of all the symbols in
    this theory's signature, such an extension does not seem to be worth the
    trouble at the moment.  As a temporary ad-hoc solution, this theory
    declaration specifies the signature, in English, in the user-defined
    attributes :sorts_description, :funs_description, and :preds_description.
    "

:sorts_description {
    All sort symbols of the form BitVec[m],
    where m is a numeral greater than 0.
}

:funs_description {
    Constant symbols bit0 and bit1 of sort BitVec[1]
}

:funs_description {
    All function symbols with arity of the form

      (concat BitVec[i] BitVec[j] BitVec[m])

    where
    - i,j,m are numerals
    - i,j > 0
    - i + j = m
}

:funs_description {
    All function symbols with arity of the form

      (extract[i:j] BitVec[m] BitVec[n])

    where
    - i,j,m,n are numerals
    - m > i >= j >= 0,
    - n = i-j+1.
}

:funs_description {
    All function symbols with arity of the form

       (op1 BitVec[m] BitVec[m])
    or
       (op2 BitVec[m] BitVec[m] BitVec[m])

    where
    - op1 is from {bvnot, bvneg}
    - op2 is from {bvand, bvor, bvadd, bvmul, bvudiv, bvurem, bvshl, bvlshr}
    - m is a numeral greater than 0
}

:preds_description {
    All predicate symbols with arity of the form

       (pred BitVec[m] BitVec[m])

    where
    - pred is from {bvult}
    - m is a numeral greater than 0
}

:definition
  "This is a core theory for fixed-size bitvectors where the operations
   of concatenation and extraction of bitvectors as well as the usual
   logical and arithmetic operations are overloaded.
   The theory is defined semantically as follows.

   The sort BitVec[m] (for m > 0) is the set of finite functions
   whose domain is the initial segment of the naturals [0...m), meaning
   that 0 is included and m is excluded, and the co-domain is {0,1}.

   The semantic interpretation [[_]] of well-sorted BitVec-terms is
   inductively defined as follows.

   - Variables

   If v is a variable of sort BitVec[m] with 0 < m, then
   [[v]] is some element of [{0,...,m-1} -> {0,1}], the set of total
   functions from {0,...,m-1} to {0,1}.

   - Constant symbols bit0 and bit1 of sort BitVec[1]

   [[bit0]] := \lambda x : [0,1). 0
   [[bit1]] := \lambda x : [0,1). 1

   - Function symbols for concatenation

   [[(concat s t)]] := \lambda x : [0...n+m).
                          if (x<m) then [[t]](x) else [[s]](x-m)
   where
   s and t are terms of sort BitVec[n] and BitVec[m], respectively,
   0 < n, 0 < m.

   - Function symbols for extraction

   [[(extract[i:j] s)]] := \lambda x : [0...i-j+1). [[s]](j+x)
   where s is of sort BitVec[l], 0 <= j <= i < l.

   - Bit-wise operations

   [[(bvnot s)]] := \lambda x : [0...m). if [[s]](x) = 0 then 1 else 0

   [[(bvand s t)]] := \lambda x : [0...m).
                         if [[s]](x) = 0 then 0 else [[t]](x)

   [[(bvor s t)]] := \lambda x : [0...m).
                         if [[s]](x) = 1 then 1 else [[t]](x)

   where s and t are both of sort BitVec[m] and 0 < m.

   - Arithmetic operations

   To define the semantics of the bitvector arithmetic operators, we first
   introduce some additional definitions:

   o (x div y) where x and y are integers with x >= 0 and y > 0 returns the
     integer part of x divided by y (i.e., truncated integer division).

   o (x rem y) where x and y are integers with x >= 0 and y > 0 returns the
     remainder when x is divided by y.  Note that we always have the following
     equivalence (for y > 0): (x div y) * y + (x rem y) = x.

   o bv2nat which takes a bitvector b: [0...m) --> {0,1}
     with 0 < m, and returns an integer in the range [0...2^m),
     and is defined as follows:

       bv2nat(b) := b(m-1)*2^{m-1} + b(m-2)*2^{m-2} + ... + b(0)*2^0

   o nat2bv[m], with 0 < m, which takes a non-negative integer
     n and returns the (unique) bitvector b: [0,...,m) -> {0,1}
     such that

       b(m-1)*2^{m-1} + ... + b(0)*2^0 = n rem 2^m

   Now, we can define the following operations.  Suppose s and t are both terms
   of sort BitVec[m], m > 0.

   [[(bvneg s)]] := nat2bv[m](2^m - bv2nat([[s]]))

   [[(bvadd s t)]] := nat2bv[m](bv2nat([[s]]) + bv2nat([[t]]))

   [[(bvmul s t)]] := nat2bv[m](bv2nat([[s]]) * bv2nat([[t]]))

   [[(bvudiv s t)]] := if bv2nat([[t]]) != 0 then
                          nat2bv[m](bv2nat([[s]]) div bv2nat([[t]]))

   [[(bvurem s t)]] := if bv2nat([[t]]) != 0 then
                          nat2bv[m](bv2nat([[s]]) rem bv2nat([[t]]))

   - Shift operations

   Suppose s and t are both terms of sort BitVec[m], m > 0.  We make use of the
   definitions given for the arithmetic operations, above.

   [[(bvshl s t)]] := nat2bv[m](bv2nat([[s]]) * 2^(bv2nat([[t]])))

   [[(bvlshr s t)]] := nat2bv[m](bv2nat([[s]]) div 2^(bv2nat([[t]])))

   Finally, we can define the binary predicate bvult:

   (bvult s t) is interpreted to be true iff bv2nat([[s]]) < bv2nat([[t]])

   Note that the semantic interpretation above is underspecified because it
   does not specify the meaning of (bvudiv s t) or (bvurem s t) in case
   bv2nat([[t]]) is 0.  Since the semantics of SMT-LIB's underlying logic
   associates *total* functions to function symbols, we then consider as models
   of this theory *any* interpretation conforming to the specifications above
   (and defining bvudiv and bvurem arbitrarily when the second argument
   evaluates to 0).  Benchmarks using this theory should only include a
   :status sat or :status unsat attribute if the status is independent of
   the particular choice of model for the theory.

  "

)
