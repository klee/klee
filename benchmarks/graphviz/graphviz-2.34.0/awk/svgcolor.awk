# 
# /*************************************************************************
# * Copyright (c) 2011 AT&T Intellectual Property 
# * All rights reserved. This program and the accompanying materials
# * are made available under the terms of the Eclipse Public License v1.0
# * which accompanies this distribution, and is available at
# * http://www.eclipse.org/legal/epl-v10.html
# *
# * Contributors: See CVS logs. Details at http://www.graphviz.org/
# *************************************************************************/
# 
# Convert SVG-1.1 color data to same RGBA format used in color_names.
# See svgcolor_names for input format.
#
# All colors assumed opaque, so A = 255 in all colors
BEGIN {
	FS = "[ ,()]*";
}
/^[ 	]*$/    { next; }
/^#/    { next; }
{
	printf ("/svg/%s %s %s %s 255\n", $1, $5, $6, $7);
}
