/************************************************************************************[Simplifier.C]
MiniSat -- Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson

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

namespace MINISAT {

static const int grow = 0;

//#define WEAKEN
//#define MATING
//#define ASSYMM

bool Solver::assymmetricBranching(Clause& c)
{
    assert(decisionLevel() == 0);

    //fprintf(stderr, "assymmetric branching on clause: "); printClause(c); fprintf(stderr, "\n");
    if (satisfied(c)){
        //fprintf(stderr, "subsumed.\n");
        return true; }

    int      old;
    vec<Lit> copy; for (int i = 0; i < c.size(); i++) copy.push(c[i]);

    do {
        assert(copy.size() == c.size());

        old = copy.size();

        //fprintf(stderr, "checking that clause is normalized\n");
        //for (int i = 0; i < copy.size(); i++)
        //    assert(value(copy[i]) == l_Undef);

        for (int i = 0; i < copy.size(); i++){
            trail_lim.push(trail.size());
            //fprintf(stderr, " -- trying to delete literal "); printLit(copy[i]); 
            for (int j = 0; j < copy.size(); j++)
                if (j != i)
                    check(enqueue(~copy[j]));

            if (propagate() != NULL){
                //fprintf(stderr, " succeeded\n");
                cancelUntil(0);
                Lit l = copy[i];
                assert(find(copy, l));
                remove(copy, l);
                if (!strengthen(c, l))
                    return false;
                i--;

                if (c.size() == 1)
                    return propagate() == NULL;
                else
                    assert(qhead == trail.size());
            }
            else
                //fprintf(stderr, " failed\n");

            cancelUntil(0);
        }

        //} while (false);
    } while (copy.size() < old);

    return true;
}

// Returns FALSE if clause is always satisfied ('out_clause' should not be used). 
//bool Solver::merge(const Clause& _ps, const Clause& _qs, Var v, vec<Lit>& out_clause)
bool Solver::merge(const Clause& _ps, const Clause& _qs, Var v, vec<Lit>& out_clause)
{  
    stats.merges++;

    bool  ps_smallest = _ps.size() < _qs.size();
    const Clause& ps =  ps_smallest ? _qs : _ps;
    const Clause& qs =  ps_smallest ? _ps : _qs;

    for (int i = 0; i < qs.size(); i++){
        if (var(qs[i]) != v){
            for (int j = 0; j < ps.size(); j++)
                if (var(ps[j]) == var(qs[i])) {
                    if (ps[j] == ~qs[i])
                        return false;
                    else
                        goto next;
                }
            out_clause.push(qs[i]);
        }
        next:;
    }

    for (int i = 0; i < ps.size(); i++)
        if (var(ps[i]) != v)
            out_clause.push(ps[i]);

    return true;
}


void Solver::gather(vec<Clause*>& clauses)
{
    //fprintf(stderr, "Gathering clauses for backwards subsumption\n");
    int ntouched = 0;
    assert(touched.size() == occurs.size());
    clauses.clear();
    for (int i = 0; i < touched.size(); i++)
        if (touched[i]){
            const vec<Clause*>& cs = getOccurs(i);
            ntouched++;
            for (int j = 0; j < cs.size(); j++)
                if (cs[j]->mark() == 0){
                    clauses.push(cs[j]);
                    cs[j]->mark(2);
                }
            touched[i] = 0;
        }

    //fprintf(stderr, "Touched variables %d of %d yields %d clauses to check\n", ntouched, touched.size(), clauses.size());
    for (int i = 0; i < clauses.size(); i++)
        clauses[i]->mark(0);
}


/*_________________________________________________________________________________________________
|
|  subsumes : (_c : ClauseId) (c : Clause&) (_d : ClauseId) (d : Clause&)  ->  bool
|
|  Description:
|     Checks if c subsumes d, and at the same time, if c can be used to simplify d by subsumption
|     resolution.
|    
|  Input:
|     Indices into the 'clauses' vector _c, _d, and references to the corresponding clauses c, d.
|
|  Result:
|     lit_Error  - No subsumption or simplification
|     lit_Undef  - Clause c subsumes d
|     l          - The literal l can be deleted from d
|________________________________________________________________________________________________@*/
inline Lit Solver::subsumes(const Clause& c, const Clause& d)
{
    stats.subsumption_checks++;
    if (d.size() < c.size() || (c.abstraction() & ~d.abstraction()) != 0)
        return lit_Error;

    Lit ret = lit_Undef;

    for (int i = 0; i < c.size(); i++) {
        // search for c[i] or ~c[i]
        for (int j = 0; j < d.size(); j++)
            if (c[i] == d[j])
                goto ok;
            else if (ret == lit_Undef && c[i] == ~d[j]){
                ret = c[i];
                goto ok;
            }

        // did not find it
        stats.subsumption_misses++;
        return lit_Error;
    ok:;
    }

    return ret;
}


