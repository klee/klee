// -*- c++ -*-
#ifndef TESTKINDS_H
#define TESTKINDS_H
// Generated automatically by genkinds.pl from ASTKind.kinds Sun Apr  4 19:39:09 2010.
// Do not edit
namespace BEEV {
  typedef enum {
    UNDEFINED,
    SYMBOL,
    BVCONST,
    BVNEG,
    BVCONCAT,
    BVOR,
    BVAND,
    BVXOR,
    BVNAND,
    BVNOR,
    BVXNOR,
    BVEXTRACT,
    BVLEFTSHIFT,
    BVRIGHTSHIFT,
    BVSRSHIFT,
    BVVARSHIFT,
    BVPLUS,
    BVSUB,
    BVUMINUS,
    BVMULTINVERSE,
    BVMULT,
    BVDIV,
    BVMOD,
    SBVDIV,
    SBVMOD,
    BVSX,
    BOOLVEC,
    ITE,
    BVGETBIT,
    BVLT,
    BVLE,
    BVGT,
    BVGE,
    BVSLT,
    BVSLE,
    BVSGT,
    BVSGE,
    EQ,
    NEQ,
    FALSE,
    TRUE,
    NOT,
    AND,
    OR,
    NAND,
    NOR,
    XOR,
    IFF,
    IMPLIES,
    READ,
    WRITE,
    ARRAY,
    BITVECTOR,
    BOOLEAN
} Kind;

extern unsigned char _kind_categories[];

inline bool is_Term_kind(Kind k) { return (_kind_categories[k] & 1); }

inline bool is_Form_kind(Kind k) { return (_kind_categories[k] & 2); }

extern const char *_kind_names[];

/** Prints symbolic name of kind */
inline ostream& operator<<(ostream &os, const Kind &kind) { os << _kind_names[kind]; return os; }


}  // end namespace


#endif
