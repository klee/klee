/* $Id$Revision: */
/* vim:set shiftwidth=4 ts=8: */

/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: See CVS logs. Details at http://www.graphviz.org/
 *************************************************************************/

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "logic.h"
#include "memory.h"
#include "arith.h"
#include "SparseMatrix.h"
#include "BinaryHeap.h"
#if PQ
#include "LinkedList.h"
#include "PriorityQueue.h"
#endif

static size_t size_of_matrix_type(int type){
  int size = 0;
  switch (type){
  case MATRIX_TYPE_REAL:
    size = sizeof(real);
    break;
  case MATRIX_TYPE_COMPLEX:
    size = 2*sizeof(real);
    break;
  case MATRIX_TYPE_INTEGER:
    size = sizeof(int);
    break;
  case MATRIX_TYPE_PATTERN:
    size = 0;
    break;
  case MATRIX_TYPE_UNKNOWN:
    size = 0;
    break;
  default:
    size = 0;
    break;
  }

  return size;
}

SparseMatrix SparseMatrix_sort(SparseMatrix A){
  SparseMatrix B;
  B = SparseMatrix_transpose(A);
  SparseMatrix_delete(A);
  A = SparseMatrix_transpose(B);
  SparseMatrix_delete(B);
  return A;
}
SparseMatrix SparseMatrix_make_undirected(SparseMatrix A){
  /* make it strictly low diag only, and set flag to undirected */
  SparseMatrix B;
  B = SparseMatrix_symmetrize(A, FALSE);
  SparseMatrix_set_undirected(B);
  return SparseMatrix_remove_upper(B);
}
SparseMatrix SparseMatrix_transpose(SparseMatrix A){
  int *ia = A->ia, *ja = A->ja, *ib, *jb, nz = A->nz, m = A->m, n = A->n, type = A->type, format = A->format;
  SparseMatrix B;
  int i, j;

  if (!A) return NULL;
  assert(A->format == FORMAT_CSR);/* only implemented for CSR right now */

  B = SparseMatrix_new(n, m, nz, type, format);
  B->nz = nz;
  ib = B->ia;
  jb = B->ja;

  for (i = 0; i <= n; i++) ib[i] = 0;
  for (i = 0; i < m; i++){
    for (j = ia[i]; j < ia[i+1]; j++){
      ib[ja[j]+1]++;
    }
  }

  for (i = 0; i < n; i++) ib[i+1] += ib[i];

  switch (A->type){
  case MATRIX_TYPE_REAL:{
    real *a = (real*) A->a;
    real *b = (real*) B->a;
    for (i = 0; i < m; i++){
      for (j = ia[i]; j < ia[i+1]; j++){
	jb[ib[ja[j]]] = i;
	b[ib[ja[j]]++] = a[j];
      }
    }
    break;
  }
  case MATRIX_TYPE_COMPLEX:{
    real *a = (real*) A->a;
    real *b = (real*) B->a;
    for (i = 0; i < m; i++){
      for (j = ia[i]; j < ia[i+1]; j++){
	jb[ib[ja[j]]] = i;
	b[2*ib[ja[j]]] = a[2*j];
	b[2*ib[ja[j]]+1] = a[2*j+1];
	ib[ja[j]]++;
      }
    }
    break;
  }
  case MATRIX_TYPE_INTEGER:{
    int *ai = (int*) A->a;
    int *bi = (int*) B->a;
    for (i = 0; i < m; i++){
      for (j = ia[i]; j < ia[i+1]; j++){
	jb[ib[ja[j]]] = i;
	bi[ib[ja[j]]++] = ai[j];
      }
    }
    break;
  }
  case MATRIX_TYPE_PATTERN:
    for (i = 0; i < m; i++){
      for (j = ia[i]; j < ia[i+1]; j++){
	jb[ib[ja[j]]++] = i;
      }
    }
    break;
  case MATRIX_TYPE_UNKNOWN:
    SparseMatrix_delete(B);
    return NULL;
  default:
    SparseMatrix_delete(B);
    return NULL;
  }


  for (i = n-1; i >= 0; i--) ib[i+1] = ib[i];
  ib[0] = 0;
  

  return B;
}

SparseMatrix SparseMatrix_symmetrize(SparseMatrix A, int pattern_symmetric_only){
  SparseMatrix B;
  if (SparseMatrix_is_symmetric(A, pattern_symmetric_only)) return SparseMatrix_copy(A);
  B = SparseMatrix_transpose(A);
  if (!B) return NULL;
  A = SparseMatrix_add(A, B);
  SparseMatrix_delete(B);
  SparseMatrix_set_symmetric(A);
  SparseMatrix_set_pattern_symmetric(A);
  return A;
}

SparseMatrix SparseMatrix_symmetrize_nodiag(SparseMatrix A, int pattern_symmetric_only){
  SparseMatrix B;
  if (SparseMatrix_is_symmetric(A, pattern_symmetric_only)) {
    B = SparseMatrix_copy(A);
    return SparseMatrix_remove_diagonal(B);
  }
  B = SparseMatrix_transpose(A);
  if (!B) return NULL;
  A = SparseMatrix_add(A, B);
  SparseMatrix_delete(B);
  SparseMatrix_set_symmetric(A);
  SparseMatrix_set_pattern_symmetric(A);
  return SparseMatrix_remove_diagonal(A);
}

int SparseMatrix_is_symmetric(SparseMatrix A, int test_pattern_symmetry_only){
  /* assume no repeated entries! */
  SparseMatrix B;
  int *ia, *ja, *ib, *jb, type, m;
  int *mask;
  int res = FALSE;
  int i, j;
  assert(A->format == FORMAT_CSR);/* only implemented for CSR right now */

  if (!A) return FALSE;

  if (SparseMatrix_known_symmetric(A)) return TRUE;
  if (test_pattern_symmetry_only && SparseMatrix_known_strucural_symmetric(A)) return TRUE;

  if (A->m != A->n) return FALSE;

  B = SparseMatrix_transpose(A);
  if (!B) return FALSE;

  ia = A->ia;
  ja = A->ja;
  ib = B->ia;
  jb = B->ja;
  m = A->m;

  mask = MALLOC(sizeof(int)*((size_t) m));
  for (i = 0; i < m; i++) mask[i] = -1;

  type = A->type;
  if (test_pattern_symmetry_only) type = MATRIX_TYPE_PATTERN;

  switch (type){
  case MATRIX_TYPE_REAL:{
    real *a = (real*) A->a;
    real *b = (real*) B->a;
    for (i = 0; i <= m; i++) if (ia[i] != ib[i]) goto RETURN;
    for (i = 0; i < m; i++){
      for (j = ia[i]; j < ia[i+1]; j++){
	mask[ja[j]] = j;
      }
      for (j = ib[i]; j < ib[i+1]; j++){
	if (mask[jb[j]] < ia[i]) goto RETURN;
      }
      for (j = ib[i]; j < ib[i+1]; j++){
	if (ABS(b[j] - a[mask[jb[j]]]) > SYMMETRY_EPSILON) goto RETURN;
      }
    }
    res = TRUE;
    break;
  }
  case MATRIX_TYPE_COMPLEX:{
    real *a = (real*) A->a;
    real *b = (real*) B->a;
    for (i = 0; i <= m; i++) if (ia[i] != ib[i]) goto RETURN;
    for (i = 0; i < m; i++){
      for (j = ia[i]; j < ia[i+1]; j++){
	mask[ja[j]] = j;
      }
      for (j = ib[i]; j < ib[i+1]; j++){
	if (mask[jb[j]] < ia[i]) goto RETURN;
      }
      for (j = ib[i]; j < ib[i+1]; j++){
	if (ABS(b[2*j] - a[2*mask[jb[j]]]) > SYMMETRY_EPSILON) goto RETURN;
	if (ABS(b[2*j+1] - a[2*mask[jb[j]]+1]) > SYMMETRY_EPSILON) goto RETURN;
      }
    }
    res = TRUE;
    break;
  }
  case MATRIX_TYPE_INTEGER:{
    int *ai = (int*) A->a;
    int *bi = (int*) B->a;
    for (i = 0; i < m; i++){
      for (j = ia[i]; j < ia[i+1]; j++){
	mask[ja[j]] = j;
      }
      for (j = ib[i]; j < ib[i+1]; j++){
	if (mask[jb[j]] < ia[i]) goto RETURN;
      }
      for (j = ib[i]; j < ib[i+1]; j++){
	if (bi[j] != ai[mask[jb[j]]]) goto RETURN;
      }
    }
    res = TRUE;
    break;
  }
  case MATRIX_TYPE_PATTERN:
    for (i = 0; i < m; i++){
      for (j = ia[i]; j < ia[i+1]; j++){
	mask[ja[j]] = j;
      }
      for (j = ib[i]; j < ib[i+1]; j++){
	if (mask[jb[j]] < ia[i]) goto RETURN;
      }
    }
    res = TRUE;
    break;
  case MATRIX_TYPE_UNKNOWN:
    goto RETURN;
    break;
  default:
    goto RETURN;
    break;
  }

  if (test_pattern_symmetry_only){
    SparseMatrix_set_pattern_symmetric(A);
  } else {
    SparseMatrix_set_symmetric(A);
    SparseMatrix_set_pattern_symmetric(A);
  }
 RETURN:
  FREE(mask);

  SparseMatrix_delete(B);
  return res;
}

static SparseMatrix SparseMatrix_init(int m, int n, int type, size_t sz, int format){
  SparseMatrix A;


  A = MALLOC(sizeof(struct SparseMatrix_struct));
  A->m = m;
  A->n = n;
  A->nz = 0;
  A->nzmax = 0;
  A->type = type;
  A->size = sz;
  switch (format){
  case FORMAT_COORD:
    A->ia = NULL;
    break;
  case FORMAT_CSC:
  case FORMAT_CSR:
  default:
    A->ia = MALLOC(sizeof(int)*((size_t)(m+1)));
  }
  A->ja = NULL;
  A->a = NULL;
  A->format = format;
  A->property = 0;
  clear_flag(A->property, MATRIX_PATTERN_SYMMETRIC);
  clear_flag(A->property, MATRIX_SYMMETRIC);
  clear_flag(A->property, MATRIX_SKEW);
  clear_flag(A->property, MATRIX_HERMITIAN);
  return A;
}

static SparseMatrix SparseMatrix_alloc(SparseMatrix A, int nz){
  int format = A->format;
  size_t nz_t = (size_t) nz; /* size_t is 64 bit on 64 bit machine. Using nz*A->size can overflow. */

  A->a = NULL;
  switch (format){
  case FORMAT_COORD:
    A->ia = MALLOC(sizeof(int)*nz_t);
    A->ja = MALLOC(sizeof(int)*nz_t);
    A->a = MALLOC(A->size*nz_t);
    break;
  case FORMAT_CSR:
  case FORMAT_CSC:
  default:
    A->ja = MALLOC(sizeof(int)*nz_t);
    if (A->size > 0 && nz_t > 0) {
      A->a = MALLOC(A->size*nz_t);
    }
    break;
  }
  A->nzmax = nz;
  return A;
}

static SparseMatrix SparseMatrix_realloc(SparseMatrix A, int nz){
  int format = A->format;
  size_t nz_t = (size_t) nz; /* size_t is 64 bit on 64 bit machine. Using nz*A->size can overflow. */

  switch (format){
  case FORMAT_COORD:
    A->ia = REALLOC(A->ia, sizeof(int)*nz_t);
    A->ja = REALLOC(A->ja, sizeof(int)*nz_t);
    if (A->size > 0) {
      if (A->a){
	A->a = REALLOC(A->a, A->size*nz_t);
      } else {
	A->a = MALLOC(A->size*nz_t);
      }
    } 
    break;
  case FORMAT_CSR:
  case FORMAT_CSC:
  default:
    A->ja = REALLOC(A->ja, sizeof(int)*nz_t);
    if (A->size > 0) {
      if (A->a){
	A->a = REALLOC(A->a, A->size*nz_t);
      } else {
	A->a = MALLOC(A->size*nz_t);
      }
    }
    break;
  }
  A->nzmax = nz;
  return A;
}

SparseMatrix SparseMatrix_new(int m, int n, int nz, int type, int format){
  /* return a sparse matrix skeleton with row dimension m and storage nz. If nz == 0, 
     only row pointers are allocated */
  SparseMatrix A;
  size_t sz;

  sz = size_of_matrix_type(type);
  A = SparseMatrix_init(m, n, type, sz, format);

  if (nz > 0) A = SparseMatrix_alloc(A, nz);
  return A;

}
SparseMatrix SparseMatrix_general_new(int m, int n, int nz, int type, size_t sz, int format){
  /* return a sparse matrix skeleton with row dimension m and storage nz. If nz == 0, 
     only row pointers are allocated. this is more general and allow elements to be 
     any data structure, not just real/int/complex etc
  */
  SparseMatrix A;

  A = SparseMatrix_init(m, n, type, sz, format);

  if (nz > 0) A = SparseMatrix_alloc(A, nz);
  return A;

}

void SparseMatrix_delete(SparseMatrix A){
  /* return a sparse matrix skeleton with row dimension m and storage nz. If nz == 0, 
     only row pointers are allocated */
  if (!A) return;
  if (A->ia) FREE(A->ia);
  if (A->ja) FREE(A->ja);
  if (A->a) FREE(A->a);
  FREE(A);
}
void SparseMatrix_print_csr(char *c, SparseMatrix A){
  int *ia, *ja;
  real *a;
  int *ai;
  int i, j, m = A->m;
  
  assert (A->format == FORMAT_CSR);
  printf("%s\n SparseArray[{",c);
  ia = A->ia;
  ja = A->ja;
  a = A->a;
  switch (A->type){
  case MATRIX_TYPE_REAL:
    a = (real*) A->a;
    for (i = 0; i < m; i++){
      for (j = ia[i]; j < ia[i+1]; j++){
	printf("{%d, %d}->%f",i+1, ja[j]+1, a[j]);
	if (j != ia[m]-1) printf(",");
      }
    }
    break;
  case MATRIX_TYPE_COMPLEX:
    a = (real*) A->a;
    for (i = 0; i < m; i++){
      for (j = ia[i]; j < ia[i+1]; j++){
	printf("{%d, %d}->%f + %f I",i+1, ja[j]+1, a[2*j], a[2*j+1]);
 	if (j != ia[m]-1) printf(",");
     }
    }
    printf("\n");
    break;
  case MATRIX_TYPE_INTEGER:
    ai = (int*) A->a;
    for (i = 0; i < m; i++){
      for (j = ia[i]; j < ia[i+1]; j++){
	printf("{%d, %d}->%d",i+1, ja[j]+1, ai[j]);
 	if (j != ia[m]-1) printf(",");
     }
    }
    printf("\n");
    break;
  case MATRIX_TYPE_PATTERN:
    for (i = 0; i < m; i++){
      for (j = ia[i]; j < ia[i+1]; j++){
	printf("{%d, %d}->_",i+1, ja[j]+1);
 	if (j != ia[m]-1) printf(",");
      }
    }
    printf("\n");
    break;
  case MATRIX_TYPE_UNKNOWN:
    return;
  default:
    return;
  }
  printf("},{%d, %d}]\n", m, A->n);

}



void SparseMatrix_print_coord(char *c, SparseMatrix A){
  int *ia, *ja;
  real *a;
  int *ai;
  int i, m = A->m;
  
  assert (A->format == FORMAT_COORD);
  printf("%s\n SparseArray[{",c);
  ia = A->ia;
  ja = A->ja;
  a = A->a;
  switch (A->type){
  case MATRIX_TYPE_REAL:
    a = (real*) A->a;
    for (i = 0; i < A->nz; i++){
      printf("{%d, %d}->%f",ia[i]+1, ja[i]+1, a[i]);
      if (i != A->nz - 1) printf(",");
    }
    printf("\n");
    break;
  case MATRIX_TYPE_COMPLEX:
    a = (real*) A->a;
    for (i = 0; i < A->nz; i++){
      printf("{%d, %d}->%f + %f I",ia[i]+1, ja[i]+1, a[2*i], a[2*i+1]);
      if (i != A->nz - 1) printf(",");
    }
    printf("\n");
    break;
  case MATRIX_TYPE_INTEGER:
    ai = (int*) A->a;
    for (i = 0; i < A->nz; i++){
      printf("{%d, %d}->%d",ia[i]+1, ja[i]+1, ai[i]);
      if (i != A->nz) printf(",");
    }
    printf("\n");
    break;
  case MATRIX_TYPE_PATTERN:
    for (i = 0; i < A->nz; i++){
      printf("{%d, %d}->_",ia[i]+1, ja[i]+1);
      if (i != A->nz - 1) printf(",");
    }
    printf("\n");
    break;
  case MATRIX_TYPE_UNKNOWN:
    return;
  default:
    return;
  }
  printf("},{%d, %d}]\n", m, A->n);

}




void SparseMatrix_print(char *c, SparseMatrix A){
  switch (A->format){
  case FORMAT_CSR:
    SparseMatrix_print_csr(c, A);
    break;
  case FORMAT_CSC:
    assert(0);/* not implemented yet...
		 SparseMatrix_print_csc(c, A);*/
    break;
  case FORMAT_COORD:
    SparseMatrix_print_coord(c, A);
    break;
  default:
    assert(0);
  }
}





