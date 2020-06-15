/* lrslib.h (vertex enumeration using lexicographic reverse search) */
#define TITLE "lrslib "
#define VERSION "v.7.1 2020.6.4"
#define AUTHOR "*Copyright (C) 1995,2020, David Avis   avis@cs.mcgill.ca "

/* This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */
/*Ver 6.1   major change is new lrsnash driver and library coded by Terje Lensberg */
/*Ver 6.0   major change is mplrs wrapper for multithreading coded by Skip Jordan  */
/*Ver 5.0   major change is plrs wrapper for multithreading coded by Gary Roumanis */
/*Ver 4.2*  library version                                      */
/******************************************************************************/
/*  See http://cgm.cs.mcgill.ca/~avis/C/lrs.html for usage instructions         */
/******************************************************************************/

#define lrs_main suf(lrs_main)
#define lrsv2_main suf(lrsv2_main)
#define lrs_run suf(lrs_run)
#define redund_run suf(redund_run)
#define redund_print suf(redund_print)
#define lrs_overflow suf(lrs_overflow)
#define cache_misses suf(cache_misses)
#define cache_tries suf(cache_tries)
#define checkcobasic suf(checkcobasic)
#define checkindex suf(checkindex)
#define checkpoint suf(checkpoint)
#define checkredund suf(checkredund)
#define copy_dict suf(copy_dict)
#define dan_selectpivot suf(dan_selectpivot)
#define ran_selectpivot suf(ran_selectpivot)
#define dict_count suf(dict_count)
#define dict_limit suf(dict_limit)
#define die_gracefully suf(die_gracefully)
#define digits_overflow suf(digits_overflow)
#define extractcols suf(extractcols)
#define getabasis suf(getabasis)
#define getnextoutput suf(getnextoutput)
#define infile suf(infile)
#define infileLen suf(infileLen)
#define infilename suf(infilename)
#define ismin suf(ismin)
#define lexmin suf(lexmin)
#define lprat suf(lprat)
#define lreadrat suf(lreadrat)
#define lrs_alloc_dat suf(lrs_alloc_dat)
#define lrs_alloc_dic suf(lrs_alloc_dic)
#define lrs_cache_to_file suf(lrs_cache_to_file)
#define lrs_cfp suf(lrs_cfp)
#define lrs_checkbound suf(lrs_checkbound)
#define lrs_checkpoint_seconds suf(lrs_checkpoint_seconds)
#define lrs_close suf(lrs_close)
#define lrs_close_outputblock suf(lrs_close_outputblock)
#define lrs_degenerate suf(lrs_degenerate)
#define lrs_dump_state suf(lrs_dump_state)
#define lrs_estimate suf(lrs_estimate)
#define lrs_exit suf(lrs_exit)
#define lrs_stdin_to_file suf(lrs_stdin_to_file)
#define lrs_file_to_cache suf(lrs_file_to_cache)
#define lrs_free_all_memory suf(lrs_free_all_memory)
#define lrs_free_dat suf(lrs_free_dat)
#define lrs_free_dic suf(lrs_free_dic)
#define lrs_free_dic2 suf(lrs_free_dic2)
#define lrs_getdic suf(lrs_getdic)
#define lrs_getfirstbasis suf(lrs_getfirstbasis)
#define lrs_getinput suf(lrs_getinput)
#define lrs_getnextbasis suf(lrs_getnextbasis)
#define lrs_getray suf(lrs_getray)
#define lrs_getsolution suf(lrs_getsolution)
#define lrs_getvertex suf(lrs_getvertex)
#define lrs_global_count suf(lrs_global_count)
#define lrs_global_list suf(lrs_global_list)
#define lrs_init suf(lrs_init)
#define lrs_setup suf(lrs_setup)
#define lrs_setup_R suf(lrs_setup_R)
#define lrs_reset suf(lrs_reset)
#define lrs_leaf suf(lrs_leaf)
#define lrs_lpoutput suf(lrs_lpoutput)
#define lrs_open_outputblock suf(lrs_open_outputblock)
#define lrs_post_output suf(lrs_post_output)
#define lrs_printcobasis suf(lrs_printcobasis)
#define lrs_print_header suf(lrs_print_header)
#define lrs_printoutput suf(lrs_printoutput)
#define lrs_printrow suf(lrs_printrow)
#define lrs_printtotals suf(lrs_printtotals)
#define lrs_ratio suf(lrs_ratio)
#define lrs_read_dat suf(lrs_read_dat)
#define lrs_read_dic suf(lrs_read_dic)
#define lrs_return_unexplored suf(lrs_return_unexplored)
#define lrs_set_digits suf(lrs_set_digits)
#define lrs_set_obj suf(lrs_set_obj)
#define lrs_set_obj_mp suf(lrs_set_obj_mp)
#define lrs_set_row suf(lrs_set_row)
#define lrs_set_row_mp suf(lrs_set_row_mp)
#define lrs_solvelp suf(lrs_solvelp)
#define lrs_solve_lp suf(lrs_solve_lp)
#define lrs_warning suf(lrs_warning)
#define outfilename suf(outfilename)
#define overflow suf(overflow)
#define pivoting suf(pivoting)
#define phaseone suf(phaseone)
#define pimat suf(pimat)
#define pivot suf(pivot)
#define primalfeasible suf(primalfeasible)
#define printA suf(printA)
#define print_basis suf(print_basis)
#define readfacets suf(readfacets)
#define readremain suf(readremain)
#define readlinearity suf(readlinearity)
#define readredund    suf(readredund)
#define removecobasicindex suf(removecobasicindex)
#define linextractcols suf(linextractcols)
#define reorder suf(reorder)
#define reorder1 suf(reorder1)
#define rescaledet suf(rescaledet)
#define rescalevolume suf(rescalevolume)
#define resize suf(resize)
#define restartpivots suf(restartpivots)
#define reverse suf(reverse)
#define save_basis suf(save_basis)
#define selectpivot suf(selectpivot)
#define timecheck suf(timecheck)
#define tmpfilename suf(tmpfilename)
#define update suf(update)
#define updatevolume suf(updatevolume)
#define update_R suf(update_R)

