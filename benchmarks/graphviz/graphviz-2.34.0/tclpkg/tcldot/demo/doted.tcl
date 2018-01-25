#!/bin/sh
# next line is a comment in tcl \
exec wish "$0" ${1+"$@"}

package require Tcldot

# doted - dot/gv graph editor - John Ellson (ellson@graphviz.org)
#
# Usage: doted <file.gv>
#
# doted displays the graph described in the input file and allows
# the user to add/delete nodes/edges, to modify their attributes,
# and to save the result.

global saveFill tk_library modified fileName printCommand g

# as the mouse moves over an object change its shading
proc mouse_anyenter {c} {
	global tk_library saveFill
	set item [string range [lindex [$c gettags current] 0] 1 end]
	set saveFill [list $item [lindex [$c itemconfigure 1$item -fill] 4]]
	$c itemconfigure 1$item -fill black \
		-stipple @$tk_library/demos/images/gray25.xbm
}

# as the mouse moves out of an object restore its shading
proc mouse_anyleave {c} {
	global saveFill
	$c itemconfigure 1[lindex $saveFill 0] \
		-fill [lindex $saveFill 1] -stipple {}
}

# if b1 is pressed over the brackground then start a node,
# if b1 is pressed over a node then start an edge
proc mouse_b1_press {c x y} {
	global startObj graphtype
	set x [$c canvasx $x]
	set y [$c canvasy $y]
	foreach item [$c find overlapping $x $y $x $y] {
		foreach tag [$c gettags $item] {
			if {[string first "node" $tag] == 1} {
				set item [string range $tag 1 end]
				if {[string equal $graphtype digraph]} {
					set startObj [$c create line $x $y $x $y \
						 -tag $item -fill red -arrow last]
				} {
					set startObj [$c create line $x $y $x $y \
						 -tag $item -fill red]
				}
				return
			}
		}
	}
	set startObj [$c create oval [expr $x - 10] [expr $y - 10] \
		[expr $x + 10] [expr $y + 10] -fill red -outline black]
}

# if node started by b1_press then move it,
# else extend edge
proc mouse_b1_motion {c x y} {
	global startObj
	set pos [$c coords $startObj]
	if {[$c type $startObj] == "line"} {
		$c coords $startObj [lindex $pos 0] [lindex $pos 1] \
			[$c canvasx $x] [$c canvasy $y]
	} {
		$c move $startObj [expr [$c canvasx $x] - [lindex $pos 0] - 10] \
			[expr [$c canvasy $y] - [lindex $pos 1] - 10]
	}
}

# complete node or edge construction.
proc mouse_b1_release {c x y} {
	global startObj modified g
	set x [$c canvasx $x]
	set y [$c canvasy $y]
	set t [$c type $startObj]
	if {$t == "line"} {
		set tail [lindex [$c gettags $startObj] 0]
		foreach item [$c find overlapping $x $y $x $y] {
			foreach tag [$c gettags $item] {
				set head [string range $tag 1 end]
				if {[string first "node" $head] == 0} {
					set e [$tail addedge $head]
					$c dtag $startObj $tail
					$c addtag 1$e withtag $startObj
					$c itemconfigure $startObj -fill black
					set modified 1
					set startObj {}
					return
				}
			}
		}
		# if we get here then edge isn't terminating on a node
		$c delete $startObj
	} {
		set n [$g addnode]
		$c addtag 1$n withtag $startObj
		$c itemconfigure $startObj -fill white
		set modified 1
	}
	set startObj {}
}

proc loadFileByName {c name} {
	global modified
	if {$modified} {
		confirm "Current graph has been modified.  Shall I overwrite it?" \
			"loadFileByNameDontAsk $c $name"
	} {
		loadFileByNameDontAsk $c $name
	}
}

