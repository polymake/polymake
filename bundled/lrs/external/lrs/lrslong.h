/* lrslong.h      (lrs long integer arithmetic library              */
/* Copyright: David Avis 2000, avis@cs.mcgill.ca                    */
/* Version 4.0, February 17, 2000                                   */

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
/******************************************************************************/
/*  See http://cgm.cs.mcgill.ca/~avis/C/lrs.html for lrs usage instructions   */
/******************************************************************************/
/* This package contains the extended precision routines used by lrs
   and some other miscellaneous routines. The maximum precision depends on
   the parameter MAX_DIGITS defined below, with usual default of 255L. This
   gives a maximum of 1020 decimal digits on 32 bit machines. The procedure
   lrs_mp_init(dec_digits) may set a smaller number of dec_digits, and this
   is useful if arrays or matrices will be used.
 */


/*
#ifdef PLRS
#include <string>
using namespace std;
#endif
*/

/***********/
/* defines */
/***********/
/*
   this is number of longwords. Increasing this won't cost you that much
   since only variables other than the A matrix are allocated this size.
   Changing affects running time in small but not very predictable ways.
 */

#define MAX_DIGITS 255L

/*
   this is in decimal digits, you pay in memory if you increase this,
   unless you override by a line with
   digits n
   before the begin line of your file.
 */
#define DEFAULT_DIGITS 100L



/**********MACHINE DEPENDENT CONSTANTS***********/
/* MAXD is 2^(k-1)-1 where k is word size       */
/* MAXDm is 2^(k/2-1)-1 where k is word size    */
/* MAXDa is 2^(k-2)-1 where k is word size      */
/* MAXD must be at least 2*BASE^2               */
/* If BASE is 10^k, use "%k.ku" for FORMAT      */
/* INTSIZE is number of bytes for integer       */
/* 64/128 bit arithmetic                        */
/************************************************/
#ifdef B128
/* 128 bit machines */                   /* compiler does not accept big constants! */
#define MAXD 9223372036854775807L        /* should be 2^127 -1 but is  2^63 - 1 */
#define MAXDm 9223372036854775807L       /* 2^63 - 1 */
#define MAXDa 9223372036854775807L        /* should be 2^126 -1 but is  2^63 - 1 */
/* max power of 10 fitting in signed int64 */
#define P10_INT64  1000000000000000000ULL

#define BASE 1000000000L
#define FORMAT "%9.9u"
#define BASE_DIG 9
#define INTSIZE 16L
#define BIT "128bit"
#else
/* 64 bit machines */
#define MAXD 9223372036854775807LL        /* 2^63 - 1 */
#define MAXDm 2147483647LL                /* 2^31 - 1 */
#define MAXDa 4611686018427387904LL       /* 2^62 - 1 */
#define BASE 1000000000L
#define FORMAT "%9.9u"
#define BASE_DIG 9
#define INTSIZE 16L
#define BIT "64bit"
#endif

#define MAXINPUT 1000		/*max length of any input rational */

#define POS 1L
#define NEG -1L
#ifndef TRUE
#define TRUE 1L
#endif
#ifndef FALSE
#define FALSE 0L
#endif
#define ONE 1L
#define TWO 2L
#define ZERO 0L

/**********************************/
/*         MACROS                 */
/* dependent on mp implementation */
/**********************************/

#ifdef SAFE
/* lazy but fast overflow checking */

#define mpsafem(a,b)             *(a)>MAXDm||*(b)>MAXDm||*(a)<-MAXDm||*(b)<-MAXDm
#define mpsafea(a,b)             *(a)>MAXDa||*(b)>MAXDa||*(a)<-MAXDa||*(b)<-MAXDa

