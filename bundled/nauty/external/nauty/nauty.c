/*****************************************************************************
*                                                                            *
*  Main source file for version 2.2 of nauty.                                *
*                                                                            *
*   Copyright (1984-2004) Brendan McKay.  All rights reserved.  Permission   *
*   Subject to the waivers and disclaimers in nauty.h.                       *
*                                                                            *
*   CHANGE HISTORY                                                           *
*       10-Nov-87 : final changes for version 1.2                            *
*        5-Dec-87 : renamed to version 1.3 (no changes to this file)         *
*       28-Sep-88 : renamed to version 1.4 (no changes to this file)         *
*       23-Mar-89 : changes for version 1.5 :                                *
*                   - add use of refine1 instead of refine for m==1          *
*                   - changes for new optionblk syntax                       *
*                   - disable tc_level use for digraphs                      *
*                   - interposed doref() interface to refine() so that       *
*                        options.invarproc can be supported                  *
*                   - declared local routines static                         *
*       28-Mar-89 : - implemented mininvarlevel/maxinvarlevel < 0 options    *
*        2-Apr-89 : - added invarproc fields in stats                        *
*        5-Apr-89 : - modified error returns from nauty()                    *
*                   - added error message to ERRFILE                         *
*                   - changed MAKEEMPTY uses to EMPTYSET                     *
*       18-Apr-89 : - added MTOOBIG and CANONGNIL                            *
*        8-May-89 : - changed firstcode[] and canoncode[] to short           *
*       10-Nov-90 : changes for version 1.6 :                                *
*                   - added dummy routine nauty_null (see dreadnaut.c)       *
*        2-Sep-91 : changes for version 1.7 :                                *
*                   - moved MULTIPLY into nauty.h                            *
*       27-Mar-92 : - changed 'n' into 'm' in error message in nauty()       *
*        5-Jun-93 : renamed to version 1.7+ (no changes to this file)        *
*       18-Aug-93 : renamed to version 1.8 (no changes to this file)         *
*       17-Sep-93 : renamed to version 1.9 (no changes to this file)         *
*       13-Jul-96 : changes for version 2.0 :                                *
*                   - added dynamic allocation                               *
*       21-Oct-98 : - made short into shortish for BIGNAUTY as needed        *
*        7-Jan-00 : - allowed n=0                                            *
*                   - added nauty_check() and a call to it                   *
*       12-Feb-00 : - used better method for target cell memory allocation   *
*                   - did a little formating of the code                     *
*       27-May-00 : - fixed error introduced on Feb 12.                      *
*                   - dynamic allocations in nauty() are now deallocated     *
*                     before return if n >= 320.                             *
*       16-Nov-00 : - use function prototypes, change UPROC to void.         *
*                   - added argument to tcellproc(), removed nvector         *
*                   - now use options.dispatch, options.groupopts is gone.   *
*       22-Apr-01 : - Added code for compilation into Magma                  *
*                   - Removed nauty_null() and EXTDEFS                       *
*        2-Oct-01 : - Improved error message for bad dispatch vector         *
*       21-Nov-01 : - use NAUTYREQUIRED in nauty_check()                     *
*       20-Dec-02 : changes for version 2.2:                                 *
*                   - made tcnode0 global                                    *
*                   - added nauty_freedyn()                                  *
*       17-Nov-03 : changed INFINITY to NAUTY_INFINITY                       *
*       14-Sep-04 : extended prototypes even to recursive functions          *
*       16-Oct-04 : disallow NULL dispatch vector                            *
*                                                                            *
*****************************************************************************/

#define ONE_WORD_SETS
#include "nauty.h"

#ifdef NAUTY_IN_MAGMA
#include "cleanup.e"
#define NAUTY_ABORT (-11)
#endif

typedef struct tcnode_struct
{
	struct tcnode_struct *next;
	set *tcellptr;
} tcnode;

/* aproto: header new_nauty_protos.h */

#ifndef NAUTY_IN_MAGMA
#if !MAXN
static int firstpathnode0(int*, int*, int, int, tcnode*);
static int othernode0(int*, int*, int, int, tcnode*);
#else
static int firstpathnode(int*, int*, int, int);
static int othernode(int*, int*, int, int);
#endif
static void firstterminal(int*, register int);
static int processnode(int*, int*, int, int);
static void recover(register int*, register int);
static void writemarker(int, int, int, int, int, int);
#endif

#if  MAXM==1
#define M 1
#else
#define M m
#endif

#define OPTCALL(proc) if (proc != NULL) (*proc)

    /* copies of some of the options: */
static boolean getcanon,digraph,writeautoms,domarkers,cartesian;
static int linelength,tc_level,mininvarlevel,maxinvarlevel,invararg;
static void (*usernodeproc)(graph*,int*,int*,int,int,int,int,int,int);
static void (*userautomproc)(int,permutation*,int*,int,int,int);
static void (*userlevelproc)
              (int*,int*,int,int*,statsblk*,int,int,int,int,int,int);
static void (*tcellproc)(graph*,int*,int*,int,int,set*,int*,int*,int,int,
                        int(*)(graph*,int*,int*,int,int,int,int),int,int);
