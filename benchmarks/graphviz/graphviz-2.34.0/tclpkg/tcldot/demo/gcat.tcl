#!/bin/sh
# next line is a comment in tcl \
exec tclsh "$0" ${1+"$@"}

package require Tcldot

#  gcat f1 [f2 ... ]
#
# merge a number of digraphs into one and output to stdout
#
# John Ellson  <ellson@graphviz.org>
# 

# merge graph $g2 into $g
proc merge {g g2} {
	foreach i [$g2 listnodes] {
		set n [$g addnode [$i showname]]
		$n setattributes [$i queryattributevalues [$i listattributes]]
	}
	foreach i [$g2 listedges] {
		foreach {t h} [$i listnodes] {break}
		set e [$g addedge [$t showname] [$h showname]]
		$e setattributes [$i queryattributevalues [$i listattributes]]
	}
}

set g [dotnew digraph]
if {$argc} {
	foreach file $argv {
		set f [open $file r]
		if {[catch {dotread $f} g2]} {
			puts stderr $g2
			exit
		}
		merge $g $g2
		close $f
	}
} {
	while {![catch {dotread stdin} g2]} {
		merge $g $g2
	}
	if {![eof stdin]} {
		puts stderr $g2
		exit
	}
}
$g write stdout CANON
