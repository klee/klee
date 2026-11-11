# Queries shared by Z3ConcatBool.kquery and STPConcatBool.kquery.
#
# Concat whose operands are 1 bit wide, as built for `bitcast <N x i1> to iN`.
# The solver builders construct expressions of width 1 as Booleans and have to
# convert them to bitvectors before concatenating them.

array x[2] : w32 -> w8 = symbolic

# CHECK: Query 0: VALID
(query [(Eq 0 (Read w8 0 x)) (Eq 1 (Read w8 1 x))]
       (Eq 1 (Concat w2 (Eq 0 (Read w8 1 x)) (Eq 0 (Read w8 0 x)))))

# CHECK: Query 1: INVALID
(query [] (Eq 0 (Concat w2 (Eq 0 (Read w8 1 x)) (Eq 0 (Read w8 0 x)))))

# CHECK: Query 2: VALID
(query [(Eq 0 (Read w8 1 x))]
       (Ule 256 (Concat w9 (Eq 0 (Read w8 1 x)) (Read w8 0 x))))

# CHECK: Query 3: VALID
(query [(Eq 0 (Read w8 1 x))]
       (Eq 1 (URem w9 (Concat w9 (Read w8 0 x) (Eq 0 (Read w8 1 x))) 2)))

# CHECK: Query 4: INVALID
# CHECK-NEXT: Expr 0: 2
(query [(Eq 0 (Read w8 1 x)) (Eq 1 (Read w8 0 x))] false
       [(Concat w2 (Eq 0 (Read w8 1 x)) (Eq 0 (Read w8 0 x)))])

# CHECK: Query 5: INVALID
# CHECK-NEXT: Array 0: x[{{[1-9][0-9]*}}, 0]
(query [(Eq 2 (Concat w2 (Eq 0 (Read w8 1 x)) (Eq 0 (Read w8 0 x))))] false
       [] [x])