#ifdef LRSLONG
#define ARITH "lrslong.h"    /* lrs long integer arithmetic package */
#else
#if defined(GMP) || defined(FLINT)
#define ARITH "lrsgmp.h"     /* lrs wrapper for gmp multiple precsion arithmetic    */
#else
#define ARITH "lrsmp.h"      /* lrs multiple precsion arithmetic    */
#endif
#endif

#include ARITH

#ifndef SIGNALS
#include <signal.h>
#include <unistd.h>
#define errcheck(s,e) if ((long)(e)==-1L){  perror(s);exit(1);}
#endif

#define CALLOC(n,s) xcalloc(n,s,__LINE__,__FILE__)

/*********************/
/*global constants   */
/*********************/
#define MAX_LRS_GLOBALS 10000L  /* number of allocated dictionaries */
#define MAXIMIZE 1L         /* maximize the lp  */
#define MINIMIZE 0L         /* maximize the lp  */
#define GE 1L               /* constraint is >= */
#define EQ 0L               /* constraint is linearity */

/*************/
/* typedefs  */
/*************/

/******************************************************************************/
/*                   Indexing after initialization                            */
/*               Basis                                    Cobasis             */
/*   ---------------------------------------    ----------------------------- */
/*  |  i  |0|1| .... |lastdv|lastdv+1|...|m|   | j  | 0 | 1 | ... |d-1|  d  | */
/*  |-----|+|+|++++++|++++++|--------|---|-|   |----|---|---|-----|---|+++++| */
/*  |B[i] |0|1| .... |lastdv|lastdv+1|...|m|   |C[j]|m+1|m+2| ... |m+d|m+d+1| */
/*   -----|+|+|++++++|++++++|????????|???|?|    ----|???|???|-----|???|+++++| */
/*                                                                            */
/* Row[i] is row location for B[i]         Col[j] is column location for C[j] */
/*  -----------------------------              -----------------------------  */
/* |   i   |0|1| ..........|m-1|m|            | j    | 0 | 1 | ... |d-1| d  | */
/* |-------|+|-|-----------|---|-|            |------|---|---|---  |---|++++| */
/* |Row[i] |0|1|...........|m-1|m|            |Col[j]| 1 | 2 | ... | d |  0 | */
/* --------|+|*|***********|***|*|             ------|***|***|*****|***|++++| */
/*                                                                            */
/*  + = remains invariant   * = indices may be permuted ? = swapped by pivot  */
/*                                                                            */
/*  m = number of input rows   n= number of input columns                     */
/*  input dimension inputd = n-1 (H-rep) or n (V-rep)                         */
/*  lastdv = inputd-nredundcol  (each redundant column removes a dec. var)    */
/*  working dimension d=lastdv-nlinearity (an input linearity removes a slack) */
/*  obj function in row 0, index 0=B[0]  col 0 has index m+d+1=C[d]           */
/*  H-rep: b-vector in col 0, A matrix in columns 1..n-1                      */
/*  V-rep: col 0 all zero, b-vector in col 1, A matrix in columns 1..n        */
/******************************************************************************/

