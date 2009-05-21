#include "AST.h"

using namespace BEEV;

int main()
{

  BeevMgr * bm = new BeevMgr();
  ASTNode s1 = bm->CreateSymbol("foo");
  s1 = bm->CreateSymbol("foo1");
  s1 = bm->CreateSymbol("foo2");
  ASTNode s2 = bm->CreateSymbol("bar");
  cout << "s1" <<  s1 << endl;
  cout << "s2" <<  s2 << endl;

  ASTNode b1 = bm->CreateBVConst(5,12);
  ASTNode b2 = bm->CreateBVConst(6,36);
  cout << "b1: " <<  b1 << endl;
  cout << "b2: " <<  b2 << endl;

  ASTNode a1 = bm->CreateNode(EQ, s1, s2);
  ASTNode a2 = bm->CreateNode(AND, s1, s2);
  a1 = bm->CreateNode(OR, s1, s2);
  ASTNode a3 = bm->CreateNode(IMPLIES, a1, a2);
  ASTNode a4 = bm->CreateNode(IMPLIES, s1, a2);
  cout << "a3" <<  a3 << endl;
  cout << "a4" <<  a4 << endl;
  return 0;
}
