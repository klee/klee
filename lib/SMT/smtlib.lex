%{
/*****************************************************************************/
/*!
 * \file smtlib.lex
 * 
 * Author: Clark Barrett
 * 
 * Created: 2005
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

#include <iostream>
#include "SMTParser.h"
#include "smtlib_parser.h"

using namespace klee;
using namespace klee::expr;

extern int smtlib_inputLine;
extern char *smtlibtext;

extern int smtliberror (const char *msg);

static int smtlibinput(std::istream& is, char* buf, int size) {
  int res;
  if(is) {
    // Set the terminator char to 0
    is.getline(buf, size-1, 0);
    // If failbit is set, but eof is not, it means the line simply
    // didn't fit; so we clear the state and keep on reading.
    bool partialStr = is.fail() && !is.eof();
    if(partialStr)
      is.clear();

    for(res = 0; res<size && buf[res] != 0; res++) ;
    if(res == size) smtliberror("Lexer bug: overfilled the buffer");
    if(!partialStr) { // Insert \n into the buffer
      buf[res++] = '\n';
      buf[res] = '\0';
    }
  } else {
    res = YY_NULL;
  }
  return res;
}

// Redefine the input buffer function to read from an istream
#define YY_INPUT(buf,result,max_size) \
  result = smtlibinput(*SMTParser::parserTemp->is, buf, max_size);

int smtlib_bufSize() { return YY_BUF_SIZE; }
YY_BUFFER_STATE smtlib_buf_state() { return YY_CURRENT_BUFFER; }

/* some wrappers for methods that need to refer to a struct.
   These are used by SMTParser. */
void *smtlib_createBuffer(int sz) {
  return (void *)smtlib_create_buffer(NULL, sz);
}
void smtlib_deleteBuffer(void *buf_state) {
  smtlib_delete_buffer((struct yy_buffer_state *)buf_state);
}
void smtlib_switchToBuffer(void *buf_state) {
  smtlib_switch_to_buffer((struct yy_buffer_state *)buf_state);
}
void *smtlib_bufState() {
  return (void *)smtlib_buf_state();
}

void smtlib_setInteractive(bool is_interactive) {
  yy_set_interactive(is_interactive);
}

// File-static (local to this file) variables and functions
static std::string _string_lit;

 static char escapeChar(char c) {
   switch(c) {
   case 'n': return '\n';
   case 't': return '\t';
   default: return c;
   }
 }

// for now, we don't have subranges.  
//
// ".."		{ return DOTDOT_TOK; }
/*OPCHAR	(['!#?\_$&\|\\@])*/

%}

%option noyywrap
%option nounput
%option noreject
%option noyymore
%option yylineno

%x	COMMENT
%x	STRING_LITERAL
%x      USER_VALUE
%s      PAT_MODE

