%{
/********************************************************************
 * AUTHORS: Vijay Ganesh, David L. Dill
 *
 * BEGIN DATE: November, 2005
 *
 * LICENSE: Please view LICENSE file in the home dir of this Program
 ********************************************************************/
// -*- c++ -*-

#include "../AST/AST.h"
using namespace std; 

  // Suppress the bogus warning suppression in bison (it generates
  // compile error)
#undef __GNUC_MINOR__

  extern int yylex(void);
  extern char* yytext;
  extern int yylineno;
  int yyerror(const char *s) {
    cout << "syntax error: line " << yylineno << "\n" << s << endl;    
    BEEV::FatalError("");
    return 1;			/* Dill: don't know what it should return */
  };

#define YYLTYPE_IS_TRIVIAL 1
#define YYMAXDEPTH 10485760
#define YYERROR_VERBOSE 1
#define YY_EXIT_FAILURE -1
%}

%union {

  unsigned int uintval;			/* for numerals in types. */
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

  //Hash_Map to hold Array Updates during parse A map from array index
  //to array values. To support the WITH construct
  BEEV::ASTNodeMap * Index_To_UpdateValue;
};

%start cmd

%token	AND_TOK			"AND"
%token	OR_TOK			"OR"
%token	NOT_TOK			"NOT"
%token	XOR_TOK			"XOR"
%token	NAND_TOK		"NAND"
%token	NOR_TOK			"NOR"
%token	IMPLIES_TOK		"=>"
%token	IFF_TOK			"<=>"

%token	IF_TOK			"IF"
%token	THEN_TOK		"THEN"
%token	ELSE_TOK		"ELSE"
%token	ELSIF_TOK		"ELSIF"
%token	END_TOK			"END"
%token	ENDIF_TOK		"ENDIF"
%token	NEQ_TOK			"/="
%token  ASSIGN_TOK              ":="

%token  BV_TOK                  "BV"
%token  BVLEFTSHIFT_TOK         "<<"
%token  BVRIGHTSHIFT_TOK        ">>"
%token  BVPLUS_TOK              "BVPLUS"
%token  BVSUB_TOK               "BVSUB"
%token  BVUMINUS_TOK            "BVUMINUS"
%token  BVMULT_TOK              "BVMULT"

%token  BVDIV_TOK               "BVDIV"
%token  BVMOD_TOK               "BVMOD"
%token  SBVDIV_TOK              "SBVDIV"
%token  SBVMOD_TOK              "SBVMOD"


%token  BVNEG_TOK               "~"
%token  BVAND_TOK               "&"
%token  BVOR_TOK                "|"
%token  BVXOR_TOK               "BVXOR"
%token  BVNAND_TOK              "BVNAND"
%token  BVNOR_TOK               "BVNOR"
%token  BVXNOR_TOK              "BVXNOR"
%token  BVCONCAT_TOK            "@"

%token  BVLT_TOK                "BVLT"
%token  BVGT_TOK                "BVGT"
%token  BVLE_TOK                "BVLE"
%token  BVGE_TOK                "BVGE"

%token  BVSLT_TOK               "BVSLT"
%token  BVSGT_TOK               "BVSGT"
%token  BVSLE_TOK               "BVSLE"
%token  BVSGE_TOK               "BVSGE"
%token  BOOL_TO_BV_TOK          "BOOLBV"
%token  BVSX_TOK                "BVSX"
%token  BOOLEXTRACT_TOK         "BOOLEXTRACT"
%token  ASSERT_TOK              "ASSERT"
%token  QUERY_TOK               "QUERY"

%token  BOOLEAN_TOK             "BOOLEAN"
%token  ARRAY_TOK               "ARRAY"
%token  OF_TOK                  "OF"
%token  WITH_TOK                "WITH"

%token  TRUELIT_TOK             "TRUE"
%token  FALSELIT_TOK            "FALSE"

%token  IN_TOK                  "IN"
%token  LET_TOK                 "LET"
//%token  COUNTEREXAMPLE_TOK      "COUNTEREXAMPLE"
%token  PUSH_TOK                "PUSH"
%token  POP_TOK                 "POP"

%left IN_TOK
%left XOR_TOK
%left IFF_TOK
%right IMPLIES_TOK
%left OR_TOK
%left AND_TOK
%left NAND_TOK
%left NOR_TOK
%left NOT_TOK
%left BVCONCAT_TOK
%left BVOR_TOK
%left BVAND_TOK
%left BVXOR_TOK
%left BVNAND_TOK
%left BVNOR_TOK
%left BVXNOR_TOK
%left BVNEG_TOK
%left BVLEFTSHIFT_TOK BVRIGHTSHIFT_TOK
%left WITH_TOK

