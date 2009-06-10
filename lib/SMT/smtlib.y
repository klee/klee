%{
/*****************************************************************************/
/*!
 * \file smtlib.y
 * 
 * Author: Clark Barrett
 * 
 * Created: Apr 30 2005
 *
 * <hr>
 *
 * License to use, copy, modify, sell and/or distribute this software
 * and its documentation for any purpose is hereby granted without
 * royalty, subject to the terms and conditions defined in the \ref
 * LICENSE file provided with this distribution.
 * 
 * <hr>
 * 
 */
/*****************************************************************************/
/* 
   This file contains the bison code for the parser that reads in CVC
   commands in SMT-LIB language.
*/

#include "parser_temp.h"
#include "klee/Expr.h"

using namespace klee;
using namespace klee::expr;

// Exported shared data
namespace CVC3 {
  extern ParserTemp* parserTemp;
}
// Define shortcuts for various things
#define TMP CVC3::parserTemp
#define EXPR CVC3::parserTemp->expr
//#define VC (CVC3::parserTemp->vc)
#define ARRAYSENABLED (CVC3::parserTemp->arrFlag)
#define BVENABLED (CVC3::parserTemp->bvFlag)
#define BVSIZE (CVC3::parserTemp->bvSize)
#define RAT(args) CVC3::newRational args
#define QUERYPARSED CVC3::parserTemp->queryParsed

// Suppress the bogus warning suppression in bison (it generates
// compile error)
#undef __GNUC_MINOR__

/* stuff that lives in smtlib.lex */
extern int smtliblex(void);

int smtliberror(const char *s)
{
  std::ostringstream ss;
  ss << CVC3::parserTemp->fileName << ":" << CVC3::parserTemp->lineNum
     << ": " << s;
  return CVC3::parserTemp->error(ss.str());
}



#define YYLTYPE_IS_TRIVIAL 1
#define YYMAXDEPTH 10485760

%}

/*
%union {
  std::string *str;
  std::vector<std::string> *strvec;
  CVC3::Expr *node;
  std::vector<CVC3::Expr> *vec;
  std::pair<std::vector<CVC3::Expr>, std::vector<std::string> > *pat_ann;
};
*/


%union {
  std::string *str;
  klee::expr::ExprHandle node;
};


%start cmd

/* strings are for better error messages.  
   "_TOK" is so macros don't conflict with kind names */
/*
%type <vec> bench_attributes sort_symbs fun_symb_decls pred_symb_decls
%type <vec> an_formulas quant_vars an_terms fun_symb patterns
%type <node> pattern 
%type <node> benchmark bench_name bench_attribute
%type <node> status fun_symb_decl fun_sig pred_symb_decl pred_sig
%type <node> an_formula quant_var an_atom prop_atom
%type <node> an_term basic_term sort_symb pred_symb
%type <node> var fvar
%type <str> logic_name quant_symb connective user_value attribute annotation
%type <strvec> annotations
%type <pat_ann> patterns_annotations
*/
%type <node> benchmark bench_name  bench_attribute  bench_attributes
%type <node> an_formula an_atom prop_atom

%token <str> NUMERAL_TOK
%token <str> SYM_TOK
%token <str> STRING_TOK
%token <str> AR_SYMB
%token <str> USER_VAL_TOK
%token TRUE_TOK
%token FALSE_TOK
%token ITE_TOK
%token NOT_TOK
%token IMPLIES_TOK
%token IF_THEN_ELSE_TOK
%token AND_TOK
%token OR_TOK
%token XOR_TOK
%token IFF_TOK
%token EXISTS_TOK
%token FORALL_TOK
%token LET_TOK
%token FLET_TOK
%token NOTES_TOK
%token CVC_COMMAND_TOK
%token SORTS_TOK
%token FUNS_TOK
%token PREDS_TOK
%token EXTENSIONS_TOK
%token DEFINITION_TOK
%token AXIOMS_TOK
%token LOGIC_TOK
%token COLON_TOK
%token LBRACKET_TOK
%token RBRACKET_TOK
%token LCURBRACK_TOK
%token RCURBRACK_TOK
%token LPAREN_TOK
%token RPAREN_TOK
%token SAT_TOK
%token UNSAT_TOK
%token UNKNOWN_TOK
%token ASSUMPTION_TOK
%token FORMULA_TOK
%token STATUS_TOK
%token BENCHMARK_TOK
%token EXTRASORTS_TOK
%token EXTRAFUNS_TOK
%token EXTRAPREDS_TOK
%token LANGUAGE_TOK
%token DOLLAR_TOK
%token QUESTION_TOK
%token DISTINCT_TOK
%token SEMICOLON_TOK
%token EOF_TOK
%token PAT_TOK
%%

cmd:
    benchmark
    {
      /*
      EXPR = *$1;
      delete $1;
      */
      EXPR = $1;
      YYACCEPT;
    }
;

