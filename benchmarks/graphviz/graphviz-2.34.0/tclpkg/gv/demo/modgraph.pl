#!/usr/bin/perl -w
# Change ^^ to the version of Perl you installed the SWIG modules / Graphviz with
#
# Change this to point to your installed graphviz lib dir
#   Normally either /usr/local/lib/graphviz/perl or /usr/lib/graphviz/perl
#use lib '/home/maxb/lib/graphviz/perl';
use gv;

use Getopt::Long;
GetOptions(\%Args, 'h|help','d|debug');
$Debug   = $Args{d} || 0;
$Modules = shift @ARGV || '/proc/modules';

die &usage if $Args{h};
die "Cannot read $Modules. $!\n" unless (-r $Modules);

$G = gv::digraph("G");
$N = gv::protonode($G);
$E = gv::protoedge($G);

gv::setv($G, "rankdir", "LR");
gv::setv($G, "nodesep", "0.05");
gv::setv($N, "shape", "box");
gv::setv($N, "width", "0");
gv::setv($N, "height", "0");
gv::setv($N, "margin", ".03");
gv::setv($N, "fontsize", "8");
gv::setv($N, "fontname", "helvetica");
gv::setv($E, "arrowsize", ".4");

open (M,"<$Modules") or die "Can't open $Modules. $!\n";

while (<M>) {
    chomp;
    my @f = split(/\s+/);
    # Should be at least three columns
    next unless scalar @f >= 3;

    # Linux 2.4 : parport 36832 1 (autoclean) [parport_pc lp]
    # Linux 2.6 : eeprom 14929 0 - Live 0xffffffff88cc5000
    my $module  = shift @f;
    my $size    = shift @f;
    my $used_by = shift @f;
    # this is ugly, needed to clean up the list of deps from 2.4 or 2.6
    my $deps    = join (' ',@f);
    $deps =~ s/ Live.*//;
    $deps =~ s/[\[\]\-(),]/ /g;

    Debug("$module");
    my $n = gv::node($G,$module);

    foreach my $d ( split(/\s+/,$deps) ){
        # gv::node($G, $d)  creates the node, if needed,
	#      but doesn't complain if it already exists
        Debug(" $d -> $module");
        gv::edge($n, gv::node($G, $d) );
    }
}

gv::layout($G, "dot");
gv::render($G, "xlib");

sub Debug {
    return unless $Debug;
    warn join(" ",@_), "\n";
}

sub usage {
    return << "end_usage";
modgraph.pl

Displays Linux kernel module dependencies from $Modules

Author:    John Ellson <ellson\@research.att.com>
Perl Port: Max Baker <max\@warped.org>

Usage: $0 [--debug] [/proc/modules]

end_usage
}
