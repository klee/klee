%{
  /********************************************************************
   * AUTHORS: Vijay Ganesh, David L. Dill
   *
   * BEGIN DATE: July, 2006
   *
   * This file is modified version of the CVCL's smtlib.y file. Please
   * see CVCL license below
   ********************************************************************/
  
  /********************************************************************
   *
   * \file smtlib.y
   * 
   * Author: Sergey Berezin, Clark Barrett
   * 
   * Created: Apr 30 2005
   *
   * <hr>
   * Copyright (C) 2004 by the Board of Trustees of Leland Stanford
   * Junior University and by New York University. 
   *
   * License to use, copy, modify, sell and/or distribute this software
   * and its documentation for any purpose is hereby granted without
   * royalty, subject to the terms and conditions defined in the \ref
   * LICENSE file provided with this distribution.  In particular:
   *
   * - The above copyright notice and this permission notice must appear
   * in all copies of the software and related documentation.
   *
   * - THE SOFTWARE IS PROVIDED "AS-IS", WITHOUT ANY WARRANTIES,
   * EXPRESSED OR IMPLIED.  USE IT AT YOUR OWN RISK.
   * 
   * <hr>
   ********************************************************************/
  // -*- c++ -*-

#include "../AST/AST.h"
  using namespace std; 
  
  // Suppress the bogus warning suppression in bison (it generates
  // compile error)
#undef __GNUC_MINOR__
  
  extern char* yytext;
  extern int yylineno;
  
  //int yylineno;

  extern int yylex(void);

  int yyerror(char *s) {
    //yylineno = 0;
    cout << "syntax error: line " << yylineno << "\n" << s << endl;
    cout << "  token: " << yytext << endl;
    BEEV::FatalError("");
    return 1;
  }

  BEEV::ASTNode query;
#define YYLTYPE_IS_TRIVIAL 1
#define YYMAXDEPTH 104857600
#define YYERROR_VERBOSE 1
#define YY_EXIT_FAILURE -1
%}

%union {  
  // FIXME: Why is this not an UNSIGNED int?
  int uintval;			/* for numerals in types. */

  // for BV32 BVCONST 
  unsigned long long ullval;

  struct {
    //stores the indexwidth and valuewidth
    //indexwidth is 0 iff type is bitvector. positive iff type is
    //array, and stores the width of the indexing bitvector
    unsigned int indexwidth;
    //width of the bitvector type
    unsigned int valuewidth;
  } indexvaluewidth;

  //ASTNode,ASTVec
  BEEV::ASTNode *node;
  BEEV::ASTVec *vec;
  std::string *str;
};

%start cmd

%type <indexvaluewidth> sort_symb sort_symbs
%type <node> status
%type <vec> bench_attributes an_formulas

%type <node> benchmark bench_name bench_attribute
%type <node> an_term an_nonbvconst_term an_formula 

%type <node> var fvar logic_name
%type <str> user_value

%token <uintval> NUMERAL_TOK
%token <ullval> BVCONST_TOK
%token <node> BITCONST_TOK
%token <node> FORMID_TOK TERMID_TOK 
%token <str> STRING_TOK
%token <str> USER_VAL_TOK
%token SOURCE_TOK
%token CATEGORY_TOK
%token DIFFICULTY_TOK
%token BITVEC_TOK
%token ARRAY_TOK
%token SELECT_TOK
%token STORE_TOK
%token TRUE_TOK
%token FALSE_TOK
%token NOT_TOK
%token IMPLIES_TOK
%token ITE_TOK
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
%token EQ_TOK
/*BV SPECIFIC TOKENS*/
%token NAND_TOK
%token NOR_TOK
%token NEQ_TOK
%token ASSIGN_TOK
%token BV_TOK
%token BOOLEAN_TOK
%token BVLEFTSHIFT_TOK
%token BVLEFTSHIFT_1_TOK
%token BVRIGHTSHIFT_TOK
%token BVRIGHTSHIFT_1_TOK
%token BVPLUS_TOK
%token BVSUB_TOK
%token BVNOT_TOK //bvneg in CVCL
%token BVMULT_TOK
%token BVDIV_TOK
%token BVMOD_TOK
%token BVNEG_TOK //bvuminus in CVCL
%token BVAND_TOK
%token BVOR_TOK
%token BVXOR_TOK
%token BVNAND_TOK
%token BVNOR_TOK
%token BVXNOR_TOK
%token BVCONCAT_TOK
%token BVLT_TOK
%token BVGT_TOK
%token BVLE_TOK
%token BVGE_TOK
%token BVSLT_TOK
%token BVSGT_TOK
%token BVSLE_TOK
%token BVSGE_TOK
%token BVSX_TOK 
%token BOOLEXTRACT_TOK
%token BOOL_TO_BV_TOK
%token BVEXTRACT_TOK

