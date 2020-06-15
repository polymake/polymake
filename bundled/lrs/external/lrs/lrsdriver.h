/* This file contains functions and variables that should not be duplicated per arithmetic */

#ifndef LRS_DRIVER_H
#define LRS_DRIVER_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lrsrestart.h"


struct lrs_dic_struct;
typedef struct lrs_dic_struct lrs_dic;

struct lrs_dat;
typedef struct lrs_dat lrs_dat;



long lrs_main (int argc, char *argv[]);    /* legacy lrs driver, argv[1]=input file, [argc-1]=output file */

long lrs1_main(int argc, char *argv[],lrs_dic **P,lrs_dat **Q, long overf,long stage,char *tmp,lrs_restart_dat *R)/*__attribute__ ((visibility ("default") ))*/;
long lrs2_main(int argc, char *argv[],lrs_dic **P,lrs_dat **Q, long overf,long stage,char *tmp,lrs_restart_dat *R)/*__attribute__ ((visibility ("default") ))*/;
long lrsgmp_main(int argc, char *argv[],lrs_dic **P,lrs_dat **Q, long overf,long stage,char *tmp,lrs_restart_dat *R)/*__attribute__ ((visibility ("default") ))*/;

char** makenewargv(int *argc,char** argv,char* tmp);
lrs_restart_dat* lrs_alloc_restart();


extern FILE *lrs_cfp;			/* output file for checkpoint information       */
extern FILE *lrs_ifp;			/* input file pointer       */
extern FILE *lrs_ofp;			/* output file pointer      */

#endif