static void (*invarproc)
	      (graph*,int*,int*,int,int,int,permutation*,int,boolean,int,int);
static FILE *outfile;
static dispatchvec dispatch;

    /* local versions of some of the arguments: */
static int m,n;
static graph *g,*canong;
static int *orbits;
static statsblk *stats;
    /* temporary versions of some stats: */
static long invapplics,invsuccesses;
static int invarsuclevel;

    /* working variables: <the "bsf leaf" is the leaf which is best guess so
                                far at the canonical leaf>  */
static int gca_first,     /* level of greatest common ancestor of current
                                node and first leaf */
           gca_canon,     /* ditto for current node and bsf leaf */
           noncheaplevel, /* level of greatest ancestor for which cheapautom
                                == FALSE */
           allsamelevel,  /* level of least ancestor of first leaf for
                             which all descendant leaves are known to be
                             equivalent */
           eqlev_first,   /* level to which codes for this node match those
                                for first leaf */
           eqlev_canon,   /* level to which codes for this node match those
                                for the bsf leaf. */
           comp_canon,    /* -1,0,1 according as code at eqlev_canon+1 is
                                <,==,> that for bsf leaf.  Also used for
                                similar purpose during leaf processing */
           samerows,      /* number of rows of canong which are correct for
                                the bsf leaf  BDM:correct description? */
           canonlevel,    /* level of bsf leaf */
           stabvertex,    /* point fixed in ancestor of first leaf at level
                                gca_canon */
           cosetindex;    /* the point being fixed at level gca_first */

static boolean needshortprune;       /* used to flag calls to shortprune */

#if !MAXN
DYNALLSTAT(set,defltwork,defltwork_sz);
DYNALLSTAT(permutation,workperm,workperm_sz);
DYNALLSTAT(set,fixedpts,fixedpts_sz);
DYNALLSTAT(permutation,firstlab,firstlab_sz);
DYNALLSTAT(permutation,canonlab,canonlab_sz);
DYNALLSTAT(short,firstcode,firstcode_sz);
DYNALLSTAT(short,canoncode,canoncode_sz);
DYNALLSTAT(shortish,firsttc,firsttc_sz);
DYNALLSTAT(set,active,active_sz);

/* In the dynamically allocated case (MAXN=0), each level of recursion
   needs one set (tcell) to represent the target cell.  This is 
   implemented by using a linked list of tcnode anchored at the root
   of the search tree.  Each node points to its child (if any) and to
   the dynamically allocated tcell.  Apart from the the first node of
   the list, each node always has a tcell good for m up to alloc_m.
   tcnodes and tcells are kept between calls to nauty, except that
   they are freed and reallocated if m gets bigger than alloc_m.  */

static tcnode tcnode0 = {NULL,NULL};
static int alloc_m = 0;

#else
static set defltwork[2*MAXM];        /* workspace in case none provided */
static permutation workperm[MAXN];   /* various scratch uses */
static set fixedpts[MAXM];           /* points which were explicitly
                                        fixed to get current node */
static permutation firstlab[MAXN],   /* label from first leaf */
                   canonlab[MAXN];   /* label from bsf leaf */
static short firstcode[MAXN+2],      /* codes for first leaf */
             canoncode[MAXN+2];      /* codes for bsf leaf */
static shortish firsttc[MAXN+2];     /* index of target cell for left path */
static set active[MAXM];             /* used to contain index to cells now
                                        active for refinement purposes */
#endif

static set *workspace,*worktop;      /* first and just-after-last addresses of
                                        work area to hold automorphism data */
static set *fmptr;                   /* pointer into workspace */


/*****************************************************************************
*                                                                            *
*  This procedure finds generators for the automorphism group of a           *
*  vertex-coloured graph and optionally finds a canonically labelled         *
*  isomorph.  A description of the data structures can be found in           *
*  nauty.h and in the "nauty User's Guide".  The Guide also gives            *
*  many more details about its use, and implementation notes.                *
*                                                                            *
*  Parameters - <r> means read-only, <w> means write-only, <wr> means both:  *
*           g <r>  - the graph                                               *
*     lab,ptn <rw> - used for the partition nest which defines the colouring *
*                  of g.  The initial colouring will be set by the program,  *
*                  using the same colour for every vertex, if                *
*                  options->defaultptn!=FALSE.  Otherwise, you must set it   *
*                  yourself (see the Guide). If options->getcanon!=FALSE,    *
*                  the contents of lab on return give the labelling of g     *
*                  corresponding to canong.  This does not change the        *
*                  initial colouring of g as defined by (lab,ptn), since     *
*                  the labelling is consistent with the colouring.           *
*     active  <r>  - If this is not NULL and options->defaultptn==FALSE,     *
*                  it is a set indicating the initial set of active colours. *
*                  See the Guide for details.                                *
*     orbits  <w>  - On return, orbits[i] contains the number of the         *
*                  least-numbered vertex in the same orbit as i, for         *
*                  i=0,1,...,n-1.                                            *
*    options  <r>  - A list of options.  See nauty.h and/or the Guide        *
*                  for details.                                              *
*      stats  <w>  - A list of statistics produced by the procedure.  See    *
*                  nauty.h and/or the Guide for details.                     *
*  workspace  <w>  - A chunk of memory for working storage.                  *
*  worksize   <r>  - The number of setwords in workspace.  See the Guide     *
*                  for guidance.                                             *
*          m  <r>  - The number of setwords in sets.  This must be at        *
*                  least ceil(n / WORDSIZE) and at most MAXM.                *
*          n  <r>  - The number of vertices.  This must be at least 1 and    *
*                  at most MAXN.                                             *
*     canong  <w>  - The canononically labelled isomorph of g.  This is      *
*                  only produced if options->getcanon!=FALSE, and can be     *
*                  given as NULL otherwise.                                  *
*                                                                            *
*  FUNCTIONS CALLED: firstpathnode(),updatecan()                             *
*                                                                            *
*****************************************************************************/

