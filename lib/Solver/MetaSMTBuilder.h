 /*
 * MetaSMTBuilder.h
 *
 *  Created on: 8 Aug 2012
 *      Author: hpalikar
 */

#ifndef METASMTBUILDER_H_
#define METASMTBUILDER_H_

#include "klee/Config/config.h"
#include "klee/Expr.h"
#include "klee/util/ExprPPrinter.h"
#include "klee/util/ArrayExprHash.h"
#include "klee/util/ExprHashMap.h"
#include "ConstantDivision.h"

#ifdef SUPPORT_METASMT

#include "llvm/Support/CommandLine.h"

#include <metaSMT/frontend/Logic.hpp>
#include <metaSMT/frontend/QF_BV.hpp>
#include <metaSMT/frontend/Array.hpp>
#include <metaSMT/support/default_visitation_unrolling_limit.hpp>
#include <metaSMT/support/run_algorithm.hpp>

#define Expr VCExpr
#define STP STP_Backend
#include <metaSMT/backend/STP.hpp>
#undef Expr
#undef STP
 
#include <boost/mpl/vector.hpp>
#include <boost/format.hpp>

using namespace metaSMT;
using namespace metaSMT::logic::QF_BV;


#define DIRECT_CONTEXT

namespace {
  llvm::cl::opt<bool>
  UseConstructHashMetaSMT("use-construct-hash-metasmt", 
                   llvm::cl::desc("Use hash-consing during metaSMT query construction."),
                   llvm::cl::init(true));
}


namespace klee {

typedef metaSMT::logic::Predicate<proto::terminal<metaSMT::logic::tag::true_tag>::type  > const MetaSMTConstTrue;
typedef metaSMT::logic::Predicate<proto::terminal<metaSMT::logic::tag::false_tag>::type  > const MetaSMTConstFalse;
typedef metaSMT::logic::Array::array MetaSMTArray;

template<typename SolverContext>
class MetaSMTBuilder;

template<typename SolverContext>
class MetaSMTArrayExprHash : public ArrayExprHash< typename SolverContext::result_type > {
    
    friend class MetaSMTBuilder<SolverContext>;
    
public:
    MetaSMTArrayExprHash() {};
    virtual ~MetaSMTArrayExprHash() {};
};

static const bool mimic_stp = true;


template<typename SolverContext>
class MetaSMTBuilder {
public:

    MetaSMTBuilder(SolverContext& solver, bool optimizeDivides) : _solver(solver), _optimizeDivides(optimizeDivides) { };
    virtual ~MetaSMTBuilder() {};
    
    typename SolverContext::result_type construct(ref<Expr> e);
    
    typename SolverContext::result_type getInitialRead(const Array *root, unsigned index);
    
    typename SolverContext::result_type getTrue() {
        return(evaluate(_solver, metaSMT::logic::True));        
    }
    
    typename SolverContext::result_type getFalse() {        
        return(evaluate(_solver, metaSMT::logic::False));        
    }
    
    typename SolverContext::result_type bvOne(unsigned width) {
        return bvZExtConst(width, 1);
    }
    
    typename SolverContext::result_type bvZero(unsigned width) {
        return bvZExtConst(width, 0);
    }
    
    typename SolverContext::result_type bvMinusOne(unsigned width) {
        return bvSExtConst(width, (int64_t) -1);
    }
    
    typename SolverContext::result_type bvConst32(unsigned width, uint32_t value) {
        return(evaluate(_solver, bvuint(value, width)));
    }
    
    typename SolverContext::result_type bvConst64(unsigned width, uint64_t value) {
         return(evaluate(_solver, bvuint(value, width)));
    }
       
    typename SolverContext::result_type bvZExtConst(unsigned width, uint64_t value);    
    typename SolverContext::result_type bvSExtConst(unsigned width, uint64_t value);
    
    //logical left and right shift (not arithmetic)
    typename SolverContext::result_type bvLeftShift(typename SolverContext::result_type expr, unsigned width, unsigned shift, unsigned shiftBits);
    typename SolverContext::result_type bvRightShift(typename SolverContext::result_type expr, unsigned width, unsigned amount, unsigned shiftBits);
    typename SolverContext::result_type bvVarLeftShift(typename SolverContext::result_type expr, typename SolverContext::result_type amount, unsigned width);
    typename SolverContext::result_type bvVarRightShift(typename SolverContext::result_type expr, typename SolverContext::result_type amount, unsigned width);
    typename SolverContext::result_type bvVarArithRightShift(typename SolverContext::result_type expr, typename SolverContext::result_type amount, unsigned width);
    
    
    typename SolverContext::result_type getArrayForUpdate(const Array *root, const UpdateNode *un);
    typename SolverContext::result_type getInitialArray(const Array *root);
    MetaSMTArray buildArray(unsigned elem_width, unsigned index_width);
        
private:
    typedef ExprHashMap< std::pair<typename SolverContext::result_type, unsigned> >  MetaSMTExprHashMap;
    typedef typename MetaSMTExprHashMap::iterator MetaSMTExprHashMapIter;
    typedef typename MetaSMTExprHashMap::const_iterator MetaSMTExprHashMapConstIter;    

    SolverContext&  _solver;
    bool _optimizeDivides;
    MetaSMTArrayExprHash<SolverContext>  _arr_hash;
    MetaSMTExprHashMap                   _constructed;
    
    typename SolverContext::result_type constructActual(ref<Expr> e, int *width_out);
    typename SolverContext::result_type construct(ref<Expr> e, int *width_out);
    
