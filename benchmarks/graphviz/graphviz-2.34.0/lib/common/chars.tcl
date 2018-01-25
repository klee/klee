#!/usr/bin/tclsh

# script to generate fonwidth tables in shapes.c

package require Gdtclft

set gd [gd create 100 100]
set white [gd color new $gd 255 255 255]
set black [gd color new $gd 0 0 0]
set fonts {times cour arial}
set size 96
set angle 0
set x 20
set y 20

foreach font $fonts {
#  set bb [gd text $gd $black $font $size $angle $x $y MM]
#  set basewidth [expr [lindex $bb 2] - [lindex $bb 0]]
set basewidth 0
  puts "static double [set font]FontWidth\[\] = \{"
  for {set i 0} {$i<32} {incr i} {
    set sizes {}
    set chars {}
    for {set j 0} {$j < 8} {incr j} {
      if {$i<4 || ($i>=16 && $i<20) || ($i==15 && $j==7)} {
        lappend sizes 0.0000
      } {
        set char [format %c [expr $j+$i*8]]
#        set bb [gd text $gd $black $font $size $angle $x $y M[set char]M]
        set bb [gd text $gd $black $font $size $angle $x $y $char]
        lappend sizes [format %.4f [expr ([lindex $bb 2] - [lindex $bb 0] - $basewidth)/double($size)]]
        lappend chars $char
      }
    }
    puts "  [join $sizes {, }], /* [join $chars {}] */"
  }
  puts "\};"
}
