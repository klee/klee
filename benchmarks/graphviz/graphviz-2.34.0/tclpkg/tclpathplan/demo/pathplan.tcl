#!/bin/sh
# next line is a comment in tcl \
exec wish "$0" ${1+"$@"}

package require Tkspline
package require Tclpathplan

########################################################################
# shape - a shape drawing tool for testing the spring layout engine
#
# John Ellson - ellson@graphviz.org - September 12, 1996

# requires dash patch

# Radio buttons select the drawing mode.
#	"draw"		- draw a closed and filled polygon
#	"stretch"   - move a vertex of a polygon, also
#				  insert additional vertices with subsequent button 1 clicks
#	"collapse"  - delete a vertex of a polygon (except last 2)
#	"move"		- move a complete polygon without altering 
#				  its shape, or move the whole canvas.
#	"rotate"	- rotate a polygon about its center
#	"scale"		- scale a polygon
#	"clone"		- copy an existing shape
#	"delete"	- remove an entire polygon object
#	"path"		- draw a line between two polygons and the 
#				  system will respond with the shortest path
#				  around all the other polygons.
#	"bezier path" - draw a line between two polygons and the 
#				  system will respond with the spline that follows
#				  the shortest path around all the other polygons.
#	"id"		- identify a polygon.  mostly for debugging.

# "draw," "stretch," "move," "path", "bezier path",  and "clone" use 
# button 1 for first though penultimate points, then button 2 to 
# complete the operation.

# "rotate" and "scale" use the button 1 to grab a polygon and
# button 2 to complete the operation.

# "collapse" and "delete" just use button 1 

# "stretch, " "move, " "collapse," and "delete" operations all act on
# a highlighted object

# "grid" constrains the locations of input points to lie on a grid of
# the specified spacing (in pixels).

# Future...
#
# some other possible operations:
#   regularize (arrange points on circle)
#   transformations: skew, distort, scale
#	   label text (inside or relative)
#   fill & outline color
#   fill & outline stipple
#   fill tile image
#   outline dash (mark, space offset)
#   outline width
#   number of peripheries
#
#   group/ungroup
#
#   raise/lower (not required if no overlap)
#
#   constraints:  no overlap
#				 no twist
#
#   resources:	shape library
#				 stipple patterns
#				 tile images
#
########################################################################

set splinecolor orange

set showmouse off