#ifdef DEBUG
#define mperrorm(a,b)            fprintf(stdout,"  : max(|a|,|b|) > %ld\n",MAXDa);lrs_overflow(1)
#define mperrora(a,b)            fprintf(stdout,"  : max(|a|,|b|) > %ld\n",MAXDa);lrs_overflow(1)
#define linint(a, ka, b, kb)    if( mpsafem(a,b) ) {fprintf(stdout, "\n*linint ");mperrorm(a,b);}  else *(a) = *(a) * ka + *(b) * kb
#define mulint(a, b, c)         if( mpsafem(a,b) ) {fprintf(stdout, "\n*mulint ");mperrorm(a,b);}  else *(c) = *(a) * *(b)
#define addint(a, b, c)         if( mpsafea(a,b) ) {fprintf(stdout, "\n*addint ");mperrora(a,b);}  else *(c) = *(a) + *(b)
#define subint(a, b, c)         if( mpsafea(a,b) ) {fprintf(stdout, "\n*subint ");mperrora(a,b);}  else *(c) = *(a) - *(b)
#define decint(a, b)            if( mpsafea(a,b) ) {fprintf(stdout, "\n*decint ");mperrora(a,b);}  else *(a) = *(a) - *(b)
#else
#define linint(a, ka, b, kb)    if( mpsafem(a,b) ) lrs_overflow(1) ; else *(a) = *(a) * ka + *(b) * kb
#define mulint(a, b, c)         if( mpsafem(a,b) ) lrs_overflow(1) ; else *(c) = *(a) * *(b)
#define addint(a, b, c)         if( mpsafea(a,b) ) lrs_overflow(1) ; else *(c) = *(a) + *(b)
#define subint(a, b, c)         if( mpsafea(a,b) ) lrs_overflow(1) ; else *(c) = *(a) - *(b)
#define decint(a, b)            if( mpsafea(a,b) ) lrs_overflow(1) ; else *(a) = *(a) - *(b)
#endif

#else
/* unprotected routines */
#define addint(a, b, c)         *(c) = *(a) + *(b)
#define subint(a, b, c)         *(c) = *(a) - *(b)
#define linint(a, ka, b, kb)    *(a) = *(a) * ka + *(b) * kb
#define mulint(a, b, c)         *(c) = *(a) * *(b)
#endif

#define unchecked_decint(a, b)  *(a) = *(a) - *(b)  /* only safe if a,b come from mulint */
#define divint(a, b, c)         *(c) = *(a) / *(b); *(a) = *(a) % *(b)
#define exactdivint(a,b,c) 	*(c) = *(a) / *(b);

#define abs128(a)		(a>0? a : -1*a)
#define changesign(a)           (*(a) = - *(a))
#define copy(a, b)              ((a)[0] = (b)[0])
#define mp_greater(a, b)           (*(a) > *(b) )
#define itomp(in, a)             *(a) =  in 


#define one(a)                  (*(a) == 1)
#define negative(a)             (*(a) < 0)
#define normalize(a)            (void) 0
#define positive(a)             (*(a) > 0)
#define sign(a)                 (*(a) < 0 ? NEG : POS)
#ifndef B128
 #define storesign(a, sa)        (*(a) = labs(*(a)) * sa)
#else
 #define storesign(a, sa)	(*(a) = abs128(*(a)) * sa)
#endif
#define zero(a)                 (*(a) == 0)


/*
 *  convert between decimal and machine (longword digits). Notice lovely
 *  implementation of ceiling function :-)
 */
#define DEC2DIG(d) ( (d) % BASE_DIG ? (d)/BASE_DIG+1 : (d)/BASE_DIG)
#define DIG2DEC(d) ((d)*BASE_DIG)

#ifndef OMIT_SIGNALS
#include <signal.h>
#include <stdlib.h>		/* labs */
#include <unistd.h>
#define errcheck(s,e) if ((long)(e)==-1L){  perror(s);exit(1);}
#endif

#define CALLOC(n,s) xcalloc(n,s,__LINE__,__FILE__)

/*************/
/* typedefs  */
/*************/
#ifndef B128
typedef long long lrs_mp[1];		/* type lrs_mp holds one long integer */
typedef long long *lrs_mp_t;
typedef long long **lrs_mp_vector;
typedef long long ***lrs_mp_matrix;
#else
typedef __int128 lrs_mp[1];	/* type lrs_mp holds one 128-bit integer */
typedef __int128 *lrs_mp_t;
typedef __int128 **lrs_mp_vector;
typedef __int128 ***lrs_mp_matrix;
#endif