%nonassoc '=' NEQ_TOK ASSIGN_TOK
%nonassoc BVLT_TOK BVLE_TOK BVGT_TOK BVGE_TOK
%nonassoc BVUMINUS_TOK BVPLUS_TOK BVSUB_TOK BVSX_TOK
%nonassoc '[' 
%nonassoc '{' '.' '('
%nonassoc BV_TOK

%type <vec>  Exprs FORM_IDs reverseFORM_IDs
%type <vec>  Asserts 
%type <node> Expr Formula IfExpr ElseRestExpr IfForm ElseRestForm Assert Query ArrayUpdateExpr
%type <Index_To_UpdateValue> Updates

%type <indexvaluewidth>  BvType BoolType ArrayType Type 

%token <node> BVCONST_TOK
%token <node> TERMID_TOK FORMID_TOK COUNTEREXAMPLE_TOK
%token <uintval> NUMERAL_TOK

%%

cmd             :      other_cmd
                |      other_cmd counterexample
                ; 

counterexample  :      COUNTEREXAMPLE_TOK ';'
                       {
			 BEEV::print_counterexample = true;			 
			 BEEV::globalBeevMgr_for_parser->PrintCounterExample(true);
		       }                              
                ;

other_cmd       :      other_cmd1
                |      Query 
                       { 
			 BEEV::globalBeevMgr_for_parser->TopLevelSAT(BEEV::globalBeevMgr_for_parser->CreateNode(BEEV::TRUE),*$1); 
			 delete $1;
		       }
                |      VarDecls Query 
                       { 
			 BEEV::globalBeevMgr_for_parser->TopLevelSAT(BEEV::globalBeevMgr_for_parser->CreateNode(BEEV::TRUE),*$2); 
			 delete $2;
		       }
                |      other_cmd1 Query
                       {
			 BEEV::ASTVec aaa = BEEV::globalBeevMgr_for_parser->GetAsserts();
			 if(aaa.size() == 0)
			   yyerror("Fatal Error: parsing:  GetAsserts() call: no assertions: ");
			 if(aaa.size() == 1)
			   BEEV::globalBeevMgr_for_parser->TopLevelSAT(aaa[0],*$2);
			 else  		   
			   BEEV::globalBeevMgr_for_parser->TopLevelSAT(BEEV::globalBeevMgr_for_parser->CreateNode(BEEV::AND,aaa),*$2);
			 delete $2;
		       }
                ;

other_cmd1      :     VarDecls Asserts
                      {
			delete $2;
                      }                 
                |     Asserts
                      {
			delete $1;
		      }
                |     other_cmd1 VarDecls Asserts
                      {
                        delete $3;
                      }
                ;

/* push            :     PUSH_TOK */
/*                       { */
/* 			BEEV::globalBeevMgr_for_parser->Push(); */
/*                       } */
/*                 | */
/*                 ; */

/* pop             :     POP_TOK */
/*                       { */
/* 			BEEV::globalBeevMgr_for_parser->Pop(); */
/*                       } */
/*                 | */
/*                 ; */

Asserts         :      Assert 
                       {
			 $$ = new BEEV::ASTVec;
			 $$->push_back(*$1);
			 BEEV::globalBeevMgr_for_parser->AddAssert(*$1);
			 delete $1;
                       }
                |      Asserts Assert
                       {
			 $1->push_back(*$2);
			 BEEV::globalBeevMgr_for_parser->AddAssert(*$2);
			 $$ = $1;
			 delete $2;
		       }
                ;

Assert          :      ASSERT_TOK Formula ';' { $$ = $2; }                
                ;

Query           :      QUERY_TOK Formula ';' { BEEV::globalBeevMgr_for_parser->AddQuery(*$2); $$ = $2;}
                ; 


/* Grammar for Variable Declaration */
VarDecls	:      VarDecl ';'
                       {
                       }
                |      VarDecls  VarDecl ';'
                       {
		       }
		;