proc nextpoint {vc c wx wy} {
	global id mode oldx oldy gain0 angle0 index grid

	set x [$c canvasx $wx]
	set y [$c canvasy $wy]
	set gx [expr $grid * int(($x / $grid) + 0.5)]
	set gy [expr $grid * int(($y / $grid) + 0.5)]
	switch $mode {
		draw {
			if [info exists id] {
				$c insert $id 0 [list $gx $gy]
			} {
				set id [$c create polygon $gx $gy $gx $gy \
					-fill red -outline #ffc000]
			}
		}
		stretch {
			if [info exists id] {
				$c insert $id $index [list $gx $gy]
			} {
				set id [$c find withtag current]
				if {$id == {}} {
					unset id
				} {
					set index [$c index $id @$x,$y]
					$c dchars $id $index
					$c insert $id $index [list $gx $gy]
				}
			}
		}
		collapse {
			set id [$c find withtag current]
			if {$id != {}} {
				set index [$c index $id @$x,$y]
				if {[llength [$c coords $id]] > 4} {$c dchars $id $index}
				$vc coords [lindex [$c gettags $id] 0] [$c coords $id]
			}
			unset id
		}
		clone {
			if [info exists id] {
				set tag [$vc insert [$c coords $id]]
				$c addtag $tag withtag $id
			}
			set t [$c find withtag current]
			if {$t != {}} {
				set id [$c create [$c type $t] [$c coords $t]]
				foreach config [$c itemconfigure $t] {
					foreach {config . . . val} $config {break}
					if {$config != "-tags"} {
						$c itemconfigure $id $config $val
					}
				}
				set oldx $gx
				set oldy $gy
			}
		}
		move {
			set id [$c find withtag current]
			if {$id == {}} {
				$c scan mark $wx $wy
			} {
				set oldx $gx
				set oldy $gy
			}
		}
		scale {
			set id [$c find withtag current]
			if {$id == {}} {
				unset id
			} {
				foreach {oldx oldy} \
					[$vc center [lindex [$c gettags $id] 0]] {break}
				set dx [expr $oldx-$x]
				set dy [expr $oldy-$y]
				set gain0 [expr sqrt($dx*$dx+$dy*$dy)]
			}
		}
		rotate {
			set id [$c find withtag current]
			if {$id == {}} {
				unset id
			} {
				foreach {oldx oldy} [$vc center [lindex [$c gettags $id] 0]] {
					break
				}
				set angle0 [expr atan2($x-$oldx, $oldy-$y)]
			}
		}
		path {
			if [info exists id] {
				set path [$c coords $id]
				if [catch {$vc path $path} path] {
					puts $path
				} {
					$c coords $id $path
					$c itemconfigure $id -fill red
					set id [$c create line $x $y $x $y \
						-fill red -state disabled]
				}
			} {
				set id [$c create line $gx $gy $gx $gy \
					-fill red -state disabled]
			}
		}
		bpath {
			if [info exists id] {
				set path [$c coords $id]
				if [catch {$vc bpath $path} path] {
					puts $path
				} {
					$c coords $id $path
					$c itemconfigure $id -fill orange
					set id [$c create line $x $y $x $y \
						-smooth spline -fill orange -state disabled]
				}
			} {
				set id [$c create line $gx $gy $gx $gy \
					-smooth spline -fill orange -state disabled]
			}
		}
		delete {
		   $vc remove [lindex [$c gettags current] 0]
		   $c delete current
		}
		triangulate {
			global mode
			if {[$vc bind triangle] == {}} {
				$vc bind triangle {
					if {$mode == "triangulate"} {
						$c create polygon %t -tag triangles \
							-fill {} -outline white -width 2 
					} {
						$c create polygon %t -tag triangles \
							-fill {} -outline white -width 2  -state hidden
					}
				}
			}
			if {$mode == "triangulate"} {
				$c itemconfigure triangles -state normal
			} {
				$c itemconfigure triangles -state hidden
			} 
			set t [$vc find $x $y]
			if {$t != {}} {
				$vc triangulate $t
			}
		}
		id {
		   set t [$vc find $x $y]
		   if {$t == {}} {
			   puts "at: $x $y ....nothing"
		   } {
			   puts "at: $x $y\nid: $t\ncoords: [$vc coords $t]"
		   }
		}
	}
}

proc lastpoint {vc c args} {
	global id mode
	if [info exists id] {
		switch $mode {
			draw {
				$c itemconfigure $id -fill darkgreen \
					-outline yellow -activeoutline #ffc000
				set tag [$vc insert [$c coords $id]]
				$c addtag $tag withtag $id
			}
			clone {
				set tag [$vc insert [$c coords $id]]
				$c addtag $tag withtag $id
			}
			move - stretch - rotate - scale {
				set t [lindex [$c gettags $id] 0]
				if {$t != {} && $t != "current"} {
					$vc coords $t [$c coords $id]
				}
			}
			path {
				set path [$c coords $id]
				if [catch {$vc path $path} path] {
					puts $path
					$c delete $id
				} {
					$c coords $id $path
					$c itemconfigure $id -fill 
				}
			}
			bpath {
				set path [$c coords $id]
				if [catch {$vc bpath $path} path] {
					puts $path
					$c delete $id
				} {
					$c coords $id $path
					$c itemconfigure $id -fill red
				}
			}
		}
		$c configure -scrollregion [$c bbox all]
		unset id
	}
}

proc motion {vc c wx wy} {
	global id mode oldx oldy gain0 angle0 index grid showmouse
	set x [$c canvasx $wx]
	set y [$c canvasy $wy]
		if {$showmouse == "on"} {
			puts -nonewline stderr "\r$x,$y [list [$vc find $x $y]]	 "
		}
	if [info exists id] {
		switch $mode {
			draw {
				set gx [expr $grid * int(($x / $grid) + 0.5)]
				set gy [expr $grid * int(($y / $grid) + 0.5)]
				$c dchars $id 0
				$c insert $id 0 [list $gx $gy]
			}
			path {
				$c dchars $id 0
				$c insert $id 0 [list $x $y]
			}
			bpath {
				$c dchars $id 0
				$c insert $id 0 [list $x $y]
			}
			move - clone {
				if {$id == {}} {
					$c scan dragto $wx $wy 1
				} {
					set gx [expr $grid * int(($x / $grid) + 0.5)]
					set gy [expr $grid * int(($y / $grid) + 0.5)]
					$c move $id [expr $gx - $oldx] [expr $gy - $oldy]
					set oldx $gx
					set oldy $gy
				}
			}
			stretch {
				set gx [expr $grid * int(($x / $grid) + 0.5)]
				set gy [expr $grid * int(($y / $grid) + 0.5)]
				$c dchars $id $index
				$c insert $id $index [list $gx $gy]
			}
			scale {
				set t [lindex [$c gettags $id] 0]
				set dx [expr $x-$oldx]
				set dy [expr $y-$oldy]
				set gain [expr sqrt($dx*$dx+$dy*$dy)/20]
				$c coords $id [$vc scale $t $gain]
			}
			rotate {
				set t [lindex [$c gettags $id] 0]
				set alpha [expr atan2($x-$oldx,$oldy-$y) - $angle0]
				$c coords $id [$vc rotate $t $alpha]
			}
		}
	}
}

proc clearpaths {vc c} {
	catch { $c delete triangles }
	foreach i [$c find all] {
		set t [$c type $i]
		if {$t == "line"} {$c delete $i}
	}
}

proc clearall {vc c} {
	catch { $c delete triangles }
	foreach i [$c find all] {
		if {[$c type $i] == "polygon"} {$vc remove [lindex [$c gettags $i] 0]}
		$c delete $i
	}
}

proc loadpaths {vc c file} {
	if [catch {open $file r} f] {
		error "unable to open file for read: $file"
	}
	clearpaths $vc $c
	while {![eof $f]} {
		set path [gets $f]
		if {$path == {}} {continue}
		if [catch {$vc bpath $path} path] {
			puts $path
		} {
			$c create line $path \
				-smooth spline -fill #ff00c0 -state disabled
		}
	}
	close $f
	$c configure -scrollregion [$c bbox all]
}

proc loadvconfig {vc c file} {
	if [catch {open $file r} f] {
		error "unable to open file for read: $file"
	}
	clearall $vc $c
	while {![eof $f]} {
		set coords [string trim [gets $f]]
		if {$coords == {}} {continue}
		set tag [$vc insert $coords]
		$c create polygon $coords \
			-tag $tag \
			-fill darkgreen \
			-outline yellow \
			-activeoutline #ffc000
	}
	close $f
	$c configure -scrollregion [$c bbox all]
}

proc savepaths {vc c file} {
	if [catch {open $file w} f] {
		error "unable to open file for write: $file"
	}
	foreach i [$c find all] {
		set t [$c type $i]
		if {$t == "line"} {
			set path [$c coords $i]
			set l [llength $path]
			set x1 [lindex $path 0]
			set y1 [lindex $path 1]
			set x2 [lindex $path [incr l -2]]
			set y2 [lindex $path [incr l]]
			puts $f "$x1 $y1 $x2 $y2"
		}
	}
	close $f
}

proc savevconfig {vc c file} {
	if [catch {open $file w} f] {
		error "unable to open file for write: $file"
	}
	foreach id [$vc list] {
		puts $f [$vc coords $id]
	}
	close $f
}

proc nextfile {} {
	global filename
	set filename [file join [file dirname $filename] [file tail $filename]]
	set files [glob [file join [file dirname $filename] *[file extension $filename]]]
	set filename [lindex $files [expr ([lsearch $files $filename] + 1) % [llength $files]]]
}

set vc [vgpane]
set mode draw
set filename "pathplan.tcl.data/unknown.dat"
frame .fl
set a [frame .fl.a]
set b [frame .fl.b]
set c [canvas $a.c \
	-relief sunken \
	-borderwidth 2 \
	-bg lightblue \
	-xscrollcommand "$b.h set" \
	-yscrollcommand "$a.v set"]
scrollbar $b.h -command "$c xview" -orient horiz
scrollbar $a.v -command "$c yview"
frame $b.pad \
	-width [expr [$a.v cget -width] + \
		[$a.v cget -bd]*2 + [$a.v cget -highlightthickness]*2 ] \
	-height [expr [$b.h cget -width] + \
		[$b.h cget -bd]*2 + [.fl.b.h cget -highlightthickness]*2 ]
frame .fr
frame .fr.bpath
pack [radiobutton .fr.bpath.bpath -text "bezier path" -value bpath \
		-highlightthickness 0 -anchor w -variable mode] \
		-side left -anchor w -fill x
pack [scale .fr.grid -orient horizontal -label grid -variable grid \
		-highlightthickness 0 -from 1 -to 100] \
	[radiobutton .fr.draw -text draw -value draw \
		-highlightthickness 0 -anchor w -variable mode] \
	[radiobutton .fr.stretch -text stretch -value stretch \
		-highlightthickness 0 -anchor w -variable mode] \
	[radiobutton .fr.collapse -text collapse -value collapse \
		-highlightthickness 0 -anchor w -variable mode] \
	[radiobutton .fr.clone -text clone -value clone \
		-highlightthickness 0 -anchor w -variable mode] \
	[radiobutton .fr.move -text move -value move \
		-highlightthickness 0 -anchor w -variable mode] \
	[radiobutton .fr.rotate -text rotate -value rotate \
		-highlightthickness 0 -anchor w -variable mode] \
	[radiobutton .fr.scale -text scale -value scale \
		-highlightthickness 0 -anchor w -variable mode] \
	[radiobutton .fr.delete -text delete -value delete \
		-highlightthickness 0 -anchor w -variable mode] \
	[radiobutton .fr.path -text path -value path \
		-highlightthickness 0 -anchor w -variable mode] \
	.fr.bpath \
	[radiobutton .fr.id -text id -value id \
		-highlightthickness 0 -anchor w -variable mode] \
	[radiobutton .fr.triangulate -text triangulate -value triangulate \
		-highlightthickness 0 -anchor w -variable mode] \
		-anchor w -fill x
frame .fr.load
pack [button .fr.load.load -text load \
		 -highlightthickness 0 -command "loadvconfig $vc $c \$filename"] \
	[button .fr.load.paths -text loadpaths \
		 -highlightthickness 0 -command "loadpaths $vc $c \$filename"] \
		 -side left -fill x -expand true
frame .fr.save
pack [button .fr.save.save -text save \
		 -highlightthickness 0 -command "savevconfig $vc $c \$filename"] \
	[button .fr.save.paths -text savepaths \
		 -highlightthickness 0 -command "savepaths $vc $c \$filename"] \
		 -side left -fill x -expand true
frame .fr.clear
pack [button .fr.clear.all -text clear -command "clearall $vc $c" \
		-highlightthickness 0] \
	[button .fr.clear.paths -text clearpaths -command "clearpaths $vc $c" \
		-highlightthickness 0] \
		 -side left -fill x -expand true
frame .fr.file
pack [entry .fr.file.name -textvar filename -highlightthickness 0] \
		 -side left -fill x -expand true
pack [button .fr.file.next -text next \
		 -highlightthickness 0 -command "nextfile"] \
		 -side left
frame .fr.quitdebug
pack [button .fr.quitdebug.debug -text debug \
		 -highlightthickness 0 -command "$vc debug"] \
	[button .fr.quitdebug.quit -text quit \
		 -highlightthickness 0 -command "exit"] \
	-side left -fill x -expand true
pack .fr.quitdebug .fr.clear .fr.save .fr.load .fr.file \
	[label .fr.flabel -anchor w -text "file"] \
	[entry .fr.coordinates -textvar coordinates -highlightthickness 0] \
	[label .fr.clabel -anchor w -text "coordinates"] \
		-side bottom -fill x -expand true
pack $a.v -side right -fill y
pack $c -side left -fill both -expand true
pack $b.h -side left -fill x -expand true
pack $b.pad -side right
pack $b -side bottom -fill x
pack $a -side top -fill both -expand true
pack .fl -side left -fill both -expand true
pack .fr -side left -fill y

bind $c <1> "nextpoint $vc $c %x %y"
bind $c <2> "lastpoint $vc $c"
bind $c <Motion> "motion $vc $c %x %y"

trace variable mode w "lastpoint $vc $c"

bind .fr.file.name <Return> {
	.fr.loadsave.load flash
	loadvconfig $vc $c $filename
}

bind .fr.coordinates <Return> {
	if {$coordinates == {}} {continue}
	set coords [split $coordinates]
	set coordinates {}
	switch $mode {
		draw {
			if [catch {$vc insert $coords} tag] {
				puts $tag
			} {
				$c create polygon $coords \
					-fill darkgreen \
					-outline yellow \
					-activeoutline #ffc000 \
					-tag $tag
			}
		}
		path {
			if [catch {$vc path $coords} coords] {
				puts $coords
			} {
				$c create line $coords -fill #ff00c0 -state disabled
			}
		}
		bpath {
			if [catch {$vc bpath $coords} coords] {
				puts $coords
			} {
				$c create line $coords \
					-smooth spline -fill orange -state disabled
			}
		}
	}
}

proc balloon_help {w msg} {
  bind $w <Enter> "after 1000   \"balloon_help_aux %W [list $msg]\""
  bind $w <Leave> "after cancel \"balloon_help_aux %W [list $msg]\"
	  catch {destroy %W.balloon_help}"
}
 
proc balloon_help_aux {w msg} {
  set t $w.balloon_help
  catch {destroy $t}
  toplevel $t
  wm overrideredirect $t 1
  pack [label $t.l -text $msg -relief groove -bd 1 -bg yellow] -fill both
  wm geometry $t +[expr [winfo rootx $w]+([winfo width $w]/2)]+[expr \
	  [winfo rooty $w]+([winfo height $w]/2)]
}

balloon_help .fr.grid "set grid size for draw operations"
balloon_help .fr.draw "draw a region. B1 foreach vertex except B2 for last"
balloon_help .fr.stretch "B1 to stretch a vertex, next B1 inserts new vertex. B2 to end"
balloon_help .fr.collapse "B1 collapses a vertex"
balloon_help .fr.clone "each B1 creates a new clone of a region, B2 to end"
balloon_help .fr.move "B1 to move, B2 to end"
balloon_help .fr.rotate "B1 to rotate, B2 to end"
balloon_help .fr.scale "B1 to scale, B2 to end"
balloon_help .fr.delete "B1 to delete a region"
balloon_help .fr.path "B1 starts a euclidean shortest path, B2 to end"
balloon_help .fr.bpath.bpath "B1 starts a bezier spline path, B2 to end"
balloon_help .fr.triangulate "B1 to display triangulation of a polygon"
balloon_help .fr.id "print the identifier of a region"
balloon_help .fr.coordinates "text entry of coordinates, alternative to button operations"
balloon_help .fr.file.name "current file name, or enter new name"
balloon_help .fr.file.next "next file with same directory and extension"
balloon_help .fr.save.paths "save paths to file"
balloon_help .fr.load.paths "load paths from file"
balloon_help .fr.save.save "save regions to file"
balloon_help .fr.load.load "load regions from file"
balloon_help .fr.clear.all "clear canvas of all regions and paths"
balloon_help .fr.clear.paths "clear canvas of all paths"
balloon_help .fr.quitdebug.quit "quit this application"
balloon_help .fr.quitdebug.debug "dump the vconfig"