typedef struct lrs_dic_struct	/* dynamic dictionary data */
{
	lrs_mp_matrix A;
	long m;			/* A has m+1 rows, row 0 is cost row            */
	long m_A;           	/* =m or m-d if nonnegative flag set            */
	long d;			/* A has d+1 columns, col 0 is b-vector         */
	long d_orig;		/* value of d as A was allocated  (E.G.)        */
	long lexflag;		/* true if lexmin basis for this vertex         */
	long depth;		/* depth of basis/vertex in reverse search tree */
	long i, j;		/* last pivot row and column pivot indices      */
	lrs_mp det;             /* current determinant of basis                 */
	lrs_mp objnum;		/* objective numerator value                    */
	lrs_mp objden;		/* objective denominator value                  */
	long *B, *Row;		/* basis, row location indices                  */
	long *C, *Col;		/* cobasis, column location indices             */
	struct lrs_dic_struct *prev, *next;
} lrs_dic;


typedef struct lrs_dat			/* global problem data   */
{
	long *redineq;          /* holds indices of rows for redundancy check  */
        lrs_mp_matrix Ain;      /* used only in redund to hold input matrix    */
	lrs_mp_vector Gcd;	/* Gcd of each row of numerators               */
	lrs_mp_vector Lcm;	/* Lcm for each row of input denominators      */
	lrs_mp_vector output;	/* One line of output dimensioned to n         */

	lrs_mp sumdet;		/* sum of determinants                          */
	lrs_mp Nvolume;		/* volume numerator                             */
	lrs_mp Dvolume;		/* volume denominator                           */
	lrs_mp boundn;		/* objective bound numerator                    */
	lrs_mp boundd;		/* objective bound denominator                  */
	long unbounded;		/* lp unbounded */
	char fname[4096];	/* program name: lrs redund fourier nash        */

	/* initially holds order used to find starting  */
	/* basis, default: m,m-1,...,2,1                */
	long *facet;		/* cobasic indices for restart if needed        */
	long *redundcol;	/* holds columns which are redundant            */
	long *inequality;	/* indices of inequalities corr. to cobasic ind */
	long *linearity;	/* holds cobasic indices of input linearities   */
	long *remain;    	/* ordered list of vars to keep in extract      */
	long *startcob;  	/* starting cobasis if given else zeroes        */
	long *minratio;		/* used for lexicographic ratio test            */
	long *temparray;		/* for sorting indices, dimensioned to d        */
	long *isave, *jsave;	/* arrays for estimator, malloc'ed at start     */
	long inputd;		/* input dimension: n-1 for H-rep, n for V-rep  */

	long m;      		/* number of rows in input file                 */
	long n;			/* number of columns in input file              */
	long lastdv;		/* index of last dec. variable after preproc    */
	/* given by inputd-nredundcol                   */
	long count[10];		/* count[0]=rays(facets)[1]=vertices(linearities)*/
		                /*  [2]=cobases [3]=pivots [4]=integer vertices  */
	long startcount[5];

	long deepest;		        /* max depth ever reached in search             */
	long nredundcol;		/* number of redundant columns                  */
	long nlinearity;		/* number of input linearities                  */
	long totalnodes;		/* count total number of tree nodes evaluated   */
	long runs;			/* probes for estimate function                 */
	long seed;			/* seed for random number generator             */
	double cest[10];		/* ests: 0=rays,1=vert,2=bases,3=vol,4=int vert */
        long nextineq;                  /* start checking redundancy from this row:def=1*/


	/**** flags  **********                         */
	long allbases;		/* TRUE if all bases should be printed          */
	long bound;                 /* TRUE if upper/lower bound on objective given */
	long countonly;             /* TRUE if only count totals should be output   */
	long debug;
	long dualdeg;		/* TRUE if start dictionary is dual degenerate  */
	long etrace;		/* turn off debug at basis # strace             */
        long extract;           /* remove linearities if any and extractcols    */
	long frequency;		/* frequency to print cobasis indices           */
	long geometric;		/* TRUE if incident vertex prints after each ray */
	long getvolume;		/* do volume calculation                        */
	long givenstart;		/* TRUE if a starting cobasis is given          */
	long giveoutput;		/* TRUE if output should be printed,used for supressing restart first output */
	long verifyredund;      /* TRUE if mplrs consumer is verifying redund output */
  	long noredundcheck;     /* TRUE if mplrs consumer skips verifying output     */
	long homogeneous;		/* TRUE if all entries in column one are zero   */
	long hull;			/* do convex hull computation if TRUE           */
	long incidence;             /* print all tight inequalities (vertices/rays) */
	long lponly;		/* true if only lp solution wanted              */
	long long maxdepth;	/* max depth to search to in treee              */
	long maximize;		/* flag for LP maximization                     */
	long maxoutput;     	/* if positive, maximum number of output lines  */
	long maxcobases;     	/* if positive, after maxcobasis unexplored subtrees reported */
	long messages;      	/* TRUE for normal lrs output, FALSE for PLRS and LRS_QUIET   */
	long minimize;		/* flag for LP minimization                     */
	long long mindepth;	/* do not backtrack above mindepth              */
        long mplrs;             /* TRUE if compiled for mplrs                   */
	long nash;                  /* TRUE for computing nash equilibria           */
	long nonnegative;		/* TRUE if last d constraints are nonnegativity */
	long polytope;		/* TRUE for facet computation of a polytope     */
	long printcobasis;		/* TRUE if all cobasis should be printed        */
	long printslack;		/* TRUE if indices of slack inequal. printed    */
	long truncate;              /* TRUE: truncate tree when moving from opt vert*/
	long verbose;               /* FALSE for minimalist output                  */
	long restart;		/* TRUE if restarting from some cobasis         */
	long strace;		/* turn on  debug at basis # strace             */
	long voronoi;		/* compute voronoi vertices by transformation   */
        long long subtreesize;  /* in estimate mode, iterates if cob_est >= subtreesize */
        long triangulation;     /* TRUE: the cobases printed triangulate the polytope */
        long newstart;          /* TRUE: lrs is restarted with new arithmetic         */

	/* Variables for saving/restoring cobasis,  db */

	long id;			/* numbered sequentially */
	char *name;			/* passed by user */

	long saved_count[5];	        /* Saves Q->count[*] */
	long *saved_C;
	lrs_mp saved_det;
	lrs_mp saved_sumdet;
	long saved_depth;
	long saved_d;

	long saved_flag;		/* There is something in the saved cobasis */

	/* Variables for cacheing dictionaries, db */
	lrs_dic *Qhead, *Qtail;

}lrs_dat, lrs_dat_p;