VarDecl		:      FORM_IDs ':' Type 
                       {
			 for(BEEV::ASTVec::iterator i=$1->begin(),iend=$1->end();i!=iend;i++) {
			   BEEV::_parser_symbol_table.insert(*i);
			   i->SetIndexWidth($3.indexwidth);
			   i->SetValueWidth($3.valuewidth);

			   //FIXME: HACK_ATTACK. this vector was hacked into the code to
			   //support a special request by Dawson' group. They want the
			   //counterexample to be printed in the order of variables declared.
			   BEEV::globalBeevMgr_for_parser->_special_print_set.push_back(*i);
			 }
			 delete $1;
		       }
                |      FORM_IDs ':' Type '=' Expr
		       {
			 //do type checking. if doesn't pass then abort
			 BEEV::globalBeevMgr_for_parser->BVTypeCheck(*$5);
			 if($3.indexwidth != $5->GetIndexWidth())
			   yyerror("Fatal Error: parsing: LET Expr: Type check fail: ");
			 if($3.valuewidth != $5->GetValueWidth())
			   yyerror("Fatal Error: parsing: LET Expr: Type check fail: ");
			 
			 for(BEEV::ASTVec::iterator i=$1->begin(),iend=$1->end();i!=iend;i++) {			   
			   //set the valuewidth of the identifier
			   i->SetValueWidth($5->GetValueWidth());
			   i->SetIndexWidth($5->GetIndexWidth());
			   
			   BEEV::globalBeevMgr_for_parser->LetExprMgr(*i,*$5);
			   delete $5;
			 }
		       }
                |      FORM_IDs ':' Type '=' Formula
                       {
			 //do type checking. if doesn't pass then abort
			 BEEV::globalBeevMgr_for_parser->BVTypeCheck(*$5);
			 if($3.indexwidth != $5->GetIndexWidth())
			   yyerror("Fatal Error: parsing: LET Expr: Type check fail: ");
			 if($3.valuewidth != $5->GetValueWidth())
			   yyerror("Fatal Error: parsing: LET Expr: Type check fail: ");
			 
			 for(BEEV::ASTVec::iterator i=$1->begin(),iend=$1->end();i!=iend;i++) {			   
			   //set the valuewidth of the identifier
			   i->SetValueWidth($5->GetValueWidth());
			   i->SetIndexWidth($5->GetIndexWidth());
			   
			   BEEV::globalBeevMgr_for_parser->LetExprMgr(*i,*$5);
			   delete $5;
			 }
		       }                
		;

reverseFORM_IDs  :      FORMID_TOK
                       {
			 $$ = new BEEV::ASTVec;		        
			 $$->push_back(*$1);
			 delete $1;
                       }
                |      FORMID_TOK ',' reverseFORM_IDs
                       {
			 $3->push_back(*$1);
			 $$ = $3;
			 delete $1;
                       }
                ;

FORM_IDs         :     reverseFORM_IDs
                      {
			$$ = new BEEV::ASTVec($1->rbegin(),$1->rend());
			delete $1;
                      }
                ;

/* Grammar for Types */
Type		:      BvType { $$ = $1; }
                |      BoolType { $$ = $1; }
                |      ArrayType { $$ = $1; }
		;		

BvType          :      BV_TOK '(' NUMERAL_TOK ')' 
                       {
                         /*((indexwidth is 0) && (valuewidth>0)) iff type is BV*/
			 $$.indexwidth = 0;
			 unsigned int length = $3;
			 if(length > 0) {
			   $$.valuewidth = length;
			 }
			 else
			  BEEV::FatalError("Fatal Error: parsing: BITVECTORS must be of positive length: \n");
		       }
                ;
BoolType        :      BOOLEAN_TOK
                       {
			 $$.indexwidth = 0;
			 $$.valuewidth = 0;
		       }
                ;
ArrayType       :      ARRAY_TOK BvType OF_TOK BvType
                       {
			 $$.indexwidth = $2.valuewidth;
			 $$.valuewidth = $4.valuewidth;
		       }
                ;

/*Grammar for ITEs which are a type of Term*/
IfExpr	        :      IF_TOK Formula THEN_TOK Expr ElseRestExpr 
                       {
			 unsigned int width = $4->GetValueWidth();
			 if (width != $5->GetValueWidth())
			   yyerror("Width mismatch in IF-THEN-ELSE");			 
			 if($4->GetIndexWidth() != $5->GetIndexWidth())
			   yyerror("Width mismatch in IF-THEN-ELSE");

			 BEEV::globalBeevMgr_for_parser->BVTypeCheck(*$2);
			 BEEV::globalBeevMgr_for_parser->BVTypeCheck(*$4);
			 BEEV::globalBeevMgr_for_parser->BVTypeCheck(*$5);
			 $$ = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::ITE, width, *$2, *$4, *$5));
			 $$->SetIndexWidth($5->GetIndexWidth());
			 BEEV::globalBeevMgr_for_parser->BVTypeCheck(*$$);
			 delete $2;
			 delete $4;
			 delete $5;
		      }
		;

