(logic QF_BV

:written_by {Silvio Ranise, Cesare Tinelli, and Clark Barrett}
:date {May 7, 2007}

:theory Fixed_Size_BitVectors

:language

 "Closed quantifier-free formulas built over an arbitrary expansion of the
  Fixed_Size_BitVectors signature with free constant symbols over the sorts
  BitVec[m] for 0 < m.  Formulas in ite terms must satisfy the same restriction
  as well, with the exception that they need not be closed (because they may be
  in the scope of a let expression).
 "

:notes
 "For quick reference, the following is a brief and informal summary of the
  legal symbols in this logic and their meaning (formal definitions are found
  either in the Fixed_Size_Bitvectors theory, or in the extensions below).

  Defined in theory Fixed_Size_Bitvectors:

    Functions/Constants:

    (bit0 BitVec[1])
      - the constant consisting of a single bit with value 0
    (bit1 BitVec[1])
      - the constant consisting of a single bit with value 1
    (concat BitVec[i] BitVec[j] BitVec[m])
      - concatenation of bitvectors of size i and j to get a new bitvector of
        size m, where m = i + j
    (extract[i:j] BitVec[m] BitVec[n])
      - extraction of bits i down to j from a bitvector of size m to yield a
        new bitvector of size n, where n = i - j + 1
    (bvnot BitVec[m] BitVec[m])
      - bitwise negation
    (bvand BitVec[m] BitVec[m] BitVec[m])
      - bitwise and
    (bvor BitVec[m] BitVec[m] BitVec[m])
      - bitwise or
    (bvneg BitVec[m] BitVec[m])
      - 2's complement unary minus
    (bvadd BitVec[m] BitVec[m] BitVec[m])
      - addition modulo 2^m
    (bvmul BitVec[m] BitVec[m] BitVec[m])
      - multiplication modulo 2^m
    (bvudiv BitVec[m] BitVec[m] BitVec[m])
      - unsigned division, truncating towards 0 (undefined if divisor is 0)
    (bvurem BitVec[m] BitVec[m] BitVec[m])
      - unsigned remainder from truncating division (undefined if divisor is 0)
    (bvshl BitVec[m] BitVec[m] BitVec[m])
      - shift left (equivalent to multiplication by 2^x where x is the value of
        the second argument)
    (bvlshr BitVec[m] BitVec[m] BitVec[m])
      - logical shift right (equivalent to unsigned division by 2^x where x is
        the value of the second argument)

    Predicates:

    (bvult BitVec[m] BitVec[m])
      - binary predicate for unsigned less than

  Defined below:

    Functions/Constants:

    Bitvector constants:
      - bvX[m] where X is a numeral in base 10 defines the bitvector constant
        with numeric value X of size m.
      - bvbinX where X is a binary numeral of length m defines the
        bitvector constant with value X and size m.
      - bvhexX where X is a hexadecimal numeral of length m defines the
        bitvector constant with value X and size 4*m.
    (bvnand BitVec[m] BitVec[m] BitVec[m])
      - bitwise nand (negation of and)
    (bvnor BitVec[m] BitVec[m] BitVec[m])
      - bitwise nor (negation of or)
    (bvxor BitVec[m] BitVec[m] BitVec[m])
      - bitwise exclusive or
    (bvxnor BitVec[m] BitVec[m] BitVec[m])
      - bitwise equivalence (equivalently, negation of bitwise exclusive or)
    (bvcomp BitVec[m] BitVec[m] BitVec[1])
      - bit comparator: equals bit1 iff all bits are equal
    (bvsub BitVec[m] BitVec[m] BitVec[m])
      - 2's complement subtraction modulo 2^m
    (bvsdiv BitVec[m] BitVec[m] BitVec[m])
      - 2's complement signed division
    (bvsrem BitVec[m] BitVec[m] BitVec[m])
      - 2's complement signed remainder (sign follows dividend)
    (bvsmod BitVec[m] BitVec[m] BitVec[m])
      - 2's complement signed remainder (sign follows divisor)
    (bvashr BitVec[m] BitVec[m] BitVec[m])
      - Arithmetic shift right, like logical shift right except that the most
        significant bits of the result always copy the most significant
        bit of the first argument.

    The following symbols are parameterized by the numeral i, where i >= 0.

    (repeat[i] BitVec[m] BitVec[i*m])
      - (repeat[i] x) means concatenate i copies of x
    (zero_extend[i] BitVec[m] BitVec[m+i])
      - (zero_extend[i] x) means extend x with zeroes to the (unsigned)
        equivalent bitvector of size m+i
    (sign_extend[i] BitVec[m] BitVec[m+i])
      - (sign_extend[i] x) means extend x to the (signed) equivalent bitvector
        of size m+i
    (rotate_left[i] BitVec[m] BitVec[m])
      - (rotate_left[i] x) means rotate bits of x to the left i times
    (rotate_right[i] BitVec[m] BitVec[m])
      - (rotate_right[i] x) means rotate bits of x to the right y times

    Predicates:

    (bvule BitVec[m] BitVec[m])
      - binary predicate for unsigned less than or equal
    (bvugt BitVec[m] BitVec[m])
      - binary predicate for unsigned greater than
    (bvuge BitVec[m] BitVec[m])
      - binary predicate for unsigned greater than or equal
    (bvslt BitVec[m] BitVec[m])
      - binary predicate for signed less than
    (bvsle BitVec[m] BitVec[m])
      - binary predicate for signed less than or equal
    (bvsgt BitVec[m] BitVec[m])
      - binary predicate for signed greater than
    (bvsge BitVec[m] BitVec[m])
      - binary predicate for signed greater than or equal

 "

:extensions
 "Below, let |exp| denote the integer resulting from the evaluation
  of the arithmetic expression exp.

  - Bitvector Constants:
    The string bv followed by the numeral n and a size [m] (as in bv13[32])
    abbreviates any term t of sort BitVec[m] built only out of the symbols in
    {concat, bit0, bit1} such that

    [[t]] = nat2bv[m](n) for n=0, ..., 2^m - 1.

    See the specification of the theory's semantics for a definition
    of the functions [[_]] and nat2bv.  Note that this convention implicitly
    considers the numeral n as a number written in base 10.

    For backward compatibility, if the size [m] is omitted, then the size is
    assumed to be 32.

    The string bvbin followed by a sequence of 0's and 1's abbreviates the
    concatenation of a similar sequence of bit0 and bit1 terms.  Thus,
    if n is the numeral represented in base 2 by the sequence of 0's and 1's
    and m is the length of the sequence, then the term represents
    nat2bv[m](n).  For example bvbin0101 is equivalent to bv5[4].

    The string bvhex followed by a sequence of digits and/or letters from A to
    F is interpreted similarly as a concatenation of bit0 and bit1 as follows.
    If n is the numeral represented in hexadecimal (base 16) by the sequence of
    digits and letters from A to F and m is four times the length of the
    sequence, then the term represents nat2bv[m](n).  For example, bvbinFF is
    equivalent to bv255[8].  Letters in the hexadecimal sequence may be in
    either upper or lower case.

  - Bitwise operators

    For all terms s,t of sort BitVec[m], where 0 < m,

    (bvnand s t) abbreviates (bvnot (bvand s t))
    (bvnor s t) abbreviates (bvnot (bvor s t))
    (bvxor s t) abbreviates (bvor (bvand s (bvnot t)) (bvand (bvnot s) t))
    (bvxnor s t) abbreviates (bvor (bvand s t) (bvand (bvnot s) (bvnot t)))
    (bvcomp s t) abbreviates (bvxnor s t) if m = 1, and
       (bvand (bvxnor (extract[|m-1|:|m-1|] s) (extract[|m-1|:|m-1|] t))
              (bvcomp (extract[|m-2|:0] s) (extract[|m-2|:0] t))) otherwise

  - Arithmetic operators

    For all terms s,t of sort BitVec[m], where 0 < m,

    (bvsub s t) abbreviates (bvadd s (bvneg t))
    (bvsdiv s t) abbreviates
      (let (?msb_s (extract[|m-1|:|m-1|] s))
      (let (?msb_t (extract[|m-1|:|m-1|] t))
      (ite (and (= ?msb_s bit0) (= ?msb_t bit0))
           (bvudiv s t)
      (ite (and (= ?msb_s bit1) (= ?msb_t bit0))
           (bvneg (bvudiv (bvneg s) t))
      (ite (and (= ?msb_s bit0) (= ?msb_t bit1))
           (bvneg (bvudiv s (bvneg t)))
           (bvudiv (bvneg s) (bvneg t)))))))
    (bvsrem s t) abbreviates
      (let (?msb_s (extract[|m-1|:|m-1|] s))
      (let (?msb_t (extract[|m-1|:|m-1|] t))
      (ite (and (= ?msb_s bit0) (= ?msb_t bit0))
           (bvurem s t)
      (ite (and (= ?msb_s bit1) (= ?msb_t bit0))
           (bvneg (bvurem (bvneg s) t))
      (ite (and (= ?msb_s bit0) (= ?msb_t bit1))
           (bvurem s (bvneg t)))
           (bvneg (bvurem (bvneg s) (bvneg t)))))))
    (bvsmod s t) abbreviates
      (let (?msb_s (extract[|m-1|:|m-1|] s))
      (let (?msb_t (extract[|m-1|:|m-1|] t))
      (ite (and (= ?msb_s bit0) (= ?msb_t bit0))
           (bvurem s t)
      (ite (and (= ?msb_s bit1) (= ?msb_t bit0))
           (bvadd (bvneg (bvurem (bvneg s) t)) t)
      (ite (and (= ?msb_s bit0) (= ?msb_t bit1))
           (bvadd (bvurem s (bvneg t)) t)
           (bvneg (bvurem (bvneg s) (bvneg t)))))))
    (bvule s t) abbreviates (or (bvult s t) (= s t))
    (bvugt s t) abbreviates (bvult t s)
    (bvuge s t) abbreviates (or (bvult t s) (= s t))
    (bvslt s t) abbreviates:
      (or (and (= (extract[|m-1|:|m-1|] s) bit1)
               (= (extract[|m-1|:|m-1|] t) bit0))
          (and (= (extract[|m-1|:|m-1|] s) (extract[|m-1|:|m-1|] t))
               (bvult s t)))
    (bvsle s t) abbreviates:
      (or (and (= (extract[|m-1|:|m-1|] s) bit1)
               (= (extract[|m-1|:|m-1|] t) bit0))
          (and (= (extract[|m-1|:|m-1|] s) (extract[|m-1|:|m-1|] t))
               (bvule s t)))
    (bvsgt s t) abbreviates (bvslt t s)
    (bvsge s t) abbreviates (bvsle t s)

  - Other operations

    For all numerals j > 1 and 0 < m, and all terms of s and t of
    sort BitVec[m],

    (bvashr s t) abbreviates 
      (ite (= (extract[|m-1|:|m-1|] s) bit0)
           (bvlshr s t)
           (bvnot (bvlshr (bvnot s) t)))

    (repeat[1] t) stands for t
    (repeat[j] t) abbreviates (concat t (repeat[|j-1|] t))

    (zero_extend[0] t) stands for t
    (zero_extend[j] t) abbreviates (concat (repeat[j] bit0) t)

    (sign_extend[0] t) stands for t
    (sign_extend[j] t) abbreviates
      (concat (repeat[j] (extract[|m-1|:|m-1|] t)) t)

    (rotate_left[0] t) stands for t
    (rotate_left[j] t) abbreviates t if m = 1, and
      (rotate_left[|j-1|]
        (concat (extract[|m-2|:0] t) (extract[|m-1|:|m-1|] t))
      otherwise

    (rotate_right[0] t) stands for t
    (rotate_right[j] t) abbreviates t if m = 1, and
      (rotate_right[|j-1|]
        (concat (extract[0:0] t) (extract[|m-1|:1] t)))
      otherwise

 "
)

