# ===-- Intersect2D.py ----------------------------------------------------===##
# 
#                      The KLEE Symbolic Virtual Machine
# 
#  This file is distributed under the University of Illinois Open Source
#  License. See LICENSE.TXT for details.
# 
# ===----------------------------------------------------------------------===##

import vec2, math

def intersectLineCircle((p, no), (C, r)):
	x = vec2.sub(p,C)
	b = 2.*vec2.dot(no, x)
	c = vec2.sqr(x) - r*r
	dist = b*b - 4*c
	
	if dist<0:
		return None
	else:
		d = math.sqrt(dist)
		t0 = (-b - d)*.5
		t1 = (-b + d)*.5
		return (t0,t1)
	
#def intersectLineCircle((lineP,lineN),(circleP,circleR)):
#	dx,dy = vec2.sub(lineP,circleP)
#	nx,ny = lineN
#
#	a =   (nx*nx + ny*ny)
#	b = 2*(dx*nx + dy*ny)
#	c =   (dx*dx + dy*dy) - circleR*circleR
#	
#	k = b*b - 4*a*c
#	if k<0:
#		return None
#	else:
#		d = math.sqrt(k)
#		t1 = (-b - d)/2*a
#		t0 = (-b + d)/2*a
#		return t0,t1
	
def intersectCircleCircle(c0P, c0R, c1P, c1R):
	v = vec2.sub(c1P, c0P)
	d = vec2.length(v)
	
	R = c0R
	r = c1R

	try:	
		x = (d*d - r*r + R*R)/(2*d)
	except ZeroDivisionError:
		if R<r:
			return 'inside',()
		elif r>R:
			return 'outside',()
		else:
			return 'coincident',()
	
	k = R*R - x*x
	if k<0:
		if x<0:
			return 'inside',()
		else:
			return 'outside',()
	else:
		y = math.sqrt(k)
		return 'intersect',(vec2.toangle(v),vec2.toangle((x,y)))