%left LBRACKET_TOK RBRACKET_TOK

%%

cmd:
    benchmark
    {
      if($1 != NULL) {
	BEEV::globalBeevMgr_for_parser->TopLevelSAT(*$1,query);
	delete $1;
      }
      YYACCEPT;
    }
;

benchmark:
    LPAREN_TOK BENCHMARK_TOK bench_name bench_attributes RPAREN_TOK
    {
      if($4 != NULL){
	if($4->size() > 1) 
	  $$ = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateNode(BEEV::AND,*$4));
	else
	  $$ = new BEEV::ASTNode((*$4)[0]);	  
	delete $4;
      }
      else {
	$$ = NULL;
      }
    }
/*   | EOF_TOK */
/*     { */
/*     } */
;

bench_name:
    FORMID_TOK
    {
    }
;

bench_attributes:
    bench_attribute
    {
      $$ = new BEEV::ASTVec;
      if ($1 != NULL) {
	$$->push_back(*$1);
	BEEV::globalBeevMgr_for_parser->AddAssert(*$1);
	delete $1;
      }
    }
  | bench_attributes bench_attribute
    {
      if ($1 != NULL && $2 != NULL) {
	$1->push_back(*$2);
	BEEV::globalBeevMgr_for_parser->AddAssert(*$2);
	$$ = $1;
	delete $2;
      }
    }
;

bench_attribute:
    COLON_TOK ASSUMPTION_TOK an_formula
    {
      //assumptions are like asserts
      $$ = $3;
    }
  | COLON_TOK FORMULA_TOK an_formula
    {
      //the query
      query = BEEV::globalBeevMgr_for_parser->CreateNode(BEEV::NOT,*$3);
      BEEV::globalBeevMgr_for_parser->AddQuery(query);
      //dummy true
      $$ = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateNode(BEEV::TRUE));
      
      
    }
  | COLON_TOK STATUS_TOK status
    {
      $$ = NULL;
    }
  | COLON_TOK LOGIC_TOK logic_name
    {
      if (!(0 == strcmp($3->GetName(),"QF_UFBV")  ||
            0 == strcmp($3->GetName(),"QF_AUFBV"))) {
	yyerror("Wrong input logic:");
      }
      $$ = NULL;
    }
  | COLON_TOK EXTRAFUNS_TOK LPAREN_TOK var_decls RPAREN_TOK
    {
      $$ = NULL;
    }
  | COLON_TOK EXTRAPREDS_TOK LPAREN_TOK var_decls RPAREN_TOK
    {
      $$ = NULL;
    }
  | annotation 
    {
      $$ = NULL;
    }
;

logic_name:
    FORMID_TOK LBRACKET_TOK NUMERAL_TOK RBRACKET_TOK
    {
      $$ = $1;
    }
  | FORMID_TOK
    {
      $$ = $1;
    }
;

status:
    SAT_TOK { $$ = NULL; }
  | UNSAT_TOK { $$ = NULL; }
  | UNKNOWN_TOK { $$ = NULL; }
;


/* annotations: */
/*     annotation */
/*     { */
/*     } */
/*   | annotations annotation */
/*     { */
/*     } */
/*   ; */
  
annotation:
    attribute 
    {
    }
  | attribute user_value 
    {
    }
;

user_value:
    USER_VAL_TOK
    {
      //cerr << "Printing user_value: " << *$1 << endl;
    }