proc loadFileByNameDontAsk {c name} {
	global fileName g
	$g delete
	$c delete all
	set modified 0
        if {[string first / $name] == 0} {
		set fileName $name
	} {
		if {[pwd] == "/"} {
			set fileName /$name
		} {
			set fileName [pwd]/$name
		}
	}
	if {[catch {open $fileName r} f]} {
		warning "Unable to open file: $fileName"
	}
	if {[catch {dotread $f} g]} {
		warning "Invalid .gv file: $fileName"
		close $f
	}
	close $f
	$g layout
	eval [$g render]
	$c configure -scrollregion [$c bbox all]
}

proc resize_canvas {c w h} {
	$c configure -scrollregion [$c bbox all]
}

proc update_entry {w x y} {
	$w.entry delete 0 end
	$w.entry insert end [$w.l.list get @$x,$y]
}

# doesn't work well with window managers that position initial window
# on the left because then all popups get obscured
#
#proc positionWindow {w} {
#	set pos [split [wm geometry .] +]
#	set x [expr [lindex $pos 1] - 350]
#	set y [expr [lindex $pos 2] + 20]
#	wm geometry $w +$x+$y
#}

proc loadFile {c} {
	global fileName

	set types {
		{{GV Graph Files} {.gv}}
		{{DOT Graph Files} {.dot}}
		{{All Files} *}
	}
	set fn [tk_getOpenFile \
		-defaultextension .gv \
		-filetypes $types \
		-initialfile $fileName]
	if {[string length $fn]} {
		loadFileByName $c $fn
	}
}

proc saveFile {type} {
	global fileName
	if {$fileName == {}} {
		saveFileAs $type
	} {
		saveFileByName $fileName $type
	}
}

proc saveFileByName {name type} {
	global fileName
	if {$name != $fileName && [file exists $name]} {
		confirm "File exists.  Shall I overwrite it?" \
			"saveFileByNameDontAsk $name $type"
	} {
		saveFileByNameDontAsk $name $type
	}
}

proc saveFileByNameDontAsk {name type} {
	global modified fileName g
	if {[catch {open $name w} f]} {
		warning "Unable to open file for write:\n$name; return"
	}
	if {$type == "gv"} {
		set type canon
		set fileName $name
		set modified 0
	}
	$g write $f $type
	close $f
	message "Graph written to:\n$name"
}

proc saveFileAs {type} {
	global fileName

	set cmap {{{CMAP Image Map Files} {.cmap}} {{All Files} *}}
	set dia {{{DIA Image Files} {.dia}} {{All Files} *}}
	set dot {{{DOT Graph Files} {.dot}} {{All Files} *}}
	set fig {{{FIG Image Files} {.fig}} {{All Files} *}}
	set gif {{{GIF Image Files} {.gif}} {{All Files} *}}
	set gv  {{{GV Graph Files} {.gv}} {{All Files} *}}
	set hpgl {{{HPGL Image Files} {.hpgl}} {{All Files} *}}
	set jpg {{{JPG Image Files} {.jpg}} {{All Files} *}}
	set mif {{{MIF Image Files} {.mif}} {{All Files} *}}
	set pcl {{{PCL Image Files} {.pcl}} {{All Files} *}}
	set pdf {{{PDF Image Files} {.pdf}} {{All Files} *}}
	set png {{{PNG Image Files} {.png}} {{All Files} *}}
	set ps {{{PostScript Files} {.ps}} {{All Files} *}}
	set svg {{{SVG Image Files} {.svg}} {{All Files} *}}
	set tiff {{{TIFF Image Files} {.tiff}} {{All Files} *}}
	set vml {{{VML Image Files} {.vml}} {{All Files} *}}
	set vtx {{{VTX Image Files} {.vtx}} {{All Files} *}}

	set fn [tk_getSaveFile \
		-defaultextension .$type \
		-filetypes [set $type] \
		-initialdir [file dirname $fileName] \
		-initialfile [file tail [file rootname $fileName]].$type]
	if {[string length $fn]} {
		saveFileByNameDontAsk $fn $type
	}
}

proc print {} {
	global g printCommand
	if {[catch {open "| $printCommand &" w} f]} {
		warning "Unable to open pipe to printer command:\n$printCommand; return"
	}
	$g write $f ps
	close $f
	message "Graph printed to:\n$printCommand"
}

