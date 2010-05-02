#!/usr/bin/python

from __future__ import division
import sys
from types import GeneratorType

import DumpTreeStream
from Graphics.Canvas import PdfCanvas
from Graphics.Geometry import vec2
import os, time
import math, os, random

def generator_fold(it):
    """generator_fold(it) -> iterator

    Given _it_, return a new iterator over the elements of _it_,
    (recursively) folding in any elements whenever an iterator
    result happens to also be a generator. This simplifies the
    writing of iterators over recursive data structures.
    """
   
    for res in it:
        if isinstance(res,GeneratorType):
            for res in generator_fold(res):
                yield res
        else:
            yield res

                
def drawTree(a, b, maxDepth, sizes, depth=0):
    yield (a, b)
    if depth<maxDepth:
        height = sizes[depth]
        yield drawTree(b, vec2.add(b, (-height,height)),
                       maxDepth, sizes, depth+1)
        yield drawTree(b, vec2.add(b, (+height,height)),
                       maxDepth, sizes, depth+1)
    
def makeTreeGraph(output, symPath, count, shuffle=False):
    random.seed(10)
    c = PdfCanvas(output, basePos=(0,0), baseScale=(72*5,72*5),
                  pageSize=(72*10,72*10))

    c.startDrawing()
    c.setColor(1,1,1)
    c.drawFilledCircle((0,0),.9)
    c.setColor(0,0,0)
    c.setLineWidth(5)
    c.drawOutlineCircle((0,0),.9)
    c.setLineWidth(1)
#    c.setColor(.2,.6,.3)

    N = 8
    sizes = [.5**(i+1) for i in range(N)]
    res = drawTree((0.,-.9), (0.,-.5), N, [1.3*s/sum(sizes) for s in sizes])
#    [c.drawLine(a,b) for a,b in generator_fold(res)]

    def mapIt(center, radius, spanAngle, (x,y)):
        a = vec2.sub(center,(0,radius))
        b = vec2.add(center,vec2.fromangle(math.pi*.5 + 2*x*spanAngle, radius))
        return vec2.lerp(a, b, y)
    m = lambda pt: mapIt((0,0),.9,math.pi*.7, pt)
    toBox = lambda pt: vec2.div(vec2.add(pt,(0.,.9)),
                                      (2*1.3,1.7))
    def lm((x,y)):
        x,y = toBox((x,y))
        N = 100
        return m((x,1-math.log(1+(1-y**4),2)))
        return m((x,(N**(1+y)-N)/(N**2-N)))
        return m((x,y))
#    c.drawOutlineBox((-.5,0),(.5,1))
    
    #[c.drawLine(lm(a),lm(b)) for a,b in generator_fold(res)]

    spanAngle = math.pi*.9
    zeroRad = vec2.length(vec2.sub(vec2.fromangle(math.pi*3/2,.9),
                                   vec2.fromangle(math.pi*.5 + spanAngle,.9)))
    oneRad = .9
    if 0:
        for i in range(N):
            t = i/(N-1)
            c.drawOutlineCircle(vec2.add((0.,-.9), (0.,t*.9)),
                                zeroRad + (oneRad-zeroRad)*t)

    def getTreePos(depth, maxDepth, index, ranges=None, depthOrder=None):
        isoT = depth/(maxDepth-1)
        isoCent = vec2.add((0.,-.9),(0.,isoT*.9))
        isoRadius = zeroRad + (oneRad-zeroRad)*isoT        
        total = 2**(depth+1)
        #        isoSpanAngle = math.pi*.5 - vec2.toangle(vec2.sub(vec2.fromangle(math.pi*.5 + spanAngle,.9),
        #                                             isoCent))
        isoSpanAngle = math.pi*.1 + (math.pi*.7 - math.pi*.1)*isoT
        if ranges:
            min,max = ranges
            if max[depth]==min[depth]:
                x = 0.
            else:
                x = (index - min[depth])/(max[depth]-min[depth])
        elif depthOrder:
            idx = depthOrder[depth].index(index)
            x = idx/(len(depthOrder[depth])-1)
        else:
            x = index/(total-1)
        return vec2.add(isoCent,vec2.fromangle(math.pi*.5 + (2*x-1)*isoSpanAngle, isoRadius))

    treeData = DumpTreeStream.getTreeStream(symPath)
    treeDataItems = treeData.items()
    treeDataItems.sort()

    N = max([len(p) for _,p in treeDataItems])
#    N = min(20,N)

    minDepthIndex = [2**(i+1) for i in range(N)]
    maxDepthIndex = [0 for i in range(N)]
    depthIndices = [set() for i in range(N+2)]
    for id,path in treeDataItems:
        depth = -1
        index = 0
        for p in path[:N]:
            depth += 1
            index = index*2 + (p=='1')
            depthIndices[depth].add(index)
            if 1:
                low = index*2
                hi = index*2+1
                mid = index*2 + .5
                M = 5
                for x in range(depth+1,min(N,depth+M)):
                    t = (x-(depth+1))/(M-1)
                    depthIndices[x].add(low + (mid-low)*t)
                    depthIndices[x].add(hi + (mid-hi)*t)
                    low *= 2
                    hi *= 2 + 1
                    mid = mid*2 + .5
            else:
                depthIndices[depth+1].add(index*2)
                depthIndices[depth+1].add(index*2+1)
                depthIndices[depth+2].add(index*2*2)
                depthIndices[depth+2].add(index*4+3)
            minDepthIndex[depth] = min(index,minDepthIndex[depth])
            maxDepthIndex[depth] = max(index,maxDepthIndex[depth])
    if 0:
        for d in range(1,N)[::-1]:
            minDepthIndex[d] = min(minDepthIndex[d-1]*2,minDepthIndex[d])
            maxDepthIndex[d] = max(maxDepthIndex[d-1]*2,maxDepthIndex[d])
    for d in range(N):
        depthIndices[d] = list(depthIndices[d])
        depthIndices[d].sort()

    def drawPoints(list):
        for pt in list:
            c.drawPoint(pt)

    def catmullRom1((p0,p1,p2,p3), t):
        return (0.5 * ((        2*p1            ) +
                       (  -p0        +   p2     ) * t +
                       ( 2*p0 - 5*p1 + 4*p2 - p3) * t*t +
                       (  -p0 + 3*p1 - 3*p2 + p3) * t*t*t))
    def catmullRom2((p0,p1,p2,p3), t):
        return (catmullRom1((p0[0],p1[0],p2[0],p3[0]),t),
                catmullRom1((p0[1],p1[1],p2[1],p3[1]),t))

    c.setLineWidth(2)
