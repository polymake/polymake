#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include <stdlib.h>
#include <limits.h>
#include "lrsdriver.h"


int
main (int argc, char *argv[])

{
#ifndef MA
  lrs_main(argc,argv);    /* legacy lrs */
  return 0;
#endif

/* hybrid arithmetic version of lrs   */

  lrs_restart_dat *R;
  lrs_dic *P;
  lrs_dat *Q;
  char* tmp;          /* when overflow occurs a new input file name is returned */

  char** newargv;    
  long overfl=0;     /*  =0 no overflow =1 restart overwrite =2 restart append */
  long overfl2=0;    /*  for B128 */
  long b128=0;   /* =1 if _int128 available */
  int lrs_stdin=0;
  int i;

#ifdef B128
  b128=1;
#endif

  P=NULL;
  Q=NULL;
  tmp=NULL;

  R = lrs_alloc_restart();
  if (R == NULL)
    exit(1);   


  if(argc == 1)
     lrs_stdin=1;

  tmp = malloc(PATH_MAX * sizeof (char));

  if ( (overfl=lrs1_main(argc,argv,&P,&Q,0,0,tmp,R)) == 0)     /* set up, read input, no run   */    
     if ( (overfl=lrs1_main(argc,argv,&P,&Q,0,1,tmp,R)) == 0)  /* run reverse search           */  
        if ( (overfl=lrs1_main(argc,argv,&P,&Q,0,2,tmp,R)) == 0) /* free memory and close      */
          goto byebye;

   if (overfl==-1)  /* unrecoverable input error */
     {
       printf("\n");
       exit(1);
     }

/* overflow condition triggered: a temporary file was created for restart */
/* create new argv for the remaining calls                                */

  newargv = makenewargv(&argc,argv,tmp);

  if(b128)
    {
     fprintf(stderr,"\n*lrs:overflow possible: restarting with 128 bit arithmetic\n");

     if  ( (overfl2=lrs2_main(argc,newargv,&P,&Q,overfl,0,tmp,R)) == 0)    
       if ( (overfl2=lrs2_main(argc,newargv,&P,&Q,overfl,1,tmp,R)) == 0)
        if ( (overfl2=lrs2_main(argc,newargv,&P,&Q,overfl,2,tmp,R)) == 0)
           goto done;
     overfl=overfl2;
    }


/* if you change tmp file name update newargv[1] */

  fprintf(stderr,"\n*lrs:overflow possible: restarting with GMP arithmetic\n");

  lrsgmp_main(argc,newargv,&P,&Q,overfl,0,tmp,R);                                      
  lrsgmp_main(argc,newargv,&P,&Q,overfl,1,tmp,R);  
  lrsgmp_main(argc,newargv,&P,&Q,overfl,2,tmp,R);    

done:
  for(i = 0; i < argc; ++i)
        free(newargv[i]);
  free(newargv);
  
byebye:
  free(R->redineq);
  free(R->facet);
  free(R);
  fprintf(stderr,"\n");
  if(lrs_stdin==1)    /* get rid of temporary file for stdin */
    remove(tmp);
  free(tmp);
  return 0;

} /* lrs.c */

