/****************************************************************************************[Solver.C]
MiniSat -- Copyright (c) 2003-2005, Niklas Een, Niklas Sorensson

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#include "Solver.h"
#include "Sort.h"
#include <cmath>

namespace MINISAT {
//=================================================================================================
// Operations on clauses:


/*_________________________________________________________________________________________________
|
|  newClause : (ps : const vec<Lit>&) (learnt : bool)  ->  [void]
|  
|  Description:
|    Allocate and add a new clause to the SAT solvers clause database. 
|  
|  Input:
|    ps     - The new clause as a vector of literals.
|    learnt - Is the clause a learnt clause? For learnt clauses, 'ps[0]' is assumed to be the
|             asserting literal. An appropriate 'enqueue()' operation will be performed on this
|             literal. One of the watches will always be on this literal, the other will be set to
|             the literal with the highest decision level.
|  
|  Effect:
|    Activity heuristics are updated.
|________________________________________________________________________________________________@*/
bool Solver::newClause(const vec<Lit>& ps_, bool learnt, bool normalized)
{
    vec<Lit>    qs;
    if (!learnt && !normalized){
        assert(decisionLevel() == 0);
        ps_.copyTo(qs);             // Make a copy of the input vector.

        // Remove duplicates:
        sortUnique(qs);

        // Check if clause is satisfied:
        for (int i = 0; i < qs.size()-1; i++){
            if (qs[i] == ~qs[i+1])
                return true; }
        for (int i = 0; i < qs.size(); i++){
            if (value(qs[i]) == l_True)
                return true; }

        // Remove false literals:
        int     i, j;
        for (i = j = 0; i < qs.size(); i++)
            if (value(qs[i]) != l_False)
                qs[j++] = qs[i];
        qs.shrink(i - j);
    }
    const vec<Lit>& ps = learnt || normalized ? ps_ : qs;     // 'ps' is now the (possibly) reduced vector of literals.

    if (ps.size() == 0)
        return false;
    else if (ps.size() == 1){
        assert(decisionLevel() == 0);
        return enqueue(ps[0]);
    }else{
        // Allocate clause:
        Clause* c   = Clause_new(ps, learnt);

        if (learnt){
            // Put the second watch on the first literal with highest decision level:
            // (requires that this method is called at the level where the clause is asserting!)
            int i;
            for (i = 1; i < ps.size() && position(trailpos[var(ps[i])]) < trail_lim.last(); i++)
                ;
            (*c)[1] = ps[i];
            (*c)[i] = ps[1];

            // Bump, enqueue, store clause:
            claBumpActivity(*c);        // (newly learnt clauses should be considered active)
            check(enqueue((*c)[0], c));
            learnts.push(c);
            stats.learnts_literals += c->size();
        }else{
            // Store clause:
            clauses.push(c);
            stats.clauses_literals += c->size();

            if (subsumption){
                c->calcAbstraction();
                for (int i = 0; i < c->size(); i++){
                    assert(!find(occurs[var((*c)[i])], c));
                    occurs[var((*c)[i])].push(c);
                    n_occ[toInt((*c)[i])]++;
                    touched[var((*c)[i])] = 1;

                    if (heap.inHeap(var((*c)[i])))
		      updateHeap(var((*c)[i]));
                }
            }

        }
        // Watch clause:
        watches[toInt(~(*c)[0])].push(c);
        watches[toInt(~(*c)[1])].push(c);
    }

    return true;
}


// Disposes a clauses and removes it from watcher lists. NOTE!
// Low-level; does NOT change the 'clauses' and 'learnts' vector.
//
void Solver::removeClause(Clause& c, bool dealloc)
{
    //fprintf(stderr, "delete %d: ", _c); printClause(c); fprintf(stderr, "\n");
    assert(c.mark() == 0);

    if (c.size() > 1){
        assert(find(watches[toInt(~c[0])], &c));
        assert(find(watches[toInt(~c[1])], &c));
        remove(watches[toInt(~c[0])], &c);
        remove(watches[toInt(~c[1])], &c); }

    if (c.learnt()) stats.learnts_literals -= c.size();
    else            stats.clauses_literals -= c.size();

    if (subsumption && !c.learnt()){
        for (int i = 0; i < c.size(); i++){
            if (dealloc){
                assert(find(occurs[var(c[i])], &c));
                remove(occurs[var(c[i])], &c); 
            }
            n_occ[toInt(c[i])]--;
            updateHeap(var(c[i]));
        }
    }

    if (dealloc)
        xfree(&c);
    else
        c.mark(1);
}