    typename SolverContext::result_type bvBoolExtract(typename SolverContext::result_type expr, int bit);
    typename SolverContext::result_type bvExtract(typename SolverContext::result_type expr, unsigned top, unsigned bottom);
    typename SolverContext::result_type eqExpr(typename SolverContext::result_type a, typename SolverContext::result_type b);
    
    typename SolverContext::result_type constructAShrByConstant(typename SolverContext::result_type expr, unsigned width, unsigned shift, 
                                                                typename SolverContext::result_type isSigned, unsigned shiftBits);
    typename SolverContext::result_type constructMulByConstant(typename SolverContext::result_type expr, unsigned width, uint64_t x);
    typename SolverContext::result_type constructUDivByConstant(typename SolverContext::result_type expr_n, unsigned width, uint64_t d);
    typename SolverContext::result_type constructSDivByConstant(typename SolverContext::result_type expr_n, unsigned width, uint64_t d);
    
    unsigned getShiftBits(unsigned amount) {
        unsigned bits = 1;
        amount--;
        while (amount >>= 1) {
            bits++;
        }
        return(bits);
    }
};

template<typename SolverContext>
typename SolverContext::result_type MetaSMTBuilder<SolverContext>::getArrayForUpdate(const Array *root, const UpdateNode *un) {
  
    if (!un) {
        return(getInitialArray(root));
    }
    else {
        typename SolverContext::result_type un_expr;
        bool hashed = _arr_hash.lookupUpdateNodeExpr(un, un_expr);

        if (!hashed) {
            un_expr = evaluate(_solver,
                               metaSMT::logic::Array::store(getArrayForUpdate(root, un->next),
                                                            construct(un->index, 0),
                                                            construct(un->value, 0)));
            _arr_hash.hashUpdateNodeExpr(un, un_expr);
        }
        return(un_expr);
    }
}

template<typename SolverContext>
typename SolverContext::result_type MetaSMTBuilder<SolverContext>::getInitialArray(const Array *root)
{
    assert(root);
    typename SolverContext::result_type array_expr;
    bool hashed = _arr_hash.lookupArrayExpr(root, array_expr);

    if (!hashed) {

        array_expr = evaluate(_solver, buildArray(root->getRange(), root->getDomain()));

        if (root->isConstantArray()) {    
            for (unsigned i = 0, e = root->size; i != e; ++i) {
                 typename SolverContext::result_type tmp =
                            evaluate(_solver, 
                                     metaSMT::logic::Array::store(array_expr,
                                                                  construct(ConstantExpr::alloc(i, root->getDomain()), 0),
                                                                  construct(root->constantValues[i], 0)));
                array_expr = tmp;
            }
        }
        _arr_hash.hashArrayExpr(root, array_expr);
    }

    return(array_expr);
}

template<typename SolverContext>
MetaSMTArray MetaSMTBuilder<SolverContext>::buildArray(unsigned elem_width, unsigned index_width) {
    return(metaSMT::logic::Array::new_array(elem_width, index_width));
}

template<typename SolverContext>
typename SolverContext::result_type MetaSMTBuilder<SolverContext>::getInitialRead(const Array *root, unsigned index) {
    assert(root); 
    assert(false);
    typename SolverContext::result_type array_exp = getInitialArray(root);
    typename SolverContext::result_type res = evaluate(
                        _solver,
                        metaSMT::logic::Array::select(array_exp, bvuint(index, root->getDomain())));    
    return(res);
}


template<typename SolverContext>  
typename SolverContext::result_type MetaSMTBuilder<SolverContext>::bvZExtConst(unsigned width, uint64_t value) {

    typename SolverContext::result_type res;

    if (width <= 64) {
        res = bvConst64(width, value);
    }
    else {
        typename SolverContext::result_type expr = bvConst64(64, value);
        typename SolverContext::result_type zero = bvConst64(64, 0);

        for (width -= 64; width > 64; width -= 64) {
             expr = evaluate(_solver, concat(zero, expr));  
        }
        res = evaluate(_solver, concat(bvConst64(width, 0), expr));  
    }

    return(res); 
}

template<typename SolverContext>  
typename SolverContext::result_type MetaSMTBuilder<SolverContext>::bvSExtConst(unsigned width, uint64_t value) {
  
  typename SolverContext::result_type res;
  
  if (width <= 64) {
      res = bvConst64(width, value); 
  }
  else {            
      // ToDo Reconsider -- note differences in STP and metaSMT for sign_extend arguments
      res = evaluate(_solver, sign_extend(width - 64, bvConst64(64, value)));		     
  }
  return(res); 
}

template<typename SolverContext>
typename SolverContext::result_type MetaSMTBuilder<SolverContext>::bvBoolExtract(typename SolverContext::result_type expr, int bit) {
    return(evaluate(_solver,
                    metaSMT::logic::equal(extract(bit, bit, expr),
                                          bvOne(1))));
}

template<typename SolverContext>
typename SolverContext::result_type MetaSMTBuilder<SolverContext>::bvExtract(typename SolverContext::result_type expr, unsigned top, unsigned bottom) {    
    return(evaluate(_solver, extract(top, bottom, expr)));
}

template<typename SolverContext>
typename SolverContext::result_type MetaSMTBuilder<SolverContext>::eqExpr(typename SolverContext::result_type a, typename SolverContext::result_type b) {    
    return(evaluate(_solver, metaSMT::logic::equal(a, b)));
}

template<typename SolverContext>
typename SolverContext::result_type MetaSMTBuilder<SolverContext>::constructAShrByConstant(typename SolverContext::result_type expr, unsigned width, unsigned amount,
                                                                                           typename SolverContext::result_type isSigned, unsigned shiftBits) {
    unsigned shift = amount & ((1 << shiftBits) - 1);
    typename SolverContext::result_type res;
    
    if (shift == 0) {
        res = expr;
    }
    else if (shift >= width) {
        res = evaluate(_solver, metaSMT::logic::Ite(isSigned, bvMinusOne(width), bvZero(width)));
    }
    else {
        res = evaluate(_solver, metaSMT::logic::Ite(isSigned,
                                                    concat(bvMinusOne(shift), bvExtract(expr, width - 1, shift)),
                                                    bvRightShift(expr, width, shift, shiftBits)));
    }
    
    return(res);
}

// width is the width of expr; x -- number of bits to shift with
template<typename SolverContext>
typename SolverContext::result_type MetaSMTBuilder<SolverContext>::constructMulByConstant(typename SolverContext::result_type expr, unsigned width, uint64_t x) {
    
    unsigned shiftBits = getShiftBits(width);
    uint64_t add, sub;
    typename SolverContext::result_type res;
    bool first = true;

    // expr*x == expr*(add-sub) == expr*add - expr*sub
    ComputeMultConstants64(x, add, sub);

    // legal, these would overflow completely
    add = bits64::truncateToNBits(add, width);
    sub = bits64::truncateToNBits(sub, width);
    
    for (int j = 63; j >= 0; j--) {
        uint64_t bit = 1LL << j;

        if ((add & bit) || (sub & bit)) {
            assert(!((add & bit) && (sub & bit)) && "invalid mult constants");

            typename SolverContext::result_type op = bvLeftShift(expr, width, j, shiftBits);

            if (add & bit) {
                if (!first) {
                    res = evaluate(_solver, bvadd(res, op));
                } else {
                    res = op;
                    first = false;
                }
            } else {
                if (!first) {
                    res = evaluate(_solver, bvsub(res, op));                    
                } else {
                     // To reconsider: vc_bvUMinusExpr in STP
                    res = evaluate(_solver, bvsub(bvZero(width), op));
                    first = false;
                }
            }
        }
    }
  
    if (first) { 
        res = bvZero(width);
    }

    return(res);
}


/* 
 * Compute the 32-bit unsigned integer division of n by a divisor d based on 
 * the constants derived from the constant divisor d.
 *
 * Returns n/d without doing explicit division.  The cost is 2 adds, 3 shifts, 
 * and a (64-bit) multiply.
 *
 * @param expr_n numerator (dividend) n as an expression
 * @param width  number of bits used to represent the value
 * @param d      the divisor
 *
 * @return n/d without doing explicit division
 */
template<typename SolverContext>
typename SolverContext::result_type MetaSMTBuilder<SolverContext>::constructUDivByConstant(typename SolverContext::result_type expr_n, unsigned width, uint64_t d) {
    
    assert(width == 32 && "can only compute udiv constants for 32-bit division");
    
    // Compute the constants needed to compute n/d for constant d without division by d.
    uint32_t mprime, sh1, sh2;
    ComputeUDivConstants32(d, mprime, sh1, sh2);
    typename SolverContext::result_type expr_sh1 = bvConst32(32, sh1);
    typename SolverContext::result_type expr_sh2 = bvConst32(32, sh2);
    
    // t1  = MULUH(mprime, n) = ( (uint64_t)mprime * (uint64_t)n ) >> 32
    typename SolverContext::result_type expr_n_64 = evaluate(_solver, concat(bvZero(32), expr_n)); //extend to 64 bits
    typename SolverContext::result_type t1_64bits = constructMulByConstant(expr_n_64, 64, (uint64_t)mprime);
    typename SolverContext::result_type t1        = bvExtract(t1_64bits, 63, 32); //upper 32 bits
    
    // n/d = (((n - t1) >> sh1) + t1) >> sh2;
    typename SolverContext::result_type n_minus_t1 = evaluate(_solver, bvsub(expr_n, t1));   
    typename SolverContext::result_type shift_sh1  = bvVarRightShift(n_minus_t1, expr_sh1, 32);
    typename SolverContext::result_type plus_t1    = evaluate(_solver, bvadd(shift_sh1, t1));    
    typename SolverContext::result_type res        = bvVarRightShift(plus_t1, expr_sh2, 32);
    
    return(res);
}

/* 
 * Compute the 32-bitnsigned integer division of n by a divisor d based on 
 * the constants derived from the constant divisor d.
 *
 * Returns n/d without doing explicit division.  The cost is 3 adds, 3 shifts, 
 * a (64-bit) multiply, and an XOR.
 *
 * @param n      numerator (dividend) as an expression
 * @param width  number of bits used to represent the value
 * @param d      the divisor
 *
 * @return n/d without doing explicit division
 */
template<typename SolverContext>
typename SolverContext::result_type MetaSMTBuilder<SolverContext>::constructSDivByConstant(typename SolverContext::result_type expr_n, unsigned width, uint64_t d) {
    
    assert(width == 32 && "can only compute udiv constants for 32-bit division");

    // Compute the constants needed to compute n/d for constant d w/o division by d.
    int32_t mprime, dsign, shpost;
    ComputeSDivConstants32(d, mprime, dsign, shpost);
    typename SolverContext::result_type expr_dsign  = bvConst32(32, dsign);
    typename SolverContext::result_type expr_shpost = bvConst32(32, shpost);
    
    // q0 = n + MULSH( mprime, n ) = n + (( (int64_t)mprime * (int64_t)n ) >> 32)
    int64_t mprime_64     = (int64_t)mprime;
    
    // ToDo Reconsider -- note differences in STP and metaSMT for sign_extend arguments
    typename SolverContext::result_type expr_n_64    = evaluate(_solver, sign_extend(64 - width, expr_n));    
    typename SolverContext::result_type mult_64      = constructMulByConstant(expr_n_64, 64, mprime_64);
    typename SolverContext::result_type mulsh        = bvExtract(mult_64, 63, 32); //upper 32-bits    
    typename SolverContext::result_type n_plus_mulsh = evaluate(_solver, bvadd(expr_n, mulsh));
    
    // Improved variable arithmetic right shift: sign extend, shift, extract.
    typename SolverContext::result_type extend_npm   = evaluate(_solver, sign_extend(64 - width, n_plus_mulsh));    
    typename SolverContext::result_type shift_npm    = bvVarRightShift(extend_npm, expr_shpost, 64);
    typename SolverContext::result_type shift_shpost = bvExtract(shift_npm, 31, 0); //lower 32-bits
    
    /////////////
    
    // XSIGN(n) is -1 if n is negative, positive one otherwise
    typename SolverContext::result_type is_signed    = bvBoolExtract(expr_n, 31);
    typename SolverContext::result_type neg_one      = bvMinusOne(32);
    typename SolverContext::result_type xsign_of_n   = evaluate(_solver, metaSMT::logic::Ite(is_signed, neg_one, bvZero(32)));    

    // q0 = (n_plus_mulsh >> shpost) - XSIGN(n)
    typename SolverContext::result_type q0           = evaluate(_solver, bvsub(shift_shpost, xsign_of_n));    
  
    // n/d = (q0 ^ dsign) - dsign
    typename SolverContext::result_type q0_xor_dsign = evaluate(_solver, bvxor(q0, expr_dsign));    
    typename SolverContext::result_type res          = evaluate(_solver, bvsub(q0_xor_dsign, expr_dsign));    
    
    return(res);
}

template<typename SolverContext>
typename SolverContext::result_type MetaSMTBuilder<SolverContext>::bvLeftShift(typename SolverContext::result_type expr, unsigned width, unsigned amount, unsigned shiftBits) {
    
    typename SolverContext::result_type res;    
    unsigned shift = amount & ((1 << shiftBits) - 1);

    if (shift == 0) {
        res = expr;
    }
    else if (shift >= width) {
        res = bvZero(width);
    }
    else {
        // stp shift does "expr @ [0 x s]" which we then have to extract,
        // rolling our own gives slightly smaller exprs
        res = evaluate(_solver, concat(extract(width - shift - 1, 0, expr),
                                       bvZero(shift)));
    }

    return(res);
}

template<typename SolverContext>
typename SolverContext::result_type MetaSMTBuilder<SolverContext>::bvRightShift(typename SolverContext::result_type expr, unsigned width, unsigned amount, unsigned shiftBits) {
  
    typename SolverContext::result_type res;    
    unsigned shift = amount & ((1 << shiftBits) - 1);

    if (shift == 0) {
        res = expr;
    } 
    else if (shift >= width) {
        res = bvZero(width);
    }
    else {
        res = evaluate(_solver, concat(bvZero(shift),
                                       extract(width - 1, shift, expr)));        
    }

    return(res);
}

// left shift by a variable amount on an expression of the specified width
template<typename SolverContext>
typename SolverContext::result_type MetaSMTBuilder<SolverContext>::bvVarLeftShift(typename SolverContext::result_type expr, typename SolverContext::result_type amount, unsigned width) {
    
    typename SolverContext::result_type res =  bvZero(width);
    
    int shiftBits = getShiftBits(width);

    //get the shift amount (looking only at the bits appropriate for the given width)
    typename SolverContext::result_type shift = evaluate(_solver, extract(shiftBits - 1, 0, amount));    

    //construct a big if-then-elif-elif-... with one case per possible shift amount
    for(int i = width - 1; i >= 0; i--) {
        res = evaluate(_solver, metaSMT::logic::Ite(eqExpr(shift, bvConst32(shiftBits, i)),
                                                    bvLeftShift(expr, width, i, shiftBits),
                                                    res));
    }
    
    // If overshifting, shift to zero    
    res = evaluate(_solver, metaSMT::logic::Ite(bvult(shift, bvConst32(shiftBits, width)),
                                                res,
                                                bvZero(width)));                                                

    return(res);
}

// logical right shift by a variable amount on an expression of the specified width
template<typename SolverContext>
typename SolverContext::result_type MetaSMTBuilder<SolverContext>::bvVarRightShift(typename SolverContext::result_type expr, typename SolverContext::result_type amount, unsigned width) {
    
    typename SolverContext::result_type res = bvZero(width);
    
    int shiftBits = getShiftBits(width);
    
    //get the shift amount (looking only at the bits appropriate for the given width)
    typename SolverContext::result_type shift = bvExtract(amount, shiftBits - 1, 0);
    
    //construct a big if-then-elif-elif-... with one case per possible shift amount
    for (int i = width - 1; i >= 0; i--) {
        res = evaluate(_solver, metaSMT::logic::Ite(eqExpr(shift, bvConst32(shiftBits, i)),
                                                    bvRightShift(expr, width, i, shiftBits),
                                                    res));
         // ToDo Reconsider widht to provide to bvRightShift
    }
    
    // If overshifting, shift to zero
    res = evaluate(_solver, metaSMT::logic::Ite(bvult(shift, bvConst32(shiftBits, width)),
                                                res,
                                                bvZero(width)));                                                

    return(res);
}

// arithmetic right shift by a variable amount on an expression of the specified width
template<typename SolverContext>
typename SolverContext::result_type MetaSMTBuilder<SolverContext>::bvVarArithRightShift(typename SolverContext::result_type expr, typename SolverContext::result_type amount, unsigned width) {
  
    int shiftBits = getShiftBits(width);

    //get the shift amount (looking only at the bits appropriate for the given width)
    typename SolverContext::result_type shift = bvExtract(amount, shiftBits - 1, 0);
    
    //get the sign bit to fill with
    typename SolverContext::result_type signedBool = bvBoolExtract(expr, width - 1);

    //start with the result if shifting by width-1
    typename SolverContext::result_type res =  constructAShrByConstant(expr, width, width - 1, signedBool, shiftBits);

    // construct a big if-then-elif-elif-... with one case per possible shift amount
    // XXX more efficient to move the ite on the sign outside all exprs?
    // XXX more efficient to sign extend, right shift, then extract lower bits?
    for (int i = width - 2; i >= 0; i--) {
        res = evaluate(_solver, metaSMT::logic::Ite(eqExpr(shift, bvConst32(shiftBits,i)),
                                                    constructAShrByConstant(expr, width, i, signedBool, shiftBits),
                                                    res));
    }
    
    // If overshifting, shift to zero
    res = evaluate(_solver, metaSMT::logic::Ite(bvult(shift, bvConst32(shiftBits, width)),
                                                res,
                                                bvZero(width)));      

    return(res);
}

template<typename SolverContext>
typename SolverContext::result_type MetaSMTBuilder<SolverContext>::construct(ref<Expr> e) {
    typename SolverContext::result_type res = construct(e, 0);
    _constructed.clear();
    return res;
}


/** if *width_out!=1 then result is a bitvector,
    otherwise it is a bool */
template<typename SolverContext>
typename SolverContext::result_type MetaSMTBuilder<SolverContext>::construct(ref<Expr> e, int *width_out) {
  
    if (!UseConstructHashMetaSMT || isa<ConstantExpr>(e)) {
        return(constructActual(e, width_out));
    } else {
        MetaSMTExprHashMapIter it = _constructed.find(e);
        if (it != _constructed.end()) {
            if (width_out) {
                *width_out = it->second.second;
            }
            return it->second.first;
        } else {
            int width = 0;
            if (!width_out) {
                 width_out = &width;
            }
            typename SolverContext::result_type res = constructActual(e, width_out);
            _constructed.insert(std::make_pair(e, std::make_pair(res, *width_out)));
            return res;
        }
    }
}

template<typename SolverContext>
typename SolverContext::result_type MetaSMTBuilder<SolverContext>::constructActual(ref<Expr> e, int *width_out)  {

    typename SolverContext::result_type res;
    
    int width = 0;
    if (!width_out) { 
        // assert(false);
        width_out = &width;
    }
    
    ++stats::queryConstructs;
    
//     llvm::errs() << "Constructing expression ";
//     ExprPPrinter::printSingleExpr(llvm::errs(), e);
//     llvm::errs() << "\n";

    switch (e->getKind()) {

        case Expr::Constant:
        {
            ConstantExpr *coe = cast<ConstantExpr>(e);
            assert(coe);
            unsigned coe_width = coe->getWidth();
            *width_out = coe_width;

            // Coerce to bool if necessary.
            if (coe_width == 1) {
                res = (coe->isTrue()) ? getTrue() : getFalse();
            }
            else if (coe_width <= 32) {
                 res = bvConst32(coe_width, coe->getZExtValue(32));
            }
            else if (coe_width <= 64) {
                 res = bvConst64(coe_width, coe->getZExtValue());
            }
            else {
                 ref<ConstantExpr> tmp = coe;
                 res = bvConst64(64, tmp->Extract(0, 64)->getZExtValue());
                 while (tmp->getWidth() > 64) {
                     tmp = tmp->Extract(64, tmp->getWidth() -  64);
                     unsigned min_width = std::min(64U, tmp->getWidth());
                     res = evaluate(_solver,
                                    concat(bvConst64(min_width, tmp->Extract(0, min_width)->getZExtValue()),
                                           res));
                 }
            }
            break;
        }

        case Expr::NotOptimized:
        { 
            NotOptimizedExpr *noe = cast<NotOptimizedExpr>(e);
            assert(noe);
            res = construct(noe->src, width_out);
            break;
        }
	
        case Expr::Select: 
        {
            SelectExpr *se = cast<SelectExpr>(e);
            assert(se);
            res = evaluate(_solver, 
                           metaSMT::logic::Ite(construct(se->cond, 0), 
                                               construct(se->trueExpr, width_out),
                                               construct(se->falseExpr, width_out)));
            break;
        }

        case Expr::Read:
        {
            ReadExpr *re = cast<ReadExpr>(e);
            assert(re && re->updates.root);
            *width_out = re->updates.root->getRange();
            // FixMe call method of Array
            res = evaluate(_solver, 
                           metaSMT::logic::Array::select(
                                  getArrayForUpdate(re->updates.root, re->updates.head),
                                  construct(re->index, 0)));
            break;
        }

        case Expr::Concat:
        {
            ConcatExpr *ce = cast<ConcatExpr>(e);
            assert(ce);
            *width_out = ce->getWidth();
            unsigned numKids = ce->getNumKids();

            if (numKids > 0) {
                res = evaluate(_solver, construct(ce->getKid(numKids-1), 0));
                for (int i = numKids - 2; i >= 0; i--) {
                    res = evaluate(_solver, concat(construct(ce->getKid(i), 0), res));
                }
            }
            break;
        }

        case Expr::Extract:
        {
            ExtractExpr *ee = cast<ExtractExpr>(e);
            assert(ee);
            // ToDo spare evaluate?
            typename SolverContext::result_type child = evaluate(_solver, construct(ee->expr, width_out));

            unsigned ee_width = ee->getWidth();
            *width_out = ee_width;

            if (ee_width == 1) {
                res = bvBoolExtract(child, ee->offset);
            }
            else {
                res = evaluate(_solver,
                               extract(ee->offset + ee_width - 1, ee->offset, child));
            }
            break;
        }

        case Expr::ZExt:
        {
            CastExpr *ce = cast<CastExpr>(e);
            assert(ce);

            int child_width = 0;
            typename SolverContext::result_type child = evaluate(_solver, construct(ce->src, &child_width));

            unsigned ce_width = ce->getWidth();
            *width_out = ce_width;

            if (child_width == 1) {
                res = evaluate(_solver, 
                               metaSMT::logic::Ite(child, bvOne(ce_width), bvZero(ce_width)));			                       
            }
            else {
                res = evaluate(_solver, zero_extend(ce_width - child_width, child));
            }

            // ToDo calculate how many zeros to add
            // Note: STP and metaSMT differ in the prototype arguments;
            // STP requires the width of the resulting bv;
            // whereas metaSMT (and Z3) require the width of the zero vector that is to be appended
            // res = evaluate(_solver, zero_extend(ce_width, construct(ce->src)));
    
            break;	  
        }
  
        case Expr::SExt:
        {
            CastExpr *ce = cast<CastExpr>(e);
            assert(ce);
    
            int child_width = 0;
            typename SolverContext::result_type child = evaluate(_solver, construct(ce->src, &child_width));
            
            unsigned ce_width = ce->getWidth();
            *width_out = ce_width;
    
            if (child_width == 1) {
                res = evaluate(_solver, 
                               metaSMT::logic::Ite(child, bvMinusOne(ce_width), bvZero(ce_width)));
            }
            else {
                // ToDo ce_width - child_width? It is only ce_width in STPBuilder
                res = evaluate(_solver, sign_extend(ce_width - child_width, child));
            }

            break;
        }
  
        case Expr::Add:
        {
            AddExpr *ae = cast<AddExpr>(e);
            assert(ae);	    
            res = evaluate(_solver, bvadd(construct(ae->left, width_out), construct(ae->right, width_out)));
            assert(*width_out != 1 && "uncanonicalized add");
            break;	  
        }  
  
        case Expr::Sub:  
        {
            SubExpr *se = cast<SubExpr>(e);
            assert(se);	    
            res = evaluate(_solver, bvsub(construct(se->left, width_out), construct(se->right, width_out))); 
            assert(*width_out != 1 && "uncanonicalized sub");
            break;	  
        }  
   
        case Expr::Mul:
        { 
            MulExpr *me = cast<MulExpr>(e);
            assert(me);
    
            typename SolverContext::result_type right_expr = evaluate(_solver, construct(me->right, width_out));
            assert(*width_out != 1 && "uncanonicalized mul");
    
            ConstantExpr *CE = dyn_cast<ConstantExpr>(me->left);
            if (CE && (CE->getWidth() <= 64)) {	        		
                res = constructMulByConstant(right_expr, *width_out, CE->getZExtValue());		
            }
            else {
                res = evaluate(_solver, bvmul(construct(me->left, width_out), right_expr));	     
            }
            break;
        }

        case Expr::UDiv:
        {
            UDivExpr *de = cast<UDivExpr>(e);
            assert(de);

            typename SolverContext::result_type left_expr = construct(de->left, width_out);
            assert(*width_out != 1 && "uncanonicalized udiv");
            bool construct_both = true;

            ConstantExpr *CE = dyn_cast<ConstantExpr>(de->right);
            if (CE && (CE->getWidth() <= 64)) {
                uint64_t divisor = CE->getZExtValue();
                if (bits64::isPowerOfTwo(divisor)) {
                    res = bvRightShift(left_expr, *width_out, bits64::indexOfSingleBit(divisor), getShiftBits(*width_out));
                    construct_both = false;
                }
                else if (_optimizeDivides) {
                    if (*width_out == 32) { //only works for 32-bit division
                        res = constructUDivByConstant(left_expr, *width_out, (uint32_t) divisor);
                        construct_both = false;
                    }
                }
            }

            if (construct_both) {	    
                res = evaluate(_solver, bvudiv(left_expr, construct(de->right, width_out))); 
            }
            break;
        }

        case Expr::SDiv:
        {
            SDivExpr *de = cast<SDivExpr>(e);
            assert(de);

            typename SolverContext::result_type left_expr = construct(de->left, width_out);
            assert(*width_out != 1 && "uncanonicalized sdiv");

            ConstantExpr *CE = dyn_cast<ConstantExpr>(de->right);
            if (CE && _optimizeDivides && (*width_out == 32)) {
                res = constructSDivByConstant(left_expr, *width_out, CE->getZExtValue(32));
            }
            else {
                res = evaluate(_solver, bvsdiv(left_expr, construct(de->right, width_out))); 
            }
            break;
        }

        case Expr::URem:
        {
            URemExpr *de = cast<URemExpr>(e);
            assert(de);

            typename SolverContext::result_type left_expr = construct(de->left, width_out);
            assert(*width_out != 1 && "uncanonicalized urem");
    
            bool construct_both = true;
            ConstantExpr *CE = dyn_cast<ConstantExpr>(de->right);
            if (CE && (CE->getWidth() <= 64)) {
      
                uint64_t divisor = CE->getZExtValue();

                if (bits64::isPowerOfTwo(divisor)) {
  
                    unsigned bits = bits64::indexOfSingleBit(divisor);		  
                    // special case for modding by 1 or else we bvExtract -1:0
                    if (bits == 0) {
                        res = bvZero(*width_out);
                        construct_both = false;
                    }
                    else {
                        res = evaluate(_solver, concat(bvZero(*width_out - bits), 
                                                       bvExtract(left_expr, bits - 1, 0)));
                        construct_both = false;
                    }
                }

                // Use fast division to compute modulo without explicit division for constant divisor.
       
                if (_optimizeDivides && *width_out == 32) { //only works for 32-bit division
                    typename SolverContext::result_type quotient = constructUDivByConstant(left_expr, *width_out, (uint32_t) divisor);
                    typename SolverContext::result_type quot_times_divisor = constructMulByConstant(quotient, *width_out, divisor);
                    res = evaluate(_solver, bvsub(left_expr, quot_times_divisor));                    
                    construct_both = false;
                }
            }
    
            if (construct_both) {
                res = evaluate(_solver, bvurem(left_expr, construct(de->right, width_out)));	      
            }
            break;  
        }

        case Expr::SRem:
        {
            SRemExpr *de = cast<SRemExpr>(e);
            assert(de);
    
            typename SolverContext::result_type left_expr = evaluate(_solver, construct(de->left, width_out));
            typename SolverContext::result_type right_expr = evaluate(_solver, construct(de->right, width_out));
            assert(*width_out != 1 && "uncanonicalized srem");
    
            bool construct_both = true;
    
#if 0 //not faster per first benchmark
            
            if (_optimizeDivides) {
                if (ConstantExpr *cre = de->right->asConstant()) {
                    uint64_t divisor = cre->asUInt64;
    
                    //use fast division to compute modulo without explicit division for constant divisor
                    if( *width_out == 32 ) { //only works for 32-bit division
                       	typename SolverContext::result_type quotient = constructSDivByConstant(left, *width_out, divisor);
                        typename SolverContext::result_type quot_times_divisor = constructMulByConstant(quotient, *width_out, divisor);
                        res = vc_bvMinusExpr( vc, *width_out, left, quot_times_divisor );
                        construct_both = false;
                    }
                }
            }
    
#endif

            if (construct_both) {     
                res = evaluate(_solver, bvsrem(left_expr, right_expr));
            }
            break;
        }

        case Expr::Not:
        {
            NotExpr *ne = cast<NotExpr>(e);
            assert(ne);
    
            typename SolverContext::result_type child = evaluate(_solver, construct(ne->expr, width_out));
            if (*width_out == 1) {
                res = evaluate(_solver, metaSMT::logic::Not(child));
            }
            else {                
                res = evaluate(_solver, bvnot(child));
            }    
            break;	  
        }
  
        case Expr::And:
        {
            AndExpr *ae = cast<AndExpr>(e);
	    assert(ae);
	    
	    typename SolverContext::result_type left_expr = evaluate(_solver, construct(ae->left, width_out));
	    typename SolverContext::result_type right_expr = evaluate(_solver, construct(ae->right, width_out));
	    
	    if (*width_out == 1) {
	        res = evaluate(_solver, metaSMT::logic::And(left_expr, right_expr));
	    }
	    else {
	        res = evaluate(_solver, bvand(left_expr, right_expr));
	    }
	    
	    break;	  
	}
	  
	case Expr::Or:
	{
	    OrExpr *oe = cast<OrExpr>(e);
	    assert(oe);
	    
	    typename SolverContext::result_type left_expr = evaluate(_solver, construct(oe->left, width_out));
	    typename SolverContext::result_type right_expr = evaluate(_solver, construct(oe->right, width_out));
	    
	    if (*width_out == 1) {
	        res = evaluate(_solver, metaSMT::logic::Or(left_expr, right_expr));
	    }
	    else {
	        res = evaluate(_solver, bvor(left_expr, right_expr));
	    }
	    
	    break;	  
	}
	  
	case Expr::Xor:
	{
	    XorExpr *xe = cast<XorExpr>(e);
	    assert(xe);
	    
	    typename SolverContext::result_type left_expr = evaluate(_solver, construct(xe->left, width_out));
	    typename SolverContext::result_type right_expr = evaluate(_solver, construct(xe->right, width_out));
	    
	    if (*width_out == 1) {
	        res = evaluate(_solver, metaSMT::logic::Xor(left_expr, right_expr));
	    }
	    else {
	        res = evaluate(_solver, bvxor(left_expr, right_expr));
	    }
	    
	    break;	  
	}
	  
	case Expr::Shl:  
	{	    
	    ShlExpr *se = cast<ShlExpr>(e);
	    assert(se);
	    
	    typename SolverContext::result_type left_expr = evaluate(_solver, construct(se->left, width_out));
	    assert(*width_out != 1 && "uncanonicalized shl");
	    
	    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(se->right)) {
	        res =  bvLeftShift(left_expr, *width_out, (unsigned) CE->getLimitedValue(), getShiftBits(*width_out));
            }
            else {
                int shiftWidth = 0;
		typename SolverContext::result_type right_expr = evaluate(_solver, construct(se->right, &shiftWidth));
		res = mimic_stp ? bvVarLeftShift(left_expr, right_expr, *width_out) : 
		                  evaluate(_solver, bvshl(left_expr, right_expr));	      
	    }    
	    
	    break;	  
	}
	
	case Expr::LShr:
	{
	    LShrExpr *lse = cast<LShrExpr>(e);
	    assert(lse);
	    
	    typename SolverContext::result_type left_expr = evaluate(_solver, construct(lse->left, width_out));	    
	    assert(*width_out != 1 && "uncanonicalized lshr");
	    
	    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(lse->right)) {
	        res =  bvRightShift(left_expr, *width_out, (unsigned) CE->getLimitedValue(), getShiftBits(*width_out));
	    }
	    else {
                int shiftWidth = 0;
		typename SolverContext::result_type right_expr = evaluate(_solver, construct(lse->right, &shiftWidth));
		res = mimic_stp ? bvVarRightShift(left_expr, right_expr, *width_out) :
				  evaluate(_solver, bvshr(left_expr, right_expr));
	    }
	    
	    break;	  
	}
	  
