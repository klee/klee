#!/usr/bin/python

import gv

g = gv.digraph("G")
n = gv.node(g,"hello")
m = gv.node(g,"world")
e = gv.edge(n,m)
gv.layout(g, "dot")
gv.render(g, "png", "gv_test.png")
gv.rm(g)