void
nauty(graph *g_arg, int *lab, int *ptn, set *active_arg,
      int *orbits_arg, optionblk *options, statsblk *stats_arg,
      set *ws_arg, int worksize, int m_arg, int n_arg, graph *canong_arg)
{
        register int i;
        int numcells;
	int retval;
#if !MAXN
	tcnode *tcp,*tcq;
#endif

    /* determine dispatch vector */

	if (options->dispatch == NULL)
	{
            fprintf(ERRFILE,">E nauty: null dispatch vector\n");
	    fprintf(ERRFILE,"Maybe you need to recompile\n");
            exit(1);
        }
	else
	    dispatch = *(options->dispatch);

	if (options->userrefproc) 
	    dispatch.refine = options->userrefproc;
	else if (dispatch.refine1 && m_arg == 1)
            dispatch.refine = dispatch.refine1;

	if (dispatch.refine == NULL || dispatch.updatecan == NULL
		|| dispatch.bestcell == NULL || dispatch.cheapautom == NULL)
	{
	    fprintf(ERRFILE,">E bad dispatch vector\n");
	    exit(1);
	}

	if (options->usertcellproc) tcellproc = options->usertcellproc;
        else                        tcellproc = targetcell;
 
    /* check for excessive sizes: */

#if !MAXN
        if (m_arg > NAUTY_INFINITY/WORDSIZE+1)
        {
            stats_arg->errstatus = MTOOBIG;
            fprintf(ERRFILE,"nauty: need m <= %d, but m=%d\n\n",
		    NAUTY_INFINITY/WORDSIZE+1,m_arg);
            return;
        }
        if (n_arg > NAUTY_INFINITY-2 || n_arg > WORDSIZE * m_arg)
        {
            stats_arg->errstatus = NTOOBIG;
            fprintf(ERRFILE,"nauty: need n <= min(%d,%d*m), but n=%d\n\n",
		    NAUTY_INFINITY-2,WORDSIZE,n_arg);
            return;
        }
#else
        if (m_arg > MAXM)
        {
            stats_arg->errstatus = MTOOBIG;
            fprintf(ERRFILE,"nauty: need m <= %d\n\n",MAXM);
            return;
        }
        if (n_arg > MAXN || n_arg > WORDSIZE * m_arg)
        {
            stats_arg->errstatus = NTOOBIG;
            fprintf(ERRFILE,
                    "nauty: need n <= min(%d,%d*m)\n\n",MAXM,WORDSIZE);
            return;
        }
#endif
	if (n_arg == 0)   /* Special code for Wendy */
	{
	    stats_arg->grpsize1 = 1.0;
	    stats_arg->grpsize2 = 0;
	    stats_arg->numorbits = 0;
	    stats_arg->numgenerators = 0;
	    stats_arg->errstatus = 0;
	    stats_arg->numnodes = 1;
	    stats_arg->numbadleaves = 0;
	    stats_arg->maxlevel = 1;
	    stats_arg->tctotal = 0;
	    stats_arg->canupdates = (options->getcanon != 0);
	    stats_arg->invapplics = 0;
	    stats_arg->invsuccesses = 0;
	    stats_arg->invarsuclevel = 0;
	    return;
	}

    /* take copies of some args, and options: */
        m = m_arg;
        n = n_arg;

	nautil_check(WORDSIZE,m,n,NAUTYVERSIONID);
	OPTCALL(dispatch.check)(WORDSIZE,m,n,NAUTYVERSIONID);

#if !MAXN
	DYNALLOC1(set,defltwork,defltwork_sz,2*m,"nauty");
	DYNALLOC1(set,fixedpts,fixedpts_sz,m,"nauty");
	DYNALLOC1(set,active,active_sz,m,"nauty");
	DYNALLOC1(permutation,workperm,workperm_sz,n,"nauty");
	DYNALLOC1(permutation,firstlab,firstlab_sz,n,"nauty");
	DYNALLOC1(permutation,canonlab,canonlab_sz,n,"nauty");
	DYNALLOC1(short,firstcode,firstcode_sz,n+2,"nauty");
	DYNALLOC1(short,canoncode,canoncode_sz,n+2,"nauty");
	DYNALLOC1(shortish,firsttc,firsttc_sz,n+2,"nauty");
        if (m > alloc_m)
        {
            tcp = tcnode0.next;
            while (tcp != NULL)
            {
                tcq = tcp->next;
                FREES(tcp->tcellptr);
                FREES(tcp);
                tcp = tcq;
            }
            alloc_m = m;
            tcnode0.next = NULL;
        }
#endif

        g = g_arg;
        orbits = orbits_arg;
        stats = stats_arg;

        getcanon = options->getcanon;
        digraph = options->digraph;
        writeautoms = options->writeautoms;
        domarkers = options->writemarkers;
        cartesian = options->cartesian;
        linelength = options->linelength;
        if (digraph) tc_level = 0;
        else         tc_level = options->tc_level;
        outfile = (options->outfile == NULL ? stdout : options->outfile);
        usernodeproc = options->usernodeproc;
        userautomproc = options->userautomproc;
        userlevelproc = options->userlevelproc;

        invarproc = options->invarproc;
        if (options->mininvarlevel < 0 && options->getcanon)
            mininvarlevel = -options->mininvarlevel;
        else
            mininvarlevel = options->mininvarlevel;
        if (options->maxinvarlevel < 0 && options->getcanon)
            maxinvarlevel = -options->maxinvarlevel;
        else
            maxinvarlevel = options->maxinvarlevel;
        invararg = options->invararg;

        if (getcanon)
            if (canong_arg == NULL)
            {
                stats_arg->errstatus = CANONGNIL;
                fprintf(ERRFILE,
                      "nauty: canong=NULL but options.getcanon=TRUE\n\n");
                return;
            }
            else
                canong = canong_arg;

    /* initialize everything: */
        if (options->defaultptn)
        {
            for (i = 0; i < n; ++i)   /* give all verts same colour */
            {
                lab[i] = i;
                ptn[i] = NAUTY_INFINITY;
            }
            ptn[n-1] = 0;
            EMPTYSET(active,m);
            ADDELEMENT(active,0);
            numcells = 1;
        }
        else
        {
            ptn[n-1] = 0;
            numcells = 0;
            for (i = 0; i < n; ++i)
                if (ptn[i] != 0) ptn[i] = NAUTY_INFINITY;
                else             ++numcells;
            if (active_arg == NULL)
            {
                EMPTYSET(active,m);
                for (i = 0; i < n; ++i)
                {
                    ADDELEMENT(active,i);
                    while (ptn[i]) ++i;
                }
            }
            else
                for (i = 0; i < M; ++i) active[i] = active_arg[i];
        }

        for (i = 0; i < n; ++i) orbits[i] = i;
        stats->grpsize1 = 1.0;
        stats->grpsize2 = 0;
        stats->numgenerators = 0;
        stats->numnodes = 0;
        stats->numbadleaves = 0;
        stats->tctotal = 0;
        stats->canupdates = 0;
        stats->numorbits = n;
        EMPTYSET(fixedpts,m);
        noncheaplevel = 1;
        eqlev_canon = -1;       /* needed even if !getcanon */

        if (worksize >= 2 * m)
            workspace = ws_arg;
        else
        {
            workspace = defltwork;
            worksize = 2 * m;
        }
        worktop = workspace + (worksize - worksize % (2 * m));
        fmptr = workspace;

    /* here goes: */
        stats->errstatus = 0;
        needshortprune = FALSE;
        invarsuclevel = NAUTY_INFINITY;
        invapplics = invsuccesses = 0;

#if !MAXN
        retval = firstpathnode0(lab,ptn,1,numcells,&tcnode0);
#else   
        retval = firstpathnode(lab,ptn,1,numcells);
#endif  

#ifdef NAUTY_IN_MAGMA
	if (retval != NAUTY_ABORT)
#endif
	{
            if (getcanon)
            {
                (*dispatch.updatecan)(g,canong,canonlab,samerows,M,n);
                for (i = 0; i < n; ++i) lab[i] = canonlab[i];
            }
            stats->invarsuclevel =
                 (invarsuclevel == NAUTY_INFINITY ? 0 : invarsuclevel);
            stats->invapplics = invapplics;
            stats->invsuccesses = invsuccesses;
	}

#if !MAXN
#ifndef NAUTY_IN_MAGMA
        if (n >= 320)
#endif
        {   
	    nautil_freedyn();
	    OPTCALL(dispatch.freedyn)();
	    nauty_freedyn();
        }
#endif  
}