static void SparseMatrix_export_csr(FILE *f, SparseMatrix A){
  int *ia, *ja;
  real *a;
  int *ai;
  int i, j, m = A->m;
  
  switch (A->type){
  case MATRIX_TYPE_REAL:
    fprintf(f,"%%%%MatrixMarket matrix coordinate real general\n");
    break;
  case MATRIX_TYPE_COMPLEX:
    fprintf(f,"%%%%MatrixMarket matrix coordinate complex general\n");
    break;
  case MATRIX_TYPE_INTEGER:
    fprintf(f,"%%%%MatrixMarket matrix coordinate integer general\n");
    break;
  case MATRIX_TYPE_PATTERN:
    fprintf(f,"%%%%MatrixMarket matrix coordinate pattern general\n");
    break;
  case MATRIX_TYPE_UNKNOWN:
    return;
  default:
    return;
  }

  fprintf(f,"%d %d %d\n",A->m,A->n,A->nz);
  ia = A->ia;
  ja = A->ja;
  a = A->a;
  switch (A->type){
  case MATRIX_TYPE_REAL:
    a = (real*) A->a;
    for (i = 0; i < m; i++){
      for (j = ia[i]; j < ia[i+1]; j++){
	fprintf(f, "%d %d %16.8g\n",i+1, ja[j]+1, a[j]);
      }
    }
    break;
  case MATRIX_TYPE_COMPLEX:
    a = (real*) A->a;
    for (i = 0; i < m; i++){
      for (j = ia[i]; j < ia[i+1]; j++){
	fprintf(f, "%d %d %16.8g %16.8g\n",i+1, ja[j]+1, a[2*j], a[2*j+1]);
     }
    }
    break;
  case MATRIX_TYPE_INTEGER:
    ai = (int*) A->a;
    for (i = 0; i < m; i++){
      for (j = ia[i]; j < ia[i+1]; j++){
	fprintf(f, "%d %d %d\n",i+1, ja[j]+1, ai[j]);
     }
    }
    break;
  case MATRIX_TYPE_PATTERN:
    for (i = 0; i < m; i++){
      for (j = ia[i]; j < ia[i+1]; j++){
	fprintf(f, "%d %d\n",i+1, ja[j]+1);
      }
    }
    break;
  case MATRIX_TYPE_UNKNOWN:
    return;
  default:
    return;
  }

}

void SparseMatrix_export_binary(char *name, SparseMatrix A, int *flag){
  FILE *f;

  *flag = 0;
  f = fopen(name, "wb");
  if (!f) {
    *flag = 1;
    return;
  }
  fwrite(&(A->m), sizeof(int), 1, f);
  fwrite(&(A->n), sizeof(int), 1, f);
  fwrite(&(A->nz), sizeof(int), 1, f);
  fwrite(&(A->nzmax), sizeof(int), 1, f);
  fwrite(&(A->type), sizeof(int), 1, f);
  fwrite(&(A->format), sizeof(int), 1, f);
  fwrite(&(A->property), sizeof(int), 1, f);
  fwrite(&(A->size), sizeof(size_t), 1, f);
  if (A->format == FORMAT_COORD){
    fwrite(A->ia, sizeof(int), A->nz, f);
  } else {
    fwrite(A->ia, sizeof(int), A->m + 1, f);
  }
  fwrite(A->ja, sizeof(int), A->nz, f);
  if (A->size > 0) fwrite(A->a, A->size, A->nz, f);
  fclose(f);

}



SparseMatrix SparseMatrix_import_binary(char *name){
  SparseMatrix A = NULL;
  int m, n, nz, nzmax, type, format, property, iread;
  size_t sz;
  FILE *f;

  f = fopen(name, "rb");

  if (!f) return NULL;
  iread = fread(&m, sizeof(int), 1, f);
  if (iread != 1) return NULL;
  iread = fread(&n, sizeof(int), 1, f);
  if (iread != 1) return NULL;
  iread = fread(&nz, sizeof(int), 1, f);
  if (iread != 1) return NULL;
  iread = fread(&nzmax, sizeof(int), 1, f);
  if (iread != 1) return NULL;
  iread = fread(&type, sizeof(int), 1, f);
  if (iread != 1) return NULL;
  iread = fread(&format, sizeof(int), 1, f);
  if (iread != 1) return NULL;
  iread = fread(&property, sizeof(int), 1, f);
  if (iread != 1) return NULL;
  iread = fread(&sz, sizeof(size_t), 1, f);
  if (iread != 1) return NULL;
  
  A = SparseMatrix_general_new(m, n, nz, type, sz, format);
  A->nz = nz;
  A->property = property;
  
  if (format == FORMAT_COORD){
    iread = fread(A->ia, sizeof(int), A->nz, f);
    if (iread != A->nz) return NULL;
  } else {
    iread = fread(A->ia, sizeof(int), A->m + 1, f);
    if (iread != A->m + 1) return NULL;
  }
  iread = fread(A->ja, sizeof(int), A->nz, f);
  if (iread != A->nz) return NULL;

  if (A->size > 0) {
    iread = fread(A->a, A->size, A->nz, f);
    if (iread != A->nz) return NULL;
  }
  fclose(f);
  return A;
}

static void SparseMatrix_export_coord(FILE *f, SparseMatrix A){
  int *ia, *ja;
  real *a;
  int *ai;
  int i;
  
  switch (A->type){
  case MATRIX_TYPE_REAL:
    fprintf(f,"%%%%MatrixMarket matrix coordinate real general\n");
    break;
  case MATRIX_TYPE_COMPLEX:
    fprintf(f,"%%%%MatrixMarket matrix coordinate complex general\n");
    break;
  case MATRIX_TYPE_INTEGER:
    fprintf(f,"%%%%MatrixMarket matrix coordinate integer general\n");
    break;
  case MATRIX_TYPE_PATTERN:
    fprintf(f,"%%%%MatrixMarket matrix coordinate pattern general\n");
    break;
  case MATRIX_TYPE_UNKNOWN:
    return;
  default:
    return;
  }

  fprintf(f,"%d %d %d\n",A->m,A->n,A->nz);
  ia = A->ia;
  ja = A->ja;
  a = A->a;
  switch (A->type){
  case MATRIX_TYPE_REAL:
    a = (real*) A->a;
    for (i = 0; i < A->nz; i++){
      fprintf(f, "%d %d %16.8g\n",ia[i]+1, ja[i]+1, a[i]);
    }
    break;
  case MATRIX_TYPE_COMPLEX:
    a = (real*) A->a;
    for (i = 0; i < A->nz; i++){
      fprintf(f, "%d %d %16.8g %16.8g\n",ia[i]+1, ja[i]+1, a[2*i], a[2*i+1]);
    }
    break;
  case MATRIX_TYPE_INTEGER:
    ai = (int*) A->a;
    for (i = 0; i < A->nz; i++){
      fprintf(f, "%d %d %d\n",ia[i]+1, ja[i]+1, ai[i]);
    }
    break;
  case MATRIX_TYPE_PATTERN:
    for (i = 0; i < A->nz; i++){
      fprintf(f, "%d %d\n",ia[i]+1, ja[i]+1);
    }
    break;
  case MATRIX_TYPE_UNKNOWN:
    return;
  default:
    return;
  }
}



void SparseMatrix_export(FILE *f, SparseMatrix A){

  switch (A->format){
  case FORMAT_CSR:
    SparseMatrix_export_csr(f, A);
    break;
  case FORMAT_CSC:
    assert(0);/* not implemented yet...
		 SparseMatrix_print_csc(c, A);*/
    break;
  case FORMAT_COORD:
    SparseMatrix_export_coord(f, A);
    break;
  default:
    assert(0);
  }
}


SparseMatrix SparseMatrix_from_coordinate_format(SparseMatrix A){
  /* convert a sparse matrix in coordinate form to one in compressed row form.*/
  int *irn, *jcn;

  void *a = A->a;

  assert(A->format == FORMAT_COORD);
  if (A->format != FORMAT_COORD) {
    return NULL;
  }
  irn = A->ia;
  jcn = A->ja;
  return SparseMatrix_from_coordinate_arrays(A->nz, A->m, A->n, irn, jcn, a, A->type, A->size);

}
SparseMatrix SparseMatrix_from_coordinate_format_not_compacted(SparseMatrix A, int what_to_sum){
  /* convert a sparse matrix in coordinate form to one in compressed row form.*/
  int *irn, *jcn;

  void *a = A->a;

  assert(A->format == FORMAT_COORD);
  if (A->format != FORMAT_COORD) {
    return NULL;
  }
  irn = A->ia;
  jcn = A->ja;
  return SparseMatrix_from_coordinate_arrays_not_compacted(A->nz, A->m, A->n, irn, jcn, a, A->type, A->size, what_to_sum);

}

static SparseMatrix SparseMatrix_from_coordinate_arrays_internal(int nz, int m, int n, int *irn, int *jcn, void *val0, int type, size_t sz, int sum_repeated){
  /* convert a sparse matrix in coordinate form to one in compressed row form.
     nz: number of entries
     irn: row indices 0-based
     jcn: column indices 0-based
     val values if not NULL
     type: matrix type
  */

  SparseMatrix A = NULL;
  int *ia, *ja;
  real *a, *val;
  int *ai, *vali;
  int i;

  assert(m > 0 && n > 0 && nz >= 0);

  if (m <=0 || n <= 0 || nz < 0) return NULL;
  A = SparseMatrix_general_new(m, n, nz, type, sz, FORMAT_CSR);
  assert(A);
  if (!A) return NULL;
  ia = A->ia;
  ja = A->ja;

  for (i = 0; i <= m; i++){
    ia[i] = 0;
  }

  switch (type){
  case MATRIX_TYPE_REAL:
    val = (real*) val0;
    a = (real*) A->a;
    for (i = 0; i < nz; i++){
      if (irn[i] < 0 || irn[i] >= m || jcn[i] < 0 || jcn[i] >= n) {
	assert(0);
	return NULL;
      }
      ia[irn[i]+1]++;
    }
    for (i = 0; i < m; i++) ia[i+1] += ia[i];
    for (i = 0; i < nz; i++){
      a[ia[irn[i]]] = val[i];
      ja[ia[irn[i]]++] = jcn[i];
    }
    for (i = m; i > 0; i--) ia[i] = ia[i - 1];
    ia[0] = 0;
    break;
  case MATRIX_TYPE_COMPLEX:
    val = (real*) val0;
    a = (real*) A->a;
    for (i = 0; i < nz; i++){
      if (irn[i] < 0 || irn[i] >= m || jcn[i] < 0 || jcn[i] >= n) {
	assert(0);
	return NULL;
      }
      ia[irn[i]+1]++;
    }
    for (i = 0; i < m; i++) ia[i+1] += ia[i];
    for (i = 0; i < nz; i++){
      a[2*ia[irn[i]]] = *(val++);
      a[2*ia[irn[i]]+1] = *(val++);
      ja[ia[irn[i]]++] = jcn[i];
    }
    for (i = m; i > 0; i--) ia[i] = ia[i - 1];
    ia[0] = 0;
    break;
  case MATRIX_TYPE_INTEGER:
    vali = (int*) val0;
    ai = (int*) A->a;
    for (i = 0; i < nz; i++){
      if (irn[i] < 0 || irn[i] >= m || jcn[i] < 0 || jcn[i] >= n) {
	assert(0);
	return NULL;
      }
      ia[irn[i]+1]++;
    }
    for (i = 0; i < m; i++) ia[i+1] += ia[i];
    for (i = 0; i < nz; i++){
      ai[ia[irn[i]]] = vali[i];
      ja[ia[irn[i]]++] = jcn[i];
    }
    for (i = m; i > 0; i--) ia[i] = ia[i - 1];
    ia[0] = 0;
    break;
  case MATRIX_TYPE_PATTERN:
    for (i = 0; i < nz; i++){
      if (irn[i] < 0 || irn[i] >= m || jcn[i] < 0 || jcn[i] >= n) {
	assert(0);
	return NULL;
      }
      ia[irn[i]+1]++;
    }
    for (i = 0; i < m; i++) ia[i+1] += ia[i];
    for (i = 0; i < nz; i++){
      ja[ia[irn[i]]++] = jcn[i];
    }
    for (i = m; i > 0; i--) ia[i] = ia[i - 1];
    ia[0] = 0;
    break;
  case MATRIX_TYPE_UNKNOWN:
    for (i = 0; i < nz; i++){
      if (irn[i] < 0 || irn[i] >= m || jcn[i] < 0 || jcn[i] >= n) {
	assert(0);
	return NULL;
      }
      ia[irn[i]+1]++;
    }
    for (i = 0; i < m; i++) ia[i+1] += ia[i];
    MEMCPY(A->a, val0, A->size*((size_t)nz));
    for (i = 0; i < nz; i++){
      ja[ia[irn[i]]++] = jcn[i];
    }
    for (i = m; i > 0; i--) ia[i] = ia[i - 1];
    ia[0] = 0;
    break;
  default:
    assert(0);
    return NULL;
  }
  A->nz = nz;



  if(sum_repeated) A = SparseMatrix_sum_repeat_entries(A, sum_repeated);
 
  return A;
}


SparseMatrix SparseMatrix_from_coordinate_arrays(int nz, int m, int n, int *irn, int *jcn, void *val0, int type, size_t sz){
  return SparseMatrix_from_coordinate_arrays_internal(nz, m, n, irn, jcn, val0, type, sz, SUM_REPEATED_ALL);
}


SparseMatrix SparseMatrix_from_coordinate_arrays_not_compacted(int nz, int m, int n, int *irn, int *jcn, void *val0, int type, size_t sz, int what_to_sum){
  return SparseMatrix_from_coordinate_arrays_internal(nz, m, n, irn, jcn, val0, type, sz, what_to_sum);
}

SparseMatrix SparseMatrix_add(SparseMatrix A, SparseMatrix B){
  int m, n;
  SparseMatrix C = NULL;
  int *mask = NULL;
  int *ia = A->ia, *ja = A->ja, *ib = B->ia, *jb = B->ja, *ic, *jc;
  int i, j, nz, nzmax;

  assert(A && B);
  assert(A->format == B->format && A->format == FORMAT_CSR);/* other format not yet supported */
  assert(A->type == B->type);
  m = A->m;
  n = A->n;
  if (m != B->m || n != B->n) return NULL;

  nzmax = A->nz + B->nz;/* just assume that no entries overlaps for speed */

  C = SparseMatrix_new(m, n, nzmax, A->type, FORMAT_CSR);
  if (!C) goto RETURN;
  ic = C->ia;
  jc = C->ja;

  mask = MALLOC(sizeof(int)*((size_t) n));

  for (i = 0; i < n; i++) mask[i] = -1;

  nz = 0;
  ic[0] = 0;
  switch (A->type){
  case MATRIX_TYPE_REAL:{
    real *a = (real*) A->a;
    real *b = (real*) B->a;
    real *c = (real*) C->a;
    for (i = 0; i < m; i++){
      for (j = ia[i]; j < ia[i+1]; j++){
	mask[ja[j]] = nz;
	jc[nz] = ja[j];
	c[nz] = a[j];
	nz++;
      }
      for (j = ib[i]; j < ib[i+1]; j++){
	if (mask[jb[j]] < ic[i]){
	  jc[nz] = jb[j];
	  c[nz++] = b[j];
	} else {
	  c[mask[jb[j]]] += b[j];
	}
      }
      ic[i+1] = nz;
    }
    break;
  }
  case MATRIX_TYPE_COMPLEX:{
    real *a = (real*) A->a;
    real *b = (real*) B->a;
    real *c = (real*) C->a;
    for (i = 0; i < m; i++){
      for (j = ia[i]; j < ia[i+1]; j++){
	mask[ja[j]] = nz;
	jc[nz] = ja[j];
	c[2*nz] = a[2*j];
	c[2*nz+1] = a[2*j+1];
	nz++;
      }
      for (j = ib[i]; j < ib[i+1]; j++){
	if (mask[jb[j]] < ic[i]){
	  jc[nz] = jb[j];
	  c[2*nz] = b[2*j];
	  c[2*nz+1] = b[2*j+1];
	  nz++;
	} else {
	  c[2*mask[jb[j]]] += b[2*j];
	  c[2*mask[jb[j]]+1] += b[2*j+1];
	}
      }
      ic[i+1] = nz;
    }
    break;
  }
  case MATRIX_TYPE_INTEGER:{
    int *a = (int*) A->a;
    int *b = (int*) B->a;
    int *c = (int*) C->a;
    for (i = 0; i < m; i++){
      for (j = ia[i]; j < ia[i+1]; j++){
	mask[ja[j]] = nz;
	jc[nz] = ja[j];
	c[nz] = a[j];
	nz++;
      }
      for (j = ib[i]; j < ib[i+1]; j++){
	if (mask[jb[j]] < ic[i]){
	  jc[nz] = jb[j];
	  c[nz] = b[j];
	  nz++;
	} else {
	  c[mask[jb[j]]] += b[j];
	}
      }
      ic[i+1] = nz;
    }
    break;
  }
  case MATRIX_TYPE_PATTERN:{
    for (i = 0; i < m; i++){
      for (j = ia[i]; j < ia[i+1]; j++){
	mask[ja[j]] = nz;
	jc[nz] = ja[j];
	nz++;
      }
      for (j = ib[i]; j < ib[i+1]; j++){
	if (mask[jb[j]] < ic[i]){
	  jc[nz] = jb[j];
	  nz++;
	} 
      }
      ic[i+1] = nz;
    }
    break;
  }
  case MATRIX_TYPE_UNKNOWN:
    break;
  default:
    break;
  }
  C->nz = nz;

 RETURN:
  if (mask) FREE(mask);

  return C;
}