ElseRestExpr	:      ELSE_TOK Expr ENDIF_TOK  { $$ = $2; }
                |      ELSIF_TOK Expr THEN_TOK Expr ElseRestExpr 
                       {
			 unsigned int width = $2->GetValueWidth();
			 if (width != $4->GetValueWidth() || width != $5->GetValueWidth())
			   yyerror("Width mismatch in IF-THEN-ELSE");
			 if ($2->GetIndexWidth() != $4->GetValueWidth() || $2->GetIndexWidth() != $5->GetValueWidth())
			   yyerror("Width mismatch in IF-THEN-ELSE");

			 BEEV::globalBeevMgr_for_parser->BVTypeCheck(*$2);
			 BEEV::globalBeevMgr_for_parser->BVTypeCheck(*$4);
			 BEEV::globalBeevMgr_for_parser->BVTypeCheck(*$5);			
			 $$ = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::ITE, width, *$2, *$4, *$5));
			 $$->SetIndexWidth($5->GetIndexWidth());
			 BEEV::globalBeevMgr_for_parser->BVTypeCheck(*$$);
			 delete $2;
			 delete $4;
			 delete $5;
		       }
		;

/* Grammar for formulas */
Formula		:     '(' Formula ')' { $$ = $2; }
		|      FORMID_TOK {  $$ = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->ResolveID(*$1)); delete $1;}
                |      BOOLEXTRACT_TOK '(' Expr ',' NUMERAL_TOK ')'
                       {
			 unsigned int width = $3->GetValueWidth();
			 if(0 > (unsigned)$5 || width <= (unsigned)$5)
			   yyerror("Fatal Error: BOOLEXTRACT: trying to boolextract a bit which beyond range");
			 
			 BEEV::ASTNode hi  =  BEEV::globalBeevMgr_for_parser->CreateBVConst(32, $5);
			 BEEV::ASTNode low =  BEEV::globalBeevMgr_for_parser->CreateBVConst(32, $5);
			 BEEV::ASTNode * n = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::BVEXTRACT,1,*$3,hi,low));
			 BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
			 BEEV::ASTNode zero = BEEV::globalBeevMgr_for_parser->CreateBVConst(1,0);			 
			 BEEV::ASTNode * out = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateNode(BEEV::EQ,*n,zero));
			 BEEV::globalBeevMgr_for_parser->BVTypeCheck(*out);

			 $$ = out;
			 delete $3;
                       }
                |      Expr '=' Expr 
                       {
			 BEEV::ASTNode * n = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateNode(BEEV::EQ, *$1, *$3));
			 BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
			 $$ = n;
			 delete $1;
			 delete $3;
		       } 
		|      Expr NEQ_TOK Expr 
                       {
			 BEEV::ASTNode * n = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateNode(BEEV::NEQ, *$1, *$3));
			 BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
			 $$ = n;
			 delete $1;
			 delete $3;
		       }
		|      NOT_TOK Formula 
                       {
			 $$ = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateNode(BEEV::NOT, *$2));
			 delete $2;
		       }
		|      Formula OR_TOK Formula %prec OR_TOK 
                       {
			 $$ = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateNode(BEEV::OR, *$1, *$3));
			 delete $1;
			 delete $3;
		       } 
		|      Formula NOR_TOK Formula
                       {
			 $$ = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateNode(BEEV::NOR, *$1, *$3));
			 delete $1;
			 delete $3;
		       } 
		|      Formula AND_TOK Formula %prec AND_TOK 
                       {
			 $$ = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateNode(BEEV::AND, *$1, *$3));
			 delete $1;
			 delete $3;
		       }
		|      Formula NAND_TOK Formula
                       {
			 $$ = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateNode(BEEV::NAND, *$1, *$3));
			 delete $1;
			 delete $3;
		       }
		|      Formula IMPLIES_TOK Formula
                       {
			 $$ = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateNode(BEEV::IMPLIES, *$1, *$3));
			 delete $1;
			 delete $3;
		       }
		|      Formula IFF_TOK Formula
                       {
			 $$ = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateNode(BEEV::IFF, *$1, *$3));
			 delete $1;
			 delete $3;
		       } 
		|      Formula XOR_TOK Formula
                       {
			 $$ = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateNode(BEEV::XOR, *$1, *$3));
			 delete $1;
			 delete $3;
		       } 
	        |      BVLT_TOK '(' Expr ',' Expr ')' 
                       {
			 BEEV::ASTNode * n = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateNode(BEEV::BVLT, *$3, *$5));
			 BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
			 $$ = n;
			 delete $3;
			 delete $5;
		       }
	        |      BVGT_TOK '(' Expr ',' Expr ')' 
                       {
			 BEEV::ASTNode * n = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateNode(BEEV::BVGT, *$3, *$5));
			 BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
			 $$ = n;
			 delete $3;
			 delete $5;
		       }
	        |      BVLE_TOK '(' Expr ',' Expr ')' 
                       {
			 BEEV::ASTNode * n = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateNode(BEEV::BVLE, *$3, *$5));
			 BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
			 $$ = n;
			 delete $3;
			 delete $5;
		       }
	        |      BVGE_TOK '(' Expr ',' Expr ')' 
                       {
			 BEEV::ASTNode * n = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateNode(BEEV::BVGE, *$3, *$5));
			 BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
			 $$ = n;
			 delete $3;
			 delete $5;
		       }
	        |      BVSLT_TOK '(' Expr ',' Expr ')' 
                       {
			 BEEV::ASTNode * n = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateNode(BEEV::BVSLT, *$3, *$5));
			 BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
			 $$ = n;
			 delete $3;
			 delete $5;
		       }
	        |      BVSGT_TOK '(' Expr ',' Expr ')' 
                       {
			 BEEV::ASTNode * n = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateNode(BEEV::BVSGT, *$3, *$5));
			 BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
			 $$ = n;
			 delete $3;
			 delete $5;
		       }
	        |      BVSLE_TOK '(' Expr ',' Expr ')' 
                       {
			 BEEV::ASTNode * n = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateNode(BEEV::BVSLE, *$3, *$5));
			 BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
			 $$ = n;
			 delete $3;
			 delete $5;
		       }
	        |      BVSGE_TOK '(' Expr ',' Expr ')' 
                       {
			 BEEV::ASTNode * n = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateNode(BEEV::BVSGE, *$3, *$5));
			 BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
			 $$ = n;
			 delete $3;
			 delete $5;
		       }
		|      IfForm
		|      TRUELIT_TOK 
                       {
			 $$ = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateNode(BEEV::TRUE)); 
			 $$->SetIndexWidth(0); 
			 $$->SetValueWidth(0);
                       }
		|      FALSELIT_TOK 
                       { 
			 $$ = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateNode(BEEV::FALSE)); 
			 $$->SetIndexWidth(0); 
			 $$->SetValueWidth(0);
		       }

                |      LET_TOK LetDecls IN_TOK Formula
                       {
			 $$ = $4;
			 //Cleanup the LetIDToExprMap
			 BEEV::globalBeevMgr_for_parser->CleanupLetIDMap();
		       }
                ;