/*********************/
/*global variables   */
/*********************/

extern long lrs_digits;		/* max permitted no. of digits   */
extern long lrs_record_digits;		/* this is the biggest acheived so far.     */

extern FILE *lrs_ifp;			/* input file pointer       */
extern FILE *lrs_ofp;			/* output file pointer      */

/*********************************************************/
/* Initialization and allocation procedures - must use!  */
/******************************************************* */

//void mulint(lrs_mp a, lrs_mp b, lrs_mp c);

long lrs_mp_init (long dec_digits, FILE * lrs_ifp, FILE * lrs_ofp);	/* max number of decimal digits, fps   */

#define lrs_alloc_mp(a)	
#define lrs_clear_mp(a)

lrs_mp_t lrs_alloc_mp_t();                      /* dynamic allocation of lrs_mp                  */
lrs_mp_vector lrs_alloc_mp_vector (long n);	/* allocate lrs_mp_vector for n+1 lrs_mp numbers */
lrs_mp_matrix lrs_alloc_mp_matrix (long m, long n);	/* allocate lrs_mp_matrix for m+1 x n+1 lrs_mp   */

void lrs_clear_mp_vector (lrs_mp_vector a, long n);
void lrs_clear_mp_matrix (lrs_mp_matrix a, long m, long n);

#ifndef MA
#define suf(func) func
#endif

#ifdef MA
#ifdef B128
#define suf(func) func##_2
#else
#define suf(func) func##_1
#endif
#endif

#define atoaa suf(atoaa)
#define atomp suf(atomp)
#define comprod suf(comprod)
#define divrat suf(divrat)
#define gcd suf(gcd)
#define getfactorial suf(getfactorial)
#define lcm suf(lcm)
#define linrat suf(linrat)
#define lrs_alloc_mp_matrix suf(lrs_alloc_mp_matrix)
#define lrs_alloc_mp_t suf(lrs_alloc_mp_t)
#define lrs_alloc_mp_vector suf(lrs_alloc_mp_vector)
#define lrs_clear_mp_matrix suf(lrs_clear_mp_matrix)
#define lrs_clear_mp_vector suf(lrs_clear_mp_vector)
#define lrs_digits suf(lrs_digits)
#define lrs_getdigits suf(lrs_getdigits)
#define lrs_overflow suf(lrs_overflow)
#define lrs_mp_init suf(lrs_mp_init)
#define lrs_record_digits suf(lrs_record_digits)
#define mptodouble suf(mptodouble)
#define mptoi suf(mptoi)
#define mpgetstr10 suf(mpgetstr10)
#define mulrat suf(mulrat)
#define myrandom suf(myrandom)
#define notimpl suf(notimpl)
#define pmp suf(pmp)
#define prat suf(prat)
#define cprat suf(cprat)
#define cpmp suf(cpmp)
#define rattodouble suf(rattodouble)
#define readmp suf(readmp)
#define readrat suf(readrat)
#define plrs_readrat suf(plrs_readrat)
#define reduce suf(reduce)
#define reducearray suf(reducearray)
#define reduceint suf(reduceint)
#define stringcpy suf(stringcpy)
#define xcalloc suf(xcalloc)

/*********************************************************/
/* Core library functions - depend on mp implementation  */
/******************************************************* */