static void dense_transpose(real *v, int m, int n){
  /* transpose an m X n matrix in place. Well, we do no really do it without xtra memory. This is possibe, but too complicated for ow */
  int i, j;
  real *u;
  u = MALLOC(sizeof(real)*((size_t) m)*((size_t) n));
  MEMCPY(u,v, sizeof(real)*((size_t) m)*((size_t) n));

  for (i = 0; i < m; i++){
    for (j = 0; j < n; j++){
      v[j*m+i] = u[i*n+j];
    }
  }
  FREE(u);
}


static void SparseMatrix_multiply_dense1(SparseMatrix A, real *v, real **res, int dim, int transposed, int res_transposed){
  /* A v or A^T v where v a dense matrix of second dimension dim. Real only for now. */
  int i, j, k, *ia, *ja, n, m;
  real *a, *u;

  assert(A->format == FORMAT_CSR);
  assert(A->type == MATRIX_TYPE_REAL);

  a = (real*) A->a;
  ia = A->ia;
  ja = A->ja;
  m = A->m;
  n = A->n;
  u = *res;

  if (!transposed){
    if (!u) u = MALLOC(sizeof(real)*((size_t) m)*((size_t) dim));
    for (i = 0; i < m; i++){
      for (k = 0; k < dim; k++) u[i*dim+k] = 0.;
      for (j = ia[i]; j < ia[i+1]; j++){
	for (k = 0; k < dim; k++) u[i*dim+k] += a[j]*v[ja[j]*dim+k];
      }
    }
    if (res_transposed) dense_transpose(u, m, dim);
  } else {
    if (!u) u = MALLOC(sizeof(real)*((size_t) n)*((size_t) dim));
    for (i = 0; i < n*dim; i++) u[i] = 0.;
    for (i = 0; i < m; i++){
      for (j = ia[i]; j < ia[i+1]; j++){
	for (k = 0; k < dim; k++) u[ja[j]*dim + k] += a[j]*v[i*dim + k];
      }
    }
    if (res_transposed) dense_transpose(u, n, dim);
  }

  *res = u;


}

static void SparseMatrix_multiply_dense2(SparseMatrix A, real *v, real **res, int dim, int transposed, int res_transposed){
  /* A v^T or A^T v^T where v a dense matrix of second dimension n or m. Real only for now.
     transposed = FALSE: A*V^T, with A dimension m x n, V dimension dim x n, v[i*n+j] gives V[i,j]. Result of dimension m x dim
     transposed = TRUE: A^T*V^T, with A dimension m x n, V dimension dim x m. v[i*m+j] gives V[i,j]. Result of dimension n x dim
  */
  real *u, *rr;
  int i, m, n;
  assert(A->format == FORMAT_CSR);
  assert(A->type == MATRIX_TYPE_REAL);
  u = *res;
  m = A->m;
  n = A->n;

  if (!transposed){
    if (!u) u = MALLOC(sizeof(real)*((size_t)m)*((size_t) dim));
    for (i = 0; i < dim; i++){
      rr = &(u[m*i]);
      SparseMatrix_multiply_vector(A, &(v[n*i]), &rr, transposed);
    }
    if (!res_transposed) dense_transpose(u, dim, m);
  } else {
    if (!u) u = MALLOC(sizeof(real)*((size_t)n)*((size_t)dim));
    for (i = 0; i < dim; i++){
      rr = &(u[n*i]);
      SparseMatrix_multiply_vector(A, &(v[m*i]), &rr, transposed);
    }
    if (!res_transposed) dense_transpose(u, dim, n);
  }

  *res = u;


}



void SparseMatrix_multiply_dense(SparseMatrix A, int ATransposed, real *v, int vTransposed, real **res, int res_transposed, int dim){
  /* depend on value of {ATranspose, vTransposed}, assume res_transposed == FALSE
     {FALSE, FALSE}: A * V, with A dimension m x n, with V of dimension n x dim. v[i*dim+j] gives V[i,j]. Result of dimension m x dim
     {TRUE, FALSE}: A^T * V, with A dimension m x n, with V of dimension m x dim. v[i*dim+j] gives V[i,j]. Result of dimension n x dim
     {FALSE, TRUE}: A*V^T, with A dimension m x n, V dimension dim x n, v[i*n+j] gives V[i,j]. Result of dimension m x dim
     {TRUE, TRUE}: A^T*V^T, with A dimension m x n, V dimension dim x m. v[i*m+j] gives V[i,j]. Result of dimension n x dim
 
     furthermore, if res_transpose d== TRUE, then the result is transposed. Hence if res_transposed == TRUE

     {FALSE, FALSE}: V^T A^T, with A dimension m x n, with V of dimension n x dim. v[i*dim+j] gives V[i,j]. Result of dimension dim x dim
     {TRUE, FALSE}: V^T A, with A dimension m x n, with V of dimension m x dim. v[i*dim+j] gives V[i,j]. Result of dimension dim x n
     {FALSE, TRUE}: V*A^T, with A dimension m x n, V dimension dim x n, v[i*n+j] gives V[i,j]. Result of dimension dim x m
     {TRUE, TRUE}: V A, with A dimension m x n, V dimension dim x m. v[i*m+j] gives V[i,j]. Result of dimension dim x n
 */

  if (!vTransposed) {
    SparseMatrix_multiply_dense1(A, v, res, dim, ATransposed, res_transposed);
  } else {
    SparseMatrix_multiply_dense2(A, v, res, dim, ATransposed, res_transposed);
  }

}



void SparseMatrix_multiply_vector(SparseMatrix A, real *v, real **res, int transposed){
  /* A v or A^T v. Real only for now. */
  int i, j, *ia, *ja, n, m;
  real *a, *u = NULL;
  int *ai;
  assert(A->format == FORMAT_CSR);
  assert(A->type == MATRIX_TYPE_REAL || A->type == MATRIX_TYPE_INTEGER);

  ia = A->ia;
  ja = A->ja;
  m = A->m;
  n = A->n;
  u = *res;

  switch (A->type){
  case MATRIX_TYPE_REAL:
    a = (real*) A->a;
    if (v){
      if (!transposed){
	if (!u) u = MALLOC(sizeof(real)*((size_t)m));
	for (i = 0; i < m; i++){
	  u[i] = 0.;
	  for (j = ia[i]; j < ia[i+1]; j++){
	    u[i] += a[j]*v[ja[j]];
	  }
	}
      } else {
	if (!u) u = MALLOC(sizeof(real)*((size_t)n));
	for (i = 0; i < n; i++) u[i] = 0.;
	for (i = 0; i < m; i++){
	  for (j = ia[i]; j < ia[i+1]; j++){
	    u[ja[j]] += a[j]*v[i];
	  }
	}
      }
    } else {
      /* v is assumed to be all 1's */
      if (!transposed){
	if (!u) u = MALLOC(sizeof(real)*((size_t)m));
	for (i = 0; i < m; i++){
	  u[i] = 0.;
	  for (j = ia[i]; j < ia[i+1]; j++){
	    u[i] += a[j];
	  }
	}
      } else {
	if (!u) u = MALLOC(sizeof(real)*((size_t)n));
	for (i = 0; i < n; i++) u[i] = 0.;
	for (i = 0; i < m; i++){
	  for (j = ia[i]; j < ia[i+1]; j++){
	    u[ja[j]] += a[j];
	  }
	}
      }
    }
    break;
  case MATRIX_TYPE_INTEGER:
    ai = (int*) A->a;
    if (v){
      if (!transposed){
	if (!u) u = MALLOC(sizeof(real)*((size_t)m));
	for (i = 0; i < m; i++){
	  u[i] = 0.;
	  for (j = ia[i]; j < ia[i+1]; j++){
	    u[i] += ai[j]*v[ja[j]];
	  }
	}
      } else {
	if (!u) u = MALLOC(sizeof(real)*((size_t)n));
	for (i = 0; i < n; i++) u[i] = 0.;
	for (i = 0; i < m; i++){
	  for (j = ia[i]; j < ia[i+1]; j++){
	    u[ja[j]] += ai[j]*v[i];
	  }
	}
      }
    } else {
      /* v is assumed to be all 1's */
      if (!transposed){
	if (!u) u = MALLOC(sizeof(real)*((size_t)m));
	for (i = 0; i < m; i++){
	  u[i] = 0.;
	  for (j = ia[i]; j < ia[i+1]; j++){
	    u[i] += ai[j];
	  }
	}
      } else {
	if (!u) u = MALLOC(sizeof(real)*((size_t)n));
	for (i = 0; i < n; i++) u[i] = 0.;
	for (i = 0; i < m; i++){
	  for (j = ia[i]; j < ia[i+1]; j++){
	    u[ja[j]] += ai[j];
	  }
	}
      }
    }
    break;
  default:
    assert(0);
    u = NULL;
  }
  *res = u;

}



SparseMatrix SparseMatrix_scaled_by_vector(SparseMatrix A, real *v, int apply_to_row){
  /* A SCALED BY VECOTR V IN ROW/COLUMN. Real only for now. */
  int i, j, *ia, *ja, m;
  real *a;
  assert(A->format == FORMAT_CSR);
  assert(A->type == MATRIX_TYPE_REAL);

  a = (real*) A->a;
  ia = A->ia;
  ja = A->ja;
  m = A->m;


  if (!apply_to_row){
    for (i = 0; i < m; i++){
      for (j = ia[i]; j < ia[i+1]; j++){
	a[j] *= v[ja[j]];
      }
    }
  } else {
    for (i = 0; i < m; i++){
      if (v[i] != 0){
	for (j = ia[i]; j < ia[i+1]; j++){
	  a[j] *= v[i];
	}
      }
    }
  }
  return A;

}
SparseMatrix SparseMatrix_multiply_by_scaler(SparseMatrix A, real s){
  /* A scaled by a number */
  int i, j, *ia, m;
  real *a;
  assert(A->format == FORMAT_CSR);
  assert(A->type == MATRIX_TYPE_REAL);

  a = (real*) A->a;
  ia = A->ia;

  m = A->m;





  for (i = 0; i < m; i++){
    for (j = ia[i]; j < ia[i+1]; j++){
      a[j] *= s;
    }
  }

  return A;

}


SparseMatrix SparseMatrix_multiply(SparseMatrix A, SparseMatrix B){
  int m;
  SparseMatrix C = NULL;
  int *mask = NULL;
  int *ia = A->ia, *ja = A->ja, *ib = B->ia, *jb = B->ja, *ic, *jc;
  int i, j, k, jj, type, nz;

  assert(A->format == B->format && A->format == FORMAT_CSR);/* other format not yet supported */

  m = A->m;
  if (A->n != B->m) return NULL;
  if (A->type != B->type){
#ifdef DEBUG
    printf("in SparseMatrix_multiply, the matrix types do not match, right now only multiplication of matrices of the same type is supported\n");
#endif
    return NULL;
  }
  type = A->type;
  
  mask = MALLOC(sizeof(int)*((size_t)(B->n)));
  if (!mask) return NULL;

  for (i = 0; i < B->n; i++) mask[i] = -1;

  nz = 0;
  for (i = 0; i < m; i++){
    for (j = ia[i]; j < ia[i+1]; j++){
      jj = ja[j];
      for (k = ib[jj]; k < ib[jj+1]; k++){
	if (mask[jb[k]] != -i - 2){
	  if ((nz+1) <= nz) {
#ifdef DEBUG_PRINT
	    fprintf(stderr,"overflow in SparseMatrix_multiply !!!\n");
#endif
	    return NULL;
	  }
	  nz++;
	  mask[jb[k]] = -i - 2;
	}
      }
    }
  }

  C = SparseMatrix_new(m, B->n, nz, type, FORMAT_CSR);
  if (!C) goto RETURN;
  ic = C->ia;
  jc = C->ja;
  
  nz = 0;

  switch (type){
  case MATRIX_TYPE_REAL:
    {
      real *a = (real*) A->a;
      real *b = (real*) B->a;
      real *c = (real*) C->a;
      ic[0] = 0;
      for (i = 0; i < m; i++){
	for (j = ia[i]; j < ia[i+1]; j++){
	  jj = ja[j];
	  for (k = ib[jj]; k < ib[jj+1]; k++){
	    if (mask[jb[k]] < ic[i]){
	      mask[jb[k]] = nz;
	      jc[nz] = jb[k];
	      c[nz] = a[j]*b[k];
	      nz++;
	    } else {
	      assert(jc[mask[jb[k]]] == jb[k]);
	      c[mask[jb[k]]] += a[j]*b[k];
	    }
	  }
	}
	ic[i+1] = nz;
      }
    }
    break;
  case MATRIX_TYPE_COMPLEX:
    {
      real *a = (real*) A->a;
      real *b = (real*) B->a;
      real *c = (real*) C->a;
      a = (real*) A->a;
      b = (real*) B->a;
      c = (real*) C->a;
      ic[0] = 0;
      for (i = 0; i < m; i++){
	for (j = ia[i]; j < ia[i+1]; j++){
	  jj = ja[j];
	  for (k = ib[jj]; k < ib[jj+1]; k++){
	    if (mask[jb[k]] < ic[i]){
	      mask[jb[k]] = nz;
	      jc[nz] = jb[k];
	      c[2*nz] = a[2*j]*b[2*k] - a[2*j+1]*b[2*k+1];/*real part */
	      c[2*nz+1] = a[2*j]*b[2*k+1] + a[2*j+1]*b[2*k];/*img part */
	      nz++;
	    } else {
	      assert(jc[mask[jb[k]]] == jb[k]);
	      c[2*mask[jb[k]]] += a[2*j]*b[2*k] - a[2*j+1]*b[2*k+1];/*real part */
	      c[2*mask[jb[k]]+1] += a[2*j]*b[2*k+1] + a[2*j+1]*b[2*k];/*img part */
	    }
	  }
	}
	ic[i+1] = nz;
      }
    }
    break;
  case MATRIX_TYPE_INTEGER:
    {
      int *a = (int*) A->a;
      int *b = (int*) B->a;
      int *c = (int*) C->a;
      ic[0] = 0;
      for (i = 0; i < m; i++){
	for (j = ia[i]; j < ia[i+1]; j++){
	  jj = ja[j];
	  for (k = ib[jj]; k < ib[jj+1]; k++){
	    if (mask[jb[k]] < ic[i]){
	      mask[jb[k]] = nz;
	      jc[nz] = jb[k];
	      c[nz] = a[j]*b[k];
	      nz++;
	    } else {
	      assert(jc[mask[jb[k]]] == jb[k]);
	      c[mask[jb[k]]] += a[j]*b[k];
	    }
	  }
	}
	ic[i+1] = nz;
      }
    }
    break;
  case MATRIX_TYPE_PATTERN:
    ic[0] = 0;
    for (i = 0; i < m; i++){
      for (j = ia[i]; j < ia[i+1]; j++){
	jj = ja[j];
	for (k = ib[jj]; k < ib[jj+1]; k++){
	  if (mask[jb[k]] < ic[i]){
	    mask[jb[k]] = nz;
	    jc[nz] = jb[k];
	    nz++;
	  } else {
	    assert(jc[mask[jb[k]]] == jb[k]);
	  }
	}
      }
      ic[i+1] = nz;
    }
    break;
  case MATRIX_TYPE_UNKNOWN:
  default:
    SparseMatrix_delete(C);
    C = NULL; goto RETURN;
    break;
  }
  
  C->nz = nz;

 RETURN:
  FREE(mask);
  return C;

}



