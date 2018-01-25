#!perl -w

# dotmcl.pl <factor> <in.gv> <out.gv>
#   <factor>  the bigger, the more clusters (values from 1.2 to 3.0)
#   <in.gv>  dot in file to clusterize
#   <out.gv> dot out file with clusters added
# Vladimir Alexiev <vladimir@worklogic.com>

# This quick hack takes a dot graph description file, adds clusters using the
# mcl utility, and writes to stdout a dot graph amended with "subgraph
# cluster_n" declarations that put the nodes in clusters.
# dot: http://www.research.att.com/sw/tools/graphviz
# mcl: http://members.ams.chello.nl/svandong/thesis/

# It passes the input file through "dot -Tplain" so it can parse it more
# easily. Restrictions on the input file:
# - node names must be plain \w+
# - the closing "}" must be on the last line, in first position, and 
#   MUST be the only closing brace in first position in the whole file

my $factor = shift;
my $dotin = shift; # why not stdin: because need to read twice
my $dotout = shift; # why not stdout: because DOS commingles mcl's stderr into
                    # dotmcl's stdout
my (@nodes, %node_index, @graph);
for (`dot -Tplain $dotin`) {
  /^node (\w+)/ and do {
    push @nodes, $1;
    $node_index{$1} = $#nodes;
    next
  };
  /^edge (\w+) (\w+)/ and do {
    $graph[$node_index{$1}][$node_index{$2}] = 1;
    $graph[$node_index{$2}][$node_index{$1}] = 1; 
    # dot handles digraphs but mcl handles undirected graphs
    next
  }
}
my $nodes = @nodes;
my $mclin = "dotmcl-in.tmp";
my $mclout = "dotmcl-out.tmp";

open (MCLIN, ">$mclin") or die "can't create $mclin: $!\n";
# mcl is a nice program but its input format sucks!
print MCLIN << "MCLHEADER"; 
(mclheader
mcltype matrix
dimensions $ {nodes}x$ {nodes}
)

(mclmatrix
begin
MCLHEADER

for (my $i=0; $i<$nodes; $i++) {
  print MCLIN $i," ";
  for (my $j=0; $j<$nodes; $j++)
    {print MCLIN $j," " if $graph[$i][$j]};
  print MCLIN "\$\n";
}
print MCLIN << "MCLFOOTER";
)
MCLFOOTER
close(MCLIN);

system("mcl $mclin --silent -v mcl -I $factor -o $mclout")==0 or 
  die "can't run 'mcl $mclin -o $mclout: $!\n";

# read in clusters
my @cluster; 
# mcl output format sucks even worse because it can have "continuation lines"
# like this:
# (mclmatrix
# begin
# 0      0   1   2   3   4   5   6   7   8   9  12  13  14  15  16  18  19  20
#       86  87  88  89  90  91  92  94  95  96  98 105 106 107 109 110 112 113
#      115 116 117 118 121 125 127 128 131 132 133 137 138 145 $
# 1     28  29  31  32  33  34  35  36  78  93 129 130 134 135 136 $
# )
open (MCLOUT, $mclout) or die "can't open $mclout: $!\n";
my $line = '';
for (<MCLOUT>) {
  /^\d+(.+)/ and $line = $1;
  /^ +\d+/ and $line .= $_;
  /\$$/ and do {
    $line =~ s/\$$//;
    my @cl = split' ', $line;
    push @cluster, [@cl] 
      unless @cl <= 1;		# don't want trivial clusters
  }
}
close(MCLOUT);

open (DOTIN, $dotin) or die "can't open $dotin: $!\n";
open (DOTOUT, ">$dotout") or die "can't create $dotout: $!\n";
for (<DOTIN>) {
  /^\}$/ and last;
  print DOTOUT;
}
for (my $i=0; $i<@cluster; $i++) {
  print DOTOUT "  subgraph cluster_$i {label=\"\" ";
  for (@{$cluster[$i]}) {print DOTOUT " $nodes[$_]"};
  print DOTOUT "}\n";
}
print DOTOUT "}\n";
close(DOTOUT);
close(DOTIN);
