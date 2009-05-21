#!/usr/bin/python

import os
import sys
import popen2

class TestError(Exception):
    pass

kLLIPath = '../../llvm/Debug/bin/lli'
kKleePath = '../../Debug/bin/klee'

def getFiles():
    for name in os.listdir('.'):
        if (name[0]!='.' and name[0]!='_' and
            (name.endswith('.ll') or name.endswith('.c'))):
            yield name

def readFile(f):
    s = ""
    while 1:
        data = f.read()
        if not data:
            break
        s += data
    return s

def testFile(name, printOutput=False):
    baseName,ext = os.path.splitext(name)
    exeFile = 'Output/linked_%s.bc'%baseName
    if printOutput:
        redirectStderr = ''
    else:
        redirectStderr = '2> /dev/null'

    if os.system('make %s > /dev/null %s'%(exeFile,redirectStderr)):
        raise TestError('make failed')

    if printOutput:
        print '-- running lli --'
    lli = popen2.Popen3('%s -force-interpreter=true %s %s'%(kLLIPath,exeFile,redirectStderr))
    lliOut = readFile(lli.fromchild)
    if lli.wait():
        raise TestError('lli execution failed')

    if printOutput:
        print lliOut

    if printOutput:
        print '-- running klee --'
    klee = popen2.Popen3('%s --no-output %s %s'%(kKleePath,exeFile,redirectStderr))
    kleeOut = readFile(klee.fromchild)
    if klee.wait():
        raise TestError('klee execution failed')
    if printOutput:
        print kleeOut
        
    if lliOut!=kleeOut:
        raise TestError('outputs differ')
        
def testOneFile(f, printOutput=False, log=None):
    try:
        testFile(f, printOutput)
        code = ['pass','xpass'][f.startswith('broken')]
        extra = ''
    except TestError,e:
        code = ['fail','xfail'][f.startswith('broken')]
        extra = str(e)

    print '%s: %s -- %s'%(code,f,extra)
    if log:
        print >>log,'%s: %s -- %s'%(code,f,extra)

def test():
    if not os.path.exists('Output'):
        os.mkdir('Output')
    log = open("Output/test.log","w")
    files = list(getFiles())
    files.sort(key = lambda n: n.lower())
    for f in files:
        testOneFile(f, log=log)
    log.close()

if __name__=='__main__':
    args = sys.argv
    args.pop(0)
    
    runAll = not args

    while args:
        arg = args.pop(0)
        if arg=='--run':
            testFile(args.pop(0), printOutput=True)
        else:
            raise ValueError,'invalid argument: %s'%arg

    if runAll:
        test()
        