benchmark:
    LPAREN_TOK BENCHMARK_TOK bench_name bench_attributes RPAREN_TOK
    {
      /*
      if (!QUERYPARSED)
        $4->push_back(CVC3::Expr(VC->listExpr("_CHECKSAT", CVC3::Expr(VC->idExpr("_TRUE_EXPR")))));
      $$ = new CVC3::Expr(VC->listExpr("_SEQ",*$4));
      delete $4;
      */
      $$ = $4;
    }
  | EOF_TOK
    { 
      TMP->done = true;
      //$$ = new CVC3::Expr();
      //$$ = ConstantExpr::create(1, 1);
      $$ = ExprHandle();
    }
;

bench_name:
    SYM_TOK
    {
      $$ = NULL;
      delete $1;
    }
;

bench_attributes:
    bench_attribute
    {
      $$ = $1;
    }
/*
    {
      $$ = new std::vector<CVC3::Expr>;
      if ($1) {
	$$->push_back(*$1);
	delete $1;
      }
    }
  | bench_attributes bench_attribute
    {
      $$ = $1;
      if ($2) {
	$$->push_back(*$2);
	delete $2;
      }
    }
*/
;

bench_attribute:
    COLON_TOK ASSUMPTION_TOK an_formula
    {
      /*
      $$ = new CVC3::Expr(VC->listExpr("_ASSERT", *$3));      
      delete $3;
      */
      $$ = $3;
    }
  | COLON_TOK FORMULA_TOK an_formula 
    {
      /*
      $$ = new CVC3::Expr(VC->listExpr("_CHECKSAT", *$3));
      QUERYPARSED = true;
      delete $3;
      */
      $$ = $3;
    }
/*
  | COLON_TOK STATUS_TOK status 
    {
      $$ = NULL;
    }
  | COLON_TOK LOGIC_TOK logic_name 
    {
      ARRAYSENABLED = false;
      BVENABLED = false;
      if (*$3 == "QF_UF") {
        $$ = new CVC3::Expr(VC->listExpr("_TYPE", VC->idExpr("U")));
      }
      else if (*$3 == "QF_A" ||
               *$3 == "QF_AX") {
        ARRAYSENABLED = true;
        std::vector<CVC3::Expr> tmp;
        tmp.push_back(VC->listExpr("_TYPE", VC->idExpr("Index")));
        tmp.push_back(VC->listExpr("_TYPE", VC->idExpr("Element")));
        tmp.push_back(VC->listExpr("_TYPEDEF", VC->idExpr("Array"),
                                   VC->listExpr("_ARRAY",
                                                VC->idExpr("Index"),
                                                VC->idExpr("Element"))));
        $$ = new CVC3::Expr(VC->listExpr("_SEQ", tmp));
	$$ = NULL;
      }
      else if (*$3 == "QF_AUFLIA" ||
               *$3 == "AUFLIA") {
        ARRAYSENABLED = true;
        std::vector<CVC3::Expr> tmp;
        tmp.push_back(VC->listExpr("_TYPEDEF", VC->idExpr("Array"),
                                   VC->listExpr("_ARRAY",
                                                VC->idExpr("_INT"),
                                                VC->idExpr("_INT"))));
        $$ = new CVC3::Expr(VC->listExpr("_SEQ", tmp));
      }
      else if (*$3 == "QF_AUFLIRA" ||
               *$3 == "AUFLIRA" ||
               *$3 == "QF_AUFNIRA" ||
               *$3 == "AUFNIRA") {
        ARRAYSENABLED = true;
        std::vector<CVC3::Expr> tmp;
        tmp.push_back(VC->listExpr("_TYPEDEF", VC->idExpr("Array1"),
                                   VC->listExpr("_ARRAY",
                                                VC->idExpr("_INT"),
                                                VC->idExpr("_REAL"))));
        tmp.push_back(VC->listExpr("_TYPEDEF", VC->idExpr("Array2"),
                                   VC->listExpr("_ARRAY",
                                                VC->idExpr("_INT"),
                                                VC->idExpr("Array1"))));
        $$ = new CVC3::Expr(VC->listExpr("_SEQ", tmp));
      }
      else if (*$3 == "QF_AUFBV" ||
               *$3 == "QF_ABV") {
        ARRAYSENABLED = true;
        BVENABLED = true;
        $$ = NULL;
//         $$ = new CVC3::Expr(VC->listExpr("_TYPEDEF", VC->idExpr("Array"),
//                                          VC->listExpr("_ARRAY",
//                                                       VC->listExpr("_BITVECTOR", VC->ratExpr(32)),
//                                                       VC->listExpr("_BITVECTOR", VC->ratExpr(8)))));
      }
      else if (*$3 == "QF_BV" ||
               *$3 == "QF_UFBV") {
        BVENABLED = true;
        $$ = NULL;
      }
    // This enables the new arith for QF_LRA, but this results in assertion failures in DEBUG mode
      else if (*$3 == "QF_LRA") {
         $$ = new CVC3::Expr(VC->listExpr("_OPTION", VC->stringExpr("arith-new"), VC->ratExpr(1)));
       }
      else {
        $$ = NULL;
      }
      delete $3;
    }
  | COLON_TOK EXTRASORTS_TOK LPAREN_TOK sort_symbs RPAREN_TOK
    {
      $$ = new CVC3::Expr(VC->listExpr("_TYPE", *$4));
      delete $4;
    }
  | COLON_TOK EXTRAFUNS_TOK LPAREN_TOK fun_symb_decls RPAREN_TOK
    {
      $$ = new CVC3::Expr(VC->listExpr("_SEQ", *$4));
      delete $4;
    }
  | COLON_TOK EXTRAPREDS_TOK LPAREN_TOK pred_symb_decls RPAREN_TOK
    {
      $$ = new CVC3::Expr(VC->listExpr("_SEQ", *$4));
      delete $4;
    }
  | COLON_TOK NOTES_TOK STRING_TOK
    {
      $$ = NULL;
      delete $3;
    }
  | COLON_TOK CVC_COMMAND_TOK STRING_TOK
    {
      $$ = new CVC3::Expr(VC->listExpr(VC->idExpr("_"+*$3)));
      delete $3;
    }
  | annotation
    {
      $$ = NULL;
      delete $1;
    }
*/
;