;

attribute:
    COLON_TOK SOURCE_TOK
    {
    }
   | COLON_TOK CATEGORY_TOK
    {
    }
   | COLON_TOK DIFFICULTY_TOK 
;

sort_symbs:
    sort_symb 
    {
      //a single sort symbol here means either a BitVec or a Boolean
      $$.indexwidth = $1.indexwidth;
      $$.valuewidth = $1.valuewidth;
    }
  | sort_symb sort_symb
    {
      //two sort symbols mean f: type --> type
      $$.indexwidth = $1.valuewidth;
      $$.valuewidth = $2.valuewidth;
    }
;

var_decls:
    var_decl
    {
    }
//  | LPAREN_TOK var_decl RPAREN_TOK
  |
    var_decls var_decl
    {
    }
;

var_decl:
    LPAREN_TOK FORMID_TOK sort_symbs RPAREN_TOK
    {
      BEEV::_parser_symbol_table.insert(*$2);
      //Sort_symbs has the indexwidth/valuewidth. Set those fields in
      //var
      $2->SetIndexWidth($3.indexwidth);
      $2->SetValueWidth($3.valuewidth);
    }
   | LPAREN_TOK FORMID_TOK RPAREN_TOK
    {
      BEEV::_parser_symbol_table.insert(*$2);
      //Sort_symbs has the indexwidth/valuewidth. Set those fields in
      //var
      $2->SetIndexWidth(0);
      $2->SetValueWidth(0);
    }
;

an_formulas:
    an_formula
    {
      $$ = new BEEV::ASTVec;
      if ($1 != NULL) {
	$$->push_back(*$1);
	delete $1;
      }
    }
  |
    an_formulas an_formula
    {
      if ($1 != NULL && $2 != NULL) {
	$1->push_back(*$2);
	$$ = $1;
	delete $2;
      }
    }
;

