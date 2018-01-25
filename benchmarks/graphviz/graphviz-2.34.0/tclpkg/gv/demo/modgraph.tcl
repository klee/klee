#!/usr/bin/tclsh

# display the kernel module dependencies 

# author: John Ellson <ellson@research.att.com>

package require gv

set f [open /proc/modules r]
set modules [read -nonewline $f]
close $f

set G [gv::digraph G]
set N [gv::protonode $G]
set E [gv::protoedge $G]

gv::setv $G rankdir LR
gv::setv $G nodesep .05
gv::setv $N shape box
gv::setv $N width 0
gv::setv $N height 0
gv::setv $N margin .03
gv::setv $N fontsize 8
gv::setv $N fontname helvetica
gv::setv $E arrowsize .4

# prepare a subraph for rootnodes - populated later
set sg [gv::graph $G rootnodes]
gv::setv $sg rank same

# extract graph from /proc/modules
foreach rec [split $modules \n] {
  set n [gv::node $G [lindex $rec 0]]
  foreach usedby [split [lindex $rec 3] ,] {
    if {[string equal $usedby -] || [string equal $usedby {}]} {continue}
    set e [gv::edge $n [gv::node $G $usedby]]
  }
}

# add all rootnodes to subgraph with rank=same
for { set n [gv::firstnode $G] } { [ gv::ok $n] } { set n [gv::nextnode $G $n] } {
  if { ! [ gv::ok [ gv::firstin $n ] ] } {
    gv::node $sg [gv::nameof $n]
  }
}

gv::layout $G dot
gv::render $G xlib
