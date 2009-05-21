%{
  /********************************************************************
   * AUTHORS: Vijay Ganesh, David L. Dill
   *
   * BEGIN DATE: July, 2006
   *
   * This file is modified version of the CVCL's smtlib.lex file. Please
   * see CVCL license below
   ********************************************************************/
  
  /********************************************************************
   * \file smtlib.lex
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
#include <iostream>
#include "../AST/AST.h"
#include "parsePL_defs.h"
  
  extern char *yytext;
  extern int yyerror (char *msg);
  
  // File-static (local to this file) variables and functions
  static std::string _string_lit;
  
  static char escapeChar(char c) {
    switch(c) {
    case 'n': return '\n';
    case 't': return '\t';
    default: return c;
    }
  }      

  extern BEEV::ASTNode SingleBitOne;
  extern BEEV::ASTNode SingleBitZero;

/* Changed for smtlib speedup */
/* bv{DIGIT}+      { yylval.node = new BEEV::ASTNode(BEEV::_bm->CreateBVConst(yytext+2, 10)); return BVCONST_TOK;} */

%}

%option noyywrap
%option nounput
%option noreject
%option noyymore
%option yylineno

%x	COMMENT
%x	STRING_LITERAL
%x      USER_VALUE

LETTER	([a-zA-Z])
DIGIT	([0-9])
OPCHAR	(['\.\_]) 
ANYTHING  ({LETTER}|{DIGIT}|{OPCHAR})

%%
[ \n\t\r\f]	{ /* sk'ip whitespace */ }
{DIGIT}+	{ yylval.uintval = strtoul(yytext, NULL, 10); return NUMERAL_TOK; }


bv{DIGIT}+	{ yylval.ullval = strtoull(yytext+2, NULL, 10); return BVCONST_TOK; }

bit{DIGIT}+     {
  		   char c = yytext[3];
		   if (c == '1') {
		     yylval.node = new BEEV::ASTNode(SingleBitOne);
		   }
		   else {
		     yylval.node = new BEEV::ASTNode(SingleBitZero);
		   }
		   return BITCONST_TOK;
		};


";"		{ BEGIN COMMENT; }
<COMMENT>"\n"	{ BEGIN INITIAL; /* return to normal mode */}
<COMMENT>.	{ /* stay in comment mode */ }

<INITIAL>"\""		{ BEGIN STRING_LITERAL;
                          _string_lit.erase(_string_lit.begin(),
                                            _string_lit.end()); }
<STRING_LITERAL>"\\".	{ /* escape characters (like \n or \") */
                          _string_lit.insert(_string_lit.end(),
                                             escapeChar(yytext[1])); }
<STRING_LITERAL>"\""	{ BEGIN INITIAL; /* return to normal mode */
			  yylval.str = new std::string(_string_lit);
                          return STRING_TOK; }
<STRING_LITERAL>.	{ _string_lit.insert(_string_lit.end(),*yytext); }


<INITIAL>"{"		{ BEGIN USER_VALUE;
                          _string_lit.erase(_string_lit.begin(),
                                            _string_lit.end()); }
<USER_VALUE>"\\"[{}] { /* escape characters */
                          _string_lit.insert(_string_lit.end(),yytext[1]); }

<USER_VALUE>"}"	        { BEGIN INITIAL; /* return to normal mode */
			  yylval.str = new std::string(_string_lit);
                          return USER_VAL_TOK; }
<USER_VALUE>"\n"        { _string_lit.insert(_string_lit.end(),'\n');}
<USER_VALUE>.	        { _string_lit.insert(_string_lit.end(),*yytext); }

"BitVec"        { return BITVEC_TOK;}
"Array"         { return ARRAY_TOK;}
"true"          { return TRUE_TOK; }
"false"         { return FALSE_TOK; }
"not"           { return NOT_TOK; }
"implies"       { return IMPLIES_TOK; }
"ite"           { return ITE_TOK;}
"if_then_else"  { return IF_THEN_ELSE_TOK; }
"and"           { return AND_TOK; }
"or"            { return OR_TOK; }
"xor"           { return XOR_TOK; }
"iff"           { return IFF_TOK; }
"let"           { return LET_TOK; }
"flet"          { return FLET_TOK; }
"notes"         { return NOTES_TOK; }
"cvc_command"   { return CVC_COMMAND_TOK; }
"sorts"         { return SORTS_TOK; }
"funs"          { return FUNS_TOK; }
"preds"         { return PREDS_TOK; }
"extensions"    { return EXTENSIONS_TOK; }
"definition"    { return DEFINITION_TOK; }
"axioms"        { return AXIOMS_TOK; }
"logic"         { return LOGIC_TOK; }
"sat"           { return SAT_TOK; }
"unsat"         { return UNSAT_TOK; }
"unknown"       { return UNKNOWN_TOK; }
"assumption"    { return ASSUMPTION_TOK; }
"formula"       { return FORMULA_TOK; }
"status"        { return STATUS_TOK; }
"difficulty"    { return DIFFICULTY_TOK; }
"benchmark"     { return BENCHMARK_TOK; }
"source"        { return SOURCE_TOK;}
"category"      { return CATEGORY_TOK;} 
"extrasorts"    { return EXTRASORTS_TOK; }
"extrafuns"     { return EXTRAFUNS_TOK; }
"extrapreds"    { return EXTRAPREDS_TOK; }
"language"      { return LANGUAGE_TOK; }
"distinct"      { return DISTINCT_TOK; }
"select"        { return SELECT_TOK; }
"store"         { return STORE_TOK; }
":"             { return COLON_TOK; }
"\["            { return LBRACKET_TOK; }
"\]"            { return RBRACKET_TOK; }
"("             { return LPAREN_TOK; }
")"             { return RPAREN_TOK; }
"$"             { return DOLLAR_TOK; }
"?"             { return QUESTION_TOK; }
"="             {return EQ_TOK;}

"nand"		{ return NAND_TOK;}
"nor"		{ return NOR_TOK;}
"/="		{ return NEQ_TOK; }
 ":="           { return ASSIGN_TOK;}
"shift_left0"   { return BVLEFTSHIFT_TOK;}
"bvshl"         { return BVLEFTSHIFT_1_TOK;}
"shift_right0"  { return BVRIGHTSHIFT_TOK;}
"bvlshr"        { return BVRIGHTSHIFT_1_TOK;}
"bvadd"         { return BVPLUS_TOK;}
"bvsub"         { return BVSUB_TOK;}
"bvnot"         { return BVNOT_TOK;}
"bvmul"         { return BVMULT_TOK;}
"bvdiv"         { return BVDIV_TOK;}
"bvmod"         { return BVMOD_TOK;}
"bvneg"         { return BVNEG_TOK;}
"bvand"         { return BVAND_TOK;}
"bvor"          { return BVOR_TOK;}
"bvxor"         { return BVXOR_TOK;}
"bvnand"        { return BVNAND_TOK;}
"bvnor"         { return BVNOR_TOK;}
"bvxnor"        { return BVXNOR_TOK;}
"concat"        { return BVCONCAT_TOK;}
"extract"       { return BVEXTRACT_TOK;}
"bvlt"          { return BVLT_TOK;}
"bvgt"          { return BVGT_TOK;}
"bvleq"         { return BVLE_TOK;}
"bvgeq"         { return BVGE_TOK;}
"bvult"         { return BVLT_TOK;}
"bvugt"         { return BVGT_TOK;}
"bvuleq"        { return BVLE_TOK;}
"bvugeq"        { return BVGE_TOK;}
"bvule"         { return BVLE_TOK;}
"bvuge"         { return BVGE_TOK;}

"bvslt"         { return BVSLT_TOK;}
"bvsgt"         { return BVSGT_TOK;}
"bvsleq"        { return BVSLE_TOK;}
"bvsgeq"        { return BVSGE_TOK;}
"bvsle"         { return BVSLE_TOK;}
"bvsge"         { return BVSGE_TOK;}

"sign_extend"   { return BVSX_TOK;} 
"boolextract"   { return BOOLEXTRACT_TOK;}
"boolbv"        { return BOOL_TO_BV_TOK;}

(({LETTER})|(_)({ANYTHING}))({ANYTHING})*	{
  BEEV::ASTNode nptr = BEEV::globalBeevMgr_for_parser->CreateSymbol(yytext); 

  // Check valuesize to see if it's a prop var.  I don't like doing
  // type determination in the lexer, but it's easier than rewriting
  // the whole grammar to eliminate the term/formula distinction.  
  yylval.node = new BEEV::ASTNode(BEEV::globalBeevMgr_for_parser->ResolveID(nptr));
  //yylval.node = new BEEV::ASTNode(nptr);
  if ((yylval.node)->GetType() == BEEV::BOOLEAN_TYPE)
    return FORMID_TOK;
  else 
    return TERMID_TOK;  
}
. { yyerror("Illegal input character."); }
%%
