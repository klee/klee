#!/usr/bin/tclsh

# get names for html-4.0 characters from:
#          http://www.w3.org/TR/REC-html40/sgml/entities.html
set f [open entities.html r]
set entity_name_length_max 0
set nr_of_entities 0
while {! [eof $f]} {
        set rec [gets $f]
        if {[scan $rec {&lt;!ENTITY %s CDATA "&amp;#%d;"; --} name val] == 2} {
                set entity($name) $val
		set entity_name_length [string length $name]
		if {$entity_name_length > $entity_name_length_max} {
			set entity_name_length_max $entity_name_length
		}
		incr nr_of_entities
        }
}
close $f

set f [open entities.h w]
puts $f "/*"
puts $f " * Generated file - do not edit directly."
puts $f " *"
puts $f " * This file was generated from:"
puts $f " *       http://www.w3.org/TR/REC-html40/sgml/entities.html"
puts $f " * by means of the script:"
puts $f " *       entities.tcl"
puts $f " */"
puts $f ""
puts $f "#ifdef __cplusplus"
puts $f "extern \"C\" {"
puts $f "#endif"
puts $f ""
puts $f "static struct entities_s {"
puts $f "	char	*name;"
puts $f "	int	value;"
puts $f "} entities\[\] = {"
foreach name [lsort [array names entity]] {
        puts $f "	{\"$name\", $entity($name)},"
}
puts $f "};"
puts $f ""
puts $f "#define ENTITY_NAME_LENGTH_MAX $entity_name_length_max"
puts $f "#define NR_OF_ENTITIES $nr_of_entities"
puts $f ""
puts $f "#ifdef __cplusplus"
puts $f "}"
puts $f "#endif"
close $f
