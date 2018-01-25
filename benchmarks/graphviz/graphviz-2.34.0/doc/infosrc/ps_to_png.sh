#! /bin/bash

# Written by Ryan Schmidt.  Oct 4, 2008

# render postscript at this magnification, then scale back down to get antialiasing
aafactor=4

# add a border of this many pixels around final image
border=4

scale() {
    echo $1 | awk -F x -v scale=$2 '{printf("%.0fx%.0f", $1 * scale, $2 * scale)}'
}

# compute inverse of aafactor for use with pamscale whose -reduce option seems broken
aafactorinv=$(echo "scale=4; 1/$aafactor" | bc)

# convert scale factor so it gives 96-dpi output instead of 72-dpi output
scale=$(echo "scale=4; $aafactor*96/72" | bc)

# compute bounding box of initial rendering
bb=$(scale $(grep PageBoundingBox $1 | awk -f sz.awk) $scale)

# compute resolution at which to render
res=$(scale 72x72 $scale)

# render it
gs -sDEVICE=ppmraw -g$bb -r$res -sOutputFile=- -dNOPAUSE -q $1 -c showpage -c quit | pnmcrop | pamscale $aafactorinv | pnmmargin -white $border | pnmtopng > $2
