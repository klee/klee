from __future__ import division
from math import ceil,floor,sqrt
import vec3
_min,_max= min,max

def inv(a):	return (-a[0], -a[1], -a[2], -a[3])

def add(a,b):	return (a[0]+b[0], a[1]+b[1], a[2]+b[2], a[3]+b[3])
def sub(a,b):	return (a[0]-b[0], a[1]-b[1], a[2]-b[2], a[3]-b[3])
def mul(a,b):	return (a[0]*b[0], a[1]*b[1], a[2]*b[2], a[3]*b[3])
def div(a,b):	return (a[0]/b[0], a[1]/b[1], a[2]/b[2], a[3]/b[3])
def mod(a,b):	return (a[0]%b[0], a[1]%b[1], a[2]%b[2], a[3]%b[3])
def dot(a,b):	return (a[0]*b[0]+ a[1]*b[1]+ a[2]*b[2]+ a[3]*b[3])

def addN(a,n):	return (a[0]+n, a[1]+n, a[2]+n, a[3]+n)
def subN(a,n):	return (a[0]-n, a[1]-n, a[2]-n, a[3]-n)
def mulN(a,n):	return (a[0]*n, a[1]*n, a[2]*n, a[3]*n)
def modN(a,n):	return (a[0]%n, a[1]%n, a[2]%n, a[3]%n)
def divN(a,n):	return (a[0]/n, a[1]/n, a[2]/n, a[3]/n)

def sqr(a):		return dot(a,a)
def length(a):	return sqrt(sqr(a))
def avg(a,b):	return mulN(add(a,b),0.5)
def normalize(a):	return mulN(a, 1.0/length(a))

def lerp(a,b,t):
	return add(mulN(a,1.0-t), mulN(b, t))

def min((a0,a1,a2,a3),(b0,b1,b2,b3)):
	return (_min(a0,b0),_min(a1,b1),_min(a2,b2),_min(a3,b3))
def max((a0,a1,a2,a3),(b0,b1,b2,b3)):
	return (_max(a0,b0),_max(a1,b1),_max(a2,b2),_max(a3,b3))

def toint(a):
	return (int(a[0]), int(a[1]), int(a[2]), int(a[3]))
def tofloor(a):
	return (floor(a[0]), floor(a[1]), floor(a[2]), floor(a[3]))
def toceil(a):
	return (ceil(a[0]), ceil(a[1]), ceil(a[2]), ceil(a[3]))
def tovec3(a):
	return vec3.divN(a, a[3])

def sumlist(l):
	return reduce(add, l)
def avglist(l):
	return mulN(sumlist(l), 1.0/len(l))
