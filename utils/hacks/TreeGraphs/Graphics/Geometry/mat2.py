# ===-- mat2.py -----------------------------------------------------------===##
# 
#                      The KLEE Symbolic Virtual Machine
# 
#  This file is distributed under the University of Illinois Open Source
#  License. See LICENSE.TXT for details.
# 
# ===----------------------------------------------------------------------===##

import vec2

def det(m):
	((m00,m01),(m10,m11))= m
	
	return m00*m11 - m01*m10

def mul(a,b):
	b_trans= zip(* b)
	return tuple([transmulvec2(b_trans, a_r) for a_r in a])

	# multiple vector v by a transposed matrix
def transmulvec2(m_trans,v):
	return tuple([vec2.dot(v, m_c) for m_c in m_trans])

def mulvec2(m,v):
	return transmulvec2(zip(* m), v)

def mulN(m,N):
	return tuple([vec2.mulN(v,N) for v in m])