proc setPrinterCommand {w} {
	global printCommand
	set printCommand [$w.printCommand get]
	message "Printer command changed to:\n$printCommand"
	destroy $w
}

proc printSetup {} {
	global printCommand
	set w .printer
	catch {destroy $w}
	toplevel $w
#	positionWindow $w
	wm title $w "Printer"
	wm iconname $w "Printer"
	label $w.message -text "Printer command:"
	frame $w.spacer -height 3m -width 20
	entry $w.printCommand 
	$w.printCommand insert end $printCommand
	bind $w.printCommand <Return> "setPrinterCommand $w"
	frame $w.buttons
	button $w.buttons.confirm -text OK -command "setPrinterCommand $w"
	button $w.buttons.cancel -text Cancel -command "destroy $w"
	pack $w.buttons.confirm $w.buttons.cancel -side left -expand 1
	pack $w.message $w.spacer $w.printCommand -side top -anchor w
	pack $w.buttons -side bottom -expand y -fill x -pady 2m
}

proc confirm {msg cmd} {
	set w .confirm
	catch {destroy $w}
	toplevel $w
#	positionWindow $w
	wm title $w "Confirm"
	wm iconname $w "Confirm"
	label $w.message -text "\n$msg\n"
	frame $w.spacer -height 3m -width 20
	frame $w.buttons
	button $w.buttons.confirm -text OK -command "$cmd; destroy $w"
	button $w.buttons.cancel -text Cancel -command "destroy $w"
	pack $w.buttons.confirm $w.buttons.cancel -side left -expand 1
	pack $w.message $w.spacer -side top -anchor w
	pack $w.buttons -side bottom -expand y -fill x -pady 2m
}

proc message {m} {
	set w .message
	catch {destroy $w}
	toplevel $w
#	positionWindow $w
	wm title $w "Message"
	wm iconname $w "Message"
	label $w.message -text "\n$m\n"
	pack $w.message -side top -anchor w
	update
	after 2000 "destroy $w"
}

proc warning {m} {
	set w .warning
	catch {destroy $w}
	toplevel $w
#	positionWindow $w
	wm title $w "Warning"
	wm iconname $w "Warning"
	label $w.message -text "\nWarning:\n\n$m"
	pack $w.message -side top -anchor w
	update
	after 2000 "destroy $w"
}

proc setoneattribute {w d a s} {
	set aa [$w.e$a.a get]
	if {$aa == {}} {
		error "no attribute name set"
	} {
		set v [$w.e$a.v get]
		eval $s $aa $v
	}
	if {$a == {}} {
		destroy $w.e
		addEntryPair $w $d $aa $v $s
		addEntryPair $w d {} {} $s
	}
}

proc addEntryPair {w d a v s} {
	pack [frame $w.e$a] -side top
	pack [entry $w.e$a.a] [entry $w.e$a.v] -side left
	if {$a != {}} {
		$w.e$a.a insert end $a
		$w.e$a.a configure -state disabled -relief flat
		$w.e$a.v insert end $v
		if {$d != "d"} {
			$w.e$a.v configure -state disabled -relief flat
		}
	}
	bind $w.e$a.a <Return> "focus $w.e$a.v"
	bind $w.e$a.v <Return> [list setoneattribute $w $d $a $s]
	pack $w.e$a -side top 
	focus $w.e$a.a
}

proc deleteobj {c o} {
	if {[string first "node" $o] == 0} {
		foreach e [$o listedges] {
			$c delete 1$e
			$c delete 0$e
			$e delete
		}
	}
	$c delete 1$o
	$c delete 0$o
	$o delete
}