// Backward subsumption + backward subsumption resolution
bool Solver::backwardSubsumptionCheck()
{
    while (subsumption_queue.size() > 0 || qhead < trail.size()){

        // if propagation queue is non empty, take the first literal and
        // create a dummy unit clause
        if (qhead < trail.size()){
            Lit l = trail[qhead++];
            (*bwdsub_tmpunit)[0] = l;
            assert(bwdsub_tmpunit->mark() == 0);
            subsumption_queue.push(bwdsub_tmpunit);
        }
        Clause&  c = *subsumption_queue.last(); subsumption_queue.pop();

        if (c.mark())
            continue;

        if (c.size() == 1 && !enqueue(c[0]))
            return false; 

        // (1) find best variable to scan
        Var best = var(c[0]);
        for (int i = 1; i < c.size(); i++)
            if (occurs[var(c[i])].size() < occurs[best].size())
                best = var(c[i]);

        // (2) search all candidates
        const vec<Clause*>& cs = getOccurs(best);

        for (int j = 0; j < cs.size(); j++)
            if (cs[j] != &c){
                if (cs[j]->mark())
                    continue;
                if (c.mark())
                    break;

                //fprintf(stderr, "backward candidate "); printClause(*cs[j]); fprintf(stderr, "\n"); 
                Lit l = subsumes(c, *cs[j]);
                if (l == lit_Undef){
                    //fprintf(stderr, "clause backwards subsumed\n");
                    //fprintf(stderr, " >> clause %d: ", cs[j]->mark()); printClause(*cs[j]); fprintf(stderr, "\n");
                    //fprintf(stderr, " >> clause %d: ", c.mark()); printClause(c); fprintf(stderr, "\n");
                    removeClause(*cs[j], false);
                }else if (l != lit_Error){
                    //fprintf(stderr, "backwards subsumption resolution\n");
                    //fprintf(stderr, " >> clause %d: ", cs[j]->mark()); printClause(*cs[j]); fprintf(stderr, "\n");
                    //fprintf(stderr, " >> clause %d: ", c.mark()); printClause(c); fprintf(stderr, "\n");

                    assert(cs[j]->size() > 1);
                    assert(find(*cs[j], ~l));

                    subsumption_queue.push(cs[j]);
                    if (!strengthen(*cs[j], ~l))
                        return false;

                    // did current candidate get deleted from cs? then check candidate at index j again
                    if (var(l) == best)
                        j--;
                }
            }
    }

    return true;
}


