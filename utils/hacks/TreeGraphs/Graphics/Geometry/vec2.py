# ===-- vec2.py -----------------------------------------------------------===##
# 
#                      The KLEE Symbolic Virtual Machine
# 
#  This file is distributed under the University of Illinois Open Source
#  License. See LICENSE.TXT for details.
# 
# ===----------------------------------------------------------------------===##

from __future__ import division
from math import ceil,floor,sqrt,atan2,pi,cos,sin
import random
_abs,_min,_max= abs,min,max

def random(rng=random):
	return (rng.random()*2-1,rng.random()*2-1)

def getangle(a):
	x,y= a
	if y>=0:
		return atan2(y,x)
	else:
		return pi*2 + atan2(y,x)
toangle = getangle

def topolar(pt):
	return getangle(pt),length(pt)
def fromangle(angle,radius=1.):
	return (cos(angle)*radius, sin(angle)*radius)
frompolar = fromangle

def rotate((x,y),angle):
	c_a,s_a = cos(angle),sin(angle)
	return (c_a*x - s_a*y, s_a*x + c_a*y)

def rotate90((x,y)):
	return (-y,x)
	
def abs(a): return (_abs(a[0]),_abs(a[1]))	
def inv(a):	return (-a[0], -a[1])

def add(a,b):	return (a[0]+b[0], a[1]+b[1])
def sub(a,b):	return (a[0]-b[0], a[1]-b[1])
def mul(a,b):	return (a[0]*b[0], a[1]*b[1])
def div(a,b):	return (a[0]/b[0], a[1]/b[1])
def mod(a,b):	return (a[0]%b[0], a[1]%b[1])
def dot(a,b):	return (a[0]*b[0]+ a[1]*b[1])

def addN(a,n):	return (a[0]+n, a[1]+n)
def subN(a,n):	return (a[0]-n, a[1]-n)
def mulN(a,n):	return (a[0]*n, a[1]*n)
def modN(a,n):	return (a[0]%n, a[1]%n)
def divN(a,n):	return (a[0]/n, a[1]/n)

def sqr(a):			return dot(a,a)
def length(a):		return sqrt(sqr(a))
def avg(a,b):		return mulN(add(a,b),0.5)
def distance(a,b):	return length(sub(a,b))

def normalize(a):
	return mulN(a, 1.0/length(a))

def normalizeOrZero(a):
	try:
		return mulN(a, 1.0/length(a))
	except ZeroDivisionError:
		return (0.0,0.0)
	
def min((a0,a1),(b0,b1)):
	return (_min(a0,b0),_min(a1,b1))
def max((a0,a1),(b0,b1)):
	return (_max(a0,b0),_max(a1,b1))

def lerp(a,b,t):
	return add(mulN(a,1.0-t), mulN(b, t))

def toint(a):
	return (int(a[0]), int(a[1]))
def tofloor(a):
	return (floor(a[0]), floor(a[1]))
def toceil(a):
	return (ceil(a[0]), ceil(a[1]))

def sumlist(l):
	return reduce(add, l)
def avglist(l):
	return mulN(sumlist(l), 1.0/len(l))