#    c.setColor(.2,.6,.3)
    lines = set()
    segments = set()
    segments2 = dict()
    allPoints = set()
    side = set()
    paths = []

    paths_to_draw = treeDataItems
    if shuffle:
        paths_to_draw = list(treeDataItems)
        random.shuffle(paths_to_draw)

    for id,path in paths_to_draw[:count]:
        depth = -1
        index = 0
        pts = [(0.,-.9)]
        for p in path[:N]:
            depth += 1
            index = index*2 + (p=='1')
            pts.append(getTreePos(depth,N,index,None,depthIndices))
            #            pts.append(getTreePos(depth,N,index,(minDepthIndex,maxDepthIndex)))
        allPoints |= set(pts)
        if len(pts)>3:
            paths.append(pts)
            for i in range(0,len(pts)-1):
                p0 = pts[max(0,i-1)]
                p1 = pts[i]
                p2 = pts[min(len(pts)-1,i+1)]
                p3 = pts[min(len(pts)-1,i+2)]
                #segments[(p1,p2)] = segments.get((p1,p2),[]) + [(p0,p1)]
                
                if (p1,p2) not in side:
                    segments.add((p0,p1,p2,p3))
                    side.add((p1,p2))
#                drawPoints((b,vec2.add(b,vec2.mulN(vec2.normalizeOrZero(vec2.sub(b,a)),l*.4)),
#                              cx,cx))
#        c.drawLineStrip(pts)
#        for a,b in zip(pts,pts[1:]):
#            lines.add((a,b))
    for a,b in lines:
        c.drawLine(a,b)
    for (p1,p2),extra in segments2.items():
        a,b = zip(*extra)
        a = vec2.divN(reduce(vec2.add, a),len(a))
        b = vec2.divN(reduce(vec2.add, b),len(b))
        c.drawLineStrip([catmullRom2((a,p1,p2,b),x/10.) for x in range(10+1)])
    if 1:
        side = set()
        nPaths = []
        for path in paths:
            path = list(path)
            path = [(0.,-.9)] + path
            depth = 0
            while len(path)>3 and (path[1],path[2]) in side:
                depth += 1
                path = path[1:]
            nPaths.append((depth,path))
            side |= set(zip(path,path[1:]))
#        for path in nPaths:
#            c.drawLineStrip(path)
        for depth,path in nPaths:
            ppts = []
            for i in range(1,len(path)-1):
                p0 = path[max(0,i-1)]
                p1 = path[i]
                p2 = path[min(len(path)-1,i+1)]
                p3 = path[min(len(path)-1,i+2)]
                for x in range(10):
                    ppts.append(catmullRom2((p0,p1,p2,p3),x/10.))
                #ppts.append(p1)
            ppts.append(path[-1])
            if 1:
                up = []
                down = []
                ref = path[0]
                for i,pt in enumerate(ppts):
                    ref = ppts[min(len(ppts)-1,i+1)]
                    vec = vec2.sub(pt,ref)
                    l = vec2.length(vec)
                    if l<.000001:
                        vec = (1.,0.)
                    else:
                        vec = vec2.rotate90(vec2.divN(vec,l))
                    width = .001 * (1. - (depth + i/20.)/N)
                    up.append(vec2.add(pt, vec2.mulN(vec, width)))
                    down.append(vec2.add(pt, vec2.mulN(vec, -width)))
                    ref = pt
#                c.drawLineStrip(up)
#                c.drawLineStrip(down)
                c.drawFilledPolygon(down + up[::-1])
            else:
                c.drawLineStrip(ppts)
#        print len(paths),sum(map(len,paths))
#        print len(nPaths),sum(map(len,nPaths))
    else:
        for P in segments:
            c.drawLineStrip([catmullRom2(P,x/10.) for x in range(10+1)])
    if 0:
        c.setPointSize(10)
        c.drawPoints(allPoints)
    
    if 0:
        N = 20
        for i in range(N+1):        
            t = 2*i/N - 1
            c.drawLine(m((t,0)), m((t,1)))
    
    c.endDrawing()
        
def main():
    from optparse import OptionParser
    op = OptionParser("usage: %prog [options] <tree-stream-path> <output-path>")
    op.add_option('','--count', dest='count', type=int, default=-1,
                  help="number of distinct paths to draw")
    op.add_option('','--shuffle', dest='shuffle', action='store_true',
                  default=False)
    opts,args = op.parse_args()

    if len(args) != 2:
        parser.error('invalid number of arguments')

    symPath,output = args
    makeTreeGraph(output, symPath, opts.count, opts.shuffle)
    
if __name__=='__main__':
    try:
        main()
    except:
        import sys,traceback
        traceback.print_exc(file= sys.__stdout__)
        sys.__stdout__.flush()