# open a requestor for object $o,
#	deletable if $d is not null, 
#	command to list attribute in $l
#	command to query attributes in $q
#	command to set attributes in $s
proc setAttributesWidget {c o d l q s} {
	set w .attributes
	catch {destroy $w}
	toplevel $w
#	positionWindow $w
	wm title $w "[$o showname] Attributes"
	wm iconname $w "Attributes"
	foreach a [eval $l] {
		if {[catch {eval $q $a} v]} {set v {}}
		addEntryPair $w $d $a $v $s
	}
	addEntryPair $w d {} {} $s
	frame $w.spacer -height 3m -width 20
	frame $w.buttons
	if {$d == "d"} {
		 button $w.buttons.delete -text Delete -command "deleteobj $c $o; destroy $w"
		 pack $w.buttons.delete -side left -expand 1
	}
	button $w.buttons.dismiss -text OK -command "destroy $w"
	pack $w.buttons.dismiss -side left -expand 1
	pack $w.buttons -side bottom -expand y -fill x -pady 2m
}

# open a requestor according to the type of graph object $obj, to allow the user to read and set attributions
proc setAttributes {c obj} {
	global g
	if {$obj == {}} {
		set obj [string range [lindex [$c gettags current] 0] 1 end]
	}
	set type [string range $obj 0 3]
	if {$type == "node" || $type == "edge"} {
		if {[string length $obj] > 4} {
			setAttributesWidget $c $obj d \
				"$obj listattributes" \
				"$obj queryattributes" \
				"$obj setattributes"
		} {
			setAttributesWidget $c $obj {} \
				"$g list[set type]attributes" \
				"$g query[set type]attributes" \
				"$g set[set type]attributes"
		}
	} {
		setAttributesWidget $c $g {} \
			"$g listattributes" \
			"$g queryattributes" \
			"$g setattributes"
	}
}

# unconditionally remove any old graph and canvas contents, the create a new graph of $type
proc newGraphDontAsk {c type} {
	global modified g graphtype
	set graphtype $type
	$c delete all
	set modified 0
	if {[info exists g]} {$g delete}
	set g [dotnew $type]
}

# upon confirmation, remove any old graph and canvas contents, the create a new graph of $type
proc newGraph {c type} {
	global modified
	if {$modified} {
		confirm "Current graph has been modified.  Shall I continue?" \
			"newGraphDontAsk $c $type"
	} {
		newGraphDontAsk $c $type
	}
}

# generate a new graph layout and update rendering on the canvas
#  this proc is attached to the green button to the lower right of the window
proc layout {c} {
	global g
	$c delete all
	$g layout
	eval [$g render]
	$c configure -scrollregion [$c bbox all]
}

# generate a help window with $msg as the contents
proc help {msg} {
	set w .help
	catch {destroy $w}
	toplevel $w
#	positionWindow $w
	wm title $w "DotEd Help"
	wm iconname $w "DotEd"
	frame $w.menu -relief raised -bd 2
	pack $w.menu -side top -fill x
	label $w.msg \
		-font -Adobe-helvetica-medium-r-normal--*-140-*-*-*-*-*-* \
		-wraplength 4i -justify left -text $msg
	pack $w.msg -side top
	frame $w.buttons
	pack  $w.buttons -side bottom -expand y -fill x -pady 2m
	button $w.buttons.dismiss -text Dismiss -command "destroy $w"
	pack $w.buttons.dismiss -side left -expand 1
}

# proc that supports zoom in/out events
proc zoom {c fact} {
        upvar #0 $c data
        set x [$c canvasx [expr {[winfo pointerx $c] - [winfo rootx $c]}]]
        set y [$c canvasy [expr {[winfo pointery $c] - [winfo rooty $c]}]]
        $c scale all $x $y $fact $fact
        set data(zdepth) [expr {$data(zdepth) * $fact}]
        after cancel $data(idle)
        set data(idle) [after idle "zoomupdate $c"]
}

