/* This file contains functions and variables that should not be duplicated per arithmetic */

#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include <stdlib.h>
#include <limits.h>
#include "lrsdriver.h"

/* Globals; these need to be here, rather than lrsdriver.h, so they are
   not multiply defined. */

FILE *lrs_cfp;			/* output file for checkpoint information       */
FILE *lrs_ifp;			/* input file pointer       */
FILE *lrs_ofp;			/* output file pointer      */

char** makenewargv(int *argc,char** argv,char *tmp)
{
  int i;
  char** newargv;

  newargv = (char**) malloc((*argc+3) * sizeof *newargv);
  for(i = 0; i < *argc; ++i)
    {
      if (i != 1)
       {
        size_t length = strlen(argv[i])+1;
        newargv[i] = (char *) malloc(length);
        strncpy(newargv[i], argv[i], length);
       }
    }
/* make tmp the new input file */
   size_t length = strlen(tmp)+1;
   newargv[1] = (char *)malloc(length);
   strncpy(newargv[1], tmp, length);
   if(*argc == 1)         /* input was stdin*/
       *argc = 2;
   newargv[*argc] = NULL;
   return newargv;
}


lrs_restart_dat*
lrs_alloc_restart()
{
  int i;

  lrs_restart_dat *R;

  R = (lrs_restart_dat *) malloc (sizeof (lrs_restart_dat));
  if (R == NULL)
    return R;  
  
  R->overide=0;     /* do not overide Q */
  R->restart=0;     /* do not do a restart */
  R->facet=NULL;    /* this will be allocated later when we know its size */
  R->d=0;
  R->maxcobases=0;
  R->maxdepth=-1;  /* will be set to MAXD in lrs*_main */
  R->mindepth=0;
  R->maxcobases=0;
  for(i=0;i<10;i++)
    R->count[i]=0;
  R->depth=0;
  R->lrs=1;
  R->redund=0;
  R->verifyredund=0;
  R->redineq = NULL;

  return R;
}

