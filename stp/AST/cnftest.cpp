// -*- c++ -*-

// Test program for CNF conversion.

#include "AST.h"

using namespace BEEV;

int main()
{
  const int size = 1;
  
  BeevMgr *bm = new BeevMgr();
  ASTNode s1 = bm->CreateSymbol("x");
  s1.SetValueWidth(size);
  
  cout << "s1" <<  s1 << endl;
  ASTNode s2 = bm->CreateSymbol("y");
  s2.SetValueWidth(size);

  cout << "s2" <<  s2 << endl;
  ASTNode s3 = bm->CreateSymbol("z");
  s3.SetValueWidth(size);
  
  cout << "s3" <<  s3 << endl;

  ASTNode bbs1 = bm->BBForm(s1);
  cout << "bitblasted s1" << endl << bbs1 << endl;
  bm->PrintClauseList(cout, bm->ToCNF(bbs1));

  ASTNode a2 = bm->CreateNode(AND, s1, s2);
  ASTNode bba2 = bm->BBForm(a2);
  cout << "bitblasted a2" << endl << bba2 << endl;
  bm->PrintClauseList(cout, bm->ToCNF(bba2));

  ASTNode a3 = bm->CreateNode(OR, s1, s2);
  ASTNode bba3 = bm->BBForm(a3);
  cout << "bitblasted a3" << endl << bba3 << endl;
  bm->PrintClauseList(cout, bm->ToCNF(bba3));

  ASTNode a4 = bm->CreateNode(EQ, s1, s2);
  ASTNode bba4 = bm->BBForm(a4);
  cout << "bitblasted a4 " << endl << bba4 << endl;

  bm->PrintClauseList(cout, bm->ToCNF(bba4));

}
