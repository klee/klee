#!/bin/sh
# next line is a comment in tcl \
exec tclsh "$0" ${1+"$@"}

package require Gdtclft

# Create a table of all HTML 4.0 Entities in a PNG image.
#
# John Ellson <ellson@graphviz.org>

#set font symbol
set font freesans

set titlefont times
set gd [gd create 1100 850]
set white [gd color new $gd 255 255 255]
set black [gd color new $gd 0 0 0]
set red [gd color new $gd 255 0 0]
set green [gd color new $gd 0 255 0]

proc incr_rowcol {} {
	upvar 1 row row col col x1 x1 x2 x2 x3 x3 y y
	if {! ($row % 40)} {
		set x1 [expr $col * 120 + 10]
		set x2 [expr $x1 + 30]
		set x3 [expr $x1 + 60]
		incr col
		set y 10
	}
	incr row
	incr y 20
}

# lay down a title on the background
gd text $gd $green $titlefont 50. .7 350 500 "HTML 4.0 Entities\r\nfont = $font"

set row 0
set col 0

# first initialize names for asci characters 32-126 to themselves
for {set val 32} {$val < 127} {incr val} {
	set entity($val) [format {%c} $val]
}

# get other names for  html-4.0 characters from: 
#          http://www.w3.org/TR/REC-html40/sgml/entities.html
set f [open entities.html r]
while {! [eof $f]} {
	set rec [gets $f]
	if {[scan $rec {&lt;!ENTITY %s CDATA "&amp;#%d;" --} name val] == 2} {
		set entity($val) $name
	}
}

# sort and render
foreach val [lsort -integer [array names entity]] {
	incr_rowcol
	if {$entity($val) == "hearts" || $entity($val) == "diams"} {
		# just for fun!
		set fontcolor $red
	} {
		set fontcolor $black
	}
	gd text $gd $black $titlefont 8. 0. $x1 $y [format {%5d} $val]
	gd text $gd $fontcolor $font 16. 0. $x2 $y [format {&#%d;} $val]
	gd text $gd $black $titlefont 12. 0. $x3 $y $entity($val)
}
close $f

set f stdout
#set f [open entities.png w]
#set f [open "| xv -" w]
gd writePNG $gd $f
#close $f
