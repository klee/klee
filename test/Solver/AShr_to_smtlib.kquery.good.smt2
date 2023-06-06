;SMTLIBv2 Query 0
(set-logic QF_AUFBV )
(declare-fun makeSymbolic0 () (Array (_ BitVec 32) (_ BitVec 8) ) )
(declare-fun makeSymbolic1 () (Array (_ BitVec 32) (_ BitVec 8) ) )
(assert (and  (not  (=  (_ bv0 8) (ite (bvuge (select  makeSymbolic1 (_ bv0 32) ) (_ bv8 8) ) (_ bv0 8) (bvashr (select  makeSymbolic0 (_ bv0 32) ) (select  makeSymbolic1 (_ bv0 32) ) ) ) ) ) (bvule  (_ bv8 8) (select  makeSymbolic1 (_ bv0 32) ) ) ) )
(check-sat)
(exit)