/*Grammar for ITEs which are Formulas */
IfForm	        :      IF_TOK Formula THEN_TOK Formula ElseRestForm 
                       {
			 $$ = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateNode(BEEV::ITE, *$2, *$4, *$5));
			 delete $2;
			 delete $4;
			 delete $5;
		      }
		;

ElseRestForm	:      ELSE_TOK Formula ENDIF_TOK  { $$ = $2; }
                |      ELSIF_TOK Formula THEN_TOK Formula ElseRestForm 
                       {
			 $$ = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateNode(BEEV::ITE, *$2, *$4, *$5));
			 delete $2;
			 delete $4;
			 delete $5;
		       }
		;

/*Grammar for a list of expressions*/
Exprs		:      Expr 
                       {
			 $$ = new BEEV::ASTVec;
			 BEEV::globalBeevMgr_for_parser->BVTypeCheck(*$1);
			 $$->push_back(*$1);
			 delete $1;
		       }
                |      Exprs ',' Expr 
                       {
			 $1->push_back(*$3);
			 BEEV::globalBeevMgr_for_parser->BVTypeCheck(*$3);
			 $$ = $1; 
			 delete $3;
		       }
		;

/* Grammar for Expr */
Expr		:      TERMID_TOK { $$ = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->ResolveID(*$1)); delete $1;}
                |      '(' Expr ')' { $$ = $2; }
	        |      BVCONST_TOK { $$ = $1; }
                |      BOOL_TO_BV_TOK '(' Formula ')'		
                       {
			 BEEV::globalBeevMgr_for_parser->BVTypeCheck(*$3);
			 BEEV::ASTNode one = BEEV::globalBeevMgr_for_parser->CreateBVConst(1,1);
			 BEEV::ASTNode zero = BEEV::globalBeevMgr_for_parser->CreateBVConst(1,0);

			 //return ITE(*$3, length(1), 0bin1, 0bin0)
			 $$ = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::ITE,1,*$3,one,zero));
			 delete $3;
                       }
		|      Expr '[' Expr ']' 
                       {			 
			 // valuewidth is same as array, indexwidth is 0.
			 unsigned int width = $1->GetValueWidth();
			 BEEV::ASTNode * n = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::READ, width, *$1, *$3));
			 BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
			 $$ = n;

			 delete $1;
			 delete $3;
		       }
                |      Expr '(' Expr ')' //array read but in the form of a uninterpreted function application
                       {
			 // valuewidth is same as array, indexwidth is 0.
			 unsigned int width = $1->GetValueWidth();
			 BEEV::ASTNode * n = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::READ, width, *$1, *$3));
			 BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
			 $$ = n;

			 delete $1;
			 delete $3;
		       }
	        |      Expr '[' NUMERAL_TOK ':' NUMERAL_TOK ']' 
                       {
			 int width = $3 - $5 + 1;
			 if (width < 0)
			   yyerror("Negative width in extract");
			 
			 if((unsigned)$3 >= $1->GetValueWidth() || (unsigned)$5 < 0)
			   yyerror("Parsing: Wrong width in BVEXTRACT\n");			 

			 BEEV::ASTNode hi  =  BEEV::globalBeevMgr_for_parser->CreateBVConst(32, $3);
			 BEEV::ASTNode low =  BEEV::globalBeevMgr_for_parser->CreateBVConst(32, $5);
			 BEEV::ASTNode * n = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::BVEXTRACT, width, *$1,hi,low));
			 BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
			 $$ = n;
			 delete $1;
		       }
	        |      BVNEG_TOK Expr 
                       {
			 unsigned int width = $2->GetValueWidth();
			 BEEV::ASTNode * n = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::BVNEG, width, *$2));
			 BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
			 $$ = n;
			 delete $2;
		       }
	        |      Expr BVAND_TOK Expr 
                       {
			 unsigned int width = $1->GetValueWidth();
			 if (width != $3->GetValueWidth()) {
			   yyerror("Width mismatch in AND");
			 }
			 BEEV::ASTNode * n = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::BVAND, width, *$1, *$3));
			 BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
			 $$ = n;
			 delete $1;
			 delete $3;
		       }
	        |      Expr BVOR_TOK Expr 
                       {
			 unsigned int width = $1->GetValueWidth();
			 if (width != $3->GetValueWidth()) {
			   yyerror("Width mismatch in OR");
			 }
			 BEEV::ASTNode * n = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::BVOR, width, *$1, *$3)); 
			 BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
			 $$ = n;
			 delete $1;
			 delete $3;
		       }
	        |      BVXOR_TOK '(' Expr ',' Expr ')' 
                       {
			 unsigned int width = $3->GetValueWidth();
			 if (width != $5->GetValueWidth()) {
			   yyerror("Width mismatch in XOR");
			 }
			 BEEV::ASTNode * n = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::BVXOR, width, *$3, *$5));
			 BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
			 $$ = n;
			 delete $3;
			 delete $5;
		       }
	        |      BVNAND_TOK '(' Expr ',' Expr ')' 
                       {
			 unsigned int width = $3->GetValueWidth();
			 if (width != $5->GetValueWidth()) {
			   yyerror("Width mismatch in NAND");
			 }
			 BEEV::ASTNode * n = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::BVNAND, width, *$3, *$5));
			 BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
			 $$ = n;

			 delete $3;
			 delete $5;
		       }
                |      BVNOR_TOK '(' Expr ',' Expr ')' 
                       {
			 unsigned int width = $3->GetValueWidth();
			 if (width != $5->GetValueWidth()) {
			   yyerror("Width mismatch in NOR");
			 }
			 BEEV::ASTNode * n = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::BVNOR, width, *$3, *$5));
			 BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
			 $$ = n;

			 delete $3;
			 delete $5;
		       }
	        |      BVXNOR_TOK '(' Expr ',' Expr ')' 
                       {
			 unsigned int width = $3->GetValueWidth();
			 if (width != $5->GetValueWidth()) {
			   yyerror("Width mismatch in NOR");
			 }
			 BEEV::ASTNode * n = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::BVXNOR, width, *$3, *$5));
			 BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
			 $$ = n;

			 delete $3;
			 delete $5;
		       }
                |      BVSX_TOK '(' Expr ',' NUMERAL_TOK ')' 
                       {
			 //width of the expr which is being sign
			 //extended. $5 is the resulting length of the
			 //signextended expr
			 BEEV::globalBeevMgr_for_parser->BVTypeCheck(*$3);
			 if($3->GetValueWidth() == $5) {
			   $$ = $3;
			 }
			 else {
			   BEEV::ASTNode width = BEEV::globalBeevMgr_for_parser->CreateBVConst(32,$5);
			   BEEV::ASTNode *n =  
			     new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::BVSX, $5,*$3,width));
			   BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
			   $$ = n;
			   delete $3;
			 }
		       }
	        |      Expr BVCONCAT_TOK Expr 
                       {
			 unsigned int width = $1->GetValueWidth() + $3->GetValueWidth();
			 BEEV::ASTNode * n = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::BVCONCAT, width, *$1, *$3));
			 BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
			 $$ = n;
			 
			 delete $1;
			 delete $3;
		       }
                |      Expr BVLEFTSHIFT_TOK NUMERAL_TOK 
                       {
			 BEEV::ASTNode zero_bits = BEEV::globalBeevMgr_for_parser->CreateZeroConst($3);
			 BEEV::ASTNode * n = 
			   new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::BVCONCAT,
											$1->GetValueWidth() + $3, *$1, zero_bits));
			 BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
			 $$ = n;
			 delete $1;
                       }
                |      Expr BVRIGHTSHIFT_TOK NUMERAL_TOK 
                       {
			 BEEV::ASTNode len = BEEV::globalBeevMgr_for_parser->CreateZeroConst($3);
			 unsigned int w = $1->GetValueWidth();

			 //the amount by which you are rightshifting
			 //is less-than/equal-to the length of input
			 //bitvector
			 if((unsigned)$3 < w) {
			   BEEV::ASTNode hi = BEEV::globalBeevMgr_for_parser->CreateBVConst(32,w-1);
			   BEEV::ASTNode low = BEEV::globalBeevMgr_for_parser->CreateBVConst(32,$3);
			   BEEV::ASTNode extract = BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::BVEXTRACT,w-$3,*$1,hi,low);
			   BEEV::ASTNode * n = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::BVCONCAT, w,len, extract));
			   BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
			   $$ = n;
			 } 
			 else
			   $$ = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateZeroConst(w));			 

			 delete $1;
                       }
                |      BVPLUS_TOK '(' NUMERAL_TOK ',' Exprs ')' 
                       {
			 BEEV::ASTNode * n = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::BVPLUS, $3, *$5));
			 BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
			 $$ = n;

			 delete $5;
                       }
                |      BVSUB_TOK '(' NUMERAL_TOK ',' Expr ',' Expr ')' 
                       {
			 BEEV::ASTNode * n = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::BVSUB, $3, *$5, *$7));
			 BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
			 $$ = n;

			 delete $5;
			 delete $7;
                       }
                |      BVUMINUS_TOK '(' Expr ')' 
                       {
			 unsigned width = $3->GetValueWidth();
			 BEEV::ASTNode * n =  new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::BVUMINUS,width,*$3));
			 BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
			 $$ = n;
			 delete $3;
                       }
                |      BVMULT_TOK '(' NUMERAL_TOK ',' Expr ',' Expr ')' 
                       {
			 BEEV::ASTNode * n = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::BVMULT, $3, *$5, *$7));
			 BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
			 $$ = n;

			 delete $5;
			 delete $7;
		       }
                |      BVDIV_TOK '(' NUMERAL_TOK ',' Expr ',' Expr ')' 
                       {
			 BEEV::ASTNode * n = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::BVDIV, $3, *$5, *$7));
			 BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
			 $$ = n;

			 delete $5;
			 delete $7;
		       }
                |      BVMOD_TOK '(' NUMERAL_TOK ',' Expr ',' Expr ')' 
                       {
			 BEEV::ASTNode * n = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::BVMOD, $3, *$5, *$7));
			 BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
			 $$ = n;

			 delete $5;
			 delete $7;
		       }
                |      SBVDIV_TOK '(' NUMERAL_TOK ',' Expr ',' Expr ')' 
                       {
			 BEEV::ASTNode * n = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::SBVDIV, $3, *$5, *$7));
			 BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
			 $$ = n;

			 delete $5;
			 delete $7;
		       }
                |      SBVMOD_TOK '(' NUMERAL_TOK ',' Expr ',' Expr ')' 
                       {
			 BEEV::ASTNode * n = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::SBVMOD, $3, *$5, *$7));
			 BEEV::globalBeevMgr_for_parser->BVTypeCheck(*n);
			 $$ = n;
			 delete $5;
			 delete $7;
		       }        
		|      IfExpr { $$ = $1; }
                |      ArrayUpdateExpr
                |      LET_TOK LetDecls IN_TOK Expr
                       {
			 $$ = $4;
			 //Cleanup the LetIDToExprMap
			 //BEEV::globalBeevMgr_for_parser->CleanupLetIDMap();
		       }
		;

