/* $Id$Revision:  */
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

#include "SparseMatrix.h"
#include "mmio.h"
#include "matrix_market.h"
#include "memory.h"
#include "assert.h"
#define MALLOC gmalloc
#define REALLOC grealloc
#define FREE free
#define MEMCPY memcpy

int mm_get_type(MM_typecode typecode)
{
    if (mm_is_complex(typecode)) {
	return MATRIX_TYPE_COMPLEX;
    } else if (mm_is_real(typecode)) {
	return MATRIX_TYPE_REAL;
    } else if (mm_is_integer(typecode)) {
	return MATRIX_TYPE_INTEGER;
    } else if (mm_is_pattern(typecode)) {
	return MATRIX_TYPE_PATTERN;
    }
    return MATRIX_TYPE_UNKNOWN;
}

static void set_mm_typecode(int type, MM_typecode * typecode)
{
    switch (type) {
    case MATRIX_TYPE_COMPLEX:
	mm_set_complex(typecode);
	break;
    case MATRIX_TYPE_REAL:
	mm_set_real(typecode);
	break;
    case MATRIX_TYPE_INTEGER:
	mm_set_integer(typecode);
	break;
    case MATRIX_TYPE_PATTERN:
	mm_set_pattern(typecode);
	break;
    default:
	break;
    }
}