bool Solver::satisfied(Clause& c) const
{
    for (int i = 0; i < c.size(); i++)
        if (value(c[i]) == l_True)
            return true;
    return false; }


bool Solver::strengthen(Clause& c, Lit l)
{
    assert(decisionLevel() == 0);
    assert(c.size() > 1);
    assert(c.mark() == 0);

    assert(toInt(~c[0]) < watches.size());
    assert(toInt(~c[1]) < watches.size());
    
    assert(find(watches[toInt(~c[0])], &c));
    assert(find(watches[toInt(~c[1])], &c));
    assert(find(c,l));

    if (c.learnt()) stats.learnts_literals -= 1;
    else            stats.clauses_literals -= 1;

    if (c[0] == l || c[1] == l){
        assert(find(watches[toInt(~l)], &c));
        remove(c,l);
        remove(watches[toInt(~l)], &c);
        if (c.size() > 1){
            assert(!find(watches[toInt(~c[1])], &c));
            watches[toInt(~c[1])].push(&c); }
        else {
            assert(find(watches[toInt(~c[0])], &c));
            remove(watches[toInt(~c[0])], &c);
            removeClause(c, false);
        }
    }
    else
        remove(c,l);
        
    assert(c.size() == 1 || find(watches[toInt(~c[0])], &c));
    assert(c.size() == 1 || find(watches[toInt(~c[1])], &c));

    if (subsumption){
        assert(find(occurs[var(l)], &c));
        remove(occurs[var(l)], &c);
        assert(!find(occurs[var(l)], &c));

        c.calcAbstraction();

        n_occ[toInt(l)]--;
        updateHeap(var(l));
    }

    return c.size() == 1 ? enqueue(c[0]) : true;
}


//=================================================================================================
// Minor methods:


// Creates a new SAT variable in the solver. If 'decision_var' is cleared, variable will not be
// used as a decision variable (NOTE! This has effects on the meaning of a SATISFIABLE result).
//
Var Solver::newVar(bool polarity, bool dvar) {
    int     index;
    index = nVars();
    watches     .push();          // (list for positive literal)
    watches     .push();          // (list for negative literal)
    reason      .push(NULL);
    assigns     .push(toInt(l_Undef));
    trailpos    .push(TrailPos(0,0));
    activity    .push(0);
    order       .newVar(polarity,dvar);
    seen        .push(0);
    touched     .push(0);
    if (subsumption){
        occurs  .push();
        n_occ   .push(0);
        n_occ   .push(0);
        heap    .setBounds(index+1);
    }
    return index; }


// Returns FALSE if immediate conflict.
bool Solver::assume(Lit p) {
    trail_lim.push(trail.size());
    return enqueue(p); }


// Revert to the state at given level.
void Solver::cancelUntil(int level) {
    if (decisionLevel() > level){
        for (int c = trail.size()-1; c >= trail_lim[level]; c--){
            Var     x  = var(trail[c]);
            assigns[x] = toInt(l_Undef);
            reason [x] = NULL; 
            order.undo(x); }
        qhead = trail_lim[level];
        trail.shrink(trail.size() - trail_lim[level]);
        trail_lim.shrink(trail_lim.size() - level);
    }
}


//=================================================================================================
// Major methods:


