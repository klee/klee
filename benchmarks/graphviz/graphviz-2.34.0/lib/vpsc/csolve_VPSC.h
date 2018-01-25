/* $Id$ $Revision$ */
/* vim:set shiftwidth=4 ts=8: */

/**
 * \brief Bridge for C programs to access solve_VPSC (which is in C++)
 *
 * Authors:
 *   Tim Dwyer <tgdwyer@gmail.com>
 *
 * Copyright (C) 2005 Authors
 *
 * This version is released under the CPL (Common Public License) with
 * the Graphviz distribution.
 * A version is also available under the LGPL as part of the Adaptagrams
 * project: http://sourceforge.net/projects/adaptagrams.  
 * If you make improvements or bug fixes to this code it would be much
 * appreciated if you could also contribute those changes back to the
 * Adaptagrams repository.
 */
#ifndef _CSOLVE_VPSC_H_
#define _CSOLVE_VPSC_H_
#ifdef __cplusplus
extern "C" {
#endif
#ifdef __cplusplus
class Variable;
#else
typedef struct Variable Variable;
#endif
Variable* newVariable(int id, double desiredPos, double weight);
void setVariableDesiredPos(Variable *, double desiredPos);
double getVariablePos(Variable*);

#ifdef __cplusplus
class Constraint;
#else
typedef struct Constraint Constraint;
#endif
Constraint* newConstraint(Variable* left, Variable* right, double gap);

#ifdef __cplusplus
class VPSC;
#else
typedef struct VPSC VPSC;
#endif
VPSC* newVPSC(int n, Variable* vs[], int m, Constraint* cs[]);
void deleteVPSC(VPSC*);
void deleteConstraint(Constraint*);
void deleteVariable(Variable*);
Constraint** newConstraints(int m);
void deleteConstraints(int m,Constraint**);
void remapInConstraints(Variable *u, Variable *v, double dgap);
void remapOutConstraints(Variable *u, Variable *v, double dgap);
int getLeftVarID(Constraint *c);
int getRightVarID(Constraint *c);
double getSeparation(Constraint *c);

#ifndef HAVE_POINTF_S
typedef struct pointf_s { double x, y; } pointf;
typedef struct { pointf LL, UR; } boxf;
#endif
int genXConstraints(int n, boxf[], Variable** vs, Constraint*** cs,
		int transitiveClosure);
int genYConstraints(int n, boxf[], Variable** vs, Constraint*** cs);

void satisfyVPSC(VPSC*);
void solveVPSC(VPSC*);
#ifdef __cplusplus
class IncVPSC;
#else
typedef struct IncVPSC IncVPSC;
#endif
VPSC* newIncVPSC(int n, Variable* vs[], int m, Constraint* cs[]);
void splitIncVPSC(IncVPSC*);
int getSplitCnt(IncVPSC *vpsc);
#ifdef __cplusplus
}
#endif
#endif /* _CSOLVE_VPSC_H_ */