/*Grammar for Array Update Expr*/
ArrayUpdateExpr : Expr WITH_TOK Updates
                  {
		    BEEV::ASTNode * result;
		    unsigned int width = $1->GetValueWidth();

		    BEEV::ASTNodeMap::iterator it = $3->begin();
		    BEEV::ASTNodeMap::iterator itend = $3->end();
		    result = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::WRITE,
							      width,
							      *$1,
							      (*it).first,
							      (*it).second));
		    result->SetIndexWidth($1->GetIndexWidth());
		    BEEV::globalBeevMgr_for_parser->BVTypeCheck(*result);
		    for(it++;it!=itend;it++) {
		      result = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->CreateTerm(BEEV::WRITE,
								width,
								*result,
								(*it).first,
								(*it).second));
		      result->SetIndexWidth($1->GetIndexWidth());
		      BEEV::globalBeevMgr_for_parser->BVTypeCheck(*result);
		    }
		    BEEV::globalBeevMgr_for_parser->BVTypeCheck(*result);
		    $$ = result;
		    delete $3;
                  }
                ;

Updates         : '[' Expr ']' ASSIGN_TOK Expr 
                  {
		    $$ = new BEEV::ASTNodeMap();
		    (*$$)[*$2] = *$5;		    
                  }
                | Updates WITH_TOK '[' Expr ']' ASSIGN_TOK Expr 
                  {		    
		    (*$1)[*$4] = *$7;
                  }
                ;