SparseMatrix SparseMatrix_import_matrix_market(FILE * f, int format)
{
    int ret_code, type;
    MM_typecode matcode;
    real *val = NULL, *v;
    int *vali = NULL, i, m, n, *I = NULL, *J = NULL, nz;
    void *vp = NULL;
    SparseMatrix A = NULL;
    int nzold;
    int c;

    if ((c = fgetc(f)) != '%') {
	ungetc(c, f);
	return NULL;
    }
    ungetc(c, f);
    if (mm_read_banner(f, &matcode) != 0) {
#ifdef DEBUG
	printf("Could not process Matrix Market banner.\n");
#endif
	return NULL;
    }


    /*  This is how one can screen matrix types if their application */
    /*  only supports a subset of the Matrix Market data types.      */

    if (!mm_is_matrix(matcode) || !mm_is_sparse(matcode)) {
	assert(0);
	/*
	   printf("Sorry, this application does not support dense matrix");
	   printf("Market Market type: [%s]\n", mm_typecode_to_str(matcode));
	 */
	return NULL;
    }

    /* find out size of sparse matrix .... */
    if ((ret_code = mm_read_mtx_crd_size(f, &m, &n, &nz)) != 0) {
	assert(0);
	return NULL;
    }
    /* reseve memory for matrices */

    I = MALLOC(nz * sizeof(int));
    J = MALLOC(nz * sizeof(int));



    switch (format) {
    case FORMAT_CSC:
	assert(0);		/* not supported yet */
	break;
    case FORMAT_CSR:
    case FORMAT_COORD:

	/* NOTE: when reading in doubles, ANSI C requires the use of the "l"  */
	/*   specifier as in "%lg", "%lf", "%le", otherwise errors will occur */
	/*  (ANSI C X3.159-1989, Sec. 4.9.6.2, p. 136 lines 13-15)            */
	type = mm_get_type(matcode);
	switch (type) {
	case MATRIX_TYPE_REAL:
	    val = (real *) malloc(nz * sizeof(real));
	    for (i = 0; i < nz; i++) {
		fscanf(f, "%d %d %lg\n", &I[i], &J[i], &val[i]);
		I[i]--;		/* adjust from 1-based to 0-based */
		J[i]--;
	    }
	    if (mm_is_symmetric(matcode)) {
		I = REALLOC(I, 2 * sizeof(int) * nz);
		J = REALLOC(J, 2 * sizeof(int) * nz);
		val = REALLOC(val, 2 * sizeof(real) * nz);
		nzold = nz;
		for (i = 0; i < nzold; i++) {
		    if (I[i] != J[i]) {
			I[nz] = J[i];
			J[nz] = I[i];
			val[nz++] = val[i];
		    }
		}
	    } else if (mm_is_skew(matcode)) {
		I = REALLOC(I, 2 * sizeof(int) * nz);
		J = REALLOC(J, 2 * sizeof(int) * nz);
		val = REALLOC(val, 2 * sizeof(real) * nz);
		vp = (void *) val;
		nzold = nz;
		for (i = 0; i < nzold; i++) {
		    assert(I[i] != J[i]);	/* skew symm has no diag */
		    I[nz] = J[i];
		    J[nz] = I[i];
		    val[nz++] = -val[i];
		}
	    } else {
		assert(!mm_is_hermitian(matcode));
	    }
	    vp = (void *) val;
	    break;
	case MATRIX_TYPE_INTEGER:
	    vali = (int *) malloc(nz * sizeof(int));
	    for (i = 0; i < nz; i++) {
		fscanf(f, "%d %d %d\n", &I[i], &J[i], &vali[i]);
		I[i]--;		/* adjust from 1-based to 0-based */
		J[i]--;
	    }
	    if (mm_is_symmetric(matcode)) {
		I = REALLOC(I, 2 * sizeof(int) * nz);
		J = REALLOC(J, 2 * sizeof(int) * nz);
		vali = REALLOC(vali, 2 * sizeof(int) * nz);
		nzold = nz;
		for (i = 0; i < nzold; i++) {
		    if (I[i] != J[i]) {
			I[nz] = J[i];
			J[nz] = I[i];
			vali[nz++] = vali[i];
		    }
		}
	    } else if (mm_is_skew(matcode)) {
		I = REALLOC(I, 2 * sizeof(int) * nz);
		J = REALLOC(J, 2 * sizeof(int) * nz);
		vali = REALLOC(vali, 2 * sizeof(int) * nz);
		vp = (void *) val;
		nzold = nz;
		for (i = 0; i < nzold; i++) {
		    assert(I[i] != J[i]);	/* skew symm has no diag */
		    I[nz] = J[i];
		    J[nz] = I[i];
		    vali[nz++] = -vali[i];
		}
	    } else {
		assert(!mm_is_hermitian(matcode));
	    }
	    vp = (void *) vali;
	    break;
	case MATRIX_TYPE_PATTERN:
	    for (i = 0; i < nz; i++) {
		fscanf(f, "%d %d\n", &I[i], &J[i]);
		I[i]--;		/* adjust from 1-based to 0-based */
		J[i]--;
	    }
	    if (mm_is_symmetric(matcode) || mm_is_skew(matcode)) {
		I = REALLOC(I, 2 * sizeof(int) * nz);
		J = REALLOC(J, 2 * sizeof(int) * nz);
		nzold = nz;
		for (i = 0; i < nzold; i++) {
		    if (I[i] != J[i]) {
			I[nz] = J[i];
			J[nz++] = I[i];
		    }
		}
	    } else {
		assert(!mm_is_hermitian(matcode));
	    }
	    break;
	case MATRIX_TYPE_COMPLEX:
	    val = (real *) malloc(2 * nz * sizeof(real));
	    v = val;
	    for (i = 0; i < nz; i++) {
		fscanf(f, "%d %d %lg %lg\n", &I[i], &J[i], &v[0], &v[1]);
		v += 2;
		I[i]--;		/* adjust from 1-based to 0-based */
		J[i]--;
	    }
	    if (mm_is_symmetric(matcode)) {
		I = REALLOC(I, 2 * sizeof(int) * nz);
		J = REALLOC(J, 2 * sizeof(int) * nz);
		val = REALLOC(val, 4 * sizeof(real) * nz);
		nzold = nz;
		for (i = 0; i < nzold; i++) {
		    if (I[i] != J[i]) {
			I[nz] = J[i];
			J[nz] = I[i];
			val[2 * nz] = val[2 * i];
			val[2 * nz + 1] = val[2 * i + 1];
			nz++;
		    }
		}
	    } else if (mm_is_skew(matcode)) {
		I = REALLOC(I, 2 * sizeof(int) * nz);
		J = REALLOC(J, 2 * sizeof(int) * nz);
		val = REALLOC(val, 4 * sizeof(real) * nz);
		vp = (void *) val;
		nzold = nz;
		for (i = 0; i < nzold; i++) {
		    assert(I[i] != J[i]);	/* skew symm has no diag */
		    I[nz] = J[i];
		    J[nz] = I[i];
		    val[2 * nz] = -val[2 * i];
		    val[2 * nz + 1] = -val[2 * i + 1];
		    nz++;

		}
	    } else if (mm_is_hermitian(matcode)) {
		I = REALLOC(I, 2 * sizeof(int) * nz);
		J = REALLOC(J, 2 * sizeof(int) * nz);
		val = REALLOC(val, 4 * sizeof(real) * nz);
		vp = (void *) val;
		nzold = nz;
		for (i = 0; i < nzold; i++) {
		    if (I[i] != J[i]) {
			I[nz] = J[i];
			J[nz] = I[i];
			val[2 * nz] = val[2 * i];
			val[2 * nz + 1] = -val[2 * i + 1];
			nz++;
		    }
		}
	    }
	    vp = (void *) val;
	    break;
	default:
	    return 0;
	}

	if (format == FORMAT_CSR) {
	    A = SparseMatrix_from_coordinate_arrays(nz, m, n, I, J, vp,
						    type, sizeof(real));
	} else {
	    A = SparseMatrix_new(m, n, 1, type, FORMAT_COORD);
	    A = SparseMatrix_coordinate_form_add_entries(A, nz, I, J, vp);
	}
	break;
    default:
	A = NULL;
    }
    FREE(I);
    FREE(J);
    FREE(val);

    if (mm_is_symmetric(matcode)) {
	SparseMatrix_set_symmetric(A);
	SparseMatrix_set_pattern_symmetric(A);
    } else if (mm_is_skew(matcode)) {
	SparseMatrix_set_skew(A);
    } else if (mm_is_hermitian(matcode)) {
	SparseMatrix_set_hemitian(A);
    }


    return A;
}


static void mm_write_comment(FILE * file, char *comment)
{
    char percent[2] = "%";
    fprintf(file, "%s %s\n", percent, comment);
}

void SparseMatrix_export_matrix_market(FILE * file, SparseMatrix A,
				       char *comment)
{
    MM_typecode matcode;

    mm_initialize_typecode(&matcode);
    mm_set_matrix(&matcode);
    mm_set_sparse(&matcode);
    mm_set_general(&matcode);
    set_mm_typecode(A->type, &matcode);

    mm_write_banner(file, matcode);
    mm_write_comment(file, comment);

    SparseMatrix_export(file, A);

}