/*
logic_name:
    SYM_TOK LBRACKET_TOK NUMERAL_TOK RBRACKET_TOK
    {
      BVSIZE = atoi($3->c_str());
      delete $3;
      $$ = $1;
    }
  | SYM_TOK
    {
      $$ = $1;
    }
;

status:
    SAT_TOK { $$ = NULL; }
  | UNSAT_TOK { $$ = NULL; }
  | UNKNOWN_TOK { $$ = NULL; }
;

sort_symbs:
    sort_symb 
    {
      $$ = new std::vector<CVC3::Expr>;
      $$->push_back(*$1);
      delete $1;
    }
  | sort_symbs sort_symb
    { 
      $1->push_back(*$2);
      $$ = $1;
      delete $2;
    }
;

fun_symb_decls:
    fun_symb_decl
    {
      $$ = new std::vector<CVC3::Expr>;
      $$->push_back(*$1);
      delete $1;
    }
  |
    fun_symb_decls fun_symb_decl
    {
      $1->push_back(*$2);
      $$ = $1;
      delete $2;
    }
;

fun_symb_decl:
    LPAREN_TOK fun_sig annotations RPAREN_TOK
    {
      $$ = $2;
      delete $3;
    }
  | LPAREN_TOK fun_sig RPAREN_TOK
    {
      $$ = $2;
    }
;

fun_sig:
    fun_symb sort_symbs
    {
      if ($2->size() == 1) {
        $$ = new CVC3::Expr(VC->listExpr("_CONST", VC->listExpr(*$1), (*$2)[0]));
      }
      else {
        CVC3::Expr tmp(VC->listExpr("_ARROW", *$2));
        $$ = new CVC3::Expr(VC->listExpr("_CONST", VC->listExpr(*$1), tmp));
      }
      delete $1;
      delete $2;
    }
;

pred_symb_decls:
    pred_symb_decl
    {
      $$ = new std::vector<CVC3::Expr>;
      $$->push_back(*$1);
      delete $1;
    }
  |
    pred_symb_decls pred_symb_decl
    {
      $1->push_back(*$2);
      $$ = $1;
      delete $2;
    }
;

pred_symb_decl:
    LPAREN_TOK pred_sig annotations RPAREN_TOK
    {
      $$ = $2;
      delete $3;
    }
  | LPAREN_TOK pred_sig RPAREN_TOK
    {
      $$ = $2;
    }
;

pred_sig:
    pred_symb sort_symbs
    {
      std::vector<CVC3::Expr> tmp(*$2);
      tmp.push_back(VC->idExpr("_BOOLEAN"));
      CVC3::Expr tmp2(VC->listExpr("_ARROW", tmp));
      $$ = new CVC3::Expr(VC->listExpr("_CONST", VC->listExpr(*$1), tmp2));
      delete $1;
      delete $2;
    }
  | pred_symb
    {
      $$ = new CVC3::Expr(VC->listExpr("_CONST", VC->listExpr(*$1),
                                       VC->idExpr("_BOOLEAN")));
      delete $1;
    }
;

an_formulas:
    an_formula
    {
      $$ = new std::vector<CVC3::Expr>;
      $$->push_back(*$1);
      delete $1;
    }
  |
    an_formulas an_formula
    {
      $1->push_back(*$2);
      $$ = $1;
      delete $2;
    }
;
*/


an_formula:
    an_atom
    {
      $$ = $1;
    }