/*Grammar for LET Expr*/
LetDecls	:	LetDecl 
		|	LetDecls ',' LetDecl 
		;

LetDecl		:	FORMID_TOK '=' Expr 
                        {
			  //Expr must typecheck
			  BEEV::globalBeevMgr_for_parser->BVTypeCheck(*$3);

			  //set the valuewidth of the identifier
			  $1->SetValueWidth($3->GetValueWidth());
			  $1->SetIndexWidth($3->GetIndexWidth());

			  //populate the hashtable from LET-var -->
			  //LET-exprs and then process them:
			  //
			  //1. ensure that LET variables do not clash
			  //1. with declared variables.
			  //
			  //2. Ensure that LET variables are not
			  //2. defined more than once
			  BEEV::globalBeevMgr_for_parser->LetExprMgr(*$1,*$3);
			  delete $1;
			  delete $3;
			}
                |	FORMID_TOK ':' Type '=' Expr
			{
			  //do type checking. if doesn't pass then abort
			  BEEV::globalBeevMgr_for_parser->BVTypeCheck(*$5);
			  
			  if($3.indexwidth != $5->GetIndexWidth())
			    yyerror("Fatal Error: parsing: LET Expr: Type check fail: ");
			  if($3.valuewidth != $5->GetValueWidth())
			    yyerror("Fatal Error: parsing: LET Expr: Type check fail: ");

			  //set the valuewidth of the identifier
			  $1->SetValueWidth($5->GetValueWidth());
			  $1->SetIndexWidth($5->GetIndexWidth());

			  BEEV::globalBeevMgr_for_parser->LetExprMgr(*$1,*$5);
			  delete $1;
			  delete $5;
			}
                |       FORMID_TOK '=' Formula
                        {
			  //Expr must typecheck
			  BEEV::globalBeevMgr_for_parser->BVTypeCheck(*$3);

			  //set the valuewidth of the identifier
			  $1->SetValueWidth($3->GetValueWidth());
			  $1->SetIndexWidth($3->GetIndexWidth());

			  //Do LET-expr management
			  BEEV::globalBeevMgr_for_parser->LetExprMgr(*$1,*$3);
			  delete $1;
			  delete $3;
			}
                |	FORMID_TOK ':' Type '=' Formula
			{
			  //do type checking. if doesn't pass then abort
			  BEEV::globalBeevMgr_for_parser->BVTypeCheck(*$5);

			  if($3.indexwidth != $5->GetIndexWidth())
			    yyerror("Fatal Error: parsing: LET Expr: Type check fail: ");
			  if($3.valuewidth != $5->GetValueWidth())
			    yyerror("Fatal Error: parsing: LET Expr: Type check fail: ");

			  //set the valuewidth of the identifier
			  $1->SetValueWidth($5->GetValueWidth());
			  $1->SetIndexWidth($5->GetIndexWidth());

			  //Do LET-expr management
			  BEEV::globalBeevMgr_for_parser->LetExprMgr(*$1,*$5);
			  delete $1;
			  delete $5;
			}                
		;

%%
