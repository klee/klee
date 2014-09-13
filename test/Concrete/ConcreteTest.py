#!/usr/bin/python

import argparse
import os
import popen2
import sys
import shutil

def readFile(f):
    s = ""
    while 1:
        data = f.read()
        if not data:
            break
        s += data
    return s

def testFile(name, klee_path, lli_path):
    baseName,ext = os.path.splitext(name)
    exeFile = 'Output/linked_%s.bc'%baseName

    print '-- building test bitcode --'
    make_cmd = 'make %s 2>&1' % (exeFile,)
    print "EXECUTING: %s" % (make_cmd,)
    sys.stdout.flush()
    if os.system(make_cmd):
        raise SystemExit('make failed')

    print '\n-- running lli --'
    lli_cmd = '%s -force-interpreter=true %s' % (lli_path, exeFile)
    print "EXECUTING: %s" % (lli_cmd,)
    lli = popen2.Popen3(lli_cmd)
    lliOut = readFile(lli.fromchild)
    if lli.wait():
        raise SystemExit('lli execution failed')

    print '-- lli output --\n%s--\n' % (lliOut,)

    print '-- running klee --'
    klee_out_path = "Output/%s.klee-out" % (baseName,)
    if os.path.exists(klee_out_path):
        shutil.rmtree(klee_out_path)
    klee_cmd = '%s --output-dir=%s --no-output %s' % (
        klee_path, klee_out_path, exeFile)
    print "EXECUTING: %s" % (klee_cmd,)
    sys.stdout.flush()
    klee = popen2.Popen3(klee_cmd)
    kleeOut = readFile(klee.fromchild)
    if klee.wait():
        raise SystemExit('klee execution failed')

    print '-- klee output --\n%s--\n' % (kleeOut,)
        
    if lliOut != kleeOut:
        raise SystemExit('outputs differ')
        
def testOneFile(f, printOutput=False):
    try:
        testFile(f, printOutput)
        code = ['pass','xpass'][f.startswith('broken')]
        extra = ''
    except TestError,e:
        code = ['fail','xfail'][f.startswith('broken')]
        extra = str(e)

    print '%s: %s -- %s'%(code,f,extra)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('test_path', help='test path')

    parser.add_argument('--klee', dest='klee_path',
                        help="path to the klee binary",
                        required=True)
    parser.add_argument('--lli', dest='lli_path',
                        help="path to the lli binary",
                        required=True)

    opts = parser.parse_args()

    test_name = os.path.basename(opts.test_path)
    testFile(test_name, opts.klee_path, opts.lli_path)

if __name__=='__main__':
    main()