/*
  | LPAREN_TOK connective an_formulas annotations RPAREN_TOK
    {
      $$ = new CVC3::Expr(VC->listExpr(*$2, *$3));
      delete $2;
      delete $3;
      delete $4;
    }
  | LPAREN_TOK connective an_formulas RPAREN_TOK
    {
      $$ = new CVC3::Expr(VC->listExpr(*$2, *$3));
      delete $2;
      delete $3;
    }
  | LPAREN_TOK quant_symb quant_vars an_formula annotations RPAREN_TOK
    {
      $$ = new CVC3::Expr(VC->listExpr(*$2, VC->listExpr(*$3), *$4));
      delete $2;
      delete $3;
      delete $4;
      delete $5;
    }
  | LPAREN_TOK quant_symb quant_vars an_formula RPAREN_TOK
    {
      $$ = new CVC3::Expr(VC->listExpr(*$2, VC->listExpr(*$3), *$4));
      delete $2;
      delete $3;
      delete $4;
    }
  | LPAREN_TOK quant_symb quant_vars an_formula patterns_annotations RPAREN_TOK
    {
      $$ = new CVC3::Expr(VC->listExpr(*$2, VC->listExpr(*$3), *$4, VC->listExpr((*$5).first)));
      delete $2;
      delete $3;
      delete $4;
      delete $5;
    }
  | LPAREN_TOK LET_TOK LPAREN_TOK var an_term RPAREN_TOK an_formula annotations RPAREN_TOK
    {
      CVC3::Expr e(VC->listExpr(VC->listExpr(*$4, *$5)));
      $$ = new CVC3::Expr(VC->listExpr("_LET", e, *$7));
      delete $4;
      delete $5;
      delete $7;
      delete $8;
    }
  | LPAREN_TOK LET_TOK LPAREN_TOK var an_term RPAREN_TOK an_formula RPAREN_TOK
    {
      CVC3::Expr e(VC->listExpr(VC->listExpr(*$4, *$5)));
      $$ = new CVC3::Expr(VC->listExpr("_LET", e, *$7));
      delete $4;
      delete $5;
      delete $7;
    }
  | LPAREN_TOK FLET_TOK LPAREN_TOK fvar an_formula RPAREN_TOK an_formula annotations RPAREN_TOK
    {
      CVC3::Expr e(VC->listExpr(VC->listExpr(*$4, *$5)));
      $$ = new CVC3::Expr(VC->listExpr("_LET", e, *$7));
      delete $4;
      delete $5;
      delete $7;
      delete $8;
    }
  | LPAREN_TOK FLET_TOK LPAREN_TOK fvar an_formula RPAREN_TOK an_formula RPAREN_TOK
    {
      CVC3::Expr e(VC->listExpr(VC->listExpr(*$4, *$5)));
      $$ = new CVC3::Expr(VC->listExpr("_LET", e, *$7));
      delete $4;
      delete $5;
      delete $7;
    }
*/
;

/*
patterns_annotations:
     patterns
     {
       $$ = new std::pair<std::vector<CVC3::Expr>, std::vector<std::string> >;
       $$->first = *$1;
       delete $1;
     }
     | annotation
     {
       $$ = new std::pair<std::vector<CVC3::Expr>, std::vector<std::string> >;
       $$->second.push_back(*$1);
       delete $1;
     }
     | patterns_annotations patterns
     {
       $1->first.insert($1->first.end(), $2->begin(), $2->end());
       $$ = $1;
       delete $2;
     }
     | patterns_annotations annotation
     {
       $1->second.push_back(*$2);
       $$ = $1;
       delete $2;
     }

patterns:
     pattern
     {
       $$ = new std::vector<CVC3::Expr >;
       $$->push_back(*$1);
       delete $1;
     }
    | patterns pattern
     {
       $1->push_back(*$2);
       $$ = $1;
       delete $2;
     }

pattern:
     PAT_TOK LCURBRACK_TOK an_terms RCURBRACK_TOK
     {
       $$ = new CVC3::Expr(VC->listExpr(*$3));
       delete $3;
     }
     | PAT_TOK LCURBRACK_TOK an_formulas RCURBRACK_TOK
     {
       $$ = new CVC3::Expr(VC->listExpr(*$3));
       delete $3;
     }



quant_vars:
    quant_var
    {
      $$ = new std::vector<CVC3::Expr>;
      $$->push_back(*$1);
      delete $1;
    }
  | quant_vars quant_var
    {
      $1->push_back(*$2);
      $$ = $1; 
      delete $2;
    }
;

quant_var: 
    LPAREN_TOK var sort_symb RPAREN_TOK
    {
      $$ = new CVC3::Expr(VC->listExpr(*$2, *$3));
      delete $2;
      delete $3;
    }
;

quant_symb:
    EXISTS_TOK
    {
      $$ = new std::string("_EXISTS");
    }
  | FORALL_TOK
    {
      $$ = new std::string("_FORALL");
    }
;

connective:
    NOT_TOK
    {
      $$ = new std::string("_NOT");
    }
  | IMPLIES_TOK
    {
      $$ = new std::string("_IMPLIES");
    }
  | IF_THEN_ELSE_TOK
    {
      $$ = new std::string("_IF");
    }
  | AND_TOK
    {
      $$ = new std::string("_AND");
    }
  | OR_TOK
    {
      $$ = new std::string("_OR");
    }
  | XOR_TOK
    {
      $$ = new std::string("_XOR");
    }
  | IFF_TOK
    {
      $$ = new std::string("_IFF");
    }
;
*/

an_atom:
    prop_atom 
    {
      $$ = $1;
    }