/*****************************************************************************
*                                                                            *
*  firstpathnode(lab,ptn,level,numcells) produces a node on the leftmost     *
*  path down the tree.  The parameters describe the level and the current    *
*  colour partition.  The set of active cells is taken from the global set   *
*  'active'.  If the refined partition is not discrete, the leftmost child   *
*  is produced by calling firstpathnode, and the other children by calling   *
*  othernode.                                                                *
*  For MAXN=0 there is an extra parameter: the address of the parent tcell   *
*  structure.                                                                *
*  The value returned is the level to return to.                             *
*                                                                            *
*  FUNCTIONS CALLED: (*usernodeproc)(),doref(),cheapautom(),                 *
*                    firstterminal(),nextelement(),breakout(),               *
*                    firstpathnode(),othernode(),recover(),writestats(),     *
*                    (*userlevelproc)(),(*tcellproc)(),shortprune()          *
*                                                                            *
*****************************************************************************/

static int
#if !MAXN
firstpathnode0(int *lab, int *ptn, int level, int numcells,
              tcnode *tcnode_parent)
#else
firstpathnode(int *lab, int *ptn, int level, int numcells)
#endif
{
        register int tv;
        int tv1,index,rtnlevel,tcellsize,tc,childcount,qinvar,refcode;
#if !MAXN
        set *tcell;
        tcnode *tcnode_this;

        tcnode_this = tcnode_parent->next;
        if (tcnode_this == NULL)
        {
            if ((tcnode_this = (tcnode*)ALLOCS(1,sizeof(tcnode))) == NULL ||
                (tcnode_this->tcellptr
                             = (set*)ALLOCS(alloc_m,sizeof(set))) == NULL)
                alloc_error("tcell");
            tcnode_parent->next = tcnode_this;
            tcnode_this->next = NULL;
        }
        tcell = tcnode_this->tcellptr;
#else
        set tcell[MAXM];
#endif

        ++stats->numnodes;

    /* refine partition : */
        doref(g,lab,ptn,level,&numcells,&qinvar,workperm,
              active,&refcode,dispatch.refine,invarproc,
              mininvarlevel,maxinvarlevel,invararg,digraph,M,n);
        firstcode[level] = (short)refcode;
        if (qinvar > 0)
        {
            ++invapplics;
            if (qinvar == 2)
            {
                ++invsuccesses;
                if (mininvarlevel < 0) mininvarlevel = level;
                if (maxinvarlevel < 0) maxinvarlevel = level;
                if (level < invarsuclevel) invarsuclevel = level;
            }
        }

        tc = -1;
        if (numcells != n)
        {
     /* locate new target cell, setting tc to its position in lab, tcell
                          to its contents, and tcellsize to its size: */
            (*tcellproc)(g,lab,ptn,level,numcells,tcell,&tcellsize,
                                        &tc,tc_level,-1,dispatch.bestcell,M,n);
            stats->tctotal += tcellsize;
        }
        firsttc[level] = tc;

    /* optionally call user-defined node examination procedure: */
        OPTCALL(usernodeproc)
                       (g,lab,ptn,level,numcells,tc,(int)firstcode[level],M,n);

        if (numcells == n)      /* found first leaf? */
        {
            firstterminal(lab,level);
            OPTCALL(userlevelproc)(lab,ptn,level,orbits,stats,0,1,1,n,0,n);
            return level-1;
        }

        if (noncheaplevel >= level
                             && !(*dispatch.cheapautom)(ptn,level,digraph,n))
            noncheaplevel = level + 1;

    /* use the elements of the target cell to produce the children: */
        index = 0;
        for (tv1 = tv = nextelement(tcell,M,-1); tv >= 0;
                                        tv = nextelement(tcell,M,tv))
        {
            if (orbits[tv] == tv)   /* ie, not equiv to previous child */
            {
                breakout(lab,ptn,level+1,tc,tv,active,M);
                ADDELEMENT(fixedpts,tv);
                cosetindex = tv;
                if (tv == tv1)
                {
#if !MAXN
                    rtnlevel = firstpathnode0(lab,ptn,level+1,numcells+1,
                                             tcnode_this);
#else
                    rtnlevel = firstpathnode(lab,ptn,level+1,numcells+1);
#endif
                    childcount = 1;
                    gca_first = level;
                    stabvertex = tv1;
                }
                else
                {
#if !MAXN
                    rtnlevel = othernode0(lab,ptn,level+1,numcells+1,
                                         tcnode_this);
#else
                    rtnlevel = othernode(lab,ptn,level+1,numcells+1);
#endif
                    ++childcount;
                }
                DELELEMENT(fixedpts,tv);
                if (rtnlevel < level)
                    return rtnlevel;
                if (needshortprune)
                {
                    needshortprune = FALSE;
                    shortprune(tcell,fmptr-M,M);
                }
                recover(ptn,level);
            }
            if (orbits[tv] == tv1)  /* ie, in same orbit as tv1 */
                ++index;
        }
        MULTIPLY(stats->grpsize1,stats->grpsize2,index);

        if (tcellsize == index && allsamelevel == level + 1)
            --allsamelevel;

        if (domarkers)
            writemarker(level,tv1,index,tcellsize,stats->numorbits,numcells);
        OPTCALL(userlevelproc)(lab,ptn,level,orbits,stats,tv1,index,tcellsize,
                                                        numcells,childcount,n);
        return level-1;
}