/***************************/
/* mplrs hooks and hacks   */
/***************************/
void lrs_open_outputblock(void); /* prevent mplrs output flushes   */
void lrs_close_outputblock(void);/* re-enable mplrs output flushes */
void lrs_return_unexplored(lrs_dic *P,lrs_dat *Q);  /* send cobasis data for unexplored nodes */
void lrs_post_output(const char *, const char *);   /* lrs call to post_output                */

#ifndef LRSLONG
void lrs_overflow(int i);
void lrs_exit(int i);
#endif

/****************/
/*      PLRS    */
/****************/
/* following provided by mplrs.c */
void post_output(const char *, const char *);
void post_R(lrs_restart_dat *R);
void open_outputblock(void);
void close_outputblock(void);
void mplrs_cleanstop(int checkpoint);
void mplrs_emergencystop(const char *);


/*******************************/
/* functions  for external use */
/*******************************/

long lrsv2_main(int argc, char *argv[],lrs_dic **P,lrs_dat **Q, long overf,long stage,char *tmp,lrs_restart_dat *R); /* called from lrsX_main where X is arithmetic type */
long lrs_run ( lrs_dic *P, lrs_dat * Q); /* main reverse search function */
long redund_run ( lrs_dic *P, lrs_dat * Q); /* main redund loop */
void  redund_print(lrs_mp_matrix Ain,lrs_dic *P,lrs_dat *Q);
lrs_dat *lrs_alloc_dat (const char *name);	/* allocate for lrs_dat structure "name"       */
lrs_dic *lrs_alloc_dic (lrs_dat * Q);	/* allocate for lrs_dic structure corr. to Q   */
long lrs_estimate (lrs_dic * P, lrs_dat * Q);	/* get estimates only and returns est number of cobases in subtree */
long lrs_read_dat (lrs_dat * Q, int argc, char *argv[]);	/* read header and set up lrs_dat               */
long lrs_read_dic (lrs_dic * P, lrs_dat * Q);	/* read input and set up problem and lrs_dic    */
long lrs_checkbound (lrs_dic *P, lrs_dat * Q);  /* TRUE if current objective value exceeds specified bound */
long lrs_getfirstbasis (lrs_dic ** P_p, lrs_dat * Q, lrs_mp_matrix * Lin,long no_output); /* gets first basis, FALSE if none,P may get changed if lin. space Lin found  no_output is TRUE supresses output headers P may get changed if lin. space Lin found    */
void lrs_getinput(lrs_dic *P,lrs_dat *Q,long *num,long *den, long m, long d); /* reads input matrix b A in lrs/cdd format */
long lrs_getnextbasis (lrs_dic ** dict_p, lrs_dat * Q, long prune); /* gets next lrs tree basis, FALSE if none backtrack if prune is TRUE                   */
long lrs_getsolution (lrs_dic * P, lrs_dat * Q, lrs_mp_vector output, long col);
long lrs_getray (lrs_dic * P, lrs_dat * Q, long col, long comment, lrs_mp_vector output);
long lrs_getvertex (lrs_dic * P, lrs_dat * Q, lrs_mp_vector output);
void lrs_close (const char *name);	/* close lrs lib program "name"                 */
long lrs_init (const char *name);	/* initialize lrslib and arithmetic package for prog "name" */
lrs_dic *lrs_setup(int argc, char *argv[], lrs_dat **Q, lrs_restart_dat *R); /* intialize P,P_orig,Q and read data */
lrs_dic *lrs_reset(lrs_dic *P_orig, lrs_dat *Q, lrs_restart_dat *R); /* restore P form P_orig reset Q using R */
void lrs_lpoutput(lrs_dic * P,lrs_dat * Q, lrs_mp_vector output); /* print LP primal and dual solutions */
void lrs_printcobasis (lrs_dic * P, lrs_dat * Q, long col); /* print cobasis for column col(verted or ray)  */
void lrs_print_header(const char *name);
void lrs_printoutput (lrs_dat * Q, lrs_mp_vector output); /* print output array                           */
void lrs_printrow (const char *name, lrs_dat * Q, lrs_mp_vector output, long rowd); /*print row of A matrix in output[0..rowd]      */
void lrs_printsol (lrs_dic * P, lrs_dat * Q, long col, long comment);	/* print out solution from col, comment= 0=normal,-1=geometric ray,1..inputd=linearity */
void lrs_printtotals (lrs_dic * P, lrs_dat * Q);/* print final totals for lrs                   */
long lrs_set_digits (long dec_digits );  /* set lrsmp digits to equiv. of decimal dec_digits */
void lrs_setup_R (lrs_dic * P, lrs_dat * Q, lrs_restart_dat * R);
long lrs_solvelp (lrs_dic * P, lrs_dat * Q, long maximize);/* solve primal feas LP:TRUE bounded else FALSE */
void lrs_warning(lrs_dat *Q, char* type, char* ss);  /* warnings sent to printer or mplrs */

