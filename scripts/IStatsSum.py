#!/usr/bin/python

from __future__ import division

import sys, os

def getSummary(input):
    inputs = [[None,iter(open(input))]]
    def getLine(elt):
        la,i = elt
        if la is None:
            try:
                ln = i.next()
            except StopIteration:
                ln = None
            except:
                raise ValueError("unexpected IO error")
            return ln
        else:
            elt[0] = None
            return la
    def getLines():
        return map(getLine,inputs)
    def putback(ln,elt):
        assert elt[0] is None
        elt[0] = ln
        
    events = None
    
    # read header (up to ob=)
    while 1:
        lns = getLines()
        ln = lns[0]
        if ln.startswith('ob='):
            break
        else:
            if ln.startswith('positions:'):
                if ln!='positions: instr line\n':
                    raise ValueError("unexpected 'positions' directive")
            elif ln.startswith('events:'):
                events = ln[len('events: '):].strip().split(' ')

    if events is None:
        raise ValueError('missing events directive')
    boolTypes = set(['Icov','Iuncov'])
    numEvents = len(events)
    eventTypes = [e in boolTypes for e in events]

    def readCalls():
        results = {}
        for elt in inputs:
            while 1:
                ln = getLine(elt)
                if ln is not None and (ln.startswith('cfl=') or ln.startswith('cfn=')):
                    if ln.startswith('cfl='):
                        cfl = ln
                        cfn = getLine(elt)
                        if not cfn.startswith('cfn='):
                            raise ValueError("unexpected cfl directive without function")
                    else:
                        cfl = None
                        cfn = ln
                    target = getLine(elt)
                    if not target.startswith('calls='):
                        raise ValueError("unexpected cfn directive with calls")
                    stat = map(int,getLine(elt).strip().split(' '))
                    key = target
                    existing = results.get(target)
                    if existing is None:
                        results[key] = [cfl,cfn,target,stat]
                    else:
                        if existing[0]!=cfl or existing[1]!=cfn:
                            raise ValueError("multiple call descriptions for a single target")
                        existing[3] = mergeStats([existing[3],stat])
                else:
                    putback(ln, elt)
                    break
        return results

    summed = [0]*len(events)
    
    # read statistics
    while 1:
        lns = getLines()
        ln = lns[0]
        if ln is None:
            break
        elif ln.startswith('fn') or ln.startswith('fl'):
            pass
        elif ln.strip():
            # an actual statistic
            data = [map(int,ln.strip().split(' ')) for ln in lns][0]
            summed = map(lambda a,b: a+b, data[2:], summed)

            # read any associated calls
            for cfl,cfn,calls,stat in readCalls().values():
                pass

    return events,summed
    
def main(args):
    from optparse import OptionParser
    op = OptionParser("usage: %prog [options] file")
    opts,args = op.parse_args()

    total = {}
    for i in args:
        events,summed = getSummary(i)
        for e,s in zip(events,summed):
            total[e] = total.get(e,[0,0])
            total[e][0] += s
            total[e][1] += 1
        print '-- %s --'%(i,)
        items = zip(events,summed)
        items.sort()
        for e,s in items:
            print '%s: %s'%(e,s)

    print '-- totals --'
    items = total.items()
    table = []    
    for e,(s,N) in items:
        table.append((str(e),str(s),str(N),str(s//N)))
    w = map(lambda l: max(map(len,l)), zip(*table))
    for (a,b,c,d) in table:
        print '%-*s: %*s (in %*s files, avg: %*s)'%(w[0],a,w[1],b,w[2],c,w[3],d)

if __name__=='__main__':
    main(sys.argv)