SparseMatrix SparseMatrix_multiply3(SparseMatrix A, SparseMatrix B, SparseMatrix C){
  int m;
  SparseMatrix D = NULL;
  int *mask = NULL;
  int *ia = A->ia, *ja = A->ja, *ib = B->ia, *jb = B->ja, *ic = C->ia, *jc = C->ja, *id, *jd;
  int i, j, k, l, ll, jj, type, nz;

  assert(A->format == B->format && A->format == FORMAT_CSR);/* other format not yet supported */

  m = A->m;
  if (A->n != B->m) return NULL;
  if (B->n != C->m) return NULL;

  if (A->type != B->type || B->type != C->type){
#ifdef DEBUG
    printf("in SparseMatrix_multiply, the matrix types do not match, right now only multiplication of matrices of the same type is supported\n");
#endif
    return NULL;
  }
  type = A->type;
  
  mask = MALLOC(sizeof(int)*((size_t)(C->n)));
  if (!mask) return NULL;

  for (i = 0; i < C->n; i++) mask[i] = -1;

  nz = 0;
  for (i = 0; i < m; i++){
    for (j = ia[i]; j < ia[i+1]; j++){
      jj = ja[j];
      for (l = ib[jj]; l < ib[jj+1]; l++){
	ll = jb[l];
	for (k = ic[ll]; k < ic[ll+1]; k++){
	  if (mask[jc[k]] != -i - 2){
	    if ((nz+1) <= nz) {
#ifdef DEBUG_PRINT
	      fprintf(stderr,"overflow in SparseMatrix_multiply !!!\n");
#endif
	      return NULL;
	    }
	    nz++;
	    mask[jc[k]] = -i - 2;
	  }
	}
      }
    }
  }

  D = SparseMatrix_new(m, C->n, nz, type, FORMAT_CSR);
  if (!D) goto RETURN;
  id = D->ia;
  jd = D->ja;
  
  nz = 0;

  switch (type){
  case MATRIX_TYPE_REAL:
    {
      real *a = (real*) A->a;
      real *b = (real*) B->a;
      real *c = (real*) C->a;
      real *d = (real*) D->a;
      id[0] = 0;
      for (i = 0; i < m; i++){
	for (j = ia[i]; j < ia[i+1]; j++){
	  jj = ja[j];
	  for (l = ib[jj]; l < ib[jj+1]; l++){
	    ll = jb[l];
	    for (k = ic[ll]; k < ic[ll+1]; k++){
	      if (mask[jc[k]] < id[i]){
		mask[jc[k]] = nz;
		jd[nz] = jc[k];
		d[nz] = a[j]*b[l]*c[k];
		nz++;
	      } else {
		assert(jd[mask[jc[k]]] == jc[k]);
		d[mask[jc[k]]] += a[j]*b[l]*c[k];
	      }
	    }
	  }
	}
	id[i+1] = nz;
      }
    }
    break;
  case MATRIX_TYPE_COMPLEX:
    {
      real *a = (real*) A->a;
      real *b = (real*) B->a;
      real *c = (real*) C->a;
      real *d = (real*) D->a;
      id[0] = 0;
      for (i = 0; i < m; i++){
	for (j = ia[i]; j < ia[i+1]; j++){
	  jj = ja[j];
	  for (l = ib[jj]; l < ib[jj+1]; l++){
	    ll = jb[l];
	    for (k = ic[ll]; k < ic[ll+1]; k++){
	      if (mask[jc[k]] < id[i]){
		mask[jc[k]] = nz;
		jd[nz] = jc[k];
		d[2*nz] = (a[2*j]*b[2*l] - a[2*j+1]*b[2*l+1])*c[2*k] 
		  - (a[2*j]*b[2*l+1] + a[2*j+1]*b[2*l])*c[2*k+1];/*real part */
		d[2*nz+1] = (a[2*j]*b[2*l+1] + a[2*j+1]*b[2*l])*c[2*k]
		  + (a[2*j]*b[2*l] - a[2*j+1]*b[2*l+1])*c[2*k+1];/*img part */
		nz++;
	      } else {
		assert(jd[mask[jc[k]]] == jc[k]);
		d[2*mask[jc[k]]] += (a[2*j]*b[2*l] - a[2*j+1]*b[2*l+1])*c[2*k] 
		  - (a[2*j]*b[2*l+1] + a[2*j+1]*b[2*l])*c[2*k+1];/*real part */
		d[2*mask[jc[k]]+1] += (a[2*j]*b[2*l+1] + a[2*j+1]*b[2*l])*c[2*k]
		  + (a[2*j]*b[2*l] - a[2*j+1]*b[2*l+1])*c[2*k+1];/*img part */
	      }
	    }
	  }
	}
	id[i+1] = nz;
      }
    }
    break;
  case MATRIX_TYPE_INTEGER:
    {
      int *a = (int*) A->a;
      int *b = (int*) B->a;
      int *c = (int*) C->a;
      int *d = (int*) D->a;
      id[0] = 0;
      for (i = 0; i < m; i++){
	for (j = ia[i]; j < ia[i+1]; j++){
	  jj = ja[j];
	  for (l = ib[jj]; l < ib[jj+1]; l++){
	    ll = jb[l];
	    for (k = ic[ll]; k < ic[ll+1]; k++){
	      if (mask[jc[k]] < id[i]){
		mask[jc[k]] = nz;
		jd[nz] = jc[k];
		d[nz] += a[j]*b[l]*c[k];
		nz++;
	      } else {
		assert(jd[mask[jc[k]]] == jc[k]);
		d[mask[jc[k]]] += a[j]*b[l]*c[k];
	      }
	    }
	  }
	}
	id[i+1] = nz;
      }
    }
    break;
  case MATRIX_TYPE_PATTERN:
    id[0] = 0;
    for (i = 0; i < m; i++){
      for (j = ia[i]; j < ia[i+1]; j++){
	jj = ja[j];
	for (l = ib[jj]; l < ib[jj+1]; l++){
	  ll = jb[l];
	  for (k = ic[ll]; k < ic[ll+1]; k++){
	    if (mask[jc[k]] < id[i]){
	      mask[jc[k]] = nz;
	      jd[nz] = jc[k];
	      nz++;
	    } else {
	      assert(jd[mask[jc[k]]] == jc[k]);
	    }
	  }
	}
      }
      id[i+1] = nz;
    }
    break;
  case MATRIX_TYPE_UNKNOWN:
  default:
    SparseMatrix_delete(D);
    D = NULL; goto RETURN;
    break;
  }
  
  D->nz = nz;

 RETURN:
  FREE(mask);
  return D;

}

/* For complex matrix:
   if what_to_sum = SUM_REPEATED_REAL_PART, we find entries {i,j,x + i y} and sum the x's if {i,j,Round(y)} are the same
   if what_to_sum = SUM_REPEATED_REAL_PART, we find entries {i,j,x + i y} and sum the y's if {i,j,Round(x)} are the same
   so a matrix like {{1,1,2+3i},{1,2,3+4i},{1,1,5+3i},{1,2,4+5i},{1,2,4+4i}} becomes
   {{1,1,2+5+3i},{1,2,3+4+4i},{1,2,4+5i}}. 

   Really this kind of thing is best handled using a 3D sparse matrix like
   {{{1,1,3},2},{{1,2,4},3},{{1,1,3},5},{{1,2,4},4}}, 
   which is then aggreted into
   {{{1,1,3},2+5},{{1,2,4},3+4},{{1,1,3},5}}
   but unfortunately I do not have such object implemented yet.
   

   For other matrix, what_to_sum = SUM_REPEATED_REAL_PART is the same as what_to_sum = SUM_REPEATED_IMAGINARY_PART
   or what_to_sum = SUM_REPEATED_ALL. In this implementation we assume that
   the {j,y} pairs are dense, so we usea 2D array for hashing 
*/
SparseMatrix SparseMatrix_sum_repeat_entries(SparseMatrix A, int what_to_sum){
  /* sum repeated entries in the same row, i.e., {1,1}->1, {1,1}->2 becomes {1,1}->3 */
  int *ia = A->ia, *ja = A->ja, type = A->type, n = A->n;
  int *mask = NULL, nz = 0, i, j, sta;

  if (what_to_sum == SUM_REPEATED_NONE) return A;

  mask = MALLOC(sizeof(int)*((size_t)n));
  for (i = 0; i < n; i++) mask[i] = -1;

  switch (type){
  case MATRIX_TYPE_REAL:
    {
      real *a = (real*) A->a;
      nz = 0;
      sta = ia[0];
      for (i = 0; i < A->m; i++){
	for (j = sta; j < ia[i+1]; j++){
	  if (mask[ja[j]] < ia[i]){
	    ja[nz] = ja[j];
	    a[nz] = a[j];
	    mask[ja[j]] = nz++;
	  } else {
	    assert(ja[mask[ja[j]]] == ja[j]);
	    a[mask[ja[j]]] += a[j];
	  }
	}
	sta = ia[i+1];
	ia[i+1] = nz;
      }
    }
    break;
  case MATRIX_TYPE_COMPLEX:
    {
      real *a = (real*) A->a;
      if (what_to_sum == SUM_REPEATED_ALL){
	nz = 0;
	sta = ia[0];
	for (i = 0; i < A->m; i++){
	  for (j = sta; j < ia[i+1]; j++){
	    if (mask[ja[j]] < ia[i]){
	      ja[nz] = ja[j];
	      a[2*nz] = a[2*j];
	      a[2*nz+1] = a[2*j+1];
	      mask[ja[j]] = nz++;
	    } else {
	      assert(ja[mask[ja[j]]] == ja[j]);
	      a[2*mask[ja[j]]] += a[2*j];
	      a[2*mask[ja[j]]+1] += a[2*j+1];
	    }
	  }
	  sta = ia[i+1];
	  ia[i+1] = nz;
	}
      } else if (what_to_sum == SUM_REPEATED_REAL_PART){
	int ymin, ymax, id;
	ymax = ymin = a[1];
	nz = 0;
	for (i = 0; i < A->m; i++){
	  for (j = ia[i]; j < ia[i+1]; j++){
	    ymax = MAX(ymax, (int) a[2*nz+1]);
	    ymin = MIN(ymin, (int) a[2*nz+1]);
	    nz++;
	  }	  
	}
	FREE(mask);
	mask = MALLOC(sizeof(int)*((size_t)n)*((size_t)(ymax-ymin+1)));
	for (i = 0; i < n*(ymax-ymin+1); i++) mask[i] = -1;

	nz = 0;
	sta = ia[0];
	for (i = 0; i < A->m; i++){
	  for (j = sta; j < ia[i+1]; j++){
	    id = ja[j] + ((int)a[2*j+1] - ymin)*n;
	    if (mask[id] < ia[i]){
	      ja[nz] = ja[j];
	      a[2*nz] = a[2*j];
	      a[2*nz+1] = a[2*j+1];
	      mask[id] = nz++;
	    } else {
	      assert(id < n*(ymax-ymin+1));
	      assert(ja[mask[id]] == ja[j]);
	      a[2*mask[id]] += a[2*j];
	      a[2*mask[id]+1] = a[2*j+1];
	    }
	  }
	  sta = ia[i+1];
	  ia[i+1] = nz;
	}
	
      } else if (what_to_sum == SUM_REPEATED_IMAGINARY_PART){
	int xmin, xmax, id;
	xmax = xmin = a[1];
	nz = 0;
	for (i = 0; i < A->m; i++){
	  for (j = ia[i]; j < ia[i+1]; j++){
	    xmax = MAX(xmax, (int) a[2*nz]);
	    xmin = MAX(xmin, (int) a[2*nz]);
	    nz++;
	  }	  
	}
	FREE(mask);
	mask = MALLOC(sizeof(int)*((size_t)n)*((size_t)(xmax-xmin+1)));
	for (i = 0; i < n*(xmax-xmin+1); i++) mask[i] = -1;

	nz = 0;
	sta = ia[0];
	for (i = 0; i < A->m; i++){
	  for (j = sta; j < ia[i+1]; j++){
	    id = ja[j] + ((int)a[2*j] - xmin)*n;
	    if (mask[id] < ia[i]){
	      ja[nz] = ja[j];
	      a[2*nz] = a[2*j];
	      a[2*nz+1] = a[2*j+1];
	      mask[id] = nz++;
	    } else {
	      assert(ja[mask[id]] == ja[j]);
	      a[2*mask[id]] = a[2*j];
	      a[2*mask[id]+1] += a[2*j+1];
	    }
	  }
	  sta = ia[i+1];
	  ia[i+1] = nz;
	}

      }
    }
    break;
  case MATRIX_TYPE_INTEGER:
    {
      int *a = (int*) A->a;
      nz = 0;
      sta = ia[0];
      for (i = 0; i < A->m; i++){
	for (j = sta; j < ia[i+1]; j++){
	  if (mask[ja[j]] < ia[i]){
	    ja[nz] = ja[j];
	    a[nz] = a[j];
	    mask[ja[j]] = nz++;
	  } else {
	    assert(ja[mask[ja[j]]] == ja[j]);
	    a[mask[ja[j]]] += a[j];
	  }
	}
	sta = ia[i+1];
	ia[i+1] = nz;	
      }
    }
    break;
  case MATRIX_TYPE_PATTERN:
    {
      nz = 0;
      sta = ia[0];
      for (i = 0; i < A->m; i++){
	for (j = sta; j < ia[i+1]; j++){
	  if (mask[ja[j]] < ia[i]){
	    ja[nz] = ja[j];
	    mask[ja[j]] = nz++;
	  } else {
	    assert(ja[mask[ja[j]]] == ja[j]);
	  }
	}
	sta = ia[i+1];
	ia[i+1] = nz;
      }
    }
    break;
  case MATRIX_TYPE_UNKNOWN:
    return NULL;
    break;
  default:
    return NULL;
    break;
  }
  A->nz = nz;
  FREE(mask);
  return A;
}

SparseMatrix SparseMatrix_coordinate_form_add_entries(SparseMatrix A, int nentries, int *irn, int *jcn, void *val){
  int nz, nzmax, i;
  
  assert(A->format == FORMAT_COORD);
  if (nentries <= 0) return A;
  nz = A->nz;
  nzmax = A->nzmax;

  if (nz + nentries >= A->nzmax){
    nzmax = nz + nentries;
     nzmax = MAX(10, (int) 0.2*nzmax) + nzmax;
    A = SparseMatrix_realloc(A, nzmax);
  }
  MEMCPY((char*) A->ia + ((size_t)nz)*sizeof(int)/sizeof(char), irn, sizeof(int)*((size_t)nentries));
  MEMCPY((char*) A->ja + ((size_t)nz)*sizeof(int)/sizeof(char), jcn, sizeof(int)*((size_t)nentries));
  if (A->size) MEMCPY((char*) A->a + ((size_t)nz)*A->size/sizeof(char), val, A->size*((size_t)nentries));
  for (i = 0; i < nentries; i++) {
    if (irn[i] >= A->m) A->m = irn[i]+1;
    if (jcn[i] >= A->n) A->n = jcn[i]+1;
  }
  A->nz += nentries;
  return A;
}


SparseMatrix SparseMatrix_remove_diagonal(SparseMatrix A){
  int i, j, *ia, *ja, nz, sta;

  if (!A) return A;

  nz = 0;
  ia = A->ia;
  ja = A->ja;
  sta = ia[0];
  switch (A->type){
  case MATRIX_TYPE_REAL:{
    real *a = (real*) A->a;
    for (i = 0; i < A->m; i++){
      for (j = sta; j < ia[i+1]; j++){
	if (ja[j] != i){
	  ja[nz] = ja[j];
	  a[nz++] = a[j];
	}
      }
      sta = ia[i+1];
      ia[i+1] = nz;
    }
    A->nz = nz;
    break;
  }
  case MATRIX_TYPE_COMPLEX:{
    real *a = (real*) A->a;
    for (i = 0; i < A->m; i++){
      for (j = sta; j < ia[i+1]; j++){
	if (ja[j] != i){
	  ja[nz] = ja[j];
	  a[2*nz] = a[2*j];
	  a[2*nz+1] = a[2*j+1];
	  nz++;
	}
      }
      sta = ia[i+1];
      ia[i+1] = nz;
    }
    A->nz = nz;
    break;
  }
  case MATRIX_TYPE_INTEGER:{
    int *a = (int*) A->a;
    for (i = 0; i < A->m; i++){
      for (j = sta; j < ia[i+1]; j++){
	if (ja[j] != i){
	  ja[nz] = ja[j];
	  a[nz++] = a[j];
	}
      }
      sta = ia[i+1];
      ia[i+1] = nz;
    }
    A->nz = nz;
    break;
  }
  case MATRIX_TYPE_PATTERN:{
    for (i = 0; i < A->m; i++){
      for (j = sta; j < ia[i+1]; j++){
	if (ja[j] != i){
	  ja[nz++] = ja[j];
	}
      }
      sta = ia[i+1];
      ia[i+1] = nz;
    }
    A->nz = nz;
    break;
  }
  case MATRIX_TYPE_UNKNOWN:
    return NULL;
  default:
    return NULL;
  }

  return A;
}


SparseMatrix SparseMatrix_remove_upper(SparseMatrix A){/* remove diag and upper diag */
  int i, j, *ia, *ja, nz, sta;

  if (!A) return A;

  nz = 0;
  ia = A->ia;
  ja = A->ja;
  sta = ia[0];
  switch (A->type){
  case MATRIX_TYPE_REAL:{
    real *a = (real*) A->a;
    for (i = 0; i < A->m; i++){
      for (j = sta; j < ia[i+1]; j++){
	if (ja[j] < i){
	  ja[nz] = ja[j];
	  a[nz++] = a[j];
	}
      }
      sta = ia[i+1];
      ia[i+1] = nz;
    }
    A->nz = nz;
    break;
  }
  case MATRIX_TYPE_COMPLEX:{
    real *a = (real*) A->a;
    for (i = 0; i < A->m; i++){
      for (j = sta; j < ia[i+1]; j++){
	if (ja[j] < i){
	  ja[nz] = ja[j];
	  a[2*nz] = a[2*j];
	  a[2*nz+1] = a[2*j+1];
	  nz++;
	}
      }
      sta = ia[i+1];
      ia[i+1] = nz;
    }
    A->nz = nz;
    break;
  }
  case MATRIX_TYPE_INTEGER:{
    int *a = (int*) A->a;
    for (i = 0; i < A->m; i++){
      for (j = sta; j < ia[i+1]; j++){
	if (ja[j] < i){
	  ja[nz] = ja[j];
	  a[nz++] = a[j];
	}
      }
      sta = ia[i+1];
      ia[i+1] = nz;
    }
    A->nz = nz;
    break;
  }
  case MATRIX_TYPE_PATTERN:{
    for (i = 0; i < A->m; i++){
      for (j = sta; j < ia[i+1]; j++){
	if (ja[j] < i){
	  ja[nz++] = ja[j];
	}
      }
      sta = ia[i+1];
      ia[i+1] = nz;
    }
    A->nz = nz;
    break;
  }
  case MATRIX_TYPE_UNKNOWN:
    return NULL;
  default:
    return NULL;
  }

  clear_flag(A->property, MATRIX_PATTERN_SYMMETRIC);
  clear_flag(A->property, MATRIX_SYMMETRIC);
  clear_flag(A->property, MATRIX_SKEW);
  clear_flag(A->property, MATRIX_HERMITIAN);
  return A;
}




