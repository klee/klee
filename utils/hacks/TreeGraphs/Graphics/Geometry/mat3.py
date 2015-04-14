import vec3,mat2

def identity():
	return ((1.0, 0.0, 0.0),
			(0.0, 1.0, 0.0),
			(0.0, 0.0, 1.0))

def fromscale(scale):
	x,y,z= scale
	x,y,z= float(x),float(y),float(z)
	return ((  x, 0.0, 0.0),
			(0.0,   y, 0.0),
			(0.0, 0.0,   z))
def fromscaleN(n):
	return fromscale((n,n,n))

def mul(a,b):
	b_trans= zip(* b)
	return tuple([transmulvec3(b_trans, a_r) for a_r in a])

	# multiple vector v by a transposed matrix
def transmulvec3(m_trans,v):
	return tuple([vec3.dot(v, m_c) for m_c in m_trans])

def mulvec3(m,v):
	return transmulvec3(zip(* m), v)

def mulN(m,N):
	return tuple([vec3.mulN(v,N) for v in m])

def det(m):
	((m00,m01,m02),
	 (m10,m11,m12),
	 (m20,m21,m22))= m

	a= m00 * mat2.det( ((m11, m12), (m21, m22)) );
	b= m10 * mat2.det( ((m01, m02), (m21, m22)) );
	c= m20 * mat2.det( ((m01, m02), (m11, m12)) );
	
	return a-b+c
