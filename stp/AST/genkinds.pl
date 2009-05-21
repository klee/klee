#!/usr/bin/perl -w

#AUTHORS: Vijay Ganesh, David L. Dill BEGIN DATE: November, 2005
#LICENSE: Please view LICENSE file in the home dir of this Program
#given a file containing kind names, one per line produces .h and .cpp
#files for the kinds.

#globals
@kindnames = ();
$minkids = 0;
$maxkids = 0;
@cat_bits = ();
@category_names = ();
%cat_index = ();

$now = localtime time;

sub read_kind_defs {
    open(KFILE, "< ASTKind.kinds") || die "Cannot open .kinds file: $!\n";
    @kindlines = <KFILE>;
    close(KFILE)
}

# create lists of things indexed by kinds.
sub split_fields {
    my $kind_cat_bits;
    # matches anything with three whitespace-delimited alphanumeric fields,
    # followed by rest of line.  Automatically ignores lines beginning with '#' and blank lines.
    for (@kindlines) {
	if (/Categories:\s+(.*)/) {
	    @category_names = split(/\s+/, $1);
	    $i = 0;
	    for (@category_names) {
		$cat_index{$_} = $i++;
		# print "cat_index{$_} = $i\n";
	    }
	}
	elsif (/^(\w+)\s+(\w+)\s+(\w+|-)\s+(.*)/) {
	    push(@kindnames, $1);
	    push(@minkids, $2);
	    push(@maxkids, $3);
	    @kind_cats = split(/\s+/, $4);
	    # build a bit vector of categories.
	    $kind_cat_bits = 0;
	    for (@kind_cats) {
		$kind_cat_bits |= (1 << int($cat_index{$_}));
	    }
	    push(@cat_bits, $kind_cat_bits); 
	}
    }
}

sub gen_h_file {
    open(HFILE, "> ASTKind.h") || die "Cannot open .h file: $!\n";

    print HFILE 
	"// -*- c++ -*-\n",
	"#ifndef TESTKINDS_H\n",
	"#define TESTKINDS_H\n",
	"// Generated automatically by genkinds.pl from ASTKind.kinds $now.\n",
	"// Do not edit\n",
	"namespace BEEV {\n  typedef enum {\n";

    for (@kindnames) {
	print HFILE "    $_,\n";
    }

    print HFILE 
	"} Kind;\n\n",
	"extern unsigned char _kind_categories[];\n\n";

    # For category named "cat", generate functions "bool is_cat_kind(k);"


    for (@category_names) {
	my $catname = $_;
	my $kind_cat_bit = (1 << int($cat_index{$catname}));
	print HFILE "inline bool is_", $catname, "_kind(Kind k) { return (_kind_categories[k] & $kind_cat_bit); }\n\n"
    }

    print HFILE
	"extern const char *_kind_names[];\n\n",
	"/** Prints symbolic name of kind */\n",
	"inline ostream& operator<<(ostream &os, const Kind &kind) { os << _kind_names[kind]; return os; }\n",
	"\n\n",
	"};  // end namespace\n",
	"\n\n#endif\n";

    close(HFILE);
}

# generate the .cpp file

sub gen_cpp_file {
    open(CPPFILE, "> ASTKind.cpp") || die "Cannot open .h file: $!\n";

    print CPPFILE
	"// Generated automatically by genkinds.h from ASTKind.kinds $now.\n",
	"// Do not edit\n",
	"namespace BEEV {\n",
	"const char * _kind_names[] =  {\n";
    for (@kindnames) {
	print CPPFILE "   \"$_\",\n";
    }
    print CPPFILE "};\n\n";

    # category bits
    print CPPFILE
	"unsigned char _kind_categories[] = {\n";
    for (@cat_bits) {
	print CPPFILE "   $_,\n";
    }
    print CPPFILE 
	"};\n",
	"\n};  // end namespace\n";

    close(CPPFILE);
}

&read_kind_defs;
&split_fields;
&gen_h_file;
&gen_cpp_file;