	case Expr::AShr:  
	{
	    AShrExpr *ase = cast<AShrExpr>(e);
	    assert(ase);
	    
	    typename SolverContext::result_type left_expr = evaluate(_solver, construct(ase->left, width_out));	    
	    assert(*width_out != 1 && "uncanonicalized ashr");
	    
	    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(ase->right)) {
	        unsigned shift = (unsigned) CE->getLimitedValue();
                typename SolverContext::result_type signedBool = bvBoolExtract(left_expr, *width_out - 1);
		res = constructAShrByConstant(left_expr, *width_out, shift, signedBool, getShiftBits(*width_out));	      
	    }
	    else {
                int shiftWidth = 0;
		typename SolverContext::result_type right_expr = evaluate(_solver, construct(ase->right, &shiftWidth));
		res = mimic_stp ? bvVarArithRightShift(left_expr, right_expr, *width_out) :
		                  evaluate(_solver, bvashr(left_expr, right_expr)); 
	    }    
	    
	    break;	  
	}
	  
	case Expr::Eq: 
	{
	     EqExpr *ee = cast<EqExpr>(e);
	     assert(ee);
	     
    	     typename SolverContext::result_type left_expr = evaluate(_solver, construct(ee->left, width_out));
	     typename SolverContext::result_type right_expr = evaluate(_solver, construct(ee->right, width_out));

	     if (*width_out==1) {
	         if (ConstantExpr *CE = dyn_cast<ConstantExpr>(ee->left)) {
		     if (CE->isTrue()) {
		         res = right_expr;  
		     }
		     else {
		         res = evaluate(_solver, metaSMT::logic::Not(right_expr));
		     }
		 }
		 else {
		     res = evaluate(_solver, metaSMT::logic::equal(left_expr, right_expr));
		 }
	     } // end of *width_out == 1
	     else {
	         *width_out = 1;
	         res = evaluate(_solver, metaSMT::logic::equal(left_expr, right_expr));
	     }
	     
	     break;
	}
	
	case Expr::Ult:
	{
	    UltExpr *ue = cast<UltExpr>(e);
	    assert(ue);
	    
	    typename SolverContext::result_type left_expr = evaluate(_solver, construct(ue->left, width_out));
	    typename SolverContext::result_type right_expr = evaluate(_solver, construct(ue->right, width_out));	    
	    
	    assert(*width_out != 1 && "uncanonicalized ult");
	    *width_out = 1;    
	    
	    res = evaluate(_solver, bvult(left_expr, right_expr));
	    break;	  
	}
	  
	case Expr::Ule:
	{
	    UleExpr *ue = cast<UleExpr>(e);
	    assert(ue);
	    
	    typename SolverContext::result_type left_expr = evaluate(_solver, construct(ue->left, width_out));
	    typename SolverContext::result_type right_expr = evaluate(_solver, construct(ue->right, width_out));	
	    
	    assert(*width_out != 1 && "uncanonicalized ule");
	    *width_out = 1;    
	    
	    res = evaluate(_solver, bvule(left_expr, right_expr));
	    break;	  
	}
	  
	case Expr::Slt:
	{
	    SltExpr *se = cast<SltExpr>(e);
	    assert(se);
	    
	    typename SolverContext::result_type left_expr = evaluate(_solver, construct(se->left, width_out));
	    typename SolverContext::result_type right_expr = evaluate(_solver, construct(se->right, width_out));	
	    
	    assert(*width_out != 1 && "uncanonicalized slt");
	    *width_out = 1;
	    
	    res = evaluate(_solver, bvslt(left_expr, right_expr));
	    break;	  
	}
	  
        case Expr::Sle:
        {
            SleExpr *se = cast<SleExpr>(e);
            assert(se);
    
            typename SolverContext::result_type left_expr = evaluate(_solver, construct(se->left, width_out));
            typename SolverContext::result_type right_expr = evaluate(_solver, construct(se->right, width_out));	
    
            assert(*width_out != 1 && "uncanonicalized sle");
            *width_out = 1;    
    
            res = evaluate(_solver, bvsle(left_expr, right_expr));
            break;	  
        }
  
        // unused due to canonicalization
#if 0
        case Expr::Ne:
        case Expr::Ugt:
        case Expr::Uge:
        case Expr::Sgt:
        case Expr::Sge:
#endif  
     
        default:
             assert(false);
             break;      
      
    };
    return res;
}


}  /* end of namespace klee */

#endif /* SUPPORT_METASMT */

#endif /* METASMTBUILDER_H_ */