SparseMatrix SparseMatrix_divide_row_by_degree(SparseMatrix A){
  int i, j, *ia, *ja;
  real deg;

  if (!A) return A;

  ia = A->ia;
  ja = A->ja;
  switch (A->type){
  case MATRIX_TYPE_REAL:{
    real *a = (real*) A->a;
    for (i = 0; i < A->m; i++){
      deg = ia[i+1] - ia[i];
      for (j = ia[i]; j < ia[i+1]; j++){
	a[j] = a[j]/deg;
      }
    }
    break;
  }
  case MATRIX_TYPE_COMPLEX:{
    real *a = (real*) A->a;
    for (i = 0; i < A->m; i++){
      deg = ia[i+1] - ia[i];
      for (j = ia[i]; j < ia[i+1]; j++){
	if (ja[j] != i){
	  a[2*j] = a[2*j]/deg;
	  a[2*j+1] = a[2*j+1]/deg;
	}
      }
    }
    break;
  }
  case MATRIX_TYPE_INTEGER:{
    assert(0);/* this operation would not make sense for int matrix */
    break;
  }
  case MATRIX_TYPE_PATTERN:{
    break;
  }
  case MATRIX_TYPE_UNKNOWN:
    return NULL;
  default:
    return NULL;
  }

  return A;
}


SparseMatrix SparseMatrix_get_real_adjacency_matrix_symmetrized(SparseMatrix A){
  /* symmetric, all entries to 1, diaginal removed */
  int i, *ia, *ja, nz, m, n;
  real *a;
  SparseMatrix B;

  if (!A) return A;
  
  nz = A->nz;
  ia = A->ia;
  ja = A->ja;
  n = A->n;
  m = A->m;

  if (n != m) return NULL;

  B = SparseMatrix_new(m, n, nz, MATRIX_TYPE_PATTERN, FORMAT_CSR);

  MEMCPY(B->ia, ia, sizeof(int)*((size_t)(m+1)));
  MEMCPY(B->ja, ja, sizeof(int)*((size_t)nz));
  B->nz = A->nz;

  A = SparseMatrix_symmetrize(B, TRUE);
  SparseMatrix_delete(B);
  A = SparseMatrix_remove_diagonal(A);
  A->a = MALLOC(sizeof(real)*((size_t)(A->nz)));
  a = (real*) A->a;
  for (i = 0; i < A->nz; i++) a[i] = 1.;
  A->type = MATRIX_TYPE_REAL;
  A->size = sizeof(real);
  return A;
}



SparseMatrix SparseMatrix_normalize_to_rowsum1(SparseMatrix A){
  int i, j;
  real sum, *a;

  if (!A) return A;
  if (A->format != FORMAT_CSR && A->type != MATRIX_TYPE_REAL) {
#ifdef DEBUG
    printf("only CSR and real matrix supported.\n");
#endif
    return A;
  }

  a = (real*) A->a;
  for (i = 0; i < A->m; i++){
    sum = 0;
    for (j = A->ia[i]; j < A->ia[i+1]; j++){
      sum += a[j];
    }
    if (sum != 0){
      for (j = A->ia[i]; j < A->ia[i+1]; j++){
	a[j] /= sum;
      }
    }
  }
  return A;
}



SparseMatrix SparseMatrix_normalize_by_row(SparseMatrix A){
  int i, j;
  real max, *a;

  if (!A) return A;
  if (A->format != FORMAT_CSR && A->type != MATRIX_TYPE_REAL) {
#ifdef DEBUG
    printf("only CSR and real matrix supported.\n");
#endif
    return A;
  }

  a = (real*) A->a;
  for (i = 0; i < A->m; i++){
    max = 0;
    for (j = A->ia[i]; j < A->ia[i+1]; j++){
      max = MAX(ABS(a[j]), max);
    }
    if (max != 0){
      for (j = A->ia[i]; j < A->ia[i+1]; j++){
	a[j] /= max;
      }
    }
  }
  return A;
}



SparseMatrix SparseMatrix_apply_fun(SparseMatrix A, double (*fun)(double x)){
  int i, j;
  real *a;


  if (!A) return A;
  if (A->format != FORMAT_CSR && A->type != MATRIX_TYPE_REAL) {
#ifdef DEBUG
    printf("only CSR and real matrix supported.\n");
#endif
    return A;
  }


  a = (real*) A->a;
  for (i = 0; i < A->m; i++){
    for (j = A->ia[i]; j < A->ia[i+1]; j++){
      a[j] = fun(a[j]);
    }
  }
  return A;
}

SparseMatrix SparseMatrix_apply_fun_general(SparseMatrix A, void (*fun)(int i, int j, int n, double *x)){
  int i, j;
  real *a;
  int len = 1;

  if (!A) return A;
  if (A->format != FORMAT_CSR || (A->type != MATRIX_TYPE_REAL&&A->type != MATRIX_TYPE_COMPLEX)) {
#ifdef DEBUG
    printf("SparseMatrix_apply_fun: only CSR and real/complex matrix supported.\n");
#endif
    return A;
  }
  if (A->type == MATRIX_TYPE_COMPLEX) len = 2;

  a = (real*) A->a;
  for (i = 0; i < A->m; i++){
    for (j = A->ia[i]; j < A->ia[i+1]; j++){
      fun(i, A->ja[j], len, &a[len*j]);
    }
  }
  return A;
}


SparseMatrix SparseMatrix_crop(SparseMatrix A, real epsilon){
  int i, j, *ia, *ja, nz, sta;

  if (!A) return A;

  nz = 0;
  ia = A->ia;
  ja = A->ja;
  sta = ia[0];
  switch (A->type){
  case MATRIX_TYPE_REAL:{
    real *a = (real*) A->a;
    for (i = 0; i < A->m; i++){
      for (j = sta; j < ia[i+1]; j++){
	if (ABS(a[j]) > epsilon){
	  ja[nz] = ja[j];
	  a[nz++] = a[j];
	}
      }
      sta = ia[i+1];
      ia[i+1] = nz;
    }
    A->nz = nz;
    break;
  }
  case MATRIX_TYPE_COMPLEX:{
    real *a = (real*) A->a;
    for (i = 0; i < A->m; i++){
      for (j = sta; j < ia[i+1]; j++){
	if (sqrt(a[2*j]*a[2*j]+a[2*j+1]*a[2*j+1]) > epsilon){
	  ja[nz] = ja[j];
	  a[2*nz] = a[2*j];
	  a[2*nz+1] = a[2*j+1];
	  nz++;
	}
      }
      sta = ia[i+1];
      ia[i+1] = nz;
    }
    A->nz = nz;
    break;
  }
  case MATRIX_TYPE_INTEGER:{
    int *a = (int*) A->a;
    for (i = 0; i < A->m; i++){
      for (j = sta; j < ia[i+1]; j++){
	if (ABS(a[j]) > epsilon){
	  ja[nz] = ja[j];
	  a[nz++] = a[j];
	}
      }
      sta = ia[i+1];
      ia[i+1] = nz;
    }
    A->nz = nz;
    break;
  }
  case MATRIX_TYPE_PATTERN:{
    break;
  }
  case MATRIX_TYPE_UNKNOWN:
    return NULL;
  default:
    return NULL;
  }

  return A;
}

SparseMatrix SparseMatrix_copy(SparseMatrix A){
  SparseMatrix B;
  if (!A) return A;
  B = SparseMatrix_new(A->m, A->n, A->nz, A->type, A->format);
  MEMCPY(B->ia, A->ia, sizeof(int)*((size_t)(A->m+1)));
  MEMCPY(B->ja, A->ja, sizeof(int)*((size_t)(A->ia[A->m])));
  if (A->a) MEMCPY(B->a, A->a, A->size*((size_t)A->nz));
  B->property = A->property;
  B->nz = A->nz;
  return B;
}

int SparseMatrix_has_diagonal(SparseMatrix A){

  int i, j, m = A->m, *ia = A->ia, *ja = A->ja;

  for (i = 0; i < m; i++){
    for (j = ia[i]; j < ia[i+1]; j++){
      if (i == ja[j]) return TRUE;
    }
  }
  return FALSE;
}

void SparseMatrix_level_sets_internal(int khops, SparseMatrix A, int root, int *nlevel, int **levelset_ptr, int **levelset, int **mask, int reinitialize_mask){
  /* mask is assumed to be initialized to negative if provided.
     . On exit, mask = levels for visited nodes (1 for root, 2 for its neighbors, etc), 
     . unless reinitialize_mask = TRUE, in which case mask = -1.
     khops: max number of hops allowed. If khops < 0, no limit is applied.
     A: the graph, undirected
     root: starting node
     nlevel: max distance to root from any node (in the connected comp)
     levelset_ptr, levelset: the level sets
   */
  int i, j, sta = 0, sto = 1, nz, ii;
  int m = A->m, *ia = A->ia, *ja = A->ja;

  if (!(*levelset_ptr)) *levelset_ptr = MALLOC(sizeof(int)*((size_t)(m+2)));
  if (!(*levelset)) *levelset = MALLOC(sizeof(int)*((size_t)m));
  if (!(*mask)) {
    *mask = malloc(sizeof(int)*((size_t)m));
    for (i = 0; i < m; i++) (*mask)[i] = UNMASKED;
  }

  *nlevel = 0;
  assert(root >= 0 && root < m);
  (*levelset_ptr)[0] = 0;
  (*levelset_ptr)[1] = 1;
  (*levelset)[0] = root;
  (*mask)[root] = 1;
  *nlevel = 1;
  nz = 1;
  sta = 0; sto = 1;
  while (sto > sta && (khops < 0 || *nlevel <= khops)){
    for (i = sta; i < sto; i++){
      ii = (*levelset)[i];
      for (j = ia[ii]; j < ia[ii+1]; j++){
	if (ii == ja[j]) continue;
	if ((*mask)[ja[j]] < 0){
	  (*levelset)[nz++] = ja[j];
	  (*mask)[ja[j]] = *nlevel + 1;
	}
      }
    }
    (*levelset_ptr)[++(*nlevel)] = nz;
    sta = sto;
    sto = nz;
  }
  if (khops < 0 || *nlevel <= khops){
    (*nlevel)--;
  }
  if (reinitialize_mask) for (i = 0; i < (*levelset_ptr)[*nlevel]; i++) (*mask)[(*levelset)[i]] = UNMASKED;
}

void SparseMatrix_level_sets(SparseMatrix A, int root, int *nlevel, int **levelset_ptr, int **levelset, int **mask, int reinitialize_mask){

  int khops = -1;

  return SparseMatrix_level_sets_internal(khops, A, root, nlevel, levelset_ptr, levelset, mask, reinitialize_mask);

}
void SparseMatrix_level_sets_khops(int khops, SparseMatrix A, int root, int *nlevel, int **levelset_ptr, int **levelset, int **mask, int reinitialize_mask){

  return SparseMatrix_level_sets_internal(khops, A, root, nlevel, levelset_ptr, levelset, mask, reinitialize_mask);

}

void SparseMatrix_weakly_connected_components(SparseMatrix A0, int *ncomp, int **comps, int **comps_ptr){
  SparseMatrix A = A0;
  int *levelset_ptr = NULL, *levelset = NULL, *mask = NULL, nlevel;
  int m = A->m, i, nn;

  if (!SparseMatrix_is_symmetric(A, TRUE)){
    A = SparseMatrix_symmetrize(A, TRUE);
  }
  if (!(*comps_ptr)) *comps_ptr = MALLOC(sizeof(int)*((size_t)(m+1)));

  *ncomp = 0;
  (*comps_ptr)[0] = 0;
  for (i = 0; i < m; i++){
    if (i == 0 || mask[i] < 0) {
      SparseMatrix_level_sets(A, i, &nlevel, &levelset_ptr, &levelset, &mask, FALSE);
      if (i == 0) *comps = levelset;
      nn = levelset_ptr[nlevel];
      levelset += nn;
      (*comps_ptr)[(*ncomp)+1] = (*comps_ptr)[(*ncomp)] + nn;
      (*ncomp)++;
    }
    
  }
  if (A != A0) SparseMatrix_delete(A);
  if (levelset_ptr) FREE(levelset_ptr);

  FREE(mask);
}



struct nodedata_struct {
  real dist;/* distance to root */
  int id;/*node id */
};
typedef struct nodedata_struct* nodedata;

static int cmp(void*i, void*j){
  nodedata d1, d2;

  d1 = (nodedata) i;
  d2 = (nodedata) j;
  if (d1->dist > d2->dist){
    return 1;
  } else if (d1->dist == d2->dist){
    return 0;
  } else {
    return -1;
  }
}

static int Dijkstra_internal(SparseMatrix A, int root, real *dist, int *nlist, int *list, real *dist_max, int *mask){
  /* Find the shortest path distance of all nodes to root. If khops >= 0, the shortest ath is of distance <= khops,

     A: the nxn connectivity matrix. Entries are assumed to be nonnegative. Absolute value will be taken if 
     .  entry value is negative.
     dist: length n. On on exit contain the distance from root to every other node. dist[root] = 0. dist[i] = distance from root to node i.
     .     if the graph is disconnetced, unreachable node have a distance -1.
     .     note: ||root - list[i]|| =!= dist[i] !!!, instead, ||root - list[i]|| == dist[list[i]]
     nlist: number of nodes visited
     list: length n. the list of node in order of their extraction from the heap. 
     .     The distance from root to last in the list should be the maximum
     dist_max: the maximum distance, should be realized at node list[nlist-1].
     mask: if NULL, not used. Othewise, only nodes i with mask[i] > 0 will be considered
     return: 0 if every node is reachable. -1 if not */

  int m = A->m, i, j, jj, *ia = A->ia, *ja = A->ja, heap_id;
  BinaryHeap h;
  real *a = NULL, *aa;
  int *ai;
  nodedata ndata, ndata_min;
  enum {UNVISITED = -2, FINISHED = -1};
  int *heap_ids; /* node ID to heap ID array. Initialised to UNVISITED. 
		   Set to FINISHED after extracted as min from heap */
  int found = 0;

  assert(SparseMatrix_is_symmetric(A, TRUE));

  assert(m == A->n);

  switch (A->type){
  case MATRIX_TYPE_COMPLEX:
    aa = (real*) A->a;
    a = MALLOC(sizeof(real)*((size_t)(A->nz)));
    for (i = 0; i < A->nz; i++) a[i] = aa[i*2];
    break;
  case MATRIX_TYPE_REAL:
    a = (real*) A->a;
    break;
  case MATRIX_TYPE_INTEGER:
    ai = (int*) A->a;
    a = MALLOC(sizeof(real)*((size_t)(A->nz)));
    for (i = 0; i < A->nz; i++) a[i] = (real) ai[i];
    break;
  case MATRIX_TYPE_PATTERN:
    a = MALLOC(sizeof(real)*((size_t)A->nz));
    for (i = 0; i < A->nz; i++) a[i] = 1.;
    break;
  default:
    assert(0);/* no such matrix type */
  }

  heap_ids = MALLOC(sizeof(int)*((size_t)m));
  for (i = 0; i < m; i++) {
    dist[i] = -1;
    heap_ids[i] = UNVISITED;
  }

  h = BinaryHeap_new(cmp);
  assert(h);

  /* add root as the first item in the heap */
  ndata = MALLOC(sizeof(struct nodedata_struct));
  ndata->dist = 0;
  ndata->id = root;
  heap_ids[root] = BinaryHeap_insert(h, ndata);
  
  assert(heap_ids[root] >= 0);/* by design heap ID from BinaryHeap_insert >=0*/

  while ((ndata_min = BinaryHeap_extract_min(h))){
    i = ndata_min->id;
    dist[i] = ndata_min->dist;
    list[found++] = i;
    heap_ids[i] = FINISHED;
    //fprintf(stderr," =================\n min extracted is id=%d, dist=%f\n",i, ndata_min->dist);
    for (j = ia[i]; j < ia[i+1]; j++){
      jj = ja[j];
      heap_id = heap_ids[jj];

      if (jj == i || heap_id == FINISHED || (mask && mask[jj] < 0)) continue;

      if (heap_id == UNVISITED){
	ndata = MALLOC(sizeof(struct nodedata_struct));
	ndata->dist = ABS(a[j]) + ndata_min->dist;
	ndata->id = jj;
	heap_ids[jj] = BinaryHeap_insert(h, ndata);
	//fprintf(stderr," set neighbor id=%d, dist=%f, hid = %d, a[%d]=%f, dist=%f\n",jj, ndata->dist, heap_ids[jj], jj, a[j], ndata->dist);

      } else {
	ndata = BinaryHeap_get_item(h, heap_id);
	ndata->dist = MIN(ndata->dist, ABS(a[j]) + ndata_min->dist);
	assert(ndata->id == jj);
	BinaryHeap_reset(h, heap_id, ndata); 
	//fprintf(stderr," reset neighbor id=%d, dist=%f, hid = %d, a[%d]=%f, dist=%f\n",jj, ndata->dist,heap_id, jj, a[j], ndata->dist);
     }
    }
    FREE(ndata_min);
  }
  *nlist = found;
  *dist_max = dist[i];


  BinaryHeap_delete(h, FREE);
  FREE(heap_ids);
  if (a && a != A->a) FREE(a);
  if (found == m || mask){
    return 0;
  } else {
    return -1;
  }
}

static int Dijkstra(SparseMatrix A, int root, real *dist, int *nlist, int *list, real *dist_max){
  return Dijkstra_internal(A, root, dist, nlist, list, dist_max, NULL);
}

static int Dijkstra_masked(SparseMatrix A, int root, real *dist, int *nlist, int *list, real *dist_max, int *mask){
  /* this makes the algorithm only consider nodes that are masked.
     nodes are masked as 1, 2, ..., mask_max, which is (the number of hops from root)+1.
     Only paths consists of nodes that are masked are allowed. */
     
  return Dijkstra_internal(A, root, dist, nlist, list, dist_max, mask);
}