/*****************************************************************************
*                                                                            *
*  othernode(lab,ptn,level,numcells) produces a node other than an ancestor  *
*  of the first leaf.  The parameters describe the level and the colour      *
*  partition.  The list of active cells is found in the global set 'active'. *
*  The value returned is the level to return to.                             *
*                                                                            *
*  FUNCTIONS CALLED: (*usernodeproc)(),doref(),refine(),recover(),           *
*                    processnode(),cheapautom(),(*tcellproc)(),shortprune(), *
*                    nextelement(),breakout(),othernode(),longprune()        *
*                                                                            *
*****************************************************************************/

static int
#if !MAXN
othernode0(int *lab, int *ptn, int level, int numcells,
          tcnode *tcnode_parent)
#else
othernode(int *lab, int *ptn, int level, int numcells)
#endif
{
        register int tv;
        int tv1,refcode,rtnlevel,tcellsize,tc,qinvar;
        short code;
#if !MAXN
        set *tcell;
        tcnode *tcnode_this;

        tcnode_this = tcnode_parent->next;
        if (tcnode_this == NULL)
        {
            if ((tcnode_this = (tcnode*)ALLOCS(1,sizeof(tcnode))) == NULL ||
                (tcnode_this->tcellptr
                         = (set*)ALLOCS(alloc_m,sizeof(set))) == NULL)
                alloc_error("tcell");
            tcnode_parent->next = tcnode_this;
            tcnode_this->next = NULL;
        }
        tcell = tcnode_this->tcellptr;
#else
        set tcell[MAXM];
#endif

#ifdef NAUTY_IN_MAGMA
        if (main_seen_interrupt) return NAUTY_ABORT;
#endif

        ++stats->numnodes;

    /* refine partition : */
        doref(g,lab,ptn,level,&numcells,&qinvar,workperm,active,
              &refcode,dispatch.refine,invarproc,mininvarlevel,maxinvarlevel,
              invararg,digraph,M,n);
        code = (short)refcode;
        if (qinvar > 0)
        {
            ++invapplics;
            if (qinvar == 2)
            {
                ++invsuccesses;
                if (level < invarsuclevel) invarsuclevel = level;
            }
        }

        if (eqlev_first == level - 1 && code == firstcode[level])
            eqlev_first = level;
        if (getcanon)
        {
            if (eqlev_canon == level - 1)
            {
                if (code < canoncode[level])
                    comp_canon = -1;
                else if (code > canoncode[level])
                    comp_canon = 1;
                else
                {
                    comp_canon = 0;
                    eqlev_canon = level;
                }
            }
            if (comp_canon > 0) canoncode[level] = code;
        }

        tc = -1;
   /* If children will be required, find new target cell and set tc to its
      position in lab, tcell to its contents, and tcellsize to its size: */

        if (numcells < n && (eqlev_first == level ||
                             getcanon && comp_canon >= 0))
        {
            if (!getcanon || comp_canon < 0)
            {
                (*tcellproc)(g,lab,ptn,level,numcells,tcell,&tcellsize,
                            &tc,tc_level,firsttc[level],dispatch.bestcell,M,n);
                if (tc != firsttc[level]) eqlev_first = level - 1;
            }
            else
                (*tcellproc)(g,lab,ptn,level,numcells,tcell,&tcellsize,
                                        &tc,tc_level,-1,dispatch.bestcell,M,n);
            stats->tctotal += tcellsize;
        }

    /* optionally call user-defined node examination procedure: */
        OPTCALL(usernodeproc)(g,lab,ptn,level,numcells,tc,(int)code,M,n);

    /* call processnode to classify the type of this node: */

        rtnlevel = processnode(lab,ptn,level,numcells);
        if (rtnlevel < level)   /* keep returning if necessary */
            return rtnlevel;
        if (needshortprune)
        {
            needshortprune = FALSE;
            shortprune(tcell,fmptr-M,M);
        }

        if (!(*dispatch.cheapautom)(ptn,level,digraph,n))
            noncheaplevel = level + 1;

    /* use the elements of the target cell to produce the children: */
        for (tv1 = tv = nextelement(tcell,M,-1); tv >= 0;
                                        tv = nextelement(tcell,M,tv))
        {
            breakout(lab,ptn,level+1,tc,tv,active,M);
            ADDELEMENT(fixedpts,tv);
#if !MAXN   
            rtnlevel = othernode0(lab,ptn,level+1,numcells+1,tcnode_this);
#else
            rtnlevel = othernode(lab,ptn,level+1,numcells+1);
#endif
            DELELEMENT(fixedpts,tv);

            if (rtnlevel < level) return rtnlevel;
    /* use stored automorphism data to prune target cell: */
            if (needshortprune)
            {
                needshortprune = FALSE;
                shortprune(tcell,fmptr-M,M);
            }
            if (tv == tv1) longprune(tcell,fixedpts,workspace,fmptr,M);

            recover(ptn,level);
        }

        return level-1;
}