an_formula:   
    TRUE_TOK
    {
      $$ = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateNode(BEEV::TRUE)); 
      $$->SetIndexWidth(0); 
      $$->SetValueWidth(0);
    }
  | FALSE_TOK
    {
      $$ = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateNode(BEEV::FALSE)); 
      $$->SetIndexWidth(0); 
      $$->SetValueWidth(0);
    }
  | fvar
    {
      $$ = $1;
    }
  | LPAREN_TOK EQ_TOK an_term an_term RPAREN_TOK
  //| LPAREN_TOK EQ_TOK an_term an_term annotations RPAREN_TOK
    {
      BEEV::ASTNode * n = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateSimplifiedEQ(*$3, *$4));
      BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
      $$ = n;
      delete $3;
      delete $4;      
    }
  | LPAREN_TOK BVSLT_TOK an_term an_term RPAREN_TOK
  //| LPAREN_TOK BVSLT_TOK an_term an_term annotations RPAREN_TOK
    {
      BEEV::ASTNode * n = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateNode(BEEV::BVSLT, *$3, *$4));
      BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
      $$ = n;
      delete $3;
      delete $4;      
    }
  | LPAREN_TOK BVSLE_TOK an_term an_term RPAREN_TOK
  //| LPAREN_TOK BVSLE_TOK an_term an_term annotations RPAREN_TOK
    {
      BEEV::ASTNode * n = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateNode(BEEV::BVSLE, *$3, *$4));
      BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
      $$ = n;
      delete $3;
      delete $4;      
    }
  | LPAREN_TOK BVSGT_TOK an_term an_term RPAREN_TOK
  //| LPAREN_TOK BVSGT_TOK an_term an_term annotations RPAREN_TOK
    {
      BEEV::ASTNode * n = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateNode(BEEV::BVSGT, *$3, *$4));
      BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
      $$ = n;
      delete $3;
      delete $4;      
    }
  | LPAREN_TOK BVSGE_TOK an_term an_term RPAREN_TOK
  //| LPAREN_TOK BVSGE_TOK an_term an_term annotations RPAREN_TOK
    {
      BEEV::ASTNode * n = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateNode(BEEV::BVSGE, *$3, *$4));
      BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
      $$ = n;
      delete $3;
      delete $4;      
    }
  | LPAREN_TOK BVLT_TOK an_term an_term RPAREN_TOK
  //| LPAREN_TOK BVLT_TOK an_term an_term annotations RPAREN_TOK
    {
      BEEV::ASTNode * n = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateNode(BEEV::BVLT, *$3, *$4));
      BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
      $$ = n;
      delete $3;
      delete $4;      
    }
  | LPAREN_TOK BVLE_TOK an_term an_term RPAREN_TOK
  //| LPAREN_TOK BVLE_TOK an_term an_term annotations RPAREN_TOK
    {
      BEEV::ASTNode * n = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateNode(BEEV::BVLE, *$3, *$4));
      BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
      $$ = n;
      delete $3;
      delete $4;      
    }
  | LPAREN_TOK BVGT_TOK an_term an_term RPAREN_TOK
  //| LPAREN_TOK BVGT_TOK an_term an_term annotations RPAREN_TOK
    {
      BEEV::ASTNode * n = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateNode(BEEV::BVGT, *$3, *$4));
      BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
      $$ = n;
      delete $3;
      delete $4;      
    }
  | LPAREN_TOK BVGE_TOK an_term an_term RPAREN_TOK
  //| LPAREN_TOK BVGE_TOK an_term an_term annotations RPAREN_TOK
    {
      BEEV::ASTNode * n = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateNode(BEEV::BVGE, *$3, *$4));
      BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
      $$ = n;
      delete $3;
      delete $4;      
    }
  | LPAREN_TOK an_formula RPAREN_TOK
    {
      $$ = $2;
    }
  | LPAREN_TOK NOT_TOK an_formula RPAREN_TOK
    {
      $$ = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateNode(BEEV::NOT, *$3));
      delete $3;
    }
  | LPAREN_TOK IMPLIES_TOK an_formula an_formula RPAREN_TOK
    {
      $$ = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateNode(BEEV::IMPLIES, *$3, *$4));
      delete $3;
      delete $4;
    }
  | LPAREN_TOK IF_THEN_ELSE_TOK an_formula an_formula an_formula RPAREN_TOK
    {
      $$ = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateNode(BEEV::ITE, *$3, *$4, *$5));
      delete $3;
      delete $4;
      delete $5;
    }
  | LPAREN_TOK AND_TOK an_formulas RPAREN_TOK
    {
      $$ = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateNode(BEEV::AND, *$3));
      delete $3;
    }
  | LPAREN_TOK OR_TOK an_formulas RPAREN_TOK
    {
      $$ = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateNode(BEEV::OR, *$3));
      delete $3;
    }
  | LPAREN_TOK XOR_TOK an_formula an_formula RPAREN_TOK
    {
      $$ = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateNode(BEEV::XOR, *$3, *$4));
      delete $3;
      delete $4;
    }
  | LPAREN_TOK IFF_TOK an_formula an_formula RPAREN_TOK
    {
      $$ = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateNode(BEEV::IFF, *$3, *$4));
      delete $3;
      delete $4;
    }
  | letexpr_mgmt an_formula RPAREN_TOK
  //| letexpr_mgmt an_formula annotations RPAREN_TOK
    {
      $$ = $2;
      //Cleanup the LetIDToExprMap
      BEEV::globalBeevMgr_for_parser->CleanupLetIDMap();			 
    }
;