real SparseMatrix_pseudo_diameter_weighted(SparseMatrix A0, int root, int aggressive, int *end1, int *end2, int *connectedQ){
  /* weighted graph. But still assume to be undirected. unsymmetric matrix ill be symmetrized */
  SparseMatrix A = A0;
  int m = A->m, i, *list = NULL, nlist;
  int flag;
  real *dist = NULL, dist_max = -1, dist0 = -1;
  int roots[5], iroots, end11, end22;
 
  if (!SparseMatrix_is_symmetric(A, TRUE)){
    A = SparseMatrix_symmetrize(A, TRUE);
  }
  assert(m == A->n);

  dist = MALLOC(sizeof(real)*((size_t)m));
  list = MALLOC(sizeof(int)*((size_t)m));
  nlist = 1;
  list[nlist-1] = root;

  assert(SparseMatrix_is_symmetric(A, TRUE));

  do {
    dist0 = dist_max;
    root = list[nlist - 1];
    flag = Dijkstra(A, root, dist, &nlist, list, &dist_max);
    //fprintf(stderr,"after Dijkstra, {%d,%d}-%f\n",root, list[nlist-1], dist_max);
    assert(dist[list[nlist-1]] == dist_max);
    assert(root == list[0]);
    assert(nlist > 0);
  } while (dist_max > dist0);

  *connectedQ = (!flag);
  assert((dist_max - dist0)/MAX(1, MAX(ABS(dist0), ABS(dist_max))) < 1.e-10);

  *end1 = root;
  *end2 = list[nlist-1];
  //fprintf(stderr,"after search for diameter, diam = %f, ends = {%d,%d}\n", dist_max, *end1, *end2);

  if (aggressive){
    iroots = 0;
    for (i = MAX(0, nlist - 6); i < nlist - 1; i++){
      roots[iroots++] = list[i];
    }
    for (i = 0; i < iroots; i++){
      root = roots[i];
      dist0 = dist_max;
      fprintf(stderr,"search for diameter again from root=%d\n", root);
      dist_max = SparseMatrix_pseudo_diameter_weighted(A, root, FALSE, &end11, &end22, connectedQ);
      if (dist_max > dist0){
	*end1 = end11; *end2 = end22;
      } else {
	dist_max = dist0;
      }
    }
    fprintf(stderr,"after aggressive search for diameter, diam = %f, ends = {%d,%d}\n", dist_max, *end1, *end2);
  }

  FREE(dist);
  FREE(list);

  if (A != A0) SparseMatrix_delete(A);
  return dist_max;

}

real SparseMatrix_pseudo_diameter_unweighted(SparseMatrix A0, int root, int aggressive, int *end1, int *end2, int *connectedQ){
  /* assume unit edge length! unsymmetric matrix ill be symmetrized */
  SparseMatrix A = A0;
  int m = A->m, i;
  int nlevel;
  int *levelset_ptr = NULL, *levelset = NULL, *mask = NULL;
  int nlevel0 = 0;
  int roots[5], iroots, enda, endb;

  if (!SparseMatrix_is_symmetric(A, TRUE)){
    A = SparseMatrix_symmetrize(A, TRUE);
  }

  assert(SparseMatrix_is_symmetric(A, TRUE));

  SparseMatrix_level_sets(A, root, &nlevel, &levelset_ptr, &levelset, &mask, TRUE);
  //  fprintf(stderr,"after level set, {%d,%d}=%d\n",levelset[0], levelset[levelset_ptr[nlevel]-1], nlevel);

  *connectedQ = (levelset_ptr[nlevel] == m);
  while (nlevel0 < nlevel){
    nlevel0 = nlevel;
    root = levelset[levelset_ptr[nlevel] - 1];
    SparseMatrix_level_sets(A, root, &nlevel, &levelset_ptr, &levelset, &mask, TRUE);
    //fprintf(stderr,"after level set, {%d,%d}=%d\n",levelset[0], levelset[levelset_ptr[nlevel]-1], nlevel);
  }
  *end1 = levelset[0];
  *end2 = levelset[levelset_ptr[nlevel]-1];

  if (aggressive){
    nlevel0 = nlevel;
    iroots = 0;
    for (i = levelset_ptr[nlevel-1]; i < MIN(levelset_ptr[nlevel],  levelset_ptr[nlevel-1]+5); i++){
      iroots++;
      roots[i - levelset_ptr[nlevel-1]] = levelset[i];
    }
    for (i = 0; i < iroots; i++){
      root = roots[i];
      nlevel = (int) SparseMatrix_pseudo_diameter_unweighted(A, root, FALSE, &enda, &endb, connectedQ);
      if (nlevel > nlevel0) {
	nlevel0 = nlevel;
	*end1 = enda;
	*end2 = endb;
      }
    }
  }

  FREE(levelset_ptr);
  FREE(levelset);
  FREE(mask);
  if (A != A0) SparseMatrix_delete(A);
  return (real) nlevel0 - 1;
}

real SparseMatrix_pseudo_diameter_only(SparseMatrix A){
  int end1, end2, connectedQ;
  return SparseMatrix_pseudo_diameter_unweighted(A, 0, FALSE, &end1, &end2, &connectedQ);
}

int SparseMatrix_connectedQ(SparseMatrix A0){
  int root = 0, nlevel, *levelset_ptr = NULL, *levelset = NULL, *mask = NULL, connected;
  SparseMatrix A = A0;

  if (!SparseMatrix_is_symmetric(A, TRUE)){
    if (A->m != A->n) return FALSE;
    A = SparseMatrix_symmetrize(A, TRUE);
  }

  SparseMatrix_level_sets(A, root, &nlevel, &levelset_ptr, &levelset, &mask, TRUE);
  connected = (levelset_ptr[nlevel] == A->m);

  FREE(levelset_ptr);
  FREE(levelset);
  FREE(mask);
  if (A != A0) SparseMatrix_delete(A);

  return connected;
}


void SparseMatrix_decompose_to_supervariables(SparseMatrix A, int *ncluster, int **cluster, int **clusterp){
  /* nodes for a super variable if they share exactly the same neighbors. This is know as modules in graph theory.
     We work on columns only and columns with the same pattern are grouped as a super variable
   */
  int *ia = A->ia, *ja = A->ja, n = A->n, m = A->m;
  int *super = NULL, *nsuper = NULL, i, j, *mask = NULL, isup, *newmap, isuper;

  super = MALLOC(sizeof(int)*((size_t)(n)));
  nsuper = MALLOC(sizeof(int)*((size_t)(n+1)));
  mask = MALLOC(sizeof(int)*((size_t)n));
  newmap = MALLOC(sizeof(int)*((size_t)n));
  nsuper++;

  isup = 0;
  for (i = 0; i < n; i++) super[i] = isup;/* every node belongs to super variable 0 by default */
  nsuper[0] = n;
  for (i = 0; i < n; i++) mask[i] = -1;
  isup++;

  for (i = 0; i < m; i++){
#ifdef DEBUG_PRINT1
    printf("\n");
    printf("doing row %d-----\n",i+1);
#endif
    for (j = ia[i]; j < ia[i+1]; j++){
      isuper = super[ja[j]];
      nsuper[isuper]--;/* those entries will move to a different super vars*/
    }
    for (j = ia[i]; j < ia[i+1]; j++){
      isuper = super[ja[j]];
      if (mask[isuper] < i){
	mask[isuper] = i;
	if (nsuper[isuper] == 0){/* all nodes in the isuper group exist in this row */
#ifdef DEBUG_PRINT1
	  printf("node %d keep super node id  %d\n",ja[j]+1,isuper+1);
#endif
	  nsuper[isuper] = 1;
	  newmap[isuper] = isuper;
	} else {
	  newmap[isuper] = isup;
	  nsuper[isup] = 1;
#ifdef DEBUG_PRINT1
	  printf("make node %d into supernode %d\n",ja[j]+1,isup+1);
#endif
	  super[ja[j]] = isup++;
	}
      } else {
#ifdef DEBUG_PRINT1
	printf("node %d join super node %d\n",ja[j]+1,newmap[isuper]+1);
#endif
	super[ja[j]] = newmap[isuper];
	nsuper[newmap[isuper]]++;
      }
    }
#ifdef DEBUG_PRINT1
    printf("nsuper=");
    for (j = 0; j < isup; j++) printf("(%d,%d),",j+1,nsuper[j]);
      printf("\n");
#endif
  }
#ifdef DEBUG_PRINT1
  for (i = 0; i < n; i++){
    printf("node %d is in supernode %d\n",i, super[i]);
  }
#endif
#ifdef PRINT
  fprintf(stderr, "n = %d, nsup = %d\n",n,isup);
#endif
  /* now accumulate super nodes */
  nsuper--;
  nsuper[0] = 0;
  for (i = 0; i < isup; i++) nsuper[i+1] += nsuper[i];

  *cluster = newmap;
  for (i = 0; i < n; i++){
    isuper = super[i];
    (*cluster)[nsuper[isuper]++] = i;
  }
  for (i = isup; i > 0; i--) nsuper[i] = nsuper[i-1];
  nsuper[0] = 0;
  *clusterp = nsuper;
  *ncluster = isup;

#ifdef PRINT
  for (i = 0; i < *ncluster; i++){
    printf("{");
    for (j = (*clusterp)[i]; j < (*clusterp)[i+1]; j++){
      printf("%d, ",(*cluster)[j]);
    }
    printf("},");
  }
  printf("\n");
#endif

  FREE(mask);
  FREE(super);

}

SparseMatrix SparseMatrix_get_augmented(SparseMatrix A){
  /* convert matrix A to an augmente dmatrix {{0,A},{A^T,0}} */
  int *irn = NULL, *jcn = NULL;
  void *val = NULL;
  int nz = A->nz, type = A->type;
  int m = A->m, n = A->n, i, j;
  SparseMatrix B = NULL;
  if (!A) return NULL;
  if (nz > 0){
    irn = MALLOC(sizeof(int)*((size_t)nz)*2);
    jcn = MALLOC(sizeof(int)*((size_t)nz)*2);
  }

  if (A->a){
    assert(A->size != 0 && nz > 0);
    val = MALLOC(A->size*2*((size_t)nz));
    MEMCPY(val, A->a, A->size*((size_t)nz));
    MEMCPY((void*)(((char*) val) + ((size_t)nz)*A->size), A->a, A->size*((size_t)nz));
  }

  nz = 0;
  for (i = 0; i < m; i++){
    for (j = (A->ia)[i]; j <  (A->ia)[i+1]; j++){
      irn[nz] = i;
      jcn[nz++] = (A->ja)[j] + m;
    }
  }
  for (i = 0; i < m; i++){
    for (j = (A->ia)[i]; j <  (A->ia)[i+1]; j++){
      jcn[nz] = i;
      irn[nz++] = (A->ja)[j] + m;
    }
  }

  B = SparseMatrix_from_coordinate_arrays(nz, m + n, m + n, irn, jcn, val, type, A->size);
  SparseMatrix_set_symmetric(B);
  SparseMatrix_set_pattern_symmetric(B);
  if (irn) FREE(irn);
  if (jcn) FREE(jcn);
  if (val) FREE(val);
  return B;

}

SparseMatrix SparseMatrix_to_square_matrix(SparseMatrix A, int bipartite_options){
  SparseMatrix B;
  switch (bipartite_options){
  case BIPARTITE_RECT:
    if (A->m == A->n) return A;
    break;
  case BIPARTITE_PATTERN_UNSYM:
    if (A->m == A->n && SparseMatrix_is_symmetric(A, TRUE)) return A;
    break;
  case BIPARTITE_UNSYM:
    if (A->m == A->n && SparseMatrix_is_symmetric(A, FALSE)) return A;
    break;
  case BIPARTITE_ALWAYS:
    break;
  default:
    assert(0);
  }
  B = SparseMatrix_get_augmented(A);
  SparseMatrix_delete(A);
  return B;
}

SparseMatrix SparseMatrix_get_submatrix(SparseMatrix A, int nrow, int ncol, int *rindices, int *cindices){
  /* get the submatrix from row/columns indices[0,...,l-1]. 
     row rindices[i] will be the new row i
     column cindices[i] will be the new column i.
     if rindices = NULL, it is assume that 1 -- nrow is needed. Same for cindices/ncol.
   */
  int nz = 0, i, j, *irn, *jcn, *ia = A->ia, *ja = A->ja, m = A->m, n = A->n;
  int *cmask, *rmask;
  void *v = NULL;
  SparseMatrix B = NULL;
  int irow = 0, icol = 0;

  if (nrow <= 0 || ncol <= 0) return NULL;

  

  rmask = MALLOC(sizeof(int)*((size_t)m));
  cmask = MALLOC(sizeof(int)*((size_t)n));
  for (i = 0; i < m; i++) rmask[i] = -1;
  for (i = 0; i < n; i++) cmask[i] = -1;

  if (rindices){
    for (i = 0; i < nrow; i++) {
      if (rindices[i] >= 0 && rindices[i] < m){
	rmask[rindices[i]] = irow++;
      }
    }
  } else {
    for (i = 0; i < nrow; i++) {
      rmask[i] = irow++;
    }
  }

  if (cindices){
    for (i = 0; i < ncol; i++) {
      if (cindices[i] >= 0 && cindices[i] < n){
	cmask[cindices[i]] = icol++;
      }
    }
  } else {
    for (i = 0; i < ncol; i++) {
      cmask[i] = icol++;
    }
  }

  for (i = 0; i < m; i++){
    if (rmask[i] < 0) continue;
    for (j = ia[i]; j < ia[i+1]; j++){
      if (cmask[ja[j]] < 0) continue;
      nz++;
    }
  }


  switch (A->type){
  case MATRIX_TYPE_REAL:{
    real *a = (real*) A->a;
    real *val;
    irn = MALLOC(sizeof(int)*((size_t)nz));
    jcn = MALLOC(sizeof(int)*((size_t)nz));
    val = MALLOC(sizeof(real)*((size_t)nz));

    nz = 0;
    for (i = 0; i < m; i++){
      if (rmask[i] < 0) continue;
      for (j = ia[i]; j < ia[i+1]; j++){
	if (cmask[ja[j]] < 0) continue;
	irn[nz] = rmask[i];
	jcn[nz] = cmask[ja[j]];
	val[nz++] = a[j];
      }
    }
    v = (void*) val;
    break;
  }
  case MATRIX_TYPE_COMPLEX:{
    real *a = (real*) A->a;
    real *val;

    irn = MALLOC(sizeof(int)*((size_t)nz));
    jcn = MALLOC(sizeof(int)*((size_t)nz));
    val = MALLOC(sizeof(real)*2*((size_t)nz));

    nz = 0;
    for (i = 0; i < m; i++){
      if (rmask[i] < 0) continue;
      for (j = ia[i]; j < ia[i+1]; j++){
	if (cmask[ja[j]] < 0) continue;
	irn[nz] = rmask[i];
	jcn[nz] = cmask[ja[j]];
	val[2*nz] = a[2*j];
	val[2*nz+1] = a[2*j+1];
	nz++;
      }
    }
    v = (void*) val;
    break;
  }
  case MATRIX_TYPE_INTEGER:{
    int *a = (int*) A->a;
    int *val;

    irn = MALLOC(sizeof(int)*((size_t)nz));
    jcn = MALLOC(sizeof(int)*((size_t)nz));
    val = MALLOC(sizeof(int)*((size_t)nz));

    nz = 0;
    for (i = 0; i < m; i++){
      if (rmask[i] < 0) continue;
      for (j = ia[i]; j < ia[i+1]; j++){
	if (cmask[ja[j]] < 0) continue;
	irn[nz] = rmask[i];
	jcn[nz] = cmask[ja[j]];
	val[nz] = a[j];
	nz++;
      }
    }
    v = (void*) val;
    break;
  }
  case MATRIX_TYPE_PATTERN:
    irn = MALLOC(sizeof(int)*((size_t)nz));
    jcn = MALLOC(sizeof(int)*((size_t)nz));
    nz = 0;
     for (i = 0; i < m; i++){
      if (rmask[i] < 0) continue;
      for (j = ia[i]; j < ia[i+1]; j++){
	if (cmask[ja[j]] < 0) continue;
	irn[nz] = rmask[i];
	jcn[nz++] = cmask[ja[j]];
      }
    }
    break;
  case MATRIX_TYPE_UNKNOWN:
    FREE(rmask);
    FREE(cmask);
    return NULL;
  default:
    FREE(rmask);
    FREE(cmask);
    return NULL;
  }

  B = SparseMatrix_from_coordinate_arrays(nz, nrow, ncol, irn, jcn, v, A->type, A->size);
  FREE(cmask);
  FREE(rmask);
  FREE(irn);
  FREE(jcn);
  if (v) FREE(v);


  return B;

}

SparseMatrix SparseMatrix_exclude_submatrix(SparseMatrix A, int nrow, int ncol, int *rindices, int *cindices){
  /* get a submatrix by excluding rows and columns */
  int *r, *c, nr, nc, i;
  SparseMatrix B;

  if (nrow <= 0 && ncol <= 0) return A;

  r = MALLOC(sizeof(int)*((size_t)A->m));
  c = MALLOC(sizeof(int)*((size_t)A->n));

  for (i = 0; i < A->m; i++) r[i] = i;
  for (i = 0; i < A->n; i++) c[i] = i;
  for (i = 0; i < nrow; i++) {
    if (rindices[i] >= 0 && rindices[i] < A->m){
      r[rindices[i]] = -1;
    }
  }
  for (i = 0; i < ncol; i++) {
    if (cindices[i] >= 0 && cindices[i] < A->n){
      c[cindices[i]] = -1;
    }
  }

  nr = nc = 0;
  for (i = 0; i < A->m; i++) {
    if (r[i] > 0) r[nr++] = r[i];
  }
  for (i = 0; i < A->n; i++) {
    if (c[i] > 0) c[nc++] = c[i];
  }

  B = SparseMatrix_get_submatrix(A, nr, nc, r, c);

  FREE(r);
  FREE(c);
  return B;

}