/*****************************************************************************
*                                                                            *
*  Process the first leaf of the tree.                                       *
*                                                                            *
*  FUNCTIONS CALLED: NONE                                                    *
*                                                                            *
*****************************************************************************/

static void
firstterminal(int *lab, int level)
{
        register int i;

        stats->maxlevel = level;
        gca_first = allsamelevel = eqlev_first = level;
        firstcode[level+1] = 077777;
        firsttc[level+1] = -1;

        for (i = 0; i < n; ++i) firstlab[i] = lab[i];

        if (getcanon)
        {
            canonlevel = eqlev_canon = gca_canon = level;
            comp_canon = 0;
            samerows = 0;
            for (i = 0; i < n; ++i) canonlab[i] = lab[i];
            for (i = 0; i <= level; ++i) canoncode[i] = firstcode[i];
            canoncode[level+1] = 077777;
            stats->canupdates = 1;
        }
}

/*****************************************************************************
*                                                                            *
*  Process a node other than the first leaf or its ancestors.  It is first   *
*  classified into one of five types and then action is taken appropriate    *
*  to that type.  The types are                                              *
*                                                                            *
*  0:   Nothing unusual.  This is just a node internal to the tree whose     *
*         children need to be generated sometime.                            *
*  1:   This is a leaf equivalent to the first leaf.  The mapping from       *
*         firstlab to lab is thus an automorphism.  After processing the     *
*         automorphism, we can return all the way to the closest invocation  *
*         of firstpathnode.                                                  *
*  2:   This is a leaf equivalent to the bsf leaf.  Again, we have found an  *
*         automorphism, but it may or may not be as useful as one from a     *
*         type-1 node.  Return as far up the tree as possible.               *
*  3:   This is a new bsf node, provably better than the previous bsf node.  *
*         After updating canonlab etc., treat it the same as type 4.         *
*  4:   This is a leaf for which we can prove that no descendant is          *
*         equivalent to the first or bsf leaf or better than the bsf leaf.   *
*         Return up the tree as far as possible, but this may only be by     *
*         one level.                                                         *
*                                                                            *
*  Types 2 and 3 can't occur if getcanon==FALSE.                             *
*  The value returned is the level in the tree to return to, which can be    *
*  anywhere up to the closest invocation of firstpathnode.                   *
*                                                                            *
*  FUNCTIONS CALLED:    isautom(),updatecan(),testcanlab(),fmperm(),         *
*                       writeperm(),(*userautomproc)(),orbjoin(),            *
*                       shortprune(),fmptn()                                 *
*                                                                            *
*****************************************************************************/

