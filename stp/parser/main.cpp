/********************************************************************
 * AUTHORS: Vijay Ganesh, David L. Dill
 *
 * BEGIN DATE: November, 2005
 *
 * LICENSE: Please view LICENSE file in the home dir of this Program
 ********************************************************************/
// -*- c++ -*-

#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>
#include <ctime>
#include <unistd.h>
#include <signal.h>
//#include <zlib.h>
#include <stdio.h>
#include "../AST/AST.h"
#include "parsePL_defs.h"
#include "../sat/Solver.h"
#include "../sat/SolverTypes.h"
#include "../sat/VarOrder.h"

#include <unistd.h>

#ifdef EXT_HASH_MAP
  using namespace __gnu_cxx;
#endif

/* GLOBAL FUNCTION: parser
 */
extern int yyparse();
//extern int smtlibparse();

/* GLOBAL VARS: Some global vars for the Main function.
 *
 */
const char * prog = "stp";
int linenum  = 1;
const char * usage = "Usage: %s [-option] [infile]\n";
std::string helpstring = "\n\n";

// Amount of memory to ask for at beginning of main.
static const intptr_t INITIAL_MEMORY_PREALLOCATION_SIZE = 4000000;

// Used only in smtlib lexer/parser
BEEV::ASTNode SingleBitOne;
BEEV::ASTNode SingleBitZero;

/******************************************************************************
 * MAIN FUNCTION: 
 *
 * step 0. Parse the input into an ASTVec.
 * step 1. Do BV Rewrites
 * step 2. Bitblasts the ASTNode.
 * step 3. Convert to CNF
 * step 4. Convert to SAT
 * step 5. Call SAT to determine if input is SAT or UNSAT
 ******************************************************************************/
int main(int argc, char ** argv) {
  char * infile;
  extern FILE *yyin;

  // Grab some memory from the OS upfront to reduce system time when individual
  // hash tables are being allocated

  if (sbrk(INITIAL_MEMORY_PREALLOCATION_SIZE) == ((void *) -1)) {
    // FIXME: figure out how to get and print the real error message.
    BEEV::FatalError("Initial allocation of memory failed.");
  }
  
  //populate the help string
  helpstring +=  "-r  : switch refinement off (optimizations are ON by default)\n";
  helpstring +=  "-w  : switch wordlevel solver off (optimizations are ON by default)\n";
  helpstring +=  "-a  : switch optimizations off (optimizations are ON by default)\n";
  helpstring +=  "-s  : print function statistics\n";
  helpstring +=  "-v  : print nodes \n";
  helpstring +=  "-c  : construct counterexample\n";
  helpstring +=  "-d  : check counterexample\n";
  helpstring +=  "-p  : print counterexample\n";
  helpstring +=  "-x  : flatten nested XORs\n";
  helpstring +=  "-h  : help\n";

  for(int i=1; i < argc;i++) {
    if(argv[i][0] == '-')
      switch(argv[i][1]) {
      case 'a' :
	BEEV::optimize = false;
	BEEV::wordlevel_solve = false;
	break;
      case 'b':
	BEEV::print_STPinput_back = true;
	break;
      case 'c':
	BEEV::construct_counterexample = true;
	break;
      case 'd':
	BEEV::construct_counterexample = true;
	BEEV::check_counterexample = true;
	break;
      case 'e':
	BEEV::variable_activity_optimize = true;
	break;
      case 'f':
	BEEV::smtlib_parser_enable = true;
	break;
      case 'h':
	fprintf(stderr,usage,prog);
	cout << helpstring;
	//BEEV::FatalError("");
	return -1;
	break;
      case 'l' :
	BEEV::linear_search = true;
	break;
      case 'n':
	BEEV::print_output = true;
	break;
      case 'p':
	BEEV::print_counterexample = true;
	break;
      case 'q':
	BEEV::print_arrayval_declaredorder = true;
	break;
      case 'r':
	BEEV::arrayread_refinement = false;
	break;
      case 's' :
	BEEV::stats = true;
	break;
      case 'u':
	BEEV::arraywrite_refinement = false;
	break;
      case 'v' :
	BEEV::print_nodes = true;
	break;
      case 'w':
	BEEV::wordlevel_solve = false;
	break;
      case 'x':
	BEEV::xor_flatten = true;
	break;
      case 'z':
	BEEV::print_sat_varorder = true;
	break;
      default:
	fprintf(stderr,usage,prog);
	cout << helpstring;
	//BEEV::FatalError("");
	return -1;
	break;
      }
    else {
      infile = argv[i];
      yyin = fopen(infile,"r");
      if(yyin == NULL) {
	fprintf(stderr,"%s: Error: cannot open %s\n",prog,infile);
	BEEV::FatalError("");
      }
    }
  }

#ifdef NATIVE_C_ARITH
#else
  CONSTANTBV::ErrCode c = CONSTANTBV::BitVector_Boot(); 
  if(0 != c) {
    cout << CONSTANTBV::BitVector_Error(c) << endl;
    return 0;
  }
#endif           

  //want to print the output always from the commandline. 
  BEEV::print_output = true;
  BEEV::globalBeevMgr_for_parser = new BEEV::BeevMgr();  

  SingleBitOne = BEEV::globalBeevMgr_for_parser->CreateOneConst(1);
  SingleBitZero = BEEV::globalBeevMgr_for_parser->CreateZeroConst(1);
  //BEEV::smtlib_parser_enable = true;
  yyparse();
}//end of Main