SparseMatrix SparseMatrix_largest_component(SparseMatrix A){
  SparseMatrix B;
  int ncomp;
  int *comps = NULL;
  int *comps_ptr = NULL;
  int i;
  int nmax, imax = 0;

  if (!A) return NULL;
  A = SparseMatrix_to_square_matrix(A, BIPARTITE_RECT);
  SparseMatrix_weakly_connected_components(A, &ncomp, &comps, &comps_ptr);
  if (ncomp == 1) {
    B = A;
  } else {
    nmax = 0;
    for (i = 0; i < ncomp; i++){
      if (nmax < comps_ptr[i+1] - comps_ptr[i]){
	nmax = comps_ptr[i+1] - comps_ptr[i];
	imax = i;
      }
    }
    B = SparseMatrix_get_submatrix(A, nmax, nmax, &comps[comps_ptr[imax]], &comps[comps_ptr[imax]]);
  }
  FREE(comps);
  FREE(comps_ptr);
  return B;


}

SparseMatrix SparseMatrix_delete_empty_columns(SparseMatrix A, int **new2old, int *nnew, int inplace){
  /* delete empty columns in A. After than number of columns will be nnew, and 
     the mapping from new matrix column to old matrix column is new2old.
     On entry, if new2old is NULL, it is allocated.
  */
  SparseMatrix B;
  int *ia, *ja;
  int *old2new;
  int i;
  old2new = MALLOC(sizeof(int)*((size_t)A->n));
  for (i = 0; i < A->n; i++) old2new[i] = -1;

  *nnew = 0;
  B = SparseMatrix_transpose(A);
  ia = B->ia; ja = B->ja;
  for (i = 0; i < B->m; i++){
    if (ia[i+1] > ia[i]){
      (*nnew)++;
    }
  }
  if (!(*new2old)) *new2old = MALLOC(sizeof(int)*((size_t)(*nnew)));

  *nnew = 0;
  for (i = 0; i < B->m; i++){
    if (ia[i+1] > ia[i]){
      (*new2old)[*nnew] = i;
      old2new[i] = *nnew;
      (*nnew)++;
    }
  }
  SparseMatrix_delete(B);

  if (inplace){
    B = A;
  } else {
    B = SparseMatrix_copy(A);
  }
  ia = B->ia; ja = B->ja;
  for (i = 0; i < ia[B->m]; i++){
    assert(old2new[ja[i]] >= 0);
    ja[i] = old2new[ja[i]];
  }
  B->n = *nnew;

  FREE(old2new);
  return B;
  

}

SparseMatrix SparseMatrix_set_entries_to_real_one(SparseMatrix A){
  real *a;
  int i;

  if (A->a) FREE(A->a);
  A->a = MALLOC(sizeof(real)*((size_t)A->nz));
  a = (real*) (A->a);
  for (i = 0; i < A->nz; i++) a[i] = 1.;
  A->type = MATRIX_TYPE_REAL;
  A->size = sizeof(real);
  return A;

}

SparseMatrix SparseMatrix_complement(SparseMatrix A, int undirected){
  /* find the complement graph A^c, such that {i,h}\in E(A_c) iff {i,j} \notin E(A). Only structural matrix is returned. */
  SparseMatrix B = A;
  int *ia, *ja;
  int m = A->m, n = A->n;
  int *mask, nz = 0;
  int *irn, *jcn;
  int i, j;

  if (undirected) B = SparseMatrix_symmetrize(A, TRUE);
  assert(m == n);

  ia = B->ia; ja = B->ja;
  mask = MALLOC(sizeof(int)*((size_t)n));
  irn = MALLOC(sizeof(int)*(((size_t)n)*((size_t)n) - ((size_t)A->nz)));
  jcn = MALLOC(sizeof(int)*(((size_t)n)*((size_t)n) - ((size_t)A->nz)));

  for (i = 0; i < n; i++){
    mask[i] = -1;
  }

  for (i = 0; i < n; i++){
    for (j = ia[i]; j < ia[i+1]; j++){
      mask[ja[j]] = i;
    }
    for (j = 0; j < n; j++){
      if (mask[j] != i){
	irn[nz] = i;
	jcn[nz++] = j;
      }
    }
  }

  if (B != A) SparseMatrix_delete(B);
  B = SparseMatrix_from_coordinate_arrays(nz, m, n, irn, jcn, NULL, MATRIX_TYPE_PATTERN, 0);
  FREE(irn);
  FREE(jcn);
  return B;
}

int SparseMatrix_k_centers(SparseMatrix D0, int weighted, int K, int root, int **centers, int centering, real **dist0){
  /*
    Input:
    D: the graph. If weighted, the entry values is used.
    weighted: whether to treat the graph as weighted
    K: the number of centers
    root: the start node to find the k center.
    centering: whether the distance should be centered so that sum_k dist[n*k+i] = 0
    Output:
    centers: the list of nodes that form the k-centers. If centers = NULL on input, it will be allocated.
    dist: of dimension k*n, dist[k*n: (k+1)*n) gives the distance of every node to the k-th center.
    return: flag. if not zero, the graph is not connected, or out of memory.
  */
  SparseMatrix D = D0;
  int m = D->m, n = D->n;
  int *levelset_ptr = NULL, *levelset = NULL, *mask = NULL;
  int aggressive = FALSE;
  int connectedQ, end1, end2;
  enum {K_CENTER_DISCONNECTED = 1, K_CENTER_MEM};
  real *dist_min = NULL, *dist_sum = NULL, dmax, dsum;
  real *dist = NULL;
  int nlist, *list = NULL;
  int flag = 0, i, j, k, nlevel;

  if (!SparseMatrix_is_symmetric(D, FALSE)){
    D = SparseMatrix_symmetrize(D, FALSE);
  }

  assert(m == n);

  dist_min = MALLOC(sizeof(real)*n);
  dist_sum = MALLOC(sizeof(real)*n);
  for (i = 0; i < n; i++) dist_sum[i] = 0;
  if (!(*centers)) *centers = MALLOC(sizeof(int)*K);
  if (!(*dist0)) *dist0 = MALLOC(sizeof(real)*K*n);
  if (!weighted){
    dist = MALLOC(sizeof(real)*n);
    SparseMatrix_pseudo_diameter_unweighted(D, root, aggressive, &end1, &end2, &connectedQ);
    if (!connectedQ) {
      flag =  K_CENTER_DISCONNECTED;
      goto RETURN;
    }
    root = end1;
    for (k = 0; k < K; k++){
      (*centers)[k] = root;
      //      fprintf(stderr,"k = %d, root = %d\n",k, root+1);
      SparseMatrix_level_sets(D, root, &nlevel, &levelset_ptr, &levelset, &mask, TRUE);
      assert(levelset_ptr[nlevel] == n);
      for (i = 0; i < nlevel; i++) {
	for (j = levelset_ptr[i]; j < levelset_ptr[i+1]; j++){
	  (*dist0)[k*n+levelset[j]] = i;
	  if (k == 0){
	    dist_min[levelset[j]] = i;
	  } else {
	    dist_min[levelset[j]] = MIN(dist_min[levelset[j]], i);
	  }
	  dist_sum[levelset[j]] += i;
	}
      }

      /* root = argmax_i min_roots dist(i, roots) */
      dmax = dist_min[0];
      dsum = dist_sum[0];
      root = 0;
      for (i = 0; i < n; i++) {
	if (dmax < dist_min[i] || (dmax == dist_min[i] && dsum < dist_sum[i])){/* tie break with avg dist */
	  dmax = dist_min[i];
	  dsum = dist_sum[i];
	  root = i;
	}
      }
    }
 } else {
    SparseMatrix_pseudo_diameter_weighted(D, root, aggressive, &end1, &end2, &connectedQ);
    if (!connectedQ) return K_CENTER_DISCONNECTED;
    root = end1;
    list = MALLOC(sizeof(int)*n);

    for (k = 0; k < K; k++){
      //fprintf(stderr,"k = %d, root = %d\n",k, root+1);
      (*centers)[k] = root;
      dist = &((*dist0)[k*n]);
      flag = Dijkstra(D, root, dist, &nlist, list, &dmax);
      if (flag){
	flag = K_CENTER_MEM;
	goto RETURN;
      }
      assert(nlist == n);
      for (i = 0; i < n; i++){
	if (k == 0){
	  dist_min[i] = dist[i];
	} else {
	  dist_min[i] = MIN(dist_min[i], dist[i]);
	}
	dist_sum[i] += dist[i];
      }

      /* root = argmax_i min_roots dist(i, roots) */
      dmax = dist_min[0];
      dsum = dist_sum[0];
      root = 0;
      for (i = 0; i < n; i++) {
	if (dmax < dist_min[i] || (dmax == dist_min[i] && dsum < dist_sum[i])){/* tie break with avg dist */
	  dmax = dist_min[i];
	  dsum = dist_sum[i];
	  root = i;
	}
      }
    }
    dist = NULL;
  }

  if (centering){
    for (i = 0; i < n; i++) dist_sum[i] /= k;
    for (k = 0; k < K; k++){
      for (i = 0; i < n; i++){
	(*dist0)[k*n+i] -= dist_sum[i];
      }
    }
  }

 RETURN:
  if (levelset_ptr) FREE(levelset_ptr);
  if (levelset) FREE(levelset);
  if (mask) FREE(mask);
  
  if (D != D0) SparseMatrix_delete(D);
  if (dist) FREE(dist);
  if (dist_min) FREE(dist_min);
  if (dist_sum) FREE(dist_sum);
  if (list) FREE(list);
  return flag;

}



int SparseMatrix_k_centers_user(SparseMatrix D0, int weighted, int K, int *centers_user, int centering, real **dist0){
  /*
    Input:
    D: the graph. If weighted, the entry values is used.
    weighted: whether to treat the graph as weighted
    K: the number of centers
    root: the start node to find the k center.
    centering: whether the distance should be centered so that sum_k dist[n*k+i] = 0
    centers_user: the list of nodes that form the k-centers, GIVEN BY THE USER
    Output:
    dist: of dimension k*n, dist[k*n: (k+1)*n) gives the distance of every node to the k-th center.
    return: flag. if not zero, the graph is not connected, or out of memory.
  */
  SparseMatrix D = D0;
  int m = D->m, n = D->n;
  int *levelset_ptr = NULL, *levelset = NULL, *mask = NULL;
  int aggressive = FALSE;
  int connectedQ, end1, end2;
  enum {K_CENTER_DISCONNECTED = 1, K_CENTER_MEM};
  real *dist_min = NULL, *dist_sum = NULL, dmax;
  real *dist = NULL;
  int nlist, *list = NULL;
  int flag = 0, i, j, k, nlevel;
  int root;

  if (!SparseMatrix_is_symmetric(D, FALSE)){
    D = SparseMatrix_symmetrize(D, FALSE);
  }

  assert(m == n);

  dist_min = MALLOC(sizeof(real)*n);
  dist_sum = MALLOC(sizeof(real)*n);
  for (i = 0; i < n; i++) dist_sum[i] = 0;
  if (!(*dist0)) *dist0 = MALLOC(sizeof(real)*K*n);
  if (!weighted){
    dist = MALLOC(sizeof(real)*n);
    root = centers_user[0];
    SparseMatrix_pseudo_diameter_unweighted(D, root, aggressive, &end1, &end2, &connectedQ);
    if (!connectedQ) {
      flag =  K_CENTER_DISCONNECTED;
      goto RETURN;
    }
    for (k = 0; k < K; k++){
      root = centers_user[k];
      SparseMatrix_level_sets(D, root, &nlevel, &levelset_ptr, &levelset, &mask, TRUE);
      assert(levelset_ptr[nlevel] == n);
      for (i = 0; i < nlevel; i++) {
	for (j = levelset_ptr[i]; j < levelset_ptr[i+1]; j++){
	  (*dist0)[k*n+levelset[j]] = i;
	  if (k == 0){
	    dist_min[levelset[j]] = i;
	  } else {
	    dist_min[levelset[j]] = MIN(dist_min[levelset[j]], i);
	  }
	  dist_sum[levelset[j]] += i;
	}
      }

    }
 } else {
    root = centers_user[0];
    SparseMatrix_pseudo_diameter_weighted(D, root, aggressive, &end1, &end2, &connectedQ);
    if (!connectedQ) return K_CENTER_DISCONNECTED;
    list = MALLOC(sizeof(int)*n);

    for (k = 0; k < K; k++){
      root = centers_user[k];
      //      fprintf(stderr,"k = %d, root = %d\n",k, root+1);
      dist = &((*dist0)[k*n]);
      flag = Dijkstra(D, root, dist, &nlist, list, &dmax);
      if (flag){
	flag = K_CENTER_MEM;
	dist = NULL;
	goto RETURN;
      }
      assert(nlist == n);
      for (i = 0; i < n; i++){
	if (k == 0){
	  dist_min[i] = dist[i];
	} else {
	  dist_min[i] = MIN(dist_min[i], dist[i]);
	}
	dist_sum[i] += dist[i];
      }

    }
    dist = NULL;
  }

  if (centering){
    for (i = 0; i < n; i++) dist_sum[i] /= k;
    for (k = 0; k < K; k++){
      for (i = 0; i < n; i++){
	(*dist0)[k*n+i] -= dist_sum[i];
      }
    }
  }

 RETURN:
  if (levelset_ptr) FREE(levelset_ptr);
  if (levelset) FREE(levelset);
  if (mask) FREE(mask);
  
  if (D != D0) SparseMatrix_delete(D);
  if (dist) FREE(dist);
  if (dist_min) FREE(dist_min);
  if (dist_sum) FREE(dist_sum);
  if (list) FREE(list);
  return flag;

}




SparseMatrix SparseMatrix_from_dense(int m, int n, real *x){
  /* wrap a mxn matrix into a sparse matrix. the {i,j} entry of the matrix is in x[i*n+j], 0<=i<m; 0<=j<n */
  int i, j, *ja;
  real *a;
  SparseMatrix A = SparseMatrix_new(m, n, m*n, MATRIX_TYPE_REAL, FORMAT_CSR);

  A->ia[0] = 0;
  for (i = 1; i <= m; i++) (A->ia)[i] = (A->ia)[i-1] + n;
  
  ja = A->ja;
  a = (real*) A->a;
  for (i = 0; i < m; i++){
    for (j = 0; j < n; j++) {
      ja[j] = j;
      a[j] = x[i*n+j];
    }
    ja += n; a += j;
  }
  A->nz = m*n;
  return A;

}


int SparseMatrix_distance_matrix(SparseMatrix D0, int weighted, real **dist0){
  /*
    Input:
    D: the graph. If weighted, the entry values is used.
    weighted: whether to treat the graph as weighted
    Output:
    dist: of dimension nxn, dist[i*n+j] gives the distance of node i to j.
    return: flag. if not zero, the graph is not connected, or out of memory.
  */
  SparseMatrix D = D0;
  int m = D->m, n = D->n;
  int *levelset_ptr = NULL, *levelset = NULL, *mask = NULL;
  real *dist = NULL;
  int nlist, *list = NULL;
  int flag = 0, i, j, k, nlevel;
  real dmax;

  if (!SparseMatrix_is_symmetric(D, FALSE)){
    D = SparseMatrix_symmetrize(D, FALSE);
  }

  assert(m == n);

  if (!(*dist0)) *dist0 = MALLOC(sizeof(real)*n*n);
  for (i = 0; i < n*n; i++) (*dist0)[i] = -1;

  if (!weighted){
    for (k = 0; k < n; k++){
      SparseMatrix_level_sets(D, k, &nlevel, &levelset_ptr, &levelset, &mask, TRUE);
      assert(levelset_ptr[nlevel] == n);
      for (i = 0; i < nlevel; i++) {
	for (j = levelset_ptr[i]; j < levelset_ptr[i+1]; j++){
	  (*dist0)[k*n+levelset[j]] = i;
	}
      }
     }
 } else {
    list = MALLOC(sizeof(int)*n);
    for (k = 0; k < n; k++){
      dist = &((*dist0)[k*n]);
      flag = Dijkstra(D, k, dist, &nlist, list, &dmax);
    }
  }

  if (levelset_ptr) FREE(levelset_ptr);
  if (levelset) FREE(levelset);
  if (mask) FREE(mask);
  
  if (D != D0) SparseMatrix_delete(D);
  if (list) FREE(list);
  return flag;

}

SparseMatrix SparseMatrix_distance_matrix_k_centers(int K, SparseMatrix D, int weighted){
  /* return a sparse matrix whichj represent teh k-center and distance from every node to them.
     The matrix will have k*n entries
   */
  int flag;
  real *dist = NULL;
  int m = D->m, n = D->n;
  int root = 0;
  int *centers = NULL;
  real d;
  int i, j, center;
  SparseMatrix B, C;
  int centering = FALSE;

  assert(m == n);

  B = SparseMatrix_new(n, n, 1, MATRIX_TYPE_REAL, FORMAT_COORD);

  flag = SparseMatrix_k_centers(D, weighted, K, root, &centers, centering, &dist);
  assert(!flag);

  for (i = 0; i < K; i++){
    center = centers[i];
    for (j = 0; j < n; j++){
      d = dist[i*n + j];
      B = SparseMatrix_coordinate_form_add_entries(B, 1, &center, &j, &d);
      B = SparseMatrix_coordinate_form_add_entries(B, 1, &j, &center, &d);
    }
  }

  C = SparseMatrix_from_coordinate_format(B);
  SparseMatrix_delete(B);

  FREE(centers);
  FREE(dist);
  return C;
}

