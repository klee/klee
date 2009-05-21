#!/usr/bin/python

from __future__ import division

import sys, os

class MergeError(Exception):
    pass

def checkAssemblies(directories):
    def read(d):
        try:
            return open(os.path.join(d,'assembly.ll')).read()
        except:
            raise MergeError("unable to open assembly for: %s"%(`d`,))
    
    reference = read(directories[0])
    for d in directories[1:]:
        if reference != read(d):
            return False
    return True

def allEqual(l):
    return not [i for i in l if i!=l[0]]

def merge(inputs, output, outputDir):
    inputs = [[None,iter(i)] for i in inputs]

    def getLine(elt):
        la,i = elt
        if la is None:
            try:
                ln = i.next()
            except StopIteration:
                ln = None
            except:
                raise MergeError("unexpected IO error")
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
            if [l for l in lns if not l.startswith('ob=')]:
                raise MergeError("headers differ")
            output.write('ob=%s\n'%(os.path.join(outputDir,'assembly.ll'),))
            break
        else:
            if ln.startswith('positions:'):
                if ln!='positions: instr line\n':
                    raise MergeError("unexpected 'positions' directive")
            elif ln.startswith('events:'):
                events = ln[len('events: '):].strip().split(' ')
            if not allEqual(lns):
                raise MergeError("headers differ")
            output.write(ln)

    if events is None:
        raise MergeError('missing events directive')
    boolTypes = set(['Icov','Iuncov'])
    numEvents = len(events)
    eventTypes = [e in boolTypes for e in events]

    def mergeStats(datas):
        if not allEqual([d[:2] for d in datas]):
            raise MergeError("instruction or line specifications differ")
        elif [d for d in datas if len(d)!=numEvents+2]:
            raise MergeError("statistics differ in event counts")
        
        result = [datas[0][0],datas[0][1]]
        for i,ev in enumerate(events):
            s = sum([d[i+2] for d in datas])
            if ev=='Icov':
                result.append(max([d[i+2] for d in datas])) # true iff any are non-zero
            elif ev=='Iuncov':
                result.append(min([d[i+2] for d in datas])) # true iff any are non-zero
            else:
                result.append(s)
        return result

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
                            raise MergeError("unexpected cfl directive without function")
                    else:
                        cfl = None
                        cfn = ln
                    target = getLine(elt)
                    if not target.startswith('calls='):
                        raise MergeError("unexpected cfn directive with calls")
                    stat = map(int,getLine(elt).strip().split(' '))
                    key = target
                    existing = results.get(target)
                    if existing is None:
                        results[key] = [cfl,cfn,target,stat]
                    else:
                        if existing[0]!=cfl or existing[1]!=cfn:
                            raise MergeError("multiple call descriptions for a single target")
                        existing[3] = mergeStats([existing[3],stat])
                else:
                    putback(ln, elt)
                    break
        return results
    
    # read statistics
    while 1:
        lns = getLines()
        ln = lns[0]
        if ln is None:
            if not allEqual(lns):
                raise MergeError("unexpected end of input")
            break
        elif ln.startswith('fn') or ln.startswith('fl'):
            if not allEqual(lns):
                raise MergeError("files differ")
            output.write(ln)
        else:
            # an actual statistic
            data = [map(int,ln.strip().split(' ')) for ln in lns]
            print >>output,' '.join(map(str,mergeStats(data)))

            # read any associated calls
            for cfl,cfn,calls,stat in readCalls().values():
                if cfl is not None:
                    output.write(cfl)
                output.write(cfn)
                output.write(calls)
                print >>output,' '.join(map(str,stat))
            
def main(args):
    from optparse import OptionParser
    op = OptionParser("usage: %prog [options] directories+ output")
    opts,args = op.parse_args()

    output = args.pop()
    directories = args

    if len(directories)<=1:
        op.error("incorrect number of arguments")

    print 'Merging:',', '.join(directories)
    print 'Into:',output

    if not checkAssemblies(directories):
        raise MergeError("executables differ")

    if not os.path.exists(output):
        os.mkdir(output)
        
    assembly = open(os.path.join(directories[0],'assembly.ll')).read()
    open(os.path.join(output,'assembly.ll'),'w').write(assembly)

    inputs = [open(os.path.join(d,'run.istats')) for d in directories]
    merge(inputs, open(os.path.join(output,'run.istats'),'w'), output)

if __name__=='__main__':
    main(sys.argv)
