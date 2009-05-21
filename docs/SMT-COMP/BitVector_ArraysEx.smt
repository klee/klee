(theory BitVector_ArraysEx

 :written_by {Clark Barrett}
 :date {May 7, 2007}
 
:sorts_description {
    All sort symbols of the form BitVec[m],
    where m is a numeral greater than 0.

    All sort symbols of the form Array[m:n],
    where m and n are numerals with m > 0 and n > 0.
}

:funs_description {
    All functions from the theory Fixed_Size_Bitvectors.
}

:funs_description {
    All function symbols with arity of the form

      (select Array[m:n] BitVec[m] BitVec[n])

    where
    - m,n are numerals
    - m > 0, n > 0
}

:funs_description {
    All function symbols with arity of the form

      (store Array[m:n] BitVec[m] BitVec[n] Array[m:n])

    where
    - m,n are numerals
    - m > 0, n > 0
}

:preds_description {
    All predicates from the theory Fixed_Size_Bitvectors.
}


 :definition 
 "This is a theory containing an infinite number of copies of the theory of
  functional arrays with extensionality: one for each pair of bitvector sorts.
  It can be formally defined as the union of the SMT-LIB theory
  Fixed_Size_Bitvectors and an infinite number of variants of the SMT-LIB
  theory ArraysEx: one for each distinct signature morphism mapping the sort Index
  to BitVec[m] and the sort Element to Bitvec[n] where m and n range over all
  positive numerals.  In each of the copies of ArraysEx, the sort Array is
  renamed to Array[m:n] and each copy of ArraysEx contributes exactly one select
  function and one store function to the infinite polymorphic family of select
  and store functions described above.
 "

 :notes
 "As in the theory Fixed_Size_Bitvectors, this theory does not
  provide a value for the formal attributes :sorts, :funs, and :preds because
  there are an infinite number of them.  See the notes in theory
  Fixed_Size_Bitvectors for details.

  If for i=1,2, T_i is an SMT-LIB theory with sorts S_i, function symbols F_i,
  predicate symbols P_i, and axioms A_i, by \"union\" of T_1 and T_2 
  we mean the theory T with sorts S_1 U S_2, function symbols F_1 U F_2,
  predicate symbols P_1 U P_2, and axioms A_1 U A_2 (where U stands for set
  theoretic union).

  The theory T is a well-defined SMT-LIB theory whenever S_1, S_2, F_1, F_2, 
  P_1, P_2 are all pairwise disjoint, as is the case for the component theories
  considered here. 
 "
)