letexpr_mgmt: 
    LPAREN_TOK LET_TOK LPAREN_TOK QUESTION_TOK FORMID_TOK an_term RPAREN_TOK
    {
     //Expr must typecheck
      BEEV::globalBeevMgr_for_parser->BVTypeCheck(*$6);
      
      //set the valuewidth of the identifier
      $5->SetValueWidth($6->GetValueWidth());
      $5->SetIndexWidth($6->GetIndexWidth());
      
      //populate the hashtable from LET-var -->
      //LET-exprs and then process them:
      //
      //1. ensure that LET variables do not clash
      //1. with declared variables.
      //
      //2. Ensure that LET variables are not
      //2. defined more than once
      BEEV::globalBeevMgr_for_parser->LetExprMgr(*$5,*$6);
      delete $5;
      delete $6;      
   }
 | LPAREN_TOK FLET_TOK LPAREN_TOK DOLLAR_TOK FORMID_TOK an_formula RPAREN_TOK 
   {
     //Expr must typecheck
     BEEV::globalBeevMgr_for_parser->BVTypeCheck(*$6);
     
     //set the valuewidth of the identifier
     $5->SetValueWidth($6->GetValueWidth());
     $5->SetIndexWidth($6->GetIndexWidth());
     
     //Do LET-expr management
     BEEV::globalBeevMgr_for_parser->LetExprMgr(*$5,*$6);
     delete $5;
     delete $6;     
   }

/* an_terms: */
/*     an_term */
/*     { */
/*     } */
/*   | an_terms an_term */
/*     { */
/*     } */
/* ; */

an_term:
    BVCONST_TOK
    {
      $$ = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateBVConst(64, $1));
    }
  | BVCONST_TOK LBRACKET_TOK NUMERAL_TOK RBRACKET_TOK
    {
      $$ = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateBVConst($3, $1));
    }
  | an_nonbvconst_term
  ;

an_nonbvconst_term: 
    BITCONST_TOK { $$ = $1; }
  | var
    {
      $$ = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->ResolveID(*$1));
      delete $1;
    }
  | LPAREN_TOK an_term RPAREN_TOK
  //| LPAREN_TOK an_term annotations RPAREN_TOK
    {
      $$ = $2;
      //$$ = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->SimplifyTerm(*$2));
      //delete $2;
    }
  | LPAREN_TOK TERMID_TOK an_term RPAREN_TOK
    {
      //ARRAY READ
      // valuewidth is same as array, indexwidth is 0.
      BEEV::ASTNode in = *$2;
      BEEV::ASTNode m = *$3;
      unsigned int width = in.GetValueWidth();
      BEEV::ASTNode * n = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::READ, width, in, m));
      BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
      $$ = n;
      delete $2;
      delete $3;
    }
  | SELECT_TOK an_term an_term
    {
      //ARRAY READ
      // valuewidth is same as array, indexwidth is 0.
      BEEV::ASTNode array = *$2;
      BEEV::ASTNode index = *$3;
      unsigned int width = array.GetValueWidth();
      BEEV::ASTNode * n = 
	new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::READ, width, array, index));
      BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
      $$ = n;
      delete $2;
      delete $3;
    }
  | STORE_TOK an_term an_term an_term
    {
      //ARRAY WRITE
      unsigned int width = $4->GetValueWidth();
      BEEV::ASTNode array = *$2;
      BEEV::ASTNode index = *$3;
      BEEV::ASTNode writeval = *$4;
      BEEV::ASTNode write_term = BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::WRITE,width,array,index,writeval);
      write_term.SetIndexWidth($2->GetIndexWidth());
      BEEV::globalBeevMgr_for_parser->BVTypeCheck(write_term);
      BEEV::ASTNode * n = new BEEV::ASTNode(write_term);
      $$ = n;
      delete $2;
      delete $3;
      delete $4;
    }
/*   | BVEXTRACT_TOK LBRACKET_TOK NUMERAL_TOK COLON_TOK NUMERAL_TOK RBRACKET_TOK BVCONST_TOK */
/*     { */
/*       // This special case is when we have an extract on top of a constant bv, which happens */
/*       // almost all the time in the smt syntax. */

/*       // $3 is high, $5 is low.  They are ints (why not unsigned? See uintval) */
/*       int hi  =  $3; */
/*       int low =  $5; */
/*       int width = hi - low + 1; */

/*       if (width < 0) */
/* 	yyerror("Negative width in extract"); */

/*       unsigned long long int val = $7; */

/*       // cut and past from BV32 const evaluator */

/*       unsigned long long int mask1 = 0xffffffffffffffffLL; */

/*       mask1 >>= 64-(hi+1); */
      
