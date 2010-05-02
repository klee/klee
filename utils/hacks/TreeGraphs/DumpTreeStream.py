#!/usr/bin/python

from __future__ import division

import sys, os, struct

def getTreeStream(path):
    data = open(path,'rb').read()
    paths = { 0 : ''}
    pos = 0
    while pos<len(data):
        id,tag = struct.unpack('II', data[pos:pos+8])
        pos += 8
        if tag&(1<<31):
            child = tag ^ (1<<31)
            paths[child] = paths[id]
        else:
            size = tag
            paths[id] += data[pos:pos+size]
            pos += size
    if pos!=len(data):
        raise IOError,'bad position'
    return paths

def writeTreeStream(path, output):
    paths = getTreeSTream(path)
    print 'Writing %d paths'%len(paths)
    for i,data in paths.items():
        if i!=0:
            f = open('%s%04d'%(output,i), 'wb')
            f.write(data)
            f.close()
            
def main(args):
    from optparse import OptionParser
    op = OptionParser("usage: %prog input outputPrefix")
    opts,args = op.parse_args()

    input,outputPrefix = args

    f = open(input,'rb')
    data = f.read()
    f.close()
    writeTreeStream(data, outputPrefix)
    
if __name__=='__main__':
    main(sys.argv)