static int
processnode(int *lab, int *ptn, int level, int numcells)
{
        register int i,code,save,newlevel;
        boolean ispruneok;
        int sr;

        code = 0;
        if (eqlev_first != level && (!getcanon || comp_canon < 0))
            code = 4;
        else if (numcells == n)
        {
            if (eqlev_first == level)
            {
                for (i = 0; i < n; ++i) workperm[firstlab[i]] = lab[i];

                if (gca_first >= noncheaplevel ||
                                   (*dispatch.isautom)(g,workperm,digraph,M,n))
                    code = 1;
            }
            if (code == 0)
                if (getcanon)
                {
                    sr = 0;
                    if (comp_canon == 0)
                    {
                        if (level < canonlevel)
                            comp_canon = 1;
                        else
                        {
                            (*dispatch.updatecan)
                                              (g,canong,canonlab,samerows,M,n);
                            samerows = n;
                            comp_canon
                                = (*dispatch.testcanlab)(g,canong,lab,&sr,M,n);
                        }
                    }
                    if (comp_canon == 0)
                    {
                        for (i = 0; i < n; ++i) workperm[canonlab[i]] = lab[i];
                        code = 2;
                    }
                    else if (comp_canon > 0)
                        code = 3;
                    else
                        code = 4;
                }
                else
                    code = 4;
        }

        if (code != 0 && level > stats->maxlevel) stats->maxlevel = level;

        switch (code)
        {
        case 0:                 /* nothing unusual noticed */
            return level;

        case 1:                 /* lab is equivalent to firstlab */
            if (fmptr == worktop) fmptr -= 2 * M;
            fmperm(workperm,fmptr,fmptr+M,M,n);
            fmptr += 2 * M;
            if (writeautoms)
                writeperm(outfile,workperm,cartesian,linelength,n);
            stats->numorbits = orbjoin(orbits,workperm,n);
            ++stats->numgenerators;
            OPTCALL(userautomproc)(stats->numgenerators,workperm,orbits,
                                        stats->numorbits,stabvertex,n);
            return gca_first;

        case 2:                 /* lab is equivalent to canonlab */
            if (fmptr == worktop) fmptr -= 2 * M;
            fmperm(workperm,fmptr,fmptr+M,M,n);
            fmptr += 2 * M;
            save = stats->numorbits;
            stats->numorbits = orbjoin(orbits,workperm,n);
            if (stats->numorbits == save)
            {
                if (gca_canon != gca_first) needshortprune = TRUE;
                return gca_canon;
            }
            if (writeautoms)
                writeperm(outfile,workperm,cartesian,linelength,n);
            ++stats->numgenerators;
            OPTCALL(userautomproc)(stats->numgenerators,workperm,orbits,
                                        stats->numorbits,stabvertex,n);
            if (orbits[cosetindex] < cosetindex)
                return gca_first;
            if (gca_canon != gca_first)
                needshortprune = TRUE;
            return gca_canon;

        case 3:                 /* lab is better than canonlab */
            ++stats->canupdates;
            for (i = 0; i < n; ++i) canonlab[i] = lab[i];
            canonlevel = eqlev_canon = gca_canon = level;
            comp_canon = 0;
            canoncode[level+1] = 077777;
            samerows = sr;
            break;

        case 4:                /* non-automorphism terminal node */
            ++stats->numbadleaves;
            break;
        }  /* end of switch statement */

    /* only cases 3 and 4 get this far: */
        if (level != noncheaplevel)
        {
            ispruneok = TRUE;
            if (fmptr == worktop) fmptr -= 2 * M;
            fmptn(lab,ptn,noncheaplevel,fmptr,fmptr+M,M,n);
            fmptr += 2 * M;
        }
        else
            ispruneok = FALSE;

        save = (allsamelevel > eqlev_canon ? allsamelevel-1 : eqlev_canon);
        newlevel = (noncheaplevel <= save ? noncheaplevel-1 : save);

        if (ispruneok && newlevel != gca_first) needshortprune = TRUE;
        return newlevel;
 }