/*_________________________________________________________________________________________________
|
|  analyze : (confl : Clause*) (out_learnt : vec<Lit>&) (out_btlevel : int&)  ->  [void]
|  
|  Description:
|    Analyze conflict and produce a reason clause.
|  
|    Pre-conditions:
|      * 'out_learnt' is assumed to be cleared.
|      * Current decision level must be greater than root level.
|  
|    Post-conditions:
|      * 'out_learnt[0]' is the asserting literal at level 'out_btlevel'.
|  
|  Effect:
|    Will undo part of the trail, upto but not beyond the assumption of the current decision level.
|________________________________________________________________________________________________@*/
void Solver::analyze(Clause* confl, vec<Lit>& out_learnt, int& out_btlevel)
{
    int            pathC = 0;
    int            btpos = -1;
    Lit            p     = lit_Undef;

    // Generate conflict clause:
    //
    out_learnt.push();      // (leave room for the asserting literal)
    int index = trail.size()-1;
    do{
        assert(confl != NULL);          // (otherwise should be UIP)
        Clause& c = *confl;

        if (c.learnt())
            claBumpActivity(c);

        for (int j = (p == lit_Undef) ? 0 : 1; j < c.size(); j++){
            Lit q = c[j];
            if (!seen[var(q)] && position(trailpos[var(q)]) >= trail_lim[0]){
	      varBumpActivity(q);
                seen[var(q)] = 1;
                if (position(trailpos[var(q)]) >= trail_lim.last())
                    pathC++;
                else{
                    out_learnt.push(q);
                    btpos = max(btpos, position(trailpos[var(q)]));
                }
            }
        }

        // Select next clause to look at:
        while (!seen[var(trail[index--])]) ;
        p     = trail[index+1];
        confl = reason[var(p)];
        seen[var(p)] = 0;
        pathC--;

    }while (pathC > 0);
    out_learnt[0] = ~p;

    // Find correct backtrack level
    for (out_btlevel = trail_lim.size()-1; out_btlevel > 0 && trail_lim[out_btlevel-1] > btpos; out_btlevel--)
        ;

    int     i, j;
    if (expensive_ccmin){
        // Simplify conflict clause (a lot):
        //
        uint    min_level = 0;
        for (i = 1; i < out_learnt.size(); i++)
            min_level |= abstractLevel(trailpos[var(out_learnt[i])]);     // (maintain an abstraction of levels involved in conflict)

        out_learnt.copyTo(analyze_toclear);
        for (i = j = 1; i < out_learnt.size(); i++)
            if (reason[var(out_learnt[i])] == NULL || !analyze_removable(out_learnt[i], min_level))
                out_learnt[j++] = out_learnt[i];
    }else{
        // Simplify conflict clause (a little):
        //
        out_learnt.copyTo(analyze_toclear);
        for (i = j = 1; i < out_learnt.size(); i++){
            Clause& c = *reason[var(out_learnt[i])];
            for (int k = 1; k < c.size(); k++)
                if (!seen[var(c[k])] && position(trailpos[var(c[k])]) >= trail_lim[0]){
                    out_learnt[j++] = out_learnt[i];
                    break; }
        }
    }

    stats.max_literals += out_learnt.size();
    out_learnt.shrink(i - j);
    stats.tot_literals += out_learnt.size();

    for (int j = 0; j < analyze_toclear.size(); j++) seen[var(analyze_toclear[j])] = 0;    // ('seen[]' is now cleared)
}


// Check if 'p' can be removed. 'min_level' is used to abort early if visiting literals at a level that cannot be removed.
//
bool Solver::analyze_removable(Lit p, uint min_level)
{
    analyze_stack.clear(); analyze_stack.push(p);
    int top = analyze_toclear.size();
    while (analyze_stack.size() > 0){
        assert(reason[var(analyze_stack.last())] != NULL);
        Clause& c = *reason[var(analyze_stack.last())]; analyze_stack.pop();

        for (int i = 1; i < c.size(); i++){
            Lit      p   = c[i];
            TrailPos tp = trailpos[var(p)];
            if (!seen[var(p)] && position(tp) >= trail_lim[0]){
                if (reason[var(p)] != NULL && (abstractLevel(tp) & min_level) != 0){
                    seen[var(p)] = 1;
                    analyze_stack.push(p);
                    analyze_toclear.push(p);
                }else{
                    for (int j = top; j < analyze_toclear.size(); j++)
                        seen[var(analyze_toclear[j])] = 0;
                    analyze_toclear.shrink(analyze_toclear.size() - top);
                    return false;
                }
            }
        }
    }

    return true;
}


/*_________________________________________________________________________________________________
|
|  analyzeFinal : (p : Lit) ->  [void]
|  
|  Description:
|    Specialized analysis procedure to express the final conflict in terms of assumptions.
|    Calculates the (possibly empty) set of assumptions that led to the assignment of 'p', and
|    stores the result in 'out_conflict'.
|________________________________________________________________________________________________@*/
void Solver::analyzeFinal(Lit p, vec<Lit>& out_conflict)
{
    out_conflict.clear();
    out_conflict.push(p);

    if (decisionLevel() == 0)
        return;

    seen[var(p)] = 1;

    int start = position(trailpos[var(p)]);
    for (int i = start; i >= trail_lim[0]; i--){
        Var     x = var(trail[i]);
        if (seen[x]){
            if (reason[x] == NULL){
                assert(position(trailpos[x]) >= trail_lim[0]);
                out_conflict.push(~trail[i]);
            }else{
                Clause& c = *reason[x];
                for (int j = 1; j < c.size(); j++)
                    if (position(trailpos[var(c[j])]) >= trail_lim[0])
                        seen[var(c[j])] = 1;
            }
            seen[x] = 0;
        }
    }
}


