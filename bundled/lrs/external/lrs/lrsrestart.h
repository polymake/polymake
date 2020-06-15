typedef struct lrs_restart_dat   /* for restarting from a given cobasis         */
{
        long *facet;            /* cobasic indices for restart                  */

        long overide;           /* TRUE if Q parameters should be updated       */
        long restart;           /* TRUE if we supply restart cobasis            */
        long lrs;               /* TRUE if we are doing a lrs run               */
        long m;                 /* number of input rows                         */
        long d;                 /* number of cobasic indices                    */
        long count[10];         /* count[0]=rays(facets) [1]=verts. [2]=base [3]=pivots */
                                /* count[4]=integer vertices [5]=1 for hull     */
                                /* [6]=nlinearities [7]=deepest                 */
        long depth;             /* depth of restart node                        */
        long maxcobases;        /* if positive, after maxcobasis unexplored subtrees reported */
        long long maxdepth;     /* max depth to search to in treee              */
        long long mindepth;     /* do not backtrack above mindepth              */

        long redund;            /* TRUE if we are doing a redund run            */
        long verifyredund;      /* a worker checks redundancy and gives output  */
        long messages;          /* TRUE if lrs should post_output messages      */
        long *redineq;          /* a list of row numbers to check redundancy    */
} lrs_restart_dat;

lrs_restart_dat* lrs_alloc_restart();