# update all text strings after zom operation is complete
proc zoomupdate {c} {
        upvar #0 $c data
        # adjust fonts
        foreach {i} [$c find all] {
                if { ! [string equal [$c type $i] text]} {continue}
                set fontsize 0
                # get original fontsize and text from tags
                #   if they were previously recorded
                foreach {tag} [$c gettags $i] {
                        scan $tag {_f%d} fontsize
                        scan $tag "_t%\[^\0\]" text
                }
                # if not, then record current fontsize and text
                #   and use them
                set font [$c itemcget $i -font]
                if {!$fontsize} {
                        set text [$c itemcget $i -text]
                        if {[llength $font] < 2} {
                                #new font API
                                set fontsize [font actual $font -size]
                        } {
                                #old font API
                                set fontsize [lindex $font 1]
                        }
                        $c addtag _f$fontsize withtag $i
                        $c addtag _t$text withtag $i
                }
                # scale font
                set newsize [expr {int($fontsize * $data(zdepth))}]
                if {abs($newsize) >= 4} {
                        if {[llength $font] < 2} {
                                #new font api
                                font configure $font -size $newsize
                        } {
                                #old font api
                                lreplace $font 1 1 $newsize
                        }
			$c itemconfigure $i -font $font -text $text
                } {
                        # suppress text if too small
                        $c itemconfigure $i -text {}
                }
        }
        set bbox [$c bbox all]
        if {[llength $bbox]} {
                $c configure -scrollregion $bbox
        } {
                $c configure -scrollregion [list -4 -4 \
                        [expr {[winfo width $c]-4}] \
                        [expr {[winfo height $c]-4}]]
        }
}

#--------------------------------------------------------------------------
set help_about "DotEd - Dot Graph Editor
Copyright (C) 1995 AT&T Bell Labs
		  (C) 1996 Lucent Technologies

Written by: John Ellson (ellson@graphviz.org)
	   and: Stephen North (north@research.att.com)

DotEd provides for the graphical editing of
directed graphs. Once a graph has been manually
entered then the dot layout algorithm can be applied 
by clicking on the button in the lower right corner
of the window."

set help_mouse "Button-1: When the cursor is over the
  background Button-1-Press will start a node, 
  Button-1-Motion (dragging the mouse with
  Button-1 still down) will move it and
  Button-1-Release will complete the node
  insertion into the graph.
 
  When the cursor is over an existing node
  then Button-1-Press will start an edge from
  that node.  Button-1-Motion will extend the
  edge and Button-1-Release over a different
  node will complete the edge.

Button-2: Button-2-Motion (click and drag) will
  reposition the canvas under the window.

Button-3: When Button-3 is clicked over a
  node or edge the attribute editor will
  be opened on that object.

Scrollwheel: Zooms canvas in/out.

Once a graph has been manually entered then
the dot layout algorithm can be applied by
clicking on the button in the lower right
corner of the window."

#--------------------------------------------------------------------------

#initialize some globals
set startObj {}
set saveFill {}
set modified 0
set fileName {no_name}
set printCommand {lpr}
set zfact 1.1

# create main window
wm title . "DotEd"
wm iconname . "DotEd"
wm minsize . 120 100
wm geometry . 400x300
frame .m -relief raised -borderwidth 1
frame .a
frame .b
set c [canvas .a.c \
	-cursor crosshair \
	-xscrollcommand ".b.h set" \
	-yscrollcommand ".a.v set" \
	-width 0 \
	-height 0 \
	-borderwidth 0]
scrollbar .b.h \
	-orient horiz \
	-relief sunken \
	-command "$c xview"
scrollbar .a.v \
	-relief sunken \
	-command "$c yview"
button .b.layout \
	-width [.a.v cget -width] \
	-height [.b.h cget -width] \
	-foreground green \
	-activeforeground green\
	-bitmap @$tk_library/demos/images/gray25.xbm \
	-command "layout $c"

# initialize zoom state
set [set c](zdepth) 1.0
set [set c](idle) {}

# create graph structure and set global "g"
newGraphDontAsk $c digraph

