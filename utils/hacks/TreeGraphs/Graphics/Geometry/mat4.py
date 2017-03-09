# ===-- mat4.py -----------------------------------------------------------===##
# 
#                      The KLEE Symbolic Virtual Machine
# 
#  This file is distributed under the University of Illinois Open Source
#  License. See LICENSE.TXT for details.
# 
# ===----------------------------------------------------------------------===##

import vec4,mat3

def identity():
	return ((1.0, 0.0, 0.0, 0.0),
			(0.0, 1.0, 0.0, 0.0),
			(0.0, 0.0, 1.0, 0.0),
			(0.0, 0.0, 0.0, 1.0))

def fromtrans(trans):
	return ((1.0, 0.0, 0.0, 0.0),
			(0.0, 1.0, 0.0, 0.0),
			(0.0, 0.0, 1.0, 0.0),
			(trans[0], trans[1], trans[2], 1.0))

def fromscale(scale):
	x,y,z= scale
	x,y,z= float(x),float(y),float(z)
	return ((  x, 0.0, 0.0, 0.0),
			(0.0,   y, 0.0, 0.0),
			(0.0, 0.0,   z, 0.0),
			(0.0, 0.0, 0.0, 1.0))
def fromscaleN(n):
	return fromscale((n,n,n))

def fromortho(left,right,bottom,top,znear,zfar):
	m0= ( 2.0/(right-left), 0.0, 0.0, 0.0)
	m1= (0.0,  2.0/(top-bottom), 0.0, 0.0)
	m2= (0.0, 0.0, -2.0/(zfar-znear), 0.0)
	m3= (	-((right+left)/(right-left)),
			-((top+bottom)/(top-bottom)),
			-((zfar+znear)/(zfar-znear)),
			1.0)
	return (m0,m1,m2,m3)

def mulN(m,N):
	return tuple([vec4.mulN(v,N) for v in m])
	
def mul(a,b):
	b_trans= zip(* b)
	return tuple([transmulvec4(b_trans, a_r) for a_r in a])

	# multiple vector v by a transposed matrix
def transmulvec4(m_trans,v):
	return tuple([vec4.dot(v, m_c) for m_c in m_trans])

def mulvec4(m,v):
	return transmulvec4(zip(* m), v)
	
def trans(m):
	((m00,m01,m02,m03),	
	 (m10,m11,m12,m13),	
	 (m20,m21,m22,m23),	
	 (m30,m31,m32,m33))= m
	 
	return (	(m00,m10,m20,m30),	
				(m01,m11,m21,m31),	
				(m02,m12,m22,m32),	
				(m03,m13,m23,m33))

def det(m):
	((m00,m01,m02,m03),	
	 (m10,m11,m12,m13),	
	 (m20,m21,m22,m23),	
	 (m30,m31,m32,m33))= m
	 
	a= m00 * mat3.det(	((m11, m12, m13), 
						 (m21, m22, m23), 
						 (m31, m32, m33)) );
	b= m10 * mat3.det(	((m01, m02, m03), 
						 (m21, m22, m23), 
						 (m31, m32, m33)) );
	c= m20 * mat3.det(  ((m01, m02, m03), 
						 (m11, m12, m13), 
						 (m31, m32, m33)) );
	d= m30 * mat3.det(	((m01, m02, m03), 
						 (m11, m12, m13), 
						 (m21, m22, m23)) );
	
	return a-b+c-d;

def adj(m):
	((m00,m01,m02,m03),	
	 (m10,m11,m12,m13),	
	 (m20,m21,m22,m23),	
	 (m30,m31,m32,m33))= m
	 
	t00=  mat3.det( ((m11, m12, m13), 
					 (m21, m22, m23),
					 (m31, m32, m33)) )
	t01= -mat3.det( ((m10, m12, m13), 
					 (m20, m22, m23),
					 (m30, m32, m33)) )
	t02=  mat3.det( ((m10, m11, m13), 
					 (m20, m21, m23),
					 (m30, m31, m33)) )
	t03= -mat3.det( ((m10, m11, m12), 
					 (m20, m21, m22),
					 (m30, m31, m32)) )


	t10= -mat3.det( ((m01, m02, m03), 
					 (m21, m22, m23),
					 (m31, m32, m33)) )
	t11=  mat3.det( ((m00, m02, m03), 
					 (m20, m22, m23),
					 (m30, m32, m33)) )
	t12= -mat3.det( ((m00, m01, m03), 
					 (m20, m21, m23),
					 (m30, m31, m33)) )
	t13=  mat3.det( ((m00, m01, m02), 
					 (m20, m21, m22),
					 (m30, m31, m32)) )

	t20=  mat3.det( ((m01, m02, m03), 
					 (m11, m12, m13),
					 (m31, m32, m33)) )
	t21= -mat3.det( ((m00, m02, m03), 
					 (m10, m12, m13),
					 (m30, m32, m33)) )
	t22=  mat3.det( ((m00, m01, m03), 
					 (m10, m11, m13),
					 (m30, m31, m33)) )
	t23= -mat3.det( ((m00, m01, m02), 
					 (m10, m11, m12),
					 (m30, m31, m32)) )

	t30= -mat3.det( ((m01, m02, m03), 
					 (m11, m12, m13),
					 (m21, m22, m23)) )
	t31=  mat3.det( ((m00, m02, m03), 
					 (m10, m12, m13),
					 (m20, m22, m23)) )
	t32= -mat3.det( ((m00, m01, m03), 
					 (m10, m11, m13),
					 (m20, m21, m23)) )
	t33=  mat3.det( ((m00, m01, m02), 
					 (m10, m11, m12),
					 (m20, m21, m22)) )
					
	return ((t00,t01,t02,t03),
			(t10,t11,t12,t13),
			(t20,t21,t22,t23),
			(t30,t31,t32,t33))
	
def inv(m):
	d= det(m)
	t= trans(adj(m))
	v= 1.0/d
	return tuple([(a*v,b*v,c*v,d*v) for a,b,c,d in t])
	
def toGL(m):
	m0,m1,m2,m3= m
	return m0+m1+m2+m3