/*       //extract val[hi:0] */
/*       val &= mask1; */
/*       //extract val[hi:low] */
/*       val >>= low; */

/*       // val is the desired BV32. */
/*       BEEV::ASTNode * n = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateBVConst(width, val)); */
/*       $$ = n; */
/*     } */
  | BVEXTRACT_TOK LBRACKET_TOK NUMERAL_TOK COLON_TOK NUMERAL_TOK RBRACKET_TOK an_term
    {
      int width = $3 - $5 + 1;
      if (width < 0)
	yyerror("Negative width in extract");
      
      if((unsigned)$3 >= $7->GetValueWidth() || (unsigned)$5 < 0)
	yyerror("Parsing: Wrong width in BVEXTRACT\n");			 
      
      BEEV::ASTNode hi  =  BEEV::globalBeevMgr_for_parser->CreateBVConst(32, $3);
      BEEV::ASTNode low =  BEEV::globalBeevMgr_for_parser->CreateBVConst(32, $5);
      BEEV::ASTNode output = BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::BVEXTRACT, width, *$7,hi,low);
      BEEV::ASTNode * n = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->SimplifyTerm(output));
      BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
      $$ = n;
      delete $7;
    }
  |  ITE_TOK an_formula an_term an_term 
    {
      unsigned int width = $3->GetValueWidth();
      if (width != $4->GetValueWidth()) {
	cerr << *$3;
	cerr << *$4;
	yyerror("Width mismatch in IF-THEN-ELSE");
      }			 
      if($3->GetIndexWidth() != $4->GetIndexWidth())
	yyerror("Width mismatch in IF-THEN-ELSE");
      
      BEEV::globalBeevMgr_for_parser->BVTypeCheck(*$2);
      BEEV::globalBeevMgr_for_parser->BVTypeCheck(*$3);
      BEEV::globalBeevMgr_for_parser->BVTypeCheck(*$4);
      $$ = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateSimplifiedTermITE(*$2, *$3, *$4));
      //$$ = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::ITE,width,*$2, *$3, *$4));
      $$->SetIndexWidth($4->GetIndexWidth());
      BEEV::globalBeevMgr_for_parser->BVTypeCheck(*$$);
      delete $2;
      delete $3;
      delete $4;
    }
  |  BVCONCAT_TOK an_term an_term 
    {
      unsigned int width = $2->GetValueWidth() + $3->GetValueWidth();
      BEEV::ASTNode * n = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::BVCONCAT, width, *$2, *$3));
      BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
      $$ = n;
      
      delete $2;
      delete $3;
    }
  |  BVNOT_TOK an_term
    {
      //this is the BVNEG (term) in the CVCL language
      unsigned int width = $2->GetValueWidth();
      BEEV::ASTNode * n = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::BVNEG, width, *$2));
      BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
      $$ = n;
      delete $2;
    }
  |  BVNEG_TOK an_term
    {
      //this is the BVUMINUS term in CVCL langauge
      unsigned width = $2->GetValueWidth();
      BEEV::ASTNode * n =  new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::BVUMINUS,width,*$2));
      BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
      $$ = n;
      delete $2;
    }
  |  BVAND_TOK an_term an_term 
    {
      unsigned int width = $2->GetValueWidth();
      if (width != $3->GetValueWidth()) {
	yyerror("Width mismatch in AND");
      }
      BEEV::ASTNode * n = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::BVAND, width, *$2, *$3));
      BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
      $$ = n;
      delete $2;
      delete $3;
    }
  |  BVOR_TOK an_term an_term 
    {
      unsigned int width = $2->GetValueWidth();
      if (width != $3->GetValueWidth()) {
	yyerror("Width mismatch in OR");
      }
      BEEV::ASTNode * n = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::BVOR, width, *$2, *$3)); 
      BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
      $$ = n;
      delete $2;
      delete $3;
    }
  |  BVXOR_TOK an_term an_term 
    {
      unsigned int width = $2->GetValueWidth();
      if (width != $3->GetValueWidth()) {
	yyerror("Width mismatch in XOR");
      }
      BEEV::ASTNode * n = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::BVXOR, width, *$2, *$3));
      BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
      $$ = n;
      delete $2;
      delete $3;
    }
  |  BVSUB_TOK an_term an_term 
    {
      unsigned int width = $2->GetValueWidth();
      if (width != $3->GetValueWidth()) {
	yyerror("Width mismatch in BVSUB");
      }
      BEEV::ASTNode * n = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::BVSUB, width, *$2, *$3));
      BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
      $$ = n;
      delete $2;
      delete $3;
    }
  |  BVPLUS_TOK an_term an_term 
    {
      unsigned int width = $2->GetValueWidth();
      if (width != $3->GetValueWidth()) {
	yyerror("Width mismatch in BVPLUS");
      }
      BEEV::ASTNode * n = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::BVPLUS, width, *$2, *$3));
      BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
      $$ = n;
      delete $2;
      delete $3;

    }
  |  BVMULT_TOK an_term an_term 
    {
      unsigned int width = $2->GetValueWidth();
      if (width != $3->GetValueWidth()) {
	yyerror("Width mismatch in BVMULT");
      }
      BEEV::ASTNode * n = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::BVMULT, width, *$2, *$3));
      BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
      $$ = n;
      delete $2;
      delete $3;
    }
  |  BVNAND_TOK an_term an_term 
    {
      unsigned int width = $2->GetValueWidth();
      if (width != $3->GetValueWidth()) {
	yyerror("Width mismatch in BVNAND");
      }
      BEEV::ASTNode * n = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::BVNAND, width, *$2, *$3));
      BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
      $$ = n;
      delete $2;
      delete $3;
    }
  |  BVNOR_TOK an_term an_term 
    {
      unsigned int width = $2->GetValueWidth();
      if (width != $3->GetValueWidth()) {
	yyerror("Width mismatch in BVNOR");
      }
      BEEV::ASTNode * n = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::BVNOR, width, *$2, *$3)); 
      BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
      $$ = n;
      delete $2;
      delete $3;
    }
  |  BVLEFTSHIFT_TOK an_term NUMERAL_TOK 
    {
      unsigned int w = $2->GetValueWidth();
      if((unsigned)$3 < w) {
	BEEV::ASTNode trailing_zeros = BEEV::globalBeevMgr_for_parser->CreateBVConst($3, 0);
	BEEV::ASTNode hi = BEEV::globalBeevMgr_for_parser->CreateBVConst(32, w-$3-1);
	BEEV::ASTNode low = BEEV::globalBeevMgr_for_parser->CreateBVConst(32,0);
	BEEV::ASTNode m = BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::BVEXTRACT,w-$3,*$2,hi,low);
	BEEV::ASTNode * n = 
	  new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::BVCONCAT,w,m,trailing_zeros));
	BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
	$$ = n;
      }
      else
	$$ = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateBVConst(w,0));
      delete $2;
    }
   |  BVLEFTSHIFT_1_TOK an_term an_term 
    {
      unsigned int w = $2->GetValueWidth();
      unsigned int shift_amt = GetUnsignedConst(*$3);
      if((unsigned)shift_amt < w) {
	BEEV::ASTNode trailing_zeros = BEEV::globalBeevMgr_for_parser->CreateBVConst(shift_amt, 0);
	BEEV::ASTNode hi = BEEV::globalBeevMgr_for_parser->CreateBVConst(32, w-shift_amt-1);
	BEEV::ASTNode low = BEEV::globalBeevMgr_for_parser->CreateBVConst(32,0);
	BEEV::ASTNode m = BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::BVEXTRACT,w-shift_amt,*$2,hi,low);
	BEEV::ASTNode * n = 
	  new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::BVCONCAT,w,m,trailing_zeros));
	BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
	$$ = n;
      }
      else {
	$$ = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateBVConst(w,0));
      }
      delete $2;
      //delete $3;
    }
  |  BVRIGHTSHIFT_TOK an_term NUMERAL_TOK 
    {
      BEEV::ASTNode leading_zeros = BEEV::globalBeevMgr_for_parser->CreateBVConst($3, 0);
      unsigned int w = $2->GetValueWidth();
      
      //the amount by which you are rightshifting
      //is less-than/equal-to the length of input
      //bitvector
      if((unsigned)$3 < w) {
	BEEV::ASTNode hi = BEEV::globalBeevMgr_for_parser->CreateBVConst(32,w-1);
	BEEV::ASTNode low = BEEV::globalBeevMgr_for_parser->CreateBVConst(32,$3);
	BEEV::ASTNode extract = BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::BVEXTRACT,w-$3,*$2,hi,low);
	BEEV::ASTNode * n = 
	  new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::BVCONCAT, w,leading_zeros, extract));
	BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
	$$ = n;
      }
      else {
	$$ = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateBVConst(w,0));
      }
      delete $2;
    }
   |  BVRIGHTSHIFT_1_TOK an_term an_term 
    {
      unsigned int shift_amt = GetUnsignedConst(*$3);
      BEEV::ASTNode leading_zeros = BEEV::globalBeevMgr_for_parser->CreateBVConst(shift_amt, 0);
      unsigned int w = $2->GetValueWidth();
      
      //the amount by which you are rightshifting
      //is less-than/equal-to the length of input
      //bitvector
      if((unsigned)shift_amt < w) {
	BEEV::ASTNode hi = BEEV::globalBeevMgr_for_parser->CreateBVConst(32,w-1);
	BEEV::ASTNode low = BEEV::globalBeevMgr_for_parser->CreateBVConst(32,shift_amt);
	BEEV::ASTNode extract = BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::BVEXTRACT,w-shift_amt,*$2,hi,low);
	BEEV::ASTNode * n = 
	  new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::BVCONCAT, w,leading_zeros, extract));
	BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
	$$ = n;
      }
      else {
	$$ = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateBVConst(w,0));
      }
      delete $2;
    }
  |  BVSX_TOK LBRACKET_TOK NUMERAL_TOK RBRACKET_TOK an_term 
    {
      BEEV::globalBeevMgr_for_parser->BVTypeCheck(*$5);
      unsigned w = $5->GetValueWidth() + $3;
      BEEV::ASTNode width = BEEV::globalBeevMgr_for_parser->CreateBVConst(32,w);
      BEEV::ASTNode *n =  new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::BVSX,w,*$5,width));
      BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
      $$ = n;
      delete $5;
    }
