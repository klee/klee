#!/usr/bin/perl

# translate a ghostscript config to a graphviz ps_font_equiv.h table
use English;
my %features = ();

my %map = (
"roman" => "serif",
"sans-serif" => "sans-Serif",
"typewriter" => "monospace"
);

# weight normal or bold
# style normal or italic

if ($#ARGV + 1 != 2) { die "usage: cf2psfe.pl fontmap.cfg ps_font_equiv.txt";}

open(CONFIG,"< $ARGV[0]");
while (<CONFIG>) {
	next if /^#/;
	if (/\[(.+)\]/) { $fontname = $1;}
	if (/features\s*=\s*(.+)/) { $features{$fontname} = $1;}
}

open(SOURCE,"< $ARGV[1]");
while (<SOURCE>) {
	my ($fontfam, $weight, $style);
	m/"([^"]+)"/;
	$f = $features{$1};
	while (($key,$value) = each(%map)) {
		$fontfam = $value if ($f =~ /$key/);
	}
	$style = ($f =~ /italic/? q("italic") : 0);
	$weight= ($f =~ /bold/? q("bold") : 0);
	if ($fontfam eq "") {warn "don't know about $1\n"; $fontfam = "fantasy";}
	$_ =~ s/},$/,\t\"$fontfam\",\t$weight,\t$style},/;
	print $_;
}