LETTER	([a-zA-Z])
DIGIT	([0-9])
OPCHAR	(['\.\_])
IDCHAR  ({LETTER}|{DIGIT}|{OPCHAR})
%%

[\n]            { SMTParser::parserTemp->lineNum++; }
[ \t\r\f]	{ /* skip whitespace */ }

{DIGIT}+	{ smtliblval.str = new std::string(smtlibtext); return NUMERAL_TOK; }

";"		{ BEGIN COMMENT; }
<COMMENT>"\n"	{ BEGIN INITIAL; /* return to normal mode */ 
                  SMTParser::parserTemp->lineNum++; }
<COMMENT>.	{ /* stay in comment mode */ }

<INITIAL>"\""		{ BEGIN STRING_LITERAL; 
                          _string_lit.erase(_string_lit.begin(),
                                            _string_lit.end()); }
<STRING_LITERAL>"\\".	{ /* escape characters (like \n or \") */
                          _string_lit.insert(_string_lit.end(),
                                             escapeChar(smtlibtext[1])); }
<STRING_LITERAL>"\""	{ BEGIN INITIAL; /* return to normal mode */ 
			  smtliblval.str = new std::string(_string_lit);
                          return STRING_TOK; }
<STRING_LITERAL>.	{ _string_lit.insert(_string_lit.end(),*smtlibtext); }

<INITIAL>":pat"		{ BEGIN PAT_MODE;
                          return PAT_TOK;}
<PAT_MODE>"}"	        { BEGIN INITIAL; 
                          return RCURBRACK_TOK; }
<INITIAL>"{"		{ BEGIN USER_VALUE;
                          _string_lit.erase(_string_lit.begin(),
                                            _string_lit.end()); }
<USER_VALUE>"\\"[{}] { /* escape characters */
                          _string_lit.insert(_string_lit.end(),smtlibtext[1]); }

<USER_VALUE>"}"	        { BEGIN INITIAL; /* return to normal mode */ 
			  smtliblval.str = new std::string(_string_lit);
                          return USER_VAL_TOK; }

<USER_VALUE>"\n"        { _string_lit.insert(_string_lit.end(),'\n');
                          SMTParser::parserTemp->lineNum++; }
<USER_VALUE>.	        { _string_lit.insert(_string_lit.end(),*smtlibtext); }

"BitVec"        { return BITVEC_TOK; }

"true"          { return TRUE_TOK; }
"false"         { return FALSE_TOK; }
"ite"           { return ITE_TOK; }
"not"           { return NOT_TOK; }
"implies"       { return IMPLIES_TOK; }
"if_then_else"  { return IF_THEN_ELSE_TOK; }
"and"           { return AND_TOK; }
"or"            { return OR_TOK; }
"xor"           { return XOR_TOK; }
"iff"           { return IFF_TOK; }
"exists"        { return EXISTS_TOK; }
"forall"        { return FORALL_TOK; }
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
"benchmark"     { return BENCHMARK_TOK; }
"extrasorts"    { return EXTRASORTS_TOK; }
"extrafuns"     { return EXTRAFUNS_TOK; }
"extrapreds"    { return EXTRAPREDS_TOK; }
"language"      { return LANGUAGE_TOK; }
"distinct"      { return DISTINCT_TOK; }
":pattern"      { return PAT_TOK; } 
":"             { return COLON_TOK; }
"\["            { return LBRACKET_TOK; }
"\]"            { return RBRACKET_TOK; }
"{"             { return LCURBRACK_TOK;}
"}"             { return RCURBRACK_TOK;}
"("             { return LPAREN_TOK; }
")"             { return RPAREN_TOK; }
"$"             { return DOLLAR_TOK; }
"?"             { return QUESTION_TOK; }


"bit0"          { return BIT0_TOK; }
"bit1"          { return BIT1_TOK; }

"concat"        { return BVCONCAT_TOK; }
"extract"       { return BVEXTRACT_TOK; }

"bvnot"         { return BVNOT_TOK; }
"bvand"         { return BVAND_TOK; }
"bvor"          { return BVOR_TOK; }
"bvneg"         { return BVNEG_TOK; }
"bvnand"        { return BVNAND_TOK; }
"bvnor"         { return BVNOR_TOK; }
"bvxor"         { return BVXOR_TOK; }
"bvxnor"        { return BVXNOR_TOK; }

"="             { return EQ_TOK; }
"bvcomp"        { return BVCOMP_TOK; }
"bvult"         { return BVULT_TOK; }
"bvule"         { return BVULE_TOK; }
"bvugt"         { return BVUGT_TOK; }
"bvuge"         { return BVUGE_TOK; }
"bvslt"         { return BVSLT_TOK; }
"bvsle"         { return BVSLE_TOK; }
"bvsgt"         { return BVSGT_TOK; }
"bvsge"         { return BVSGE_TOK; }

"bvadd"         { return BVADD_TOK; }
"bvsub"         { return BVSUB_TOK; }
"bvmul"         { return BVMUL_TOK; }
"bvudiv"        { return BVUDIV_TOK; }
"bvurem"        { return BVUREM_TOK; }
"bvsdiv"        { return BVSDIV_TOK; }
"bvsrem"        { return BVSREM_TOK; }
"bvsmod"        { return BVSMOD_TOK; }

"bvshl"         { return BVSHL_TOK; }
"bvlshr"        { return BVLSHR_TOK; }
"bvashr"        { return BVASHR_TOK; }

"repeat"        { return REPEAT_TOK; }
"zero_extend"   { return ZEXT_TOK; }
"sign_extend"   { return SEXT_TOK; }
"rotate_left"   { return ROL_TOK; }
"rotate_right"  { return ROR_TOK; }


"bv"[0-9]+              { smtliblval.str = new std::string(smtlibtext); return BV_TOK; }
"bvbin"[0-1]+	        { smtliblval.str = new std::string(smtlibtext); return BVBIN_TOK; }
"bvhex"[0-9,A-F,a-f]+	{ smtliblval.str = new std::string(smtlibtext); return BVHEX_TOK; }


({LETTER})({IDCHAR})* {smtliblval.str = new std::string(smtlibtext); return SYM_TOK; }

<<EOF>>         { return EOF_TOK; }

. { smtliberror("Illegal input character."); }
%%

