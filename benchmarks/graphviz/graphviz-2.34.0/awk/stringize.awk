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

BEGIN	{ s = ARGV[1]; gsub (".*/", "", s); gsub("\\.","_",s); printf("static const char *%s[] = {\n",s); }
/^#/	{ print $0; next; }
		{ gsub("\\\\","&&",$0); printf("\"%s\",\n",$0); }
END		{ printf("(char*)0 };\n"); }
