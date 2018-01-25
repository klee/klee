#!/usr/bin/env tclsh

source [lindex $argv 0]/gv_doc_langs.tcl

set TEMPLATE [lindex $argv 0]/gv_doc_template.tcl

proc gv_doc_synopsis {} {
	global SYNOPSIS
	return [join $SYNOPSIS "\n.br\n"]
}

proc gv_doc_usage {} {
	global USAGE
	return [join $USAGE "\n.P\n"]
}

proc gv_doc_commands {} {
	global TYPES nameprefix paramstart paramsep paramend srcdir

	set fn $srcdir/gv.i
	set f [open $fn r]
	set t [read $f [file size $fn]]
	close $f

	regsub {.*?%\{} $t {} t
	regsub {%\}.*} $t {} t
	regsub -all {extern} $t {} t

	set res {}
	foreach rec [split $t \n] {
		set rec [string trim $rec " \t;)"]
		if {[string length $rec] == 0} {continue}
		if {[regsub -- {/\*\*\* (.*) \*/} $rec {\1} c]} {
			lappend res .TP $c .br
			continue
		}
		if {[regsub -- {/\*\* (.*) \*/} $rec {\1} c]} {
			lappend res .TP "\\fB$c\\fR" .br
			continue
		}
		if {[regsub -- {/\*.*\*/} $rec {} c]} {
			continue
		}
		if {[regsub -- {//} $rec {} c]} {
			continue
		}
		if {[regsub -- {#} $rec {} c]} {
			continue
		}
		set i 0
		set params {}
		foreach type_name [split $rec "(,"] {
			set type_name [string trim $type_name]
			regsub -all {[ \t]+(\**)} $type_name {\1 } type_name
			set type [lrange $type_name 0 end-1]
			set name [lindex $type_name end]
			if {! $i} {
				set func $name
				set functype $type
			} {
				lappend params $type $name
			}
			incr i
		}
		set par {}
		foreach {paramtype param} $params {
			if {[info exists TYPES([list $paramtype $param])]} {
				lappend par "$TYPES([list $paramtype $param])"
			} {
				lappend par "$TYPES($paramtype) $param"
			}
		}
		if {[string length $TYPES($functype)]} {
			lappend res "\\fI$TYPES($functype)\\fR \\fB$nameprefix$func\\fR \\fI$paramstart[join $par $paramsep]$paramend\\fR" .br
		} {
			lappend res "\\fB$nameprefix$func\\fR \\fI$paramstart[join $par $paramsep]$paramend\\fR" .br
		}
			
	}
	return [join $res \n]
}

foreach lang [array names LANGS] {
	array set PROPS $LANGS($lang)
	array set TYPES $PROPS(TYPES)
	foreach {nameprefix paramstart paramsep paramend} $PROPS(SYNTAX) {break}
	set SYNOPSIS $PROPS(SYNOPSIS)
	set USAGE $PROPS(USAGE)
	set srcdir [lindex $argv 0]
	set f [open gv.3[string tolower $lang] w]
	set ft [open $TEMPLATE r]
	puts $f [subst [read $ft [file size $TEMPLATE]]]
	close $ft
	close $f
	array unset PROPS
	array unset TYPES
}