/*****************************************************************************
*                                                                            *
*  Recover the partition nest at level 'level' and update various other      *
*  parameters.                                                               *
*                                                                            *
*  FUNCTIONS CALLED: NONE                                                    *
*                                                                            *
*****************************************************************************/

static void
recover(int *ptn, int level)
{
        register int i;

        for (i = 0; i < n; ++i)
            if (ptn[i] > level) ptn[i] = NAUTY_INFINITY;

        if (level < noncheaplevel) noncheaplevel = level + 1;
        if (level < eqlev_first) eqlev_first = level;
        if (getcanon)
        {
            if (level < gca_canon) gca_canon = level;
            if (level <= eqlev_canon)
            {
                eqlev_canon = level;
                comp_canon = 0;
            }
        }
}

/*****************************************************************************
*                                                                            *
*  Write statistics concerning an ancestor of the first leaf.                *
*                                                                            *
*  level = its level                                                         *
*  tv = the vertex fixed to get the first child = the smallest-numbered      *
*               vertex in the target cell                                    *
*  cellsize = the size of the target cell                                    *
*  index = the number of vertices in the target cell which were equivalent   *
*               to tv = the index of the stabiliser of tv in the group       *
*               fixing the colour partition at this level                    *
*                                                                            *
*  numorbits = the number of orbits of the group generated by all the        *
*               automorphisms so far discovered                              *
*                                                                            *
*  numcells = the total number of cells in the equitable partition at this   *
*               level                                                        *
*                                                                            *
*  FUNCTIONS CALLED: itos(),putstring()                                      *
*                                                                            *
*****************************************************************************/

static void
writemarker(int level, int tv, int index, int tcellsize,
            int numorbits, int numcells)
{
        char s[30];

#define PUTINT(i) itos(i,s); putstring(outfile,s)
#define PUTSTR(x) putstring(outfile,x)

        PUTSTR("level ");
        PUTINT(level);
        PUTSTR(":  ");
        if (numcells != numorbits)
        {
            PUTINT(numcells);
            PUTSTR(" cell");
            if (numcells == 1) PUTSTR("; ");
            else               PUTSTR("s; ");
        }
        PUTINT(numorbits);
        PUTSTR(" orbit");
        if (numorbits == 1) PUTSTR("; ");
        else                PUTSTR("s; ");
        PUTINT(tv+labelorg);
        PUTSTR(" fixed; index ");
        PUTINT(index);
        if (tcellsize != index)
        {
            PUTSTR("/");
            PUTINT(tcellsize);
        }
        PUTSTR("\n");
}

/*****************************************************************************
*                                                                            *
*  nauty_check() checks that this file is compiled compatibly with the       *
*  given parameters.   If not, call exit(1).                                 *
*                                                                            *
*****************************************************************************/

void
nauty_check(int wordsize, int m, int n, int version)
{
        if (wordsize != WORDSIZE)
        {
            fprintf(ERRFILE,"Error: WORDSIZE mismatch in nauty.c\n");
            exit(1);
        }

#if MAXN
        if (m > MAXM)
        {
            fprintf(ERRFILE,"Error: MAXM inadequate in nauty.c\n");
            exit(1);
        }

        if (n > MAXN)
        {
            fprintf(ERRFILE,"Error: MAXN inadequate in nauty.c\n");
            exit(1);
        }
#endif

#ifdef BIGNAUTY
        if ((version & 1) == 0)
        {   
            fprintf(ERRFILE,"Error: BIGNAUTY mismatch in nauty.c\n");
            exit(1);
        }
#else
        if ((version & 1) == 1)
        {   
            fprintf(ERRFILE,"Error: BIGNAUTY mismatch in nauty.c\n");
            exit(1);
        }
#endif

	if (version < NAUTYREQUIRED)
	{
	    fprintf(ERRFILE,"Error: nauty.c version mismatch\n");
	    exit(1);
	}
}

/*****************************************************************************
*                                                                            *
*  nauty_freedyn() frees all the dynamic memory used in this module.         *
*                                                                            *
*****************************************************************************/

void
nauty_freedyn(void)
{
#if !MAXN
	tcnode *tcp,*tcq;

        tcp = tcnode0.next;
        while (tcp != NULL)
        {   
            tcq = tcp->next;
            FREES(tcp->tcellptr);
            FREES(tcp);
            tcp = tcq;
        }
        alloc_m = 0;
        tcnode0.next = NULL;
        DYNFREE(firsttc,firsttc_sz);
        DYNFREE(canoncode,canoncode_sz);
        DYNFREE(firstcode,firstcode_sz);
        DYNFREE(workperm,workperm_sz);
        DYNFREE(canonlab,canonlab_sz);
        DYNFREE(firstlab,firstlab_sz);
        DYNFREE(defltwork,defltwork_sz);
	DYNFREE(fixedpts,fixedpts_sz);
	DYNFREE(active,active_sz);
#endif  
}
