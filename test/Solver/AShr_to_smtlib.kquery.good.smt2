;SMTLIBv2 Query 0
(set-logic QF_AUFBV )
(declare-fun shift () (Array (_ BitVec 32) (_ BitVec 8) ) )
(declare-fun value () (Array (_ BitVec 32) (_ BitVec 8) ) )
(assert (and  (=  false (=  (_ bv0 8) (ite (bvuge (select  shift (_ bv0 32) ) (_ bv8 8) ) (_ bv0 8) (bvashr (select  value (_ bv0 32) ) (select  shift (_ bv0 32) ) ) ) ) ) (bvule  (_ bv8 8) (select  shift (_ bv0 32) ) ) ) )
(check-sat)
(exit)