long lrs_stdin_to_file (char *name);
long lrs_file_to_cache(FILE *ifp);
long lrs_cache_to_file(char *name, const char *args);


/*******************************/
/* functions  for internal use */
/*******************************/



/*******************************/
/* basic dictionary functions  */
/*******************************/
long getabasis (lrs_dic * P, lrs_dat * Q, long order[]); /* Try to find a starting basis  */
void getnextoutput (lrs_dic * P, lrs_dat * Q, long i, long col, lrs_mp out);	/* get A[B[i][col] and copy to out */
long ismin (lrs_dic * P, lrs_dat * Q, long r, long s); /* test if A[r][s] is a min ratio for col s */
long lexmin (lrs_dic * P, lrs_dat * Q, long col); /* test A to see if current basis is lexmin       */
void pivot (lrs_dic * P, lrs_dat * Q, long bas, long cob);	/* Qpivot routine for array A  */
long primalfeasible (lrs_dic * P, lrs_dat * Q);	/* Do dual pivots to get primal feasibility       */
long lrs_ratio (lrs_dic * P, lrs_dat * Q, long col); /* find lex min. ratio  */
long removecobasicindex (lrs_dic * P, lrs_dat * Q, long k);	/* remove C[k] from problem  */
long linextractcols (lrs_dic * P, lrs_dat * Q);	/* preprocess to remove input linearities */
long restartpivots (lrs_dic * P, lrs_dat * Q); /* restart problem from given cobasis   */
long reverse (lrs_dic * P, lrs_dat * Q, long *r, long s); /* TRUE if B[*r] C[s] is a reverse lex-pos pivot  */
long selectpivot (lrs_dic * P, lrs_dat * Q, long *r, long *s);	/* select pivot indices using lexicographic rule  */
long dan_selectpivot (lrs_dic * P, lrs_dat * Q, long *r, long *s); /* select pivot indices using dantzig-lex rule    */
long ran_selectpivot (lrs_dic * P, lrs_dat * Q, long *r, long *s); /* select pivot indices using randomedge-lex rule    */
void update (lrs_dic * P, lrs_dat * Q, long *i, long *j); /* update the B,C, LOC arrays after a pivot       */
void updatevolume (lrs_dic * P, lrs_dat * Q); /* rescale determinant and update the volume      */


