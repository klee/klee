from __future__ import division

import math
import vec3, vec4

def identity():
	return (0.0,0.0,0.0,1.0)

def fromaxisangle(axisangle):
	axis,angle= axisangle
	ang_2= angle/2.0
	s_ang= math.sin(ang_2)
	c_ang= math.cos(ang_2)
	
	q= vec3.mulN(axis, s_ang) + (c_ang,)
	return normalize(q)

def fromnormals(n1,n2):
	axis,angle= vec3.normalize(vec3.cross(n1, n2)), math.acos(vec3.dot(n1, n2))
	return fromaxisangle((axis,angle))

	# avoid trigonmetry
def fromnormals_faster(n1,n2):
	axis= vec3.normalize(vec3.cross(n1, n2))

	half_n= vec3.normalize(vec3.add(n1, n2))
	cos_half_angle= vec3.dot(n1, half_n)
	sin_half_angle= 1.0 - cos_half_angle**2
	
	return vec3.mulN(axis, sin_half_angle) + (cos_half_angle,)

def fromvectors(v1,v2):
	return fromnormals(vec3.normalize(v1), vec3.normalize(v2))

def magnitude(q):
	return vec4.length(q)

def normalize(q):
	return vec4.divN(q, magnitude(q))

def conjugate(q):
	x,y,z,w= q
	return (-x, -y, -z, w)

def mulvec3(q, v):
	t= mul(q, v+(0.0,))
	t= mul(t, conjugate(q))
	return t[:3]

def mul(a, b):
	ax,ay,az,aw= a
	bx,by,bz,bw= b

	x= aw*bx + ax*bw + ay*bz - az*by
	y= aw*by + ay*bw + az*bx - ax*bz
	z= aw*bz + az*bw + ax*by - ay*bx 
	w= aw*bw - ax*bx - ay*by - az*bz

	return (x,y,z,w)

def toaxisangle(q):
	tw= math.acos(q[3])
	scale= math.sin(tw)
	angle= tw*2.0

	try:
		axis= vec3.divN(q[:3], scale)
	except ZeroDivisionError:
		axis= (1.0,0.0,0.0)

	return axis,angle

def tomat3x3(q):
	x,y,z,w= q

	m0= (	1.0 -	2.0 * ( y*y + z*z ),
					2.0 * ( x*y - z*w ),
					2.0 * ( x*z + y*w ))
	m1=	(			2.0 * ( x*y + z*w ),
			1.0 -	2.0 * ( x*x + z*z ),
					2.0 * ( y*z - x*w )) 
	m2=	(			2.0 * ( x*z - y*w ),
					2.0 * ( y*z + x*w ),
			1.0 -	2.0 * ( x*x + y*y ))

	return m0,m1,m2

def tomat4x4(q):
	m0,m1,m2= tomat3x3(q)
	return (m0 + (0.0,),
			m1 + (0.0,),
			m2 + (0.0,),
			(0.0, 0.0, 0.0, 1.0))

def slerp(a, b, t):
	raise NotImplementedError

	cos_omega= vec4.dot(a, b)
	
	if (cos_omega<0.0):
		cos_omega= -cos_omega
		b= vec4.neg(b)

	imega= math.acos(cos_omega)
	t= sin(t*omega)/sin(omega)

	return vec4.lerp(a, b, t)