SparseMatrix SparseMatrix_distance_matrix_khops(int khops, SparseMatrix D0, int weighted){
  /*
    Input:
    khops: number of hops allow. If khops < 0, this will give a dense distances. Otherwise it gives a sparse matrix that represent the k-neighborhood graph
    D: the graph. If weighted, the entry values is used.
    weighted: whether to treat the graph as weighted
    Output:
    DD: of dimension nxn. DD[i,j] gives the shortest path distance, subject to the fact that the short oath must be of <= khops.
    return: flag. if not zero, the graph is not connected, or out of memory.
  */
  SparseMatrix D = D0, B, C;
  int m = D->m, n = D->n;
  int *levelset_ptr = NULL, *levelset = NULL, *mask = NULL;
  real *dist = NULL;
  int nlist, *list = NULL;
  int flag = 0, i, j, k, itmp, nlevel;
  real dmax, dtmp;

  if (!SparseMatrix_is_symmetric(D, FALSE)){
    D = SparseMatrix_symmetrize(D, FALSE);
  }

  assert(m == n);
 
  B = SparseMatrix_new(n, n, 1, MATRIX_TYPE_REAL, FORMAT_COORD);

  if (!weighted){
    for (k = 0; k < n; k++){
      SparseMatrix_level_sets_khops(khops, D, k, &nlevel, &levelset_ptr, &levelset, &mask, TRUE);
      for (i = 0; i < nlevel; i++) {
	for (j = levelset_ptr[i]; j < levelset_ptr[i+1]; j++){
	  itmp = levelset[j]; dtmp = i;
	  if (k != itmp) B = SparseMatrix_coordinate_form_add_entries(B, 1, &k, &itmp, &dtmp);
	}
      }
     }
  } else {
    list = MALLOC(sizeof(int)*n);
    dist = MALLOC(sizeof(real)*n);
    /*
    Dijkstra_khops(khops, D, 60, dist, &nlist, list, &dmax);
    for (j = 0; j < nlist; j++){
      fprintf(stderr,"{%d,%d}=%f,",60,list[j],dist[list[j]]);
    }
    fprintf(stderr,"\n");
    Dijkstra_khops(khops, D, 94, dist, &nlist, list, &dmax);
    for (j = 0; j < nlist; j++){
      fprintf(stderr,"{%d,%d}=%f,",94,list[j],dist[list[j]]);
    }
    fprintf(stderr,"\n");
    exit(1);

    */

    for (k = 0; k < n; k++){
      SparseMatrix_level_sets_khops(khops, D, k, &nlevel, &levelset_ptr, &levelset, &mask, FALSE);
      assert(nlevel-1 <= khops);/* the first level is the root */
      flag = Dijkstra_masked(D, k, dist, &nlist, list, &dmax, mask);
      assert(!flag);
      for (i = 0; i < nlevel; i++) {
	for (j = levelset_ptr[i]; j < levelset_ptr[i+1]; j++){
	  assert(mask[levelset[j]] == i+1);
	  mask[levelset[j]] = -1;
	}
      }
      for (j = 0; j < nlist; j++){
	itmp = list[j]; dtmp = dist[itmp];
	if (k != itmp) B = SparseMatrix_coordinate_form_add_entries(B, 1, &k, &itmp, &dtmp);
      }
   }
  }

  C = SparseMatrix_from_coordinate_format(B);
  SparseMatrix_delete(B);

  if (levelset_ptr) FREE(levelset_ptr);
  if (levelset) FREE(levelset);
  if (mask) FREE(mask);
  if (dist) FREE(dist);

  if (D != D0) SparseMatrix_delete(D);
  if (list) FREE(list);
  /* I can not find a reliable way to make the matrix symmetric. Right now I use a mask array to
     limit consider of only nodes with in k hops, but even this is not symmetric. e.g.,
     . 10  10    10  10
     .A---B---C----D----E
     .           2 |    | 2
     .             G----F
     .               2
     If we set hops = 4, and from A, it can not see F (which is 5 hops), hence distance(A,E) =40
     but from E, it can see all nodes (all within 4 hops), so distance(E, A)=36.
     .
     may be there is a better way to ensure symmetric, but for now we just symmetrize it
  */
  D = SparseMatrix_symmetrize(C, FALSE);
  SparseMatrix_delete(C);
  return D;

}

#if PQ
void SparseMatrix_kcore_decomposition(SparseMatrix A, int *coreness_max0, int **coreness_ptr0, int **coreness_list0){
  /* give an undirected graph A, find the k-coreness of each vertex
     A: a graph. Will be made undirected and self loop removed
     coreness_max: max core number.
     coreness_ptr: array of size (coreness_max + 2), element [corness_ptr[i], corness_ptr[i+1])
     . of array coreness_list gives the vertices with core i, i <= coreness_max
     coreness_list: array of size n = A->m
  */
  SparseMatrix B;
  int i, j, *ia, *ja, n = A->m, nz, istatus, neighb;
  PriorityQueue pq = NULL;
  int gain, deg, k, deg_max = 0, deg_old;
  int *coreness_ptr, *coreness_list, coreness_now;
  int *mask;

  assert(A->m == A->n);
  B = SparseMatrix_symmetrize(A, FALSE);
  B = SparseMatrix_remove_diagonal(B);
  ia = B->ia;
  ja = B->ja;

  mask = MALLOC(sizeof(int)*n);
  for (i = 0; i < n; i++) mask[i] = -1;

  pq = PriorityQueue_new(n, n-1);
  for (i = 0; i < n; i++){
    deg = ia[i+1] - ia[i];
    deg_max = MAX(deg_max, deg);
    gain = n - 1 - deg;
    pq = PriorityQueue_push(pq, i, gain);
    //fprintf(stderr,"insert %d with gain %d\n",i, gain);
  }


  coreness_ptr = MALLOC(sizeof(int)*(deg_max+2));
  coreness_list = MALLOC(sizeof(int)*n);
  deg_old = 0;
  coreness_ptr[deg_old] = 0;
  coreness_now = 0;

  nz = 0;
  while (PriorityQueue_pop(pq, &k, &gain)){
    deg = (n-1) - gain;
    if (deg > deg_old) {
      //fprintf(stderr,"deg = %d, cptr[%d--%d]=%d\n",deg, deg_old + 1, deg, nz);
      for (j = deg_old + 1; j <= deg; j++) coreness_ptr[j] = nz;
      coreness_now = deg;
      deg_old = deg;
    } 
    coreness_list[nz++] = k;
    mask[k] = coreness_now;
    //fprintf(stderr,"=== \nremove node %d with gain %d, mask with %d, nelement=%d\n",k, gain, coreness_now, pq->count);
    for (j = ia[k]; j < ia[k+1]; j++){
      neighb = ja[j];
      if (mask[neighb] < 0){
	gain = PriorityQueue_get_gain(pq, neighb);
	//fprintf(stderr,"update node %d with gain %d, nelement=%d\n",neighb, gain+1, pq->count);
	istatus = PriorityQueue_remove(pq, neighb);
	assert(istatus != 0);
	pq = PriorityQueue_push(pq, neighb, gain + 1);
      }
    }
  }
  coreness_ptr[coreness_now + 1] = nz;

  *coreness_max0 = coreness_now;
  *coreness_ptr0 = coreness_ptr;
  *coreness_list0 = coreness_list;

  if (Verbose){
    for (i = 0; i <= coreness_now; i++){
      if (coreness_ptr[i+1] - coreness_ptr[i] > 0){
	fprintf(stderr,"num_in_core[%d] = %d: ",i, coreness_ptr[i+1] - coreness_ptr[i]);
#if 0
	for (j = coreness_ptr[i]; j < coreness_ptr[i+1]; j++){
	  fprintf(stderr,"%d,",coreness_list[j]);
	}
#endif
	fprintf(stderr,"\n");
      }
    }
  }
  if (Verbose) 


  if (B != A) SparseMatrix_delete(B);
  FREE(mask);
}

void SparseMatrix_kcoreness(SparseMatrix A, int **coreness){

  int coreness_max, *coreness_ptr = NULL, *coreness_list = NULL, i, j;
  
  if (!(*coreness)) coreness = MALLOC(sizeof(int)*A->m);

  SparseMatrix_kcore_decomposition(A, &coreness_max, &coreness_ptr, &coreness_list);

  for (i = 0; i <= coreness_max; i++){
    for (j = coreness_ptr[i]; j < coreness_ptr[i+1]; j++){
      (*coreness)[coreness_list[j]] = i;
    }
  }

  assert(coreness_ptr[coreness_ptr[coreness_max+1]] = A->m);

}




void SparseMatrix_khair_decomposition(SparseMatrix A, int *hairness_max0, int **hairness_ptr0, int **hairness_list0){
  /* define k-hair as the largest subgraph of the graph such that the degree of each node is <=k.
     Give an undirected graph A, find the k-hairness of each vertex
     A: a graph. Will be made undirected and self loop removed
     hairness_max: max hair number.
     hairness_ptr: array of size (hairness_max + 2), element [corness_ptr[i], corness_ptr[i+1])
     . of array hairness_list gives the vertices with hair i, i <= hairness_max
     hairness_list: array of size n = A->m
  */
  SparseMatrix B;
  int i, j, jj, *ia, *ja, n = A->m, nz, istatus, neighb;
  PriorityQueue pq = NULL;
  int gain, deg = 0, k, deg_max = 0, deg_old;
  int *hairness_ptr, *hairness_list, l;
  int *mask;

  assert(A->m == A->n);
  B = SparseMatrix_symmetrize(A, FALSE);
  B = SparseMatrix_remove_diagonal(B);
  ia = B->ia;
  ja = B->ja;

  mask = MALLOC(sizeof(int)*n);
  for (i = 0; i < n; i++) mask[i] = -1;

  pq = PriorityQueue_new(n, n-1);
  for (i = 0; i < n; i++){
    deg = ia[i+1] - ia[i];
    deg_max = MAX(deg_max, deg);
    gain = deg;
    pq = PriorityQueue_push(pq, i, gain);
  }


  hairness_ptr = MALLOC(sizeof(int)*(deg_max+2));
  hairness_list = MALLOC(sizeof(int)*n);
  deg_old = deg_max;
  hairness_ptr[deg_old + 1] = n;

  nz = n - 1;
  while (PriorityQueue_pop(pq, &k, &gain)){
    deg = gain;
    mask[k] = deg;

    if (deg < deg_old) {
      //fprintf(stderr,"cptr[%d--%d]=%d\n",deg, deg_old + 1, nz);
      for (j = deg_old; j >= deg; j--) hairness_ptr[j] = nz + 1;

      for (jj = hairness_ptr[deg_old]; jj < hairness_ptr [deg_old+1]; jj++){
	l = hairness_list[jj];
	//fprintf(stderr,"=== \nremove node hairness_list[%d]= %d, mask with %d, nelement=%d\n",jj, l, deg_old, pq->count);
	for (j = ia[l]; j < ia[l+1]; j++){
	  neighb = ja[j];
	  if (neighb == k) deg--;/* k was masked. But we do need to update ts degree */
	  if (mask[neighb] < 0){
	    gain = PriorityQueue_get_gain(pq, neighb);
	    //fprintf(stderr,"update node %d with deg %d, nelement=%d\n",neighb, gain-1, pq->count);
	    istatus = PriorityQueue_remove(pq, neighb);
	    assert(istatus != 0);
	    pq = PriorityQueue_push(pq, neighb, gain - 1);
	  }
	}
      }
      mask[k] = 0;/* because a bunch of nodes are removed, k may not be the best node! Unmask */
      pq = PriorityQueue_push(pq, k, deg);
      PriorityQueue_pop(pq, &k, &gain);
      deg = gain;
      mask[k] = deg;
      deg_old = deg;
    }
    //fprintf(stderr,"-------- node with highes deg is %d, deg = %d\n",k,deg);
    //fprintf(stderr,"hairness_lisrt[%d]=%d, mask[%d] = %d\n",nz,k, k, deg);
    assert(deg == deg_old);
    hairness_list[nz--] = k;
  }
  hairness_ptr[deg] = nz + 1;
  assert(nz + 1 == 0);
  for (i = 0; i < deg; i++) hairness_ptr[i] = 0;
  
  *hairness_max0 = deg_max;
  *hairness_ptr0 = hairness_ptr;
  *hairness_list0 = hairness_list;

  if (Verbose){
    for (i = 0; i <= deg_max; i++){
      if (hairness_ptr[i+1] - hairness_ptr[i] > 0){
	fprintf(stderr,"num_in_hair[%d] = %d: ",i, hairness_ptr[i+1] - hairness_ptr[i]);
#if 0
	for (j = hairness_ptr[i]; j < hairness_ptr[i+1]; j++){
	  fprintf(stderr,"%d,",hairness_list[j]);
	}
#endif
	fprintf(stderr,"\n");
      }
    }
  }
  if (Verbose) 


  if (B != A) SparseMatrix_delete(B);
  FREE(mask);
}


void SparseMatrix_khairness(SparseMatrix A, int **hairness){

  int hairness_max, *hairness_ptr = NULL, *hairness_list = NULL, i, j;
  
  if (!(*hairness)) hairness = MALLOC(sizeof(int)*A->m);

  SparseMatrix_khair_decomposition(A, &hairness_max, &hairness_ptr, &hairness_list);

  for (i = 0; i <= hairness_max; i++){
    for (j = hairness_ptr[i]; j < hairness_ptr[i+1]; j++){
      (*hairness)[hairness_list[j]] = i;
    }
  }

  assert(hairness_ptr[hairness_ptr[hairness_max+1]] = A->m);

}
#endif

void SparseMatrix_page_rank(SparseMatrix A, real teleport_probablity, int weighted, real epsilon, real **page_rank){
  /* A(i,j)/Sum_k A(i,k) gives the probablity of the random surfer walking from i to j
     A: n x n  square matrix
     weighted: whether to use the wedge weights (matrix entries)
     page_rank: array of length n. If *page_rank was null on entry, will be assigned.

 */
  int n = A->n;
  int i, j;
  int *ia = A->ia, *ja = A->ja;
  real *x, *y, *diag, res;
  real *a = NULL;
  int iter = 0;

  assert(A->m == n);
  assert(teleport_probablity >= 0);

  if (weighted){
    switch (A->type){
    case MATRIX_TYPE_REAL:
      a = (real*) A->a;
      break;
    case MATRIX_TYPE_COMPLEX:/* take real part */
      a = MALLOC(sizeof(real)*n);
      for (i = 0; i < n; i++) a[i] = ((real*) A->a)[2*i];
      break;
    case MATRIX_TYPE_INTEGER:
      a = MALLOC(sizeof(real)*n);
      for (i = 0; i < n; i++) a[i] = ((int*) A->a)[i];
      break;
    case MATRIX_TYPE_PATTERN:
    case MATRIX_TYPE_UNKNOWN:
    default:
      weighted = FALSE;
      break;
    }
  }


  if (!(*page_rank)) *page_rank = MALLOC(sizeof(real)*n);
  x = *page_rank;

  diag = MALLOC(sizeof(real)*n);
  for (i = 0; i < n; i++) diag[i] = 0;
  y = MALLOC(sizeof(real)*n);

  for (i = 0; i < n; i++) x[i] = 1./n;

  /* find the column sum */
  if (weighted){
    for (i = 0; i < n; i++){
      for (j = ia[i]; j < ia[i+1]; j++){
	if (ja[j] == i) continue;
	diag[i] += ABS(a[j]);
      }
    }
  } else {
    for (i = 0; i < n; i++){
      for (j = ia[i]; j < ia[i+1]; j++){
	if (ja[j] == i) continue;
	diag[i]++;
      }
    }
  }
  for (i = 0; i < n; i++) diag[i] = 1./MAX(diag[i], MACHINEACC);

  /* iterate */
  do {
    iter++;
    for (i = 0; i < n; i++) y[i] = 0;
    if (weighted){
      for (i = 0; i < n; i++){
	for (j = ia[i]; j < ia[i+1]; j++){
	  if (ja[j] == i) continue;
	  y[ja[j]] += a[j]*x[i]*diag[i];
	}
      }
    } else {
      for (i = 0; i < n; i++){
	for (j = ia[i]; j < ia[i+1]; j++){
	  if (ja[j] == i) continue;
	  y[ja[j]] += x[i]*diag[i];
	}
      }
    }
    for (i = 0; i < n; i++){
      y[i] = (1-teleport_probablity)*y[i] + teleport_probablity/n;
    }

    /*
    fprintf(stderr,"\n============\nx=");
    for (i = 0; i < n; i++) fprintf(stderr,"%f,",x[i]);
    fprintf(stderr,"\nx=");
    for (i = 0; i < n; i++) fprintf(stderr,"%f,",y[i]);
    fprintf(stderr,"\n");
    */

    res = 0;
    for (i = 0; i < n; i++) res += ABS(x[i] - y[i]);
    if (Verbose) fprintf(stderr,"page rank iter -- %d, res = %f\n",iter, res);
    MEMCPY(x, y, sizeof(real)*n);
  } while (res > epsilon);

  FREE(y);
  FREE(diag);
  if (a && a != A->a) FREE(a);
}
