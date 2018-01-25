#!/usr/bin/lua

-- display the kernel module dependencies

-- author: John Ellson <ellson@research.att.com>

require('gv')

modules = io.lines('/proc/modules') 

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

for rec in modules do
   for mod, usedbylist in string.gfind(rec, "([_%w]+) %w+ %w+ ([-,_%w]+)") do
      n = gv.node(G, mod)
      for usedby in string.gfind(usedbylist, "([-_%w]+)") do
         if (usedby ~= '-') and (usedby ~= '') then
            gv.edge(n, gv.node(G, usedby))
         end
      end
   end
end

gv.layout(G, 'dot')
gv.render(G, 'gtk')