# canvas bindings
bind $c <Configure> "resize_canvas $c %w %h"
bind $c <ButtonPress-1> "mouse_b1_press $c %x %y"
bind $c <B1-Motion> "mouse_b1_motion $c %x %y"
bind $c <ButtonRelease-1> "mouse_b1_release $c %x %y"
bind $c <Button-2> "$c scan mark %x %y"
bind $c <B2-Motion> "$c scan dragto %x %y 1"
bind $c <Button-3> "setAttributes $c {}"
bind $c <Button-4> "zoom $c $zfact"
bind $c <Button-5> "zoom $c [expr {1.0/$zfact}]"

# canvas item bindings
$c bind all <Any-Enter> "mouse_anyenter $c"
$c bind all <Any-Leave> "mouse_anyleave $c"

menubutton .m.file -text "File" -underline 0 -menu .m.file.m
menu .m.file.m
.m.file.m add command -label "Load ..." -underline 0 \
	-command "loadFile $c"
.m.file.m add command -label "New - directed" -underline 0 \
	-command "newGraph $c digraph"
.m.file.m add command -label "New - undirected" -underline 6 \
	-command "newGraph $c graph"
.m.file.m add command -label "Save" -underline 0 \
	-command "saveFile gv"
.m.file.m add command -label "Save As ..." -underline 5 \
	-command "saveFileAs gv"
.m.file.m add separator
.m.file.m add cascade -label "Export" -underline 1 \
	-menu .m.file.m.export
menu .m.file.m.export
.m.file.m.export add command -label "CMAP ..." -underline 0 \
	-command "saveFileAs cmap"
.m.file.m.export add command -label "DIA ..." -underline 0 \
	-command "saveFileAs dia"
.m.file.m.export add command -label "FIG ..." -underline 0 \
	-command "saveFileAs fig"
.m.file.m.export add command -label "GIF ..." -underline 0 \
	-command "saveFileAs gif"
.m.file.m.export add command -label "HPGL ..." -underline 0 \
	-command "saveFileAs hpgl"
.m.file.m.export add command -label "MIF ..." -underline 0 \
	-command "saveFileAs mif"
.m.file.m.export add command -label "PDF ..." -underline 0 \
	-command "saveFileAs pdf"
.m.file.m.export add command -label "PNG ..." -underline 0 \
	-command "saveFileAs png"
.m.file.m.export add command -label "PS ..." -underline 0 \
	-command "saveFileAs ps"
.m.file.m.export add command -label "SVG ..." -underline 0 \
	-command "saveFileAs svg"
.m.file.m.export add command -label "TIFF ..." -underline 0 \
	-command "saveFileAs tiff"
.m.file.m.export add command -label "VML ..." -underline 0 \
	-command "saveFileAs vml"
.m.file.m.export add command -label "VTX ..." -underline 0 \
	-command "saveFileAs vtx"
.m.file.m add separator
.m.file.m add command -label "Print Setup ..." -underline 0 \
	-command "printSetup"
.m.file.m add command -label "Print" -underline 0 \
	-command "print"
.m.file.m add separator
.m.file.m add command -label "Exit" -underline 0 -command "exit"
menubutton .m.graph -text "Graph" -underline 0 -menu .m.graph.m
menu .m.graph.m
.m.graph.m add command -label "Graph Attributes" -underline 0 \
	-command "setAttributes $c graph"
.m.graph.m add command -label "Node Attributes" -underline 0 \
	-command "setAttributes $c node"
.m.graph.m add command -label "Edge Attributes" -underline 0 \
	-command "setAttributes $c edge"
menubutton .m.help -text "Help" -underline 0 -menu .m.help.m
menu .m.help.m
.m.help.m add command -label "About DotEd" -underline 0 \
	-command {help $help_about}
.m.help.m add command -label "Mouse Operations" -underline 0 \
	-command {help $help_mouse}

pack append .m .m.file {left} .m.graph {left} .m.help {right}
pack append .a $c {left expand fill} .a.v {right filly}
pack append .b .b.h {left expand fillx} .b.layout {right}
pack append . .m {top fillx} .a {expand fill} .b {bottom fillx}
tk_menuBar .m.file .m.graph .m.help

if {$argc} {loadFileByNameDontAsk $c [lindex $argv 0]}