bool Solver::eliminateVar(Var v, bool fail)
{
    assert(hasVarProp(v, p_frozen));

    vec<Clause*>  pos, neg;
    const vec<Clause*>& cls = getOccurs(v);

    if (value(v) != l_Undef || cls.size() == 0)
        return true;

    //fprintf(stderr, "trying to eliminate var %d\n", v+1);
    for (int i = 0; i < cls.size(); i++){
        //fprintf(stderr, "clause: "); printClause(*cls[i]); fprintf(stderr, "\n");
        if (find(*cls[i], Lit(v)))
            pos.push(cls[i]);
        else{
            assert(find(*cls[i], ~Lit(v)));
            neg.push(cls[i]);
        }
    }

#ifdef WEAKEN
    vec<int> posc(pos.size(), 0);
    vec<int> negc(neg.size(), 0);
#endif
    // check if number of clauses decreases
    int      cnt = 0;
    vec<Lit> resolvent;
    for (int i = 0; i < pos.size(); i++)
        for (int j = 0; j < neg.size(); j++){
            resolvent.clear();
            if (merge(*pos[i], *neg[j], v, resolvent)){
                cnt++;
#ifdef WEAKEN
                posc[i]++;
                negc[j]++;
#endif
            }
#ifndef WEAKEN
            if (cnt > cls.size() + grow)
                return true;
#else
#ifdef MATING
            if (cnt > cls.size() + grow)
                if (posc[i] > 0)
                    break;
#endif
#endif
            assert(pos.size() <= n_occ[toInt(Lit(v))]);
            assert(neg.size() <= n_occ[toInt(~Lit(v))]);
        }

#ifdef WEAKEN
#ifdef MATING
    for (int i = 0; i < neg.size(); i++)
        if (negc[i] == 0)
            for (int j = 0; j < pos.size(); j++){
                resolvent.clear();
                if (merge(*neg[i], *pos[j], v, resolvent)){
                    negc[i]++;
                    break;
                }
            }
#endif
    for (int i = 0; i < pos.size(); i++)
        if (posc[i] == 0)
            removeClause(*pos[i], false);

    for (int i = 0; i < neg.size(); i++)
        if (negc[i] == 0)
            removeClause(*neg[i], false);

    if (cnt > cls.size() + grow)
        return true;
#endif    
    //if (pos.size() != n_occ[toInt(Lit(v))])
    //    fprintf(stderr, "pos.size() = %d, n_occ[toInt(Lit(v))] = %d\n", pos.size(), n_occ[toInt(Lit(v))]);
    assert(pos.size() == n_occ[toInt(Lit(v))]);
    //if (neg.size() != n_occ[toInt(~Lit(v))])
    //    fprintf(stderr, "neg.size() = %d, n_occ[toInt(Lit(v))] = %d\n", neg.size(), n_occ[toInt(Lit(v))]);
    assert(neg.size() == n_occ[toInt(~Lit(v))]);
    assert(cnt <= cls.size() + grow);
    setVarProp(v, p_decisionvar, false);

    // produce clauses in cross product
    int top = clauses.size();
    for (int i = 0; i < pos.size(); i++)
        for (int j = 0; j < neg.size(); j++){
            resolvent.clear();
#ifdef WEAKEN
            if (pos[i]->mark() == 1)
                break;
            if (neg[j]->mark() == 1)
                continue;
#endif

            if (merge(*pos[i], *neg[j], v, resolvent)){
                int i, j;
                for (i = j = 0; i < resolvent.size(); i++)
                    if (value(resolvent[i]) == l_True)
                        goto next;
                    else if (value(resolvent[i]) == l_Undef)
                        resolvent[j++] = resolvent[i];
                resolvent.shrink(i - j);

                if (resolvent.size() == 1){
                    if (!enqueue(resolvent[0]))
                        return false;
                }else{
                    int apa = clauses.size();
                    check(newClause(resolvent, false, true));
                    assert(apa + 1 == clauses.size());
                }
            }
            next:;
        }

    if (fail){
        fprintf(stderr, "eliminated var %d, %d <= %d\n", v+1, cnt, cls.size());
        fprintf(stderr, "previous clauses:\n");
        for (int i = 0; i < cls.size(); i++){
            printClause(*cls[i]);
            fprintf(stderr, "\n");
        }
        
        fprintf(stderr, "new clauses:\n");
        for (int i = top; i < clauses.size(); i++){
            printClause(*clauses[i]);
            fprintf(stderr, "\n");
        }

        assert(0); }

    //fprintf(stderr, "eliminated var %d, %d <= %d\n", v+1, cnt, cls.size());
    //fprintf(stderr, "previous clauses:\n");
    //for (int i = 0; i < cls.size(); i++){
    //    printClause(*cls[i]);
    //    fprintf(stderr, "\n");
    //}
    //
    //fprintf(stderr, "new clauses:\n");
    //for (int i = top; i < clauses.size(); i++){
    //    printClause(*clauses[i]);
    //    fprintf(stderr, "\n");
    //}

    // delete + store old clauses
    eliminated_var.push(v);
    eliminated_lim.push(eliminated.size());
    for (int i = 0; i < cls.size(); i++){
        eliminated.push(Clause_new(*cls[i]));

#ifdef WEAKEN
        if (cls[i]->mark() == 0)
#endif
            removeClause(*cls[i], false); 

    }

    assert(subsumption_queue.size() == 0);
    for (int i = top; i < clauses.size(); i++)
#ifdef ASSYMM
        if (clauses[i]->mark() == 0)
            if (!assymmetricBranching(*clauses[i]))
                return false;
            else
                subsumption_queue.push(clauses[i]);
#else
        if (clauses[i]->mark() == 0)
            subsumption_queue.push(clauses[i]);
#endif

    return backwardSubsumptionCheck();
}


