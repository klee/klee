//===-- StringModel.cpp ----------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "StringModel.h"

namespace klee {

StringModel::StringModel() {
	/*********************************************/
	/* [6] zero, one and minusOne as ref<Expr>'s */
	/*********************************************/
	one      = BvToIntExpr::create(ConstantExpr::create(1,Expr::Int64));
	zero     = BvToIntExpr::create(ConstantExpr::create(0,Expr::Int64));
	minusOne = SubExpr::create(zero,one);
	x00      = StrConstExpr::create("\\x00");
}

StrModel StringModel::modelStrcmp(
  const MemoryObject* moP, ref<Expr> s1,
  const MemoryObject* moQ, ref<Expr> s2) {
 /*********************************/
	/* [7] AB, svar, offset and size */
	/*********************************/
	ref<Expr> ABp_size = moP->getIntSizeExpr();
	ref<Expr> ABq_size = moQ->getIntSizeExpr();
	ref<Expr> offset_p = BvToIntExpr::create(moP->getOffsetExpr(s1));
	ref<Expr> offset_q = BvToIntExpr::create(moQ->getOffsetExpr(s2));
 	ref<Expr> ABp      = StrVarExpr::create(moP->getABSerial());
	ref<Expr> ABq      = StrVarExpr::create(moQ->getABSerial());
	ref<Expr> p_size   = SubExpr::create(ABp_size,offset_p);
	ref<Expr> q_size   = SubExpr::create(ABq_size,offset_q);
	ref<Expr> p_var    = StrSubstrExpr::create(ABp,offset_p, p_size);
	ref<Expr> q_var    = StrSubstrExpr::create(ABq,offset_q, q_size);

	/*****************************/
	/* [8] NULL temination stuff */
	/*****************************/
	ref<Expr> firstIdxOf_x00_in_p = StrFirstIdxOfExpr::create(p_var,x00);
	ref<Expr> firstIdxOf_x00_in_q = StrFirstIdxOfExpr::create(q_var,x00);

	ref<Expr> p_is_not_NULL_terminated = EqExpr::create(firstIdxOf_x00_in_p,minusOne);
	ref<Expr> q_is_not_NULL_terminated = EqExpr::create(firstIdxOf_x00_in_q,minusOne);

	ref<Expr> p_is_NULL_terminated = NotExpr::create(p_is_not_NULL_terminated);
	ref<Expr> q_is_NULL_terminated = NotExpr::create(q_is_not_NULL_terminated);
	
	ref<Expr> p_and_q_are_both_NULL_terminated = AndExpr::create(
		p_is_NULL_terminated,
		q_is_NULL_terminated);

	/************************************************************/
	/* [10] Finally ... check whether p and q are identical ... */
	/************************************************************/
	ref<Expr> final_exp = StrEqExpr::create(
			StrSubstrExpr::create(p_var,zero,firstIdxOf_x00_in_p),
			StrSubstrExpr::create(q_var,zero,firstIdxOf_x00_in_q));

   return std::make_pair(
 	  	SelectExpr::create(
	  		final_exp,
	  		ConstantExpr::create(0,Expr::Int32),
	  		ConstantExpr::create(1,Expr::Int32)),
      p_and_q_are_both_NULL_terminated  
   );
}

} //end klee namespace