/*******************************/
/* other functions using P,Q   */
/*******************************/
long lrs_degenerate (lrs_dic * P, lrs_dat * Q);	/* TRUE if the dictionary is primal degenerate    */
long extractcols (lrs_dic * P, lrs_dat * Q);	/* preprocess to just extract given cols */
void print_basis (FILE * fp, lrs_dat * Q);
void printA (lrs_dic * P, lrs_dat * Q);	/* raw print of dictionary, bases for debugging   */
void pimat (lrs_dic * P, long r, long s, lrs_mp Nt, const char *name); /* print the row r col s of A                     */
long readfacets (lrs_dat * Q, long facet[]);	/* read and check facet list                      */
long readlinearity (lrs_dat * Q);       	/* read and check linearity list                  */
long readremain    (lrs_dat * Q);       	/* read and check remain vars list for extract    */
long readredund (lrs_dat * Q);	                /* read and check redundancy list                 */
void rescaledet (lrs_dic * P, lrs_dat * Q, lrs_mp Vnum, lrs_mp Vden);	/* rescale determinant to get its volume */
void rescalevolume (lrs_dic * P, lrs_dat * Q, lrs_mp Vnum, lrs_mp Vden);	/* adjust volume for dimension          */
long lrs_leaf(lrs_dic *P, lrs_dat *Q);                    /* true if current dictionary is leaf of reverse search tree  */
void update_R(lrs_dic *P, lrs_dat *Q, lrs_restart_dat *R);

