#!/usr/bin/python

# display the kernel module dependencies

# author: Michael Hohn <mhhohn@lbl.gov>
#  based on: modgraph.tcl by John Ellson <ellson@research.att.com>

import sys
import gv

modules = open("/proc/modules", 'r').readlines()

G = gv.digraph("G")
N = gv.protonode(G)
E = gv.protoedge(G)

gv.setv(G, 'rankdir', 'LR')
gv.setv(G, 'nodesep', '0.05')
gv.setv(N, 'shape', 'box')
gv.setv(N, 'width', '0')
gv.setv(N, 'height', '0')
gv.setv(N, 'margin', '.03')
gv.setv(N, 'fontsize', '8')
gv.setv(N, 'fontname', 'helvetica')
gv.setv(E, 'arrowsize', '.4')

for rec in modules:
   fields = rec.split(' ')
   n = gv.node(G, fields[0])
   for usedby in fields[3].split(','):
      if (usedby != '-') & (usedby != ''):
         gv.edge(n, gv.node(G, usedby))

gv.layout(G, 'dot')
gv.render(G, 'xlib')