/*_________________________________________________________________________________________________
|
|  enqueue : (p : Lit) (from : Clause*)  ->  [bool]
|  
|  Description:
|    Puts a new fact on the propagation queue as well as immediately updating the variable's value.
|    Should a conflict arise, FALSE is returned.
|  
|  Input:
|    p    - The fact to enqueue
|    from - [Optional] Fact propagated from this (currently) unit clause. Stored in 'reason[]'.
|           Default value is NULL (no reason).
|  
|  Output:
|    TRUE if fact was enqueued without conflict, FALSE otherwise.
|________________________________________________________________________________________________@*/
bool Solver::enqueue(Lit p, Clause* from)
{

    if (value(p) != l_Undef)
        return value(p) != l_False;
    else{
        assigns [var(p)] = toInt(lbool(!sign(p)));
        trailpos[var(p)] = TrailPos(trail.size(),decisionLevel());
        reason  [var(p)] = from;
        trail.push(p);
        return true;
    }
}


/*_________________________________________________________________________________________________
|
|  propagate : [void]  ->  [Clause*]
|  
|  Description:
|    Propagates all enqueued facts. If a conflict arises, the conflicting clause is returned,
|    otherwise NULL.
|  
|    Post-conditions:
|      * the propagation queue is empty, even if there was a conflict.
|________________________________________________________________________________________________@*/
Clause* Solver::propagate()
{
    if (decisionLevel() == 0 && subsumption)
        return backwardSubsumptionCheck() ? NULL : propagate_tmpempty;

    Clause* confl = NULL;
    //fprintf(stderr, "propagate, qhead = %d, qtail = %d\n", qhead, qtail);
    while (qhead < trail.size()){
        stats.propagations++;
        simpDB_props--;

        Lit            p   = trail[qhead++];     // 'p' is enqueued fact to propagate.
        vec<Clause*>&  ws  = watches[toInt(p)];
        Clause         **i, **j, **end;

        for (i = j = (Clause**)ws, end = i + ws.size();  i != end;){
            Clause& c = **i++;
            
            // Make sure the false literal is data[1]:
            Lit false_lit = ~p;
            if (c[0] == false_lit)
                c[0] = c[1], c[1] = false_lit;

            assert(c[1] == false_lit);

            // If 0th watch is true, then clause is already satisfied.
            Lit first = c[0];
            if (value(first) == l_True){
                *j++ = &c;
            }else{
                // Look for new watch:
                for (int k = 2; k < c.size(); k++)
                    if (value(c[k]) != l_False){
                        c[1] = c[k]; c[k] = false_lit;
                        watches[toInt(~c[1])].push(&c);
                        goto FoundWatch; }

                // Did not find watch -- clause is unit under assignment:
                *j++ = &c;
                if (!enqueue(first, &c)){
                    confl = &c;
                    qhead = trail.size();
                    // Copy the remaining watches:
                    while (i < end)
                        *j++ = *i++;
                }
            FoundWatch:;
            }
        }
        ws.shrink(i - j);
    }

    return confl;
}


/*_________________________________________________________________________________________________
|
|  reduceDB : ()  ->  [void]
|  
|  Description:
|    Remove half of the learnt clauses, minus the clauses locked by the current assignment. Locked
|    clauses are clauses that are reason to some assignment. Binary clauses are never removed.
|________________________________________________________________________________________________@*/
struct reduceDB_lt { bool operator () (Clause* x, Clause* y) { return x->size() > 2 && (y->size() == 2 || x->activity() < y->activity()); } };
void Solver::reduceDB()
{
    int     i, j;
    double  extra_lim = cla_inc / learnts.size();    // Remove any clause below this activity

    sort(learnts, reduceDB_lt());
    for (i = j = 0; i < learnts.size() / 2; i++){
        if (learnts[i]->size() > 2 && !locked(*learnts[i]))
            removeClause(*learnts[i]);
        else
            learnts[j++] = learnts[i];
    }
    for (; i < learnts.size(); i++){
        if (learnts[i]->size() > 2 && !locked(*learnts[i]) && learnts[i]->activity() < extra_lim)
            removeClause(*learnts[i]);
        else
            learnts[j++] = learnts[i];
    }
    learnts.shrink(i - j);
}