/***************************************************/
/* Routines for redundancy checking                */
/***************************************************/
long checkredund (lrs_dic * P, lrs_dat * Q);/* solve primal lp to check redund of obj fun. returns TRUE if redundant, else FALSE          */
long checkcobasic (lrs_dic * P, lrs_dat * Q, long index); /* TRUE if index is cobasic and nondegenerate  FALSE if basic, or degen. cobasic, where it will get pivoted out  */
long checkindex (lrs_dic * P, lrs_dat * Q, long index); /* index=0 non-red.,1 red., 2 input linearity NOTE: row is returned all zero if redundant!!  */


/***************************************************/
/* Routines for caching and restoring dictionaries */
/***************************************************/
void lrs_free_dic ( lrs_dic *P, lrs_dat *Q);
void lrs_free_dic2 ( lrs_dic *P, lrs_dat *Q);  /* same as lrs_free_dic but no cache*/
void lrs_free_dat ( lrs_dat *Q);
void copy_dict (lrs_dat * global, lrs_dic * dest, lrs_dic * src);
lrs_dic *alloc_memory (lrs_dat * Q);
lrs_dic * lrs_getdic(lrs_dat *Q);
lrs_dic *resize (lrs_dic * P, lrs_dat * Q);
void lrs_free_all_memory(lrs_dic * P, lrs_dat * Q);

/*******************************/
/* utilities                   */
/*******************************/
void lprat (const char *name, long Num, long Den);   /* Print Num/Den without reducing  */
long lreadrat (long *Num, long *Den);   /* read a rational string and convert to long integers            */
void reorder (long a[], long range);	/* reorder array in increasing order with one misplaced element   */
void reorder1 (long a[], long b[], long newone, long range); /* reorder array a in increasing order with misplaced element newone elements of b go along for the ride */

/***************************/
/* lp_solve like functions */
/***************************/
long lrs_solve_lp(lrs_dic *P, lrs_dat *Q);/* solve lp only for given dictionary */
void lrs_set_row(lrs_dic *P, lrs_dat *Q, long row, long num[], long den[], long ineq);/* load row i of dictionary from num[]/den[] ineq=GE       */ 
void lrs_set_row_mp(lrs_dic *P, lrs_dat *Q, long row, lrs_mp_vector num, lrs_mp_vector den, long ineq);/* same as lrs_set_row except num/den is lrs_mp type       */
void lrs_set_obj(lrs_dic *P, lrs_dat *Q, long num[], long den[], long max); /* set up objective function with coeffs num[]/den[] max=MAXIMIZE or MINIMIZE  */
void lrs_set_obj_mp(lrs_dic *P, lrs_dat *Q, lrs_mp_vector num, lrs_mp_vector den, long max);/* same as lrs_set_obj but num/den has lrs_mp type */