/*
  | LPAREN_TOK prop_atom annotations RPAREN_TOK 
    {
      $$ = $2;
      delete $3;
    }
  | LPAREN_TOK pred_symb an_terms annotations RPAREN_TOK
    {
      if ($4->size() == 1 && (*$4)[0] == "transclose" &&
          $3->size() == 2) {
        $$ = new CVC3::Expr(VC->listExpr("_TRANS_CLOSURE",
                                        *$2, (*$3)[0], (*$3)[1]));
      }
      else {
        std::vector<CVC3::Expr> tmp;
        tmp.push_back(*$2);
        tmp.insert(tmp.end(), $3->begin(), $3->end());
        $$ = new CVC3::Expr(VC->listExpr(tmp));
      }
      delete $2;
      delete $3;
      delete $4;
    }
  | LPAREN_TOK pred_symb an_terms RPAREN_TOK
    {
      std::vector<CVC3::Expr> tmp;
      tmp.push_back(*$2);
      tmp.insert(tmp.end(), $3->begin(), $3->end());
      $$ = new CVC3::Expr(VC->listExpr(tmp));
      delete $2;
      delete $3;
    }
  | LPAREN_TOK DISTINCT_TOK an_terms annotations RPAREN_TOK
    {
      $$ = new CVC3::Expr(VC->listExpr("_DISTINCT", *$3));

//       std::vector<CVC3::Expr> tmp;
//       tmp.push_back(*$2);
//       tmp.insert(tmp.end(), $3->begin(), $3->end());
//       $$ = new CVC3::Expr(VC->listExpr(tmp));
//       for (unsigned i = 0; i < (*$3).size(); ++i) {
//         tmp.push_back(($3)[i])
// 	for (unsigned j = i+1; j < (*$3).size(); ++j) {
// 	  tmp.push_back(VC->listExpr("_NEQ", (*$3)[i], (*$3)[j]));
// 	}
//       }
//       $$ = new CVC3::Expr(VC->listExpr("_AND", tmp));
      delete $3;
      delete $4;
    }
  | LPAREN_TOK DISTINCT_TOK an_terms RPAREN_TOK
    {
      $$ = new CVC3::Expr(VC->listExpr("_DISTINCT", *$3));
//       std::vector<CVC3::Expr> tmp;
//       for (unsigned i = 0; i < (*$3).size(); ++i) {
// 	for (unsigned j = i+1; j < (*$3).size(); ++j) {
// 	  tmp.push_back(VC->listExpr("_NEQ", (*$3)[i], (*$3)[j]));
// 	}
//       }
//       $$ = new CVC3::Expr(VC->listExpr("_AND", tmp));
      delete $3;
    }
*/
;

prop_atom:
    TRUE_TOK
    {
      //$$ = new CVC3::Expr(VC->idExpr("_TRUE_EXPR"));
      $$ = ConstantExpr::create(1, 1);
    }
  | FALSE_TOK
    { 
      //$$ = new CVC3::Expr(VC->idExpr("_FALSE_EXPR"));
      $$ = ConstantExpr::create(0, 1);;
    }
/*
  | fvar
    {
      $$ = $1;
    }
  | pred_symb 
    {
      $$ = $1;
    }
*/
;  

