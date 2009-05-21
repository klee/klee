/****************************************************************************************[Solver.h]
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

#ifndef Solver_h
#define Solver_h

#include "SolverTypes.h"
#include "VarOrder.h"

namespace MINISAT {
// Redfine if you want output to go somewhere else:
#define reportf(format, args...) ( printf(format , ## args), fflush(stdout) )


//=================================================================================================
// Solver -- the main class:
struct SolverStats {
    int64   starts, decisions, propagations, conflicts;
    int64   clauses_literals, learnts_literals, max_literals, tot_literals;
    int64   subsumption_checks, subsumption_misses, merges;
    SolverStats() : 
        starts(0), decisions(0), propagations(0), conflicts(0)
      , clauses_literals(0), learnts_literals(0), max_literals(0), tot_literals(0) 
      , subsumption_checks(0), subsumption_misses(0), merges(0)
    { }
};


struct SearchParams {
    double  var_decay, clause_decay, random_var_freq;
    double  restart_inc, learntsize_inc, learntsize_factor;
    int     restart_first;
    
    SearchParams(double v = 0.95, double c = 0.999, double r = 0.02,
                 double ri = 1.5, double li = 1.1, double lf = (double)1/(double)3,
                 int rf = 100) : 
        var_decay(v), clause_decay(c), random_var_freq(r),
        restart_inc(ri), learntsize_inc(li), learntsize_factor(lf),
        restart_first(rf) { }
};

  struct ElimLt {
    const vec<int>& n_occ;
    
    ElimLt(const vec<int>& no) : n_occ(no) {}
    int  cost      (Var x)        const { return n_occ[toInt(Lit(x))] * n_occ[toInt(~Lit(x))]; }
    bool operator()(Var x, Var y) const { return cost(x) < cost(y); } 
  };

class Solver {
protected:
    // Solver state:    
    bool                ok;               // If FALSE,the constraints are already unsatisfiable. 
                                          // No part of solver state may be used!
    vec<Clause*>        clauses;          // List of problem clauses.
    vec<Clause*>        learnts;          // List of learnt clauses.
    int                 n_bin_clauses;    // Keep track of number of binary clauses "inlined" into the watcher lists (we do this primarily to get identical behavior to the version without the binary clauses trick).
    double              cla_inc;          // Amount to bump next clause with.
    double              cla_decay;        // INVERSE decay factor for clause activity: stores 1/decay.

    vec<double>         activity;         // A heuristic measurement of the activity of a variable.
    double              var_inc;          // Amount to bump next variable with.
    double              var_decay;        // INVERSE decay factor for variable activity: stores 1/decay. Use negative value for static variable order.
    VarOrder            order;            // Keeps track of the decision variable order.
    vec<char>           properties;       // TODO: describe!!!

    vec<vec<Clause*> >  watches;          // 'watches[lit]' is a list of constraints watching 'lit' (will go there if literal becomes true).
    vec<char>           assigns;          // The current assignments (lbool:s stored as char:s).
    vec<Lit>            trail;            // Assignment stack; stores all assigments made in the order they were made.
    vec<int>            trail_lim;        // Separator indices for different decision levels in 'trail'.
    vec<Clause*>        reason;           // 'reason[var]' is the clause that implied the variables current value, or 'NULL' if none.
    vec<TrailPos>       trailpos;         // 'trailpos[var]' contains the position in the trail at wich the assigment was made.
    int                 qhead;            // Head of queue (as index into the trail -- no more explicit propagation queue in MiniSat).
    int                 simpDB_assigns;   // Number of top-level assignments since last execution of 'simplifyDB()'.
    int64               simpDB_props;     // Remaining number of propagations that must be made before next execution of 'simplifyDB()'.
    vec<Lit>            assumptions;      // Current set of assumptions provided to solve by the user.

    bool                subsumption;
    vec<char>           touched;
    vec<vec<Clause*> >  occurs;
    vec<int>            n_occ;
    Heap<ElimLt>        heap;
    vec<Clause*>        subsumption_queue;

    vec<Clause*>        eliminated;
    vec<int>            eliminated_lim;
    vec<Var>            eliminated_var;

    // Temporaries (to reduce allocation overhead). Each variable is prefixed by the method in which it is
    // used, exept 'seen' wich is used in several places.
    //
    vec<char>           seen;
    vec<Lit>            analyze_stack;
    vec<Lit>            analyze_toclear;
    Clause*             propagate_tmpempty;
    Clause*             propagate_tmpbin;
    Clause*             analyze_tmpbin;
    Clause*             bwdsub_tmpunit;

    vec<Lit>            addBinary_tmp;
    vec<Lit>            addTernary_tmp;

    // Main internal methods:
    //
    bool        assume           (Lit p);
    void        cancelUntil      (int level);
    void        record           (const vec<Lit>& clause);

    void        analyze          (Clause* confl, vec<Lit>& out_learnt, int& out_btlevel); // (bt = backtrack)
    bool        analyze_removable(Lit p, uint min_level);                                 // (helper method for 'analyze()')
    void        analyzeFinal     (Lit p, vec<Lit>& out_conflict);
    bool        enqueue          (Lit fact, Clause* from = NULL);
    Clause*     propagate        ();
    void        reduceDB         ();
    Lit         pickBranchLit    ();
    lbool       search           (int nof_conflicts, int nof_learnts);
    double      progressEstimate ();

    // Variable properties:
    void        setVarProp (Var v, uint prop, bool b) { order.setVarProp(v, prop, b); }
    bool        hasVarProp (Var v, uint prop) const   { return order.hasVarProp(v, prop); }
    void        updateHeap (Var v) { 
        if (hasVarProp(v, p_frozen))
            heap.update(v); }

    // Simplification methods:
    //
    void cleanOcc (Var v) {
        assert(subsumption);
        vec<Clause*>& occ = occurs[v];
        int i, j;
        for (i = j = 0; i < occ.size(); i++)
            if (occ[i]->mark() != 1)
                occ[j++] = occ[i];
        occ.shrink(i - j); 
    }

    vec<Clause*>& getOccurs                (Var x) { cleanOcc(x); return occurs[x]; }
    void          gather                   (vec<Clause*>& clauses);
    Lit           subsumes                 (const Clause& c, const Clause& d);
    bool          assymmetricBranching     (Clause& c);
    bool          merge                    (const Clause& _ps, const Clause& _qs, Var v, vec<Lit>& out_clause);
    
    bool          backwardSubsumptionCheck ();
    bool          eliminateVar             (Var v, bool fail = false);
    bool          eliminate                ();
    void          extendModel              ();

    // Activity:
    //
    void     varBumpActivity(Lit p) {
        if (var_decay < 0) return;     // (negative decay means static variable order -- don't bump)
        if ( (activity[var(p)] += var_inc) > 1e100 ) varRescaleActivity();
        order.update(var(p)); }
    void     varDecayActivity  () { if (var_decay >= 0) var_inc *= var_decay; }
    void     varRescaleActivity();
    void     claDecayActivity  () { cla_inc *= cla_decay; }
    void     claRescaleActivity();

    // Operations on clauses:
    //
    bool     newClause(const vec<Lit>& ps, bool learnt = false, bool normalized = false);
    void     claBumpActivity (Clause& c) { if ( (c.activity() += cla_inc) > 1e20 ) claRescaleActivity(); }
    bool     locked          (const Clause& c) const { return reason[var(c[0])] == &c; }
    bool     satisfied       (Clause& c) const;
    bool     strengthen      (Clause& c, Lit l);
    void     removeClause    (Clause& c, bool dealloc = true);

    int      decisionLevel() const { return trail_lim.size(); }

public:
    Solver() : ok               (true)
             , n_bin_clauses    (0)
             , cla_inc          (1)
             , cla_decay        (1)
             , var_inc          (1)
             , var_decay        (1)
             , order            (assigns, activity)
             , qhead            (0)
             , simpDB_assigns   (-1)
             , simpDB_props     (0)
             , subsumption      (true)
             , heap             (n_occ)
             , params           ()
             , expensive_ccmin  (true)
             , verbosity        (0)
             , progress_estimate(0)
             {
                vec<Lit> dummy(2,lit_Undef);
                propagate_tmpbin   = Clause_new(dummy);
                analyze_tmpbin     = Clause_new(dummy);
                dummy.pop();
                bwdsub_tmpunit     = Clause_new(dummy);
                dummy.pop();
                propagate_tmpempty = Clause_new(dummy);
                addBinary_tmp .growTo(2);
                addTernary_tmp.growTo(3);
             }

   ~Solver() {
       xfree(propagate_tmpbin);
       xfree(analyze_tmpbin);
       xfree(bwdsub_tmpunit);
       xfree(propagate_tmpempty);
       for (int i = 0; i < eliminated.size(); i++) xfree(eliminated[i]);
       for (int i = 0; i < learnts.size();    i++) xfree(learnts[i]);
       for (int i = 0; i < clauses.size();    i++) xfree(clauses[i]); }

    // Helpers: (semi-internal)
    //
    lbool   value(Var x) const { return toLbool(assigns[x]); }
    lbool   value(Lit p) const { return sign(p) ? ~toLbool(assigns[var(p)]) : toLbool(assigns[var(p)]); }

    int     nAssigns()   { return trail.size(); }
    int     nClauses()   { return clauses.size(); }
    int     nLearnts()   { return learnts.size(); }
    int     nConflicts() { return (int)stats.conflicts; }

    // Statistics: (read-only member variable)
    //
    SolverStats     stats;

    // Mode of operation:
    //
    SearchParams    params;             // Restart frequency etc.
    bool            expensive_ccmin;    // Controls conflict clause minimization. TRUE by default.
    int             verbosity;          // Verbosity level. 0=silent, 1=some progress report, 2=everything

    // Problem specification:
    //
    Var     newVar    (bool polarity = true, bool dvar = true);
    int     nVars     ()                    { return assigns.size(); }
    bool    addUnit   (Lit p)               { return ok && (ok = enqueue(p)); }
    bool    addBinary (Lit p, Lit q)        { addBinary_tmp [0] = p; addBinary_tmp [1] = q; return addClause(addBinary_tmp); }
    bool    addTernary(Lit p, Lit q, Lit r) { addTernary_tmp[0] = p; addTernary_tmp[1] = q; addTernary_tmp[2] = r; return addClause(addTernary_tmp); }
    bool    addClause (const vec<Lit>& ps)  { if (ok && !newClause(ps)) ok = false; return ok; }

    // Variable mode:
    // 
    void    freezeVar    (Var v) { setVarProp(v, p_frozen, true); updateHeap(v); }

    // Solving:
    //
    bool    okay         () { return ok; }       // FALSE means solver is in a conflicting state
    bool    simplifyDB   (bool expensive = true);
    bool    solve        (const vec<Lit>& assumps);
    bool    solve        () { vec<Lit> tmp; return solve(tmp); }
    void    turnOffSubsumption() {
        subsumption = false;
        occurs.clear(true);
        n_occ.clear(true);
    }

    double      progress_estimate;  // Set by 'search()'.
    vec<lbool>  model;              // If problem is satisfiable, this vector contains the model (if any).
    vec<Lit>    conflict;           // If problem is unsatisfiable (possibly under assumptions), this vector represent the conflict clause expressed in the assumptions.

  double  returnActivity(int i) { return activity[i];}
  void    updateInitialActivity(int i, double act) {activity[i] = act; order.heap.update(i);}
};


//=================================================================================================
// Debug:


#define L_LIT    "%sx%d"
#define L_lit(p) sign(p)?"~":"", var(p)

// Just like 'assert()' but expression will be evaluated in the release version as well.
inline void check(bool expr) { assert(expr); }

static void printLit(Lit l)
{
    fprintf(stderr, "%s%d", sign(l) ? "-" : "", var(l)+1);
}

template<class C>
static void printClause(const C& c)
{
    for (int i = 0; i < c.size(); i++){
        printLit(c[i]);
        fprintf(stderr, " ");
    }
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#ifdef _MSC_VER

#include <ctime>

static inline double cpuTime(void) {
    return (double)clock() / CLOCKS_PER_SEC; }

static inline int64 memUsed() {
    return 0; }

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#else

#include <sys/time.h>
#include <sys/resource.h>

static inline double cpuTime(void) {
    struct rusage ru;
    getrusage(RUSAGE_SELF, &ru);
    return (double)ru.ru_utime.tv_sec + (double)ru.ru_utime.tv_usec / 1000000; }

#if defined(__linux__) || defined(__CYGWIN__)
static inline int memReadStat(int field)
{
    char    name[256];
    pid_t pid = getpid();
    sprintf(name, "/proc/%d/statm", pid);
    FILE*   in = fopen(name, "rb");
    if (in == NULL) return 0;
    int     value;
    for (; field >= 0; field--) {
      int res = fscanf(in, "%d", &value);
      (void) res;
    }
    fclose(in);
    return value;
}

static inline int64 memUsed() { return (int64)memReadStat(0) * (int64)getpagesize(); }
#else
// use this by default. Mac OS X (Darwin) does not define an os type
//defined(__FreeBSD__)

static inline int64 memUsed(void) {
    struct rusage ru;
    getrusage(RUSAGE_SELF, &ru);
    return ru.ru_maxrss*1024; }

#endif

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#endif

//=================================================================================================
};
#endif