/*_________________________________________________________________________________________________
|
|  simplifyDB : [void]  ->  [bool]
|  
|  Description:
|    Simplify the clause database according to the current top-level assigment. Currently, the only
|    thing done here is the removal of satisfied clauses, but more things can be put here.
|________________________________________________________________________________________________@*/
bool Solver::simplifyDB(bool expensive)
{
    assert(decisionLevel() == 0);
    if (!ok || propagate() != NULL)
        return ok = false;

    if (nAssigns() == simpDB_assigns || 
        (!subsumption && simpDB_props > 0)) // (nothing has changed or preformed a simplification too recently)
        return true;

    if (subsumption){
        if (expensive && !eliminate())
            return ok = false;

        // Move this cleanup code to its own method ?
        int      i , j;
        vec<Var> dirty;
        for (i = 0; i < clauses.size(); i++)
            if (clauses[i]->mark() == 1){
                Clause& c = *clauses[i];
                for (int k = 0; k < c.size(); k++)
                    if (!seen[var(c[k])]){
                        seen[var(c[k])] = 1;
                        dirty.push(var(c[k]));
                    }
            }
        
        for (i = 0; i < dirty.size(); i++){
            cleanOcc(dirty[i]);
            seen[dirty[i]] = 0;
        }

        for (i = j = 0; i < clauses.size(); i++)
            if (clauses[i]->mark() == 1)
                xfree(clauses[i]);
            else
                clauses[j++] = clauses[i];
        clauses.shrink(i - j);
    }

    // Remove satisfied clauses:
    for (int type = 0; type < (subsumption ? 1 : 2); type++){  // (only scan learnt clauses if subsumption is on)
        vec<Clause*>& cs = type ? learnts : clauses;
        int     j  = 0;
        for (int i = 0; i < cs.size(); i++){
            assert(cs[i]->mark() == 0);
            if (satisfied(*cs[i]))
                removeClause(*cs[i]);
            else
                cs[j++] = cs[i];
        }
        cs.shrink(cs.size()-j);
    }
    order.cleanup();

    simpDB_assigns = nAssigns();
    simpDB_props   = stats.clauses_literals + stats.learnts_literals;   // (shouldn't depend on 'stats' really, but it will do for now)

    return true;
}


/*_________________________________________________________________________________________________
|
|  search : (nof_conflicts : int) (nof_learnts : int) (params : const SearchParams&)  ->  [lbool]
|  
|  Description:
|    Search for a model the specified number of conflicts, keeping the number of learnt clauses
|    below the provided limit. NOTE! Use negative value for 'nof_conflicts' or 'nof_learnts' to
|    indicate infinity.
|  
|  Output:
|    'l_True' if a partial assigment that is consistent with respect to the clauseset is found. If
|    all variables are decision variables, this means that the clause set is satisfiable. 'l_False'
|    if the clause set is unsatisfiable. 'l_Undef' if the bound on number of conflicts is reached.
|________________________________________________________________________________________________@*/
lbool Solver::search(int nof_conflicts, int nof_learnts)
{
    assert(ok);
    int         backtrack_level;
    int         conflictC = 0;
    vec<Lit>    learnt_clause;

    stats.starts++;
    var_decay = 1 / params.var_decay;
    cla_decay = 1 / params.clause_decay;

    for (;;){
        Clause* confl = propagate();
        if (confl != NULL){
            // CONFLICT
            stats.conflicts++; conflictC++;
            if (decisionLevel() == 0) return l_False;

            learnt_clause.clear();
            analyze(confl, learnt_clause, backtrack_level);
            cancelUntil(backtrack_level);
            newClause(learnt_clause, true);
            varDecayActivity();
            claDecayActivity();

        }else{
            // NO CONFLICT

            if (nof_conflicts >= 0 && conflictC >= nof_conflicts){
                // Reached bound on number of conflicts:
                progress_estimate = progressEstimate();
                cancelUntil(0);
                return l_Undef; }

            // Simplify the set of problem clauses:
            if (decisionLevel() == 0 && !simplifyDB())
                return l_False;

            if (nof_learnts >= 0 && learnts.size()-nAssigns() >= nof_learnts)
                // Reduce the set of learnt clauses:
                reduceDB();

            Lit next = lit_Undef;

            if (decisionLevel() < assumptions.size()){
                // Perform user provided assumption:
                next = assumptions[decisionLevel()]; 
                if (value(next) == l_False){
                    analyzeFinal(~next, conflict);
                    return l_False; }
            }else{
                // New variable decision:
                stats.decisions++;
                next = order.select(params.random_var_freq, decisionLevel());		
	    }
            if (next == lit_Undef)
                // Model found:
                return l_True;

            check(assume(next));
        }
    }
}


