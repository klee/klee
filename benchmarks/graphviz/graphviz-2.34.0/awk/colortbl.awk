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

function rgb_to_hsv(r,g,b) {
	r = r / 255.0; g = g / 255.0; b = b / 255.0;
	max = r; if (max < g) max = g; if (max < b) max = b;
	min = r; if (min > g) min = g; if (min > b) min = b;
	v = max;
	if (max != 0) s = (max - min) / max;
	else s = 0;
	if (s == 0) h = 0;
	else {
		delta = max - min;
		rc = (max - r)/delta;
		gc = (max - g)/delta;
		bc = (max - b)/delta;
		if (r == max) h = bc - gc;
		else {
			if (g == max) h = 2.0 + (rc - bc);
			else h = 4.0 + (gc - rc);
		}
		h = h * 60.0;
		if (h < 0.0) h = h + 360.0;
	}
	h = h / 360.0 * 255.0;
	s = s * 255.0;
	v = v * 255.0;
}

BEGIN	{ s = ARGV[1]; gsub("\\.","_",s); printf("static hsvrgbacolor_t %s[] = {\n",s); }
/^$/	{ next; }
/^#/	{ next; }
		{
			rgb_to_hsv($2,$3,$4);
			printf("{\"%s\",%d,%d,%d,%d,%d,%d,%d},\n",$1,h,s,v,$2,$3,$4,$5);
		}
END		{ printf("};\n"); }