;
  
sort_symb:
    BITVEC_TOK LBRACKET_TOK NUMERAL_TOK RBRACKET_TOK
    {
      // Just return BV width.  If sort is BOOL, width is 0.
      // Otherwise, BITVEC[w] returns w. 
      //
      //((indexwidth is 0) && (valuewidth>0)) iff type is BV
      $$.indexwidth = 0;
      unsigned int length = $3;
      if(length > 0) {
	$$.valuewidth = length;
      }
      else {
	BEEV::FatalError("Fatal Error: parsing: BITVECTORS must be of positive length: \n");
      }
    }
   | ARRAY_TOK LBRACKET_TOK NUMERAL_TOK COLON_TOK NUMERAL_TOK RBRACKET_TOK
    {
      unsigned int index_len = $3;
      unsigned int value_len = $5;
      if(index_len > 0) {
	$$.indexwidth = $3;
      }
      else {
	BEEV::FatalError("Fatal Error: parsing: BITVECTORS must be of positive length: \n");
      }

      if(value_len > 0) {
	$$.valuewidth = $5;
      }
      else {
	BEEV::FatalError("Fatal Error: parsing: BITVECTORS must be of positive length: \n");
      }
    }
;

var:
    FORMID_TOK 
    {
      $$ = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->ResolveID(*$1)); 
      delete $1;      
    }
   | TERMID_TOK
    {
      $$ = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->ResolveID(*$1));
      delete $1;
    }
   | QUESTION_TOK TERMID_TOK
    {
      $$ = $2;
    }
;

fvar:
    DOLLAR_TOK FORMID_TOK
    {
      $$ = $2; 
    }
  | FORMID_TOK
    {
      $$ = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->ResolveID(*$1)); 
      delete $1;      
    }   
;
%%