// Return search-space coverage. Not extremely reliable.
//
double Solver::progressEstimate()
{
    double  progress = 0;
    double  F = 1.0 / nVars();

    for (int i = 0; i <= decisionLevel(); i++){
        int beg = i == 0 ? 0 : trail_lim[i - 1];
        int end = i == decisionLevel() ? trail.size() : trail_lim[i];
        progress += pow(F, i) * (end - beg);
    }

    return progress / nVars();
}


// Divide all variable activities by 1e100.
//
void Solver::varRescaleActivity()
{
    for (int i = 0; i < nVars(); i++)
        activity[i] *= 1e-100;
    var_inc *= 1e-100;
}


// Divide all constraint activities by 1e100.
//
void Solver::claRescaleActivity()
{
    for (int i = 0; i < learnts.size(); i++)
        learnts[i]->activity() *= 1e-20;
    cla_inc *= 1e-20;
}


/*_________________________________________________________________________________________________
|
|  solve : (assumps : const vec<Lit>&)  ->  [bool]
|  
|  Description:
|    Top-level solve.
|________________________________________________________________________________________________@*/
bool Solver::solve(const vec<Lit>& assumps)
{
    model.clear();
    conflict.clear();

    if (!simplifyDB(true)) return false;


    double  nof_conflicts = params.restart_first;
    double  nof_learnts   = nClauses() * params.learntsize_factor;
    lbool   status        = l_Undef;
    assumps.copyTo(assumptions);

    if (verbosity >= 1){
        printf("==================================[MINISAT]====================================\n");
        printf("| Conflicts |          ORIGINAL         |          LEARNT          | Progress |\n");
        printf("|           |    Vars  Clauses Literals |    Limit  Clauses Lit/Cl |          |\n");
        printf("===============================================================================\n");
    }

    // Search:
    while (status == l_Undef){
        if (verbosity >= 1)
            //printf("| %9d | %7d %8d | %7d %7d %8d %7.1f | %6.3f %% |\n", (int)stats.conflicts, nClauses(), (int)stats.clauses_literals, (int)nof_learnts, nLearnts(), (int)stats.learnts_literals, (double)stats.learnts_literals/nLearnts(), progress_estimate*100);
            printf("| %9d | %7d %8d %8d | %8d %8d %6.0f | %6.3f %% |\n", (int)stats.conflicts, order.size(), nClauses(), (int)stats.clauses_literals, (int)nof_learnts, nLearnts(), (double)stats.learnts_literals/nLearnts(), progress_estimate*100);
        status = search((int)nof_conflicts, (int)nof_learnts);
        nof_conflicts *= params.restart_inc;
        nof_learnts   *= params.learntsize_inc;
    }

    if (verbosity >= 1) {
        printf("==============================================================================\n");
        fflush(stdout);
    }

    if (status == l_True){
        // Copy model:
        extendModel();
#if 1
        //fprintf(stderr, "Verifying model.\n");
        for (int i = 0; i < clauses.size(); i++)
            assert(satisfied(*clauses[i]));
        for (int i = 0; i < eliminated.size(); i++)
            assert(satisfied(*eliminated[i]));
#endif
        model.growTo(nVars());
        for (int i = 0; i < nVars(); i++) model[i] = value(i);
    }else{
        assert(status == l_False);
        if (conflict.size() == 0)
            ok = false;
    }

    cancelUntil(0);
    return status == l_True;
}
} //end of MINISAT namespace