/*
an_terms:
    an_term
    {
      $$ = new std::vector<CVC3::Expr>;
      $$->push_back(*$1);
      delete $1;
    }
  | an_terms an_term
    {
      $1->push_back(*$2);
      $$ = $1; 
      delete $2;
    }
;

an_term:
    basic_term 
    {
      $$ = $1;
    }
  | LPAREN_TOK basic_term annotations RPAREN_TOK 
    {
      $$ = $2;
      delete $3;
    }
  | LPAREN_TOK fun_symb an_terms annotations RPAREN_TOK
    {
      $2->insert($2->end(), $3->begin(), $3->end());
      $$ = new CVC3::Expr(VC->listExpr(*$2));
      delete $2;
      delete $3;
      delete $4;
    }
  | LPAREN_TOK fun_symb an_terms RPAREN_TOK
    {
      $2->insert($2->end(), $3->begin(), $3->end());
      $$ = new CVC3::Expr(VC->listExpr(*$2));
      delete $2;
      delete $3;
    }
  | LPAREN_TOK ITE_TOK an_formula an_term an_term annotations RPAREN_TOK
    {
      $$ = new CVC3::Expr(VC->listExpr("_IF", *$3, *$4, *$5));
      delete $3;
      delete $4;
      delete $5;
      delete $6;
    }
  | LPAREN_TOK ITE_TOK an_formula an_term an_term RPAREN_TOK
    {
      $$ = new CVC3::Expr(VC->listExpr("_IF", *$3, *$4, *$5));
      delete $3;
      delete $4;
      delete $5;
    }
;

basic_term:
  | TRUE_TOK
    {
      $$ = new CVC3::Expr(VC->idExpr("_TRUE_EXPR"));
    }
  | FALSE_TOK
    { 
      $$ = new CVC3::Expr(VC->idExpr("_FALSE_EXPR"));
    }
  | fvar
    {
      $$ = $1;
    }
  | var
    {
      $$ = $1;
    }
  | fun_symb 
    {
      if ($1->size() == 1) {
        $$ = new CVC3::Expr(((*$1)[0]));
      }
      else {
        $$ = new CVC3::Expr(VC->listExpr(*$1));
      }
      delete $1;
    }
;

annotations:
    annotation
    {
      $$ = new std::vector<std::string>;
      $$->push_back(*$1);
      delete $1;
    }
  | annotations annotation
    {
      $1->push_back(*$2);
      $$ = $1;
      delete $2;
    }
  ;
  

annotation:
    attribute
    { $$ = $1; }
  | attribute user_value
    { $$ = $1; }
;

user_value:
    USER_VAL_TOK
    {
      $$ = NULL;
      delete $1;
    }
;


sort_symb:
    SYM_TOK LBRACKET_TOK NUMERAL_TOK RBRACKET_TOK
    {
      if (BVENABLED && *$1 == "BitVec") {
        $$ = new CVC3::Expr(VC->listExpr("_BITVECTOR", VC->ratExpr(*$3)));
      }
      else {
        $$ = new CVC3::Expr(VC->listExpr(*$1, VC->ratExpr(*$3)));
      }
      delete $1;
      delete $3;
    }
  | SYM_TOK LBRACKET_TOK NUMERAL_TOK COLON_TOK NUMERAL_TOK RBRACKET_TOK
    {
      if (BVENABLED && ARRAYSENABLED && *$1 == "Array") {
        $$ = new CVC3::Expr(VC->listExpr("_ARRAY",
                                         VC->listExpr("_BITVECTOR", VC->ratExpr(*$3)),
                                         VC->listExpr("_BITVECTOR", VC->ratExpr(*$5))));
      }
      else {
        $$ = new CVC3::Expr(VC->listExpr(*$1, VC->ratExpr(*$3), VC->ratExpr(*$5)));
      }
      delete $1;
      delete $3;
      delete $5;
    }
  | SYM_TOK 
    { 
      if (*$1 == "Real") {
	$$ = new CVC3::Expr(VC->idExpr("_REAL"));
      } else if (*$1 == "Int") {
	$$ = new CVC3::Expr(VC->idExpr("_INT"));
      } else {
	$$ = new CVC3::Expr(VC->idExpr(*$1));
      }
      delete $1;
    }
;

pred_symb:
    SYM_TOK
    {
      if (BVENABLED && (*$1 == "bvlt" || *$1 == "bvult")) {
        $$ = new CVC3::Expr(VC->idExpr("_BVLT"));
      }
      else if (BVENABLED && (*$1 == "bvleq" || *$1 == "bvule")) {
        $$ = new CVC3::Expr(VC->idExpr("_BVLE"));
      }
      else if (BVENABLED && (*$1 == "bvgeq" || *$1 == "bvuge")) {
        $$ = new CVC3::Expr(VC->idExpr("_BVGE"));
      }
      else if (BVENABLED && (*$1 == "bvgt" || *$1 == "bvugt")) {
        $$ = new CVC3::Expr(VC->idExpr("_BVGT"));
      }
      else if (BVENABLED && *$1 == "bvslt") {
        $$ = new CVC3::Expr(VC->idExpr("_BVSLT"));
      }
      else if (BVENABLED && (*$1 == "bvsleq" || *$1 == "bvsle")) {
        $$ = new CVC3::Expr(VC->idExpr("_BVSLE"));
      }
      else if (BVENABLED && (*$1 == "bvsgeq" || *$1 == "bvsge")) {
        $$ = new CVC3::Expr(VC->idExpr("_BVSGE"));
      }
      else if (BVENABLED && *$1 == "bvsgt") {
        $$ = new CVC3::Expr(VC->idExpr("_BVSGT"));
      }
      else {
        $$ = new CVC3::Expr(VC->idExpr(*$1));
      }
      delete $1;
    }
  | AR_SYMB 
    { 
      if ($1->length() == 1) {
	switch ((*$1)[0]) {
	  case '=': $$ = new CVC3::Expr(VC->idExpr("_EQ")); break;
	  case '<': $$ = new CVC3::Expr(VC->idExpr("_LT")); break;
	  case '>': $$ = new CVC3::Expr(VC->idExpr("_GT")); break;
	  default: $$ = new CVC3::Expr(VC->idExpr(*$1)); break;
	}
      }
      else {
	if (*$1 == "<=") {
	  $$ = new CVC3::Expr(VC->idExpr("_LE"));
	} else if (*$1 == ">=") {
	  $$ = new CVC3::Expr(VC->idExpr("_GE"));
	}
	else $$ = new CVC3::Expr(VC->idExpr(*$1));
      }
      delete $1;
    }
;

fun_symb:
    SYM_TOK LBRACKET_TOK NUMERAL_TOK RBRACKET_TOK
    {
      $$ = new std::vector<CVC3::Expr>;
      if (BVENABLED && *$1 == "repeat") {
        $$->push_back(VC->idExpr("_BVREPEAT"));
      }
      else if (BVENABLED && *$1 == "zero_extend") {
        $$->push_back(VC->idExpr("_BVZEROEXTEND"));
      }
      else if (BVENABLED && *$1 == "sign_extend") {
        $$->push_back(VC->idExpr("_SX"));
        $$->push_back(VC->idExpr("_smtlib"));
      }
      else if (BVENABLED && *$1 == "rotate_left") {
        $$->push_back(VC->idExpr("_BVROTL"));
      }
      else if (BVENABLED && *$1 == "rotate_right") {
        $$->push_back(VC->idExpr("_BVROTR"));
      }
      else if (BVENABLED &&
               $1->size() > 2 &&
               (*$1)[0] == 'b' &&
               (*$1)[1] == 'v') {
        int i = 2;
        while ((*$1)[i] >= '0' && (*$1)[i] <= '9') ++i;
        if ((*$1)[i] == '\0') {
          $$->push_back(VC->idExpr("_BVCONST"));
          $$->push_back(VC->ratExpr($1->substr(2), 10));
        }
        else $$->push_back(VC->idExpr(*$1));
      }
      else $$->push_back(VC->idExpr(*$1));
      $$->push_back(VC->ratExpr(*$3));
      delete $1;
      delete $3;
    }
  | SYM_TOK LBRACKET_TOK NUMERAL_TOK COLON_TOK NUMERAL_TOK RBRACKET_TOK
    {
      $$ = new std::vector<CVC3::Expr>;
      if (BVENABLED && *$1 == "extract") {
        $$->push_back(VC->idExpr("_EXTRACT"));
      }
      else $$->push_back(VC->idExpr(*$1));
      $$->push_back(VC->ratExpr(*$3));
      $$->push_back(VC->ratExpr(*$5));
      delete $1;
      delete $3;
      delete $5;
    }
  | SYM_TOK
    {
      $$ = new std::vector<CVC3::Expr>;
      if (BVENABLED && (*$1 == "bvlt" || *$1 == "bvult")) {
        $$->push_back(VC->idExpr("_BVLT"));
      }
      else if (BVENABLED && (*$1 == "bvleq" || *$1 == "bvule")) {
        $$->push_back(VC->idExpr("_BVLE"));
      }
      else if (BVENABLED && (*$1 == "bvgeq" || *$1 == "bvuge")) {
        $$->push_back(VC->idExpr("_BVGE"));
      }
      else if (BVENABLED && (*$1 == "bvgt" || *$1 == "bvugt")) {
        $$->push_back(VC->idExpr("_BVGT"));
      }
      else if (BVENABLED && *$1 == "bvslt") {
        $$->push_back(VC->idExpr("_BVSLT"));
      }
      else if (BVENABLED && (*$1 == "bvsleq" || *$1 == "bvsle")) {
        $$->push_back(VC->idExpr("_BVSLE"));
      }
      else if (BVENABLED && (*$1 == "bvsgeq" || *$1 == "bvsge")) {
        $$->push_back(VC->idExpr("_BVSGE"));
      }
      else if (BVENABLED && *$1 == "bvsgt") {
        $$->push_back(VC->idExpr("_BVSGT"));
      }
      else if (ARRAYSENABLED && *$1 == "select") {
        $$->push_back(VC->idExpr("_READ"));
      }
      else if (ARRAYSENABLED && *$1 == "store") {
        $$->push_back(VC->idExpr("_WRITE"));
      }
      else if (BVENABLED && *$1 == "bit0") {
        $$->push_back(VC->idExpr("_BVCONST"));
        $$->push_back(VC->ratExpr(0));
        $$->push_back(VC->ratExpr(1));
      }
      else if (BVENABLED && *$1 == "bit1") {
        $$->push_back(VC->idExpr("_BVCONST"));
        $$->push_back(VC->ratExpr(1));
        $$->push_back(VC->ratExpr(1));
      }
      else if (BVENABLED && *$1 == "concat") {
        $$->push_back(VC->idExpr("_CONCAT"));
      }
      else if (BVENABLED && *$1 == "bvnot") {
        $$->push_back(VC->idExpr("_BVNEG"));
      }
      else if (BVENABLED && *$1 == "bvand") {
        $$->push_back(VC->idExpr("_BVAND"));
      }
      else if (BVENABLED && *$1 == "bvor") {
        $$->push_back(VC->idExpr("_BVOR"));
      }
      else if (BVENABLED && *$1 == "bvneg") {
        $$->push_back(VC->idExpr("_BVUMINUS"));
      }
      else if (BVENABLED && *$1 == "bvadd") {
        $$->push_back(VC->idExpr("_BVPLUS"));
      }
      else if (BVENABLED && *$1 == "bvmul") {
        $$->push_back(VC->idExpr("_BVMULT"));
      }
      else if (BVENABLED && *$1 == "bvudiv") {
        $$->push_back(VC->idExpr("_BVUDIV"));
      }
      else if (BVENABLED && *$1 == "bvurem") {
        $$->push_back(VC->idExpr("_BVUREM"));
      }
      else if (BVENABLED && *$1 == "bvshl") {
        $$->push_back(VC->idExpr("_BVSHL"));
      }
      else if (BVENABLED && *$1 == "bvlshr") {
        $$->push_back(VC->idExpr("_BVLSHR"));
      }

      else if (BVENABLED && *$1 == "bvxor") {
        $$->push_back(VC->idExpr("_BVXOR"));
      }
      else if (BVENABLED && *$1 == "bvxnor") {
        $$->push_back(VC->idExpr("_BVXNOR"));
      }
      else if (BVENABLED && *$1 == "bvcomp") {
        $$->push_back(VC->idExpr("_BVCOMP"));
      }

      else if (BVENABLED && *$1 == "bvsub") {
        $$->push_back(VC->idExpr("_BVSUB"));
      }
      else if (BVENABLED && *$1 == "bvsdiv") {
        $$->push_back(VC->idExpr("_BVSDIV"));
      }
      else if (BVENABLED && *$1 == "bvsrem") {
        $$->push_back(VC->idExpr("_BVSREM"));
      }
      else if (BVENABLED && *$1 == "bvsmod") {
        $$->push_back(VC->idExpr("_BVSMOD"));
      }
      else if (BVENABLED && *$1 == "bvashr") {
        $$->push_back(VC->idExpr("_BVASHR"));
      }

      // For backwards compatibility:
      else if (BVENABLED && *$1 == "shift_left0") {
        $$->push_back(VC->idExpr("_CONST_WIDTH_LEFTSHIFT"));
      }
      else if (BVENABLED && *$1 == "shift_right0") {
        $$->push_back(VC->idExpr("_RIGHTSHIFT"));
      }
      else if (BVENABLED && *$1 == "sign_extend") {
        $$->push_back(VC->idExpr("_SX"));
        $$->push_back(VC->idExpr("_smtlib"));
      }

      // Bitvector constants
      else if (BVENABLED &&
               $1->size() > 2 &&
               (*$1)[0] == 'b' &&
               (*$1)[1] == 'v') {
        bool done = false;
        if ((*$1)[2] >= '0' && (*$1)[2] <= '9') {
          int i = 3;
          while ((*$1)[i] >= '0' && (*$1)[i] <= '9') ++i;
          if ((*$1)[i] == '\0') {
            $$->push_back(VC->idExpr("_BVCONST"));
            $$->push_back(VC->ratExpr($1->substr(2), 10));
            $$->push_back(VC->ratExpr(32));
            done = true;
          }
        }
        else if ($1->size() > 5) {
          std::string s = $1->substr(0,5);
          if (s == "bvbin") {
            int i = 5;
            while ((*$1)[i] >= '0' && (*$1)[i] <= '1') ++i;
            if ((*$1)[i] == '\0') {
              $$->push_back(VC->idExpr("_BVCONST"));
              $$->push_back(VC->ratExpr($1->substr(5), 2));
              $$->push_back(VC->ratExpr(i-5));
              done = true;
            }
          }
          else if (s == "bvhex") {
            int i = 5;
            char c = (*$1)[i];
            while ((c >= '0' && c <= '9') ||
                   (c >= 'a' && c <= 'f') ||
                   (c >= 'A' && c <= 'F')) {
              ++i;
              c =(*$1)[i];
            }
            if ((*$1)[i] == '\0') {
              $$->push_back(VC->idExpr("_BVCONST"));
              $$->push_back(VC->ratExpr($1->substr(5), 16));
              $$->push_back(VC->ratExpr(i-5));
              done = true;
            }
          }
        }
        if (!done) $$->push_back(VC->idExpr(*$1));
      }
      else {
        $$->push_back(VC->idExpr(*$1));
      }
      delete $1;
    }
  | AR_SYMB 
    { 
      $$ = new std::vector<CVC3::Expr>;
      if ($1->length() == 1) {
	switch ((*$1)[0]) {
	case '+': $$->push_back(VC->idExpr("_PLUS")); break;
	case '-': $$->push_back(VC->idExpr("_MINUS")); break;
	case '*': $$->push_back(VC->idExpr("_MULT")); break;
	case '~': $$->push_back(VC->idExpr("_UMINUS")); break;
	case '/': $$->push_back(VC->idExpr("_DIVIDE")); break;
        case '=': $$->push_back(VC->idExpr("_EQ")); break;
        case '<': $$->push_back(VC->idExpr("_LT")); break;
        case '>': $$->push_back(VC->idExpr("_GT")); break;
	default: $$->push_back(VC->idExpr(*$1));
	}
      }
      else {
	if (*$1 == "<=") {
	  $$->push_back(VC->idExpr("_LE"));
	} else if (*$1 == ">=") {
	  $$->push_back(VC->idExpr("_GE"));
	}
	else $$->push_back(VC->idExpr(*$1));
      }
      delete $1;
    }
  | NUMERAL_TOK
    {
      $$ = new std::vector<CVC3::Expr>;
      $$->push_back(VC->ratExpr(*$1));
      delete $1;
    }
;

attribute:
    COLON_TOK SYM_TOK
    {
      $$ = $2;
    }
;

var:
    QUESTION_TOK SYM_TOK
    {
      $$ = new CVC3::Expr(VC->idExpr(*$2));
      delete $2;
    }
;

fvar:
    DOLLAR_TOK SYM_TOK
    {
      $$ = new CVC3::Expr(VC->idExpr(*$2));
      delete $2;
    }
;
*/

%%