void atomp (const char s[], lrs_mp a);	/* convert string to lrs_mp integer               */
long compare (lrs_mp a, lrs_mp b);	/* a ? b and returns -1,0,1 for <,=,> */
void gcd (lrs_mp u, lrs_mp v);	/* returns u=gcd(u,v) destroying v                */
void mptodouble (lrs_mp a, double *x);	/* convert lrs_mp to double                       */
long mptoi (lrs_mp a);		/* convert lrs_mp to long integer */
char *mpgetstr10(char *, lrs_mp); /* convert lrs_mp to string */
#ifdef PLRS
long plrs_readrat (lrs_mp Na, lrs_mp Da, const char * rat);	/* take a rational number and convert to lrs_mp   */
#endif
char *cprat(const char *name, lrs_mp Nt, lrs_mp Dt); /* mp rat to char  */
char *cpmp(const char *name, lrs_mp Nt);             /* mp int to char  */
void pmp (const char *name, lrs_mp a);	/* print the long precision integer a             */
void prat (const char *name, lrs_mp Nt, lrs_mp Dt);	/* reduce and print  Nt/Dt                        */
void readmp (lrs_mp a);		/* read an integer and convert to lrs_mp          */
long readrat (lrs_mp Na, lrs_mp Da);	/* read a rational or int and convert to lrs_mp   */
void reduce (lrs_mp Na, lrs_mp Da);	/* reduces Na Da by gcd(Na,Da)                    */

/*********************************************************/
/* Standard arithmetic & misc. functions                 */
/* should be independent of mp implementation            */
/******************************************************* */

void atoaa (const char in[], char num[], char den[]);	/* convert rational string in to num/den strings  */
long atos (char s[]);		/* convert s to integer                           */
long comprod (lrs_mp Na, lrs_mp Nb, lrs_mp Nc, lrs_mp Nd);	/* +1 if Na*Nb > Nc*Nd,-1 if Na*Nb > Nc*Nd else 0 */
void divrat (lrs_mp Na, lrs_mp Da, lrs_mp Nb, lrs_mp Db, lrs_mp Nc, lrs_mp Dc);
						       /* computes Nc/Dc = (Na/Da) /( Nb/Db ) and reduce */
void getfactorial (lrs_mp factorial, long k);	/* compute k factorial in lrs_mp                  */
void linrat (lrs_mp Na, lrs_mp Da, long ka, lrs_mp Nb, lrs_mp Db, long kb, lrs_mp Nc, lrs_mp Dc);
void lcm (lrs_mp a, lrs_mp b);	/* a = least common multiple of a, b; b is saved  */
void mulrat (lrs_mp Na, lrs_mp Da, lrs_mp Nb, lrs_mp Db, lrs_mp Nc, lrs_mp Dc);
						       /* computes Nc/Dc=(Na/Da)*(Nb/Db) and reduce      */
long myrandom (long num, long nrange);	/* return a random number in range 0..nrange-1    */
void notimpl (const char *s);	/* bail out - help!                               */
void rattodouble (lrs_mp a, lrs_mp b, double *x);	/* convert lrs_mp rational to double              */
void reduceint (lrs_mp Na, lrs_mp Da);	/* divide Na by Da and return it                  */
void reducearray (lrs_mp_vector p, long n);	/* find gcd of p[0]..p[n-1] and divide through by */
void scalerat (lrs_mp Na, lrs_mp Da, long ka);	/* scales rational by ka                          */

/********************************/
/* Matrix and vector allocation */
/********************************/
void lrs_clear_mp_vector (lrs_mp_vector p, long n);
lrs_mp_vector lrs_alloc_mp_vector(long n);
void lrs_clear_mp_vector(lrs_mp_vector p, long n);
lrs_mp_matrix lrs_alloc_mp_matrix(long m, long n);
void lrs_clear_mp_matrix(lrs_mp_matrix p, long m, long n);
long lrs_mp_init(long dec_digits, FILE *fpin, FILE *fpout);


/* how big are numbers? */
extern long lrs_digits;		/* max permitted no. of digits   */
extern long lrs_record_digits;	/* this is the biggest achieved so far. */

/**********************************/
/* Miscellaneous functions        */
/******************************** */

void lrs_getdigits (long *a, long *b);	/* send digit information to user                         */

void stringcpy (char *s, char *t);	/* copy t to s pointer version                            */

void *calloc ();
void *malloc ();
void *xcalloc (long n, long s, long l, const char *f);

void lrs_default_digits_overflow ();
void lrs_exit(int i);   
void lrs_overflow(int i);   
/* end of  lrs_long.h (vertex enumeration using lexicographic reverse search) */