void Solver::extendModel()
{
    assert(eliminated_var.size() == eliminated_lim.size());
    for (int i = eliminated_var.size()-1; i >= 0; i--){
        Var v = eliminated_var[i];
        Lit l = lit_Undef;

        //fprintf(stderr, "extending var %d\n", v+1);

        for (int j = eliminated_lim[i]; j < (i+1 >= eliminated_lim.size() ? eliminated.size() : eliminated_lim[i+1]); j++){
            assert(j < eliminated.size());
            Clause& c = *eliminated[j];

            //fprintf(stderr, "checking clause: "); printClause(c); fprintf(stderr, "\n");

            for (int k = 0; k < c.size(); k++)
                if (var(c[k]) == v)
                    l = c[k];
                else if (value(c[k]) != l_False)
                    goto next;

            assert(l != lit_Undef);
            //fprintf(stderr, "Fixing var %d to %d\n", v+1, !sign(l));

            assigns[v] = toInt(lbool(!sign(l)));
            break;

        next:;
        }

        if (value(v) == l_Undef)
            assigns[v] = toInt(l_True);
    }
}


bool Solver::eliminate()
{
    assert(subsumption);

    int cnt = 0;
    //fprintf(stderr, "eliminating variables\n");

#ifdef INVARIANTS
    // check that all clauses are simplified
    fprintf(stderr, "Checking that all clauses are normalized prior to variable elimination\n");
    for (int i = 0; i < clauses.size(); i++)
        if (clauses[i]->mark() == 0){
            Clause& c = *clauses[i];
            for (int j = 0; j < c.size(); j++)
                assert(value(c[j]) == l_Undef);
        }
    fprintf(stderr, "done.\n");
#endif

    for (;;){
        gather(subsumption_queue);

        if (subsumption_queue.size() == 0 && heap.size() == 0)
            break;

        //fprintf(stderr, "backwards subsumption: %10d\n", subsumption_queue.size());
        if (!backwardSubsumptionCheck())
            return false;

        //fprintf(stderr, "variable elimination:  %10d\n", heap.size());
        cnt = 0;
        for (;;){
            assert(!heap.empty() || heap.size() == 0);
            if (heap.empty())
                break;

            Var elim = heap.getmin();

            assert(hasVarProp(elim, p_frozen));

            //for (int i = 1; i < heap.heap.size(); i++)
            //    assert(heap.comp(elim, heap.heap[i]) || !heap.comp(elim, heap.heap[i]));

            //if (cnt++ % 100 == 0)
            //    fprintf(stderr, "left %10d\r", heap.size());
            
            if (!eliminateVar(elim))
                return false;
        }
    }
#ifdef INVARIANTS
    // check that no more subsumption is possible
    fprintf(stderr, "Checking that no more subsumption is possible\n");
    cnt = 0;
    for (int i = 0; i < clauses.size(); i++){
        if (cnt++ % 1000 == 0)
            fprintf(stderr, "left %10d\r", clauses.size() - i);
        for (int j = 0; j < i; j++)
            assert(clauses[i]->mark() ||
                   clauses[j]->mark() ||
                   subsumes(*clauses[i], *clauses[j]) == lit_Error);
    }
    fprintf(stderr, "done.\n");

    // check that no more elimination is possible
    fprintf(stderr, "Checking that no more elimination is possible\n");
    for (int i = 0; i < nVars(); i++){
        if (hasVarProp(i, p_frozen))
            eliminateVar(i, true);
    }
    fprintf(stderr, "done.\n");

#endif

    assert(qhead == trail.size());

    return true;
}
};
