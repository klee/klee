#!/bin/sh
# next line is a comment in tcl \
exec wish "$0" ${1+"$@"}

package require Tkspline

########################################################################
# splinetest.tcl - a test of spline rendering using the Tkspline extension
#
# John Ellson - ellson@graphviz.org 
########################################################################

# create canvas and frame for toolbar
pack [canvas .c -bg white -height 350] \
    -side top -fill both -expand true
pack [frame .f] -side top -fill x

proc nextpoint {c wx wy} {
    global id oldx oldy

    if {[info exists id]} {
        unset id
    } {
        set id [$c find withtag current]
        if {[string length $id] == 0} {
            # not a Point
            unset id
        } {
            # $id != {} so must be a Point because everything else is disabled
            set oldx [$c canvasx $wx]
            set oldy [$c canvasy $wy]
        }
    }
}

proc motion {c wx wy} {
    global id oldx oldy
    set x [$c canvasx $wx]
    set y [$c canvasy $wy]
    if [info exists id] {
        foreach {items idx} [$c gettags $id] {break}
        # update single coord in three variants of shape
        foreach itm [split $items .] {
               $c dchars $itm $idx
               $c insert $itm $idx [list $x $y]
        }
        # move control point
        $c move $id [expr $x - $oldx] [expr $y - $oldy]
    }
    set oldx $x
    set oldy $y
}

# itemwithcontrolpoints - create a "line" or "polygon" with
#    straight + spline + smooth  line variants and draggable 
#    control points.
#
# c = canvas handle
# item = "line" | "polygon"
# coords = x y x y ... list of control point locations
#
proc itemwithcontrolpoints {c item coords} {
    if {[string equal $item "line"]} {
        set opts {}
    } {
        set opts {-fill {} -outline black}
    }
    set lid [eval $c create $item $coords $opts \
        -state disabled -tag Line]
    set slid [eval $c create $item $coords $opts \
        -smooth true -state disabled -tag Smooth]
    set bzlid [eval $c create $item $coords $opts \
        -smooth spline -state disabled -tag Spline]
    set idx 0
    foreach {x y} $coords {
        set id [$c create oval -2 -2 2 2 \
            -fill yellow -outline black \
            -tags [list $lid.$slid.$bzlid $idx Point]]
        $c move $id $x $y 
        incr idx 2
    }
}
    
# show objects by type (from toolbar buttons)
proc show {typ show} {
    if {$show} {
        if {[string equal $typ "Point"]} {
            .c itemconfigure $typ -state normal
        } {
            .c itemconfigure $typ -state disabled
        }
    } {
        .c itemconfigure $typ -state hidden
    }
}

proc print {} {
    global tcl_platform
    if {[string equal $tcl_platform(platform) "windows"]} {
        # platform == "windows"
        if {[catch {package require Printer}] 
        && [catch {package require printer}]} {
            # Can't do this--no printer package
            puts stderr "No \"Printer\" package found"
        } {
            # Ask the user to select a printer
            set hdc [ printer dialog select ]
            # Now send raw postscript output to the device
            printer send -hdc $hdc -postscript \
                -data [.c postscript]
        }
    } {
        # platform != "windows" ... then probably unix
        set chan [open "| lpr" w]
        .c postscript -channel $chan
        close $chan
    }
}

# populate toolbar
foreach typ {Exit Print} {
    set cmd [string tolower $typ]
    pack [button .f.$cmd \
        -text $typ -width 6 -command $cmd \
        -padx 0 -pady 0 -width 6] \
            -side right
}

foreach typ {Line Smooth Spline Point} {
    pack [checkbutton .f.[string tolower $typ] \
        -text $typ -variable var$typ -width 6 \
        -selectcolor yellow -command "show $typ \$var$typ" \
        -indicatoron false] \
            -side left -fill both
    set var$typ 1
}

# mouse bindings on canvas objects
bind .c <1> "nextpoint .c %x %y"
bind .c <Motion> "motion .c %x %y"

# populate with demo objects
itemwithcontrolpoints .c line \
    {50 50 50 100 100 100 100 50 100 20 150 20 150 50}
itemwithcontrolpoints .c polygon \
    {50 200 100 150 150 200 150 250 100 300 50 250}

set help [.c create text 10 125 \
    -anchor w \
    -text "You can drag the points\nwith mouse button 1"]
after 5000 {.c delete $help}
