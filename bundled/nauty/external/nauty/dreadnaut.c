/*****************************************************************************
*                                                                            *
* This is the main file for dreadnaut() version 2.2, which is a test-bed     *
*   for nauty() version 2.2.                                                 *
*                                                                            *
*   Copyright (1984-2004) Brendan McKay.  All rights reserved.               *
*   Subject to the waivers and disclaimers in nauty.h.                       *
*                                                                            *
*   CHANGE HISTORY                                                           *
*       10-Nov-87 : final changes for version 1.2                            *
*        5-Dec-87 - replaced all uses of fscanf() by appropriate uses        *
*                   of the new procedures readinteger() and readstring()     *
*                 - changed the '<' command slightly.  If a file of the      *
*                   given name cannot be openned, an attempt is made to      *
*                   open a file with the same name extended by DEFEXT.       *
*                 - improved error processing for 'n' command.               *
*       28-Sep-88 : changes for version 1.4 :                                *
*                 - replaced incorrect %d by %ld in fprintf for ? command    *
*       23-Mar-89 : changes for version 1.5 :                                *
*                 - added optional startup message                           *
*                 - enabled use of refine1 in 'i' command                    *
*                 - implemented $$ command                                   *
*                 - replaced ALLOCS test by DYNALLOC test                    *
*                 - modified @ command and added # command                   *
*                 - declared local procedures static                         *
*       25-Mar-89 - implemented k command                                    *
*       27-Mar-89 - implemented * and I commands                             *
*       29-Mar-89 - implemented K command                                    *
*        2-Apr-89 - added reporting of vertex-invariant statistics           *
*        2-Apr-89 - added ## command                                         *
*        4-Apr-89 - added triples(), quadruples(), adjtriang()               *
*                 - updated error reporting for nauty()                      *
*        5-Apr-89 - removed flushline() from g and e commands                *
*                 - added T command                                          *
*        6-Apr-89 - added cellquads() and distances()                        *
*       26-Apr-89 - modified ? command, added & and && commands              *
*                 - added indsets(), cliques(), cellquins()                  *
*       18-Aug-89 - made g, lab, canong dynamically allocated always         *
*        2-Mar-90 - added celltrips(), cellcliq(), cellind()                 *
*       13-Mar-90 - changed canong and savedg in output to h and h'          *
*       19-Mar-90 - revised help() a little                                  *
*       19-Apr-90 : changes for version 1.6                                  *
*                 - rewrote "*" command to avoid bug in Pyramid C compiler   *
*       20-Apr-90 - rewrote above rewrite to avoid bug in SUN3 gcc           *
*       23-Apr-90 - undid above rewrite and fixed *my* bug <blush> by        *
*                   making NUMINVARS have type int.  Sorry, gcc.             *
*       10-Nov-90 - added calls to null routines (see comment on code)       *
*       27-Aug-92 : renamed to version 1.7, no changes to this file          *
*        5-Jun-93 : renamed to version 1.7+, no changes to this file         *
*       18-Aug-93 : renamed to version 1.8, no changes to this file          *
*       17-Sep-93 : changes for version 1.9 :                                *
*                 - added invariant adjacencies()                            *
*        7-Jun-96 : changes for version 2.0 :                                *
*                 - added invariants cellfano() and cellfano2()              *
*                 - made y=10 the default                                    *
*       11-Jul-96 - added dynamic allocation                                 *
*                 - rewrote h command and added H command                    *
*                 - implemented M and R commands                             *
*       15-Aug-96 - changed z command to use sethash()                       *
*       30-Aug-96 - no need to declare seed; already in naututil.h           *
*       12-Sep-96 - let i and I commands use userrefproc                     *
*        9-Dec-96 - made y=infinity the default                              *
*        6-Sep-97 - allocated arrays before accepting commands               *
*        7-Sep-97 - make g,canong,savedg 1-d arrays even statically          *
*       22-Sep-97 - undid error introduced on 7-Sep (worksize)               *
*        9-Jan-00 - used *_check() instead of *_null()                       *
*       12-Feb-00 - minor code formatting                                    *
*       17-Aug-00 - now use tc_level from DEFAULTOPTIONS                     *
*       16-Nov-00 - made changes listed in nauty.h                           *
*       22-Apr-01 - include nautyinv.h                                       *
*                 - improve worksize processing for MAXN=0                   *
*        5-May-01 - k=0 1 automatic for *, also K=3 or K=0                   *
*        2-Jun-01 - added __ command for digraph converse                    *
*       18-Oct-01 - moved WORKSIZE to here                                   *
*       21-Nov-01 - use NAUTYREQUIRED in *_check() calls                     *
*        1-Sep-02 - Undid the previous change                                *
*       17-Nov-03 - Changed INFINITY to NAUTY_INFINITY                       *
*       15-Nov-04 - Completed all prototypes                                 *
*                                                                            *
*****************************************************************************/

#include "naututil.h"    /* which includes nauty.h, which includes stdio.h */
#include "nautinv.h"    /* which includes nauty.h, which includes stdio.h */

#define PM(x) ((x) ? '+' : '-')
#define SS(n,sing,plur)  (n),((n)==1?(sing):(plur))
#define WORKSIZE 60

#ifdef  CPUDEFS
CPUDEFS                 /* data decls. for CPUTIME */
#endif

#define INFILE fileptr[curfile]
#define OUTFILE outfile

#if !MAXN
DYNALLSTAT(graph,g,g_sz);
DYNALLSTAT(graph,canong,canong_sz);
DYNALLSTAT(graph,savedg,savedg_sz);
DYNALLSTAT(setword,workspace,workspace_sz);
DYNALLSTAT(int,lab,lab_sz);
DYNALLSTAT(int,ptn,ptn_sz);
DYNALLSTAT(int,orbits,orbits_sz);
DYNALLSTAT(int,savedlab,savedlab_sz);
DYNALLSTAT(permutation,perm,perm_sz);
DYNALLSTAT(set,active,active_sz);
#else
static graph g[MAXM*1L*MAXN];
static graph canong[MAXM*1L*MAXN];
static graph savedg[MAXM*1L*MAXN];
static setword workspace[MAXM*2L*WORKSIZE];
static int lab[MAXN];
static int ptn[MAXN];
static int orbits[MAXN];
static int savedlab[MAXN];
static permutation perm[MAXN];
static set active[MAXM];
#endif

static DEFAULTOPTIONS_GRAPH(options);
static statsblk stats;
static int curfile;
static FILE *fileptr[MAXIFILES];
static FILE *outfile;
static char def_ext[] = DEFEXT;
static boolean firstpath;       /* used in usernode() */

#define U_NODE  1               /* masks for u values */
#define U_AUTOM 2
#define U_LEVEL 4
#define U_TCELL 8
#define U_REF  16

#ifndef  NODEPROC
#define NODEPROC usernode
#else
extern void NODEPROC(graph*,int*,int*,int,int,int,int,int,int);
#endif

#ifndef  AUTOMPROC
#define AUTOMPROC userautom
#else
extern void AUTOMPROC(int,permutation*,int*,int,int,int);
#endif

#ifndef  LEVELPROC
#define LEVELPROC userlevel
#else
extern void LEVELPROC(int*,int*,int,int*,statsblk*,int,int,int,int,int,int);
#endif

#ifndef  TCELLPROC
#define TCELLPROC NULL
#else
extern void TCELLPROC(graph*,int*,int*,int,int,set*,int*,int*,int,int,
              int(*)(graph*,int*,int*,int,int,int,int),int,int);
#endif

#ifndef  REFPROC
#define REFPROC NULL
#else
extern void REFPROC(graph*,int*,int*,int,int*,permutation*,set*,int*,int,int);
#endif

#ifndef  INVARPROC
#define INVARPROC NULL
#define INVARPROCNAME "none"
#else
extern void INVARPROC(graph*,int*,int*,int,int,int,permutation*,
                      int,boolean,int,int);
#define INVARPROCNAME "user-defined"
#endif

static struct invarrec
{
    void (*entrypoint)(graph*,int*,int*,int,int,int,permutation*,
                      int,boolean,int,int);
    char *name;
} invarproc[]
    = {{INVARPROC, INVARPROCNAME},
       {NULL, "none"},
       {twopaths,    "twopaths"},
       {adjtriang,   "adjtriang"},
       {triples,     "triples"},
       {quadruples,  "quadruples"},
       {celltrips,   "celltrips"},
       {cellquads,   "cellquads"},
       {cellquins,   "cellquins"},
       {distances,   "distances"},
       {indsets,     "indsets"},
       {cliques,     "cliques"},
       {cellcliq,    "cellcliq"},
       {cellind,     "cellind"},
       {adjacencies, "adjacencies"},
       {cellfano,    "cellfano"},
       {cellfano2,   "cellfano2"}};
#define NUMINVARS ((int)(sizeof(invarproc)/sizeof(struct invarrec)))

static void help(FILE*, int);
static void userautom(int,permutation*,int*,int,int,int);
static void usernode(graph*,int*,int*,int,int,int,int,int,int);
static void userlevel(int*,int*,int,int*,statsblk*,int,int,int,int,int,int);

#ifdef  EXTRADECLS
EXTRADECLS
#endif

/*****************************************************************************
*                                                                            *
*  This is a program which illustrates the use of nauty.                     *
*  Commands are read from stdin, and may be separated by white space,        *
*  commas or not separated.  Output is written to stdout.                    *
*  For a short description, see the nauty User's Guide.                      *
*                                                                            *
*****************************************************************************/

int
main(int argc, char *argv[])
{
        int m,n,newm,newn;
        boolean gvalid,ovalid,cvalid,pvalid,minus,prompt,doquot;
        int i,worksize,numcells,refcode,umask,qinvar;
        int oldorg;
        char *s1,*s2,*invarprocname;
        int c,d;
        register long li;
        set *gp;
        double timebefore,timeafter;
        char filename[200];
        int sgn,sgorg,nperm;
	int multiplicity;
	boolean options_writeautoms,options_writemarkers;
	long zseed;

        curfile = 0;
        fileptr[curfile] = stdin;
        prompt = DOPROMPT(INFILE);
        outfile = stdout;
	options_writeautoms = options_writemarkers = TRUE;
        n = m = 1;
        worksize = 2*WORKSIZE;

#if !MAXN
	n = WORDSIZE;
        DYNALLOC2(graph,g,g_sz,n,m,"dreadnaut");
        DYNALLOC1(int,lab,lab_sz,n,"dreadnaut");
        DYNALLOC1(int,ptn,ptn_sz,n,"dreadnaut");
        DYNALLOC1(setword,workspace,workspace_sz,
                                            worksize,"dreadnaut");
        DYNALLOC1(int,orbits,orbits_sz,n,"dreadnaut");
        DYNALLOC1(permutation,perm,perm_sz,n,"dreadnaut");
        DYNALLOC1(set,active,active_sz,m,"dreadnaut");
	n = 1;
#endif

#ifdef  INITSEED
        INITSEED;
	ran_init(seed);
#endif

        umask = 0;
        pvalid = FALSE;
        gvalid = FALSE;
        ovalid = FALSE;
        cvalid = FALSE;
        minus = FALSE;
        labelorg = oldorg = 0;
        multiplicity = 1;

#ifdef  INITIALIZE
        INITIALIZE;
#endif

        invarprocname = "none";
        if (prompt)
        {
#ifdef BIGNAUTY
            fprintf(PROMPTFILE,"Dreadnaut version %s [BIG].\n",NAUTYVERSION);
#else
            fprintf(PROMPTFILE,"Dreadnaut version %s.\n",NAUTYVERSION);
#endif
            fprintf(PROMPTFILE,"> ");
        }

        nauty_check(WORDSIZE,1,1,NAUTYVERSIONID);
        nautinv_check(WORDSIZE,1,1,NAUTYVERSIONID);
        nautil_check(WORDSIZE,1,1,NAUTYVERSIONID);
        naututil_check(WORDSIZE,1,1,NAUTYVERSIONID);

        while (curfile >= 0)
            if ((c = getc(INFILE)) == EOF || c == '\004')
            {
                fclose(INFILE);
                --curfile;
                if (curfile >= 0) prompt = DOPROMPT(INFILE);
            }
            else switch (c)
            {
            case '\n':  /* possibly issue prompt */
                if (prompt)
                    fprintf(PROMPTFILE,"> ");
                minus = FALSE;
                break;

            case ' ':   /* do nothing */
            case '\t':
#ifndef  NLMAP
            case '\r':
#endif
            case '\f':
                break;

            case '-':   /* remember this for next time */
                minus = TRUE;
                break;

            case '+':   /* forget - */
            case ',':
            case ';':
                minus = FALSE;
                break;

            case '<':   /* new input file */
                minus = FALSE;
                if (curfile == MAXIFILES - 1)
                    fprintf(ERRFILE,"exceeded maximum input nesting of %d\n\n",
                            MAXIFILES);
                if (!readstring(INFILE,filename,200))
                {
                    fprintf(ERRFILE,
                            "missing file name on '>' command : ignored\n\n");
                    break;
                }
                if ((fileptr[curfile+1] = fopen(filename,"r")) == NULL)
                {
                    for (s1 = filename; *s1 != '\0'; ++s1) {}
                    for (s2 = def_ext; (*s1 = *s2) != '\0'; ++s1, ++s2) {}
                    fileptr[curfile+1] = fopen(filename,"r");
                }
                if (fileptr[curfile+1] != NULL)
                {
                    ++curfile;
                    prompt = DOPROMPT(INFILE);
                    if (prompt)
                        fprintf(PROMPTFILE,"> ");
                }
                else
                    fprintf(ERRFILE,"can't open input file\n\n");
                break;

            case '>':   /* new output file */
                if ((d = getc(INFILE)) != '>')
                    ungetc((char)d,INFILE);
                if (minus)
                {
                    minus = FALSE;
                    if (outfile != stdout)
                    {
                        fclose(outfile);
                        outfile = stdout;
                    }
                }
                else
                {
                    if (!readstring(INFILE,filename,200))
                    {
                        fprintf(ERRFILE,
                            "improper file name, reverting to stdout\n\n");
                        outfile = stdout;
                        break;
                    }
                    OPENOUT(outfile,filename,d=='>');
                    if (outfile == NULL)
                    {
                        fprintf(ERRFILE,
                            "can't open output file, reverting to stdout\n\n");
                        outfile = stdout;
                    }
                }
                break;

            case '!':   /* ignore rest of line */
                do
                    c = getc(INFILE);
                while (c != '\n' && c != EOF);
                if (c == '\n') ungetc('\n',INFILE);
                break;

            case 'n':   /* read n value */
                minus = FALSE;
                i = getint(INFILE);
                if (i <= 0 || (MAXN && i > MAXN)
			   || (!MAXN && i > NAUTY_INFINITY-2))
                    fprintf(ERRFILE,
                         " n can't be less than 1 or more than %d\n\n",
                           MAXN ? MAXN : NAUTY_INFINITY-2);
                else
                {
                    gvalid = FALSE;
                    ovalid = FALSE;
                    cvalid = FALSE;
                    pvalid = FALSE;
                    n = i;
                    m = (n + WORDSIZE - 1) / WORDSIZE;
#if !MAXN
		    worksize = 2 * m * WORKSIZE;
		    DYNALLOC2(graph,g,g_sz,n,m,"dreadnaut");
		    DYNALLOC1(int,lab,lab_sz,n,"dreadnaut");
		    DYNALLOC1(int,ptn,ptn_sz,n,"dreadnaut");
                    DYNALLOC1(setword,workspace,workspace_sz,
                                                        worksize,"dreadnaut");
		    DYNALLOC1(int,orbits,orbits_sz,n,"dreadnaut");
		    DYNALLOC1(permutation,perm,perm_sz,n,"dreadnaut");
		    DYNALLOC1(set,active,active_sz,m,"dreadnaut");
#endif
                }
                break;

            case 'g':   /* read graph */
                minus = FALSE;
                readgraph(INFILE,g,options.digraph,prompt,FALSE,
                          options.linelength,m,n);
                gvalid = TRUE;
                cvalid = FALSE;
                ovalid = FALSE;
                break;

            case 'e':   /* edit graph */
                minus = FALSE;
                readgraph(INFILE,g,options.digraph,prompt,gvalid,
                          options.linelength,m,n);
                gvalid = TRUE;
                cvalid = FALSE;
                ovalid = FALSE;
                break;

            case 'r':   /* relabel graph and current partition */
                minus = FALSE;
                if (gvalid)
                {
#if !MAXN
		    DYNALLOC2(graph,canong,canong_sz,n,m,"dreadnaut");
#endif
                    readvperm(INFILE,perm,prompt,n,&nperm);
                    relabel(g,(pvalid ? lab : NULL),perm,canong,m,n);
                    cvalid = FALSE;
                    ovalid = FALSE;
                }
                else
                    fprintf(ERRFILE,"g is not defined\n\n");
                break;

            case 'R':   /* form subgraph */
                if (gvalid)
                {
#if !MAXN
                    DYNALLOC2(graph,canong,canong_sz,n,m,"dreadnaut");
#endif
                    readvperm(INFILE,perm,prompt,n,&nperm);
		    if (minus && nperm == n || !minus && nperm == 0)
			fprintf(ERRFILE,"can't form null graph\n\n");
		    else if (minus)
		    {
                        sublabel(g,perm+nperm,n-nperm,canong,m,n);
			n = n - nperm;
		    }
		    else
                    {
                        sublabel(g,perm,nperm,canong,m,n);
                        n = nperm;
                    }
                    cvalid = FALSE;
		    pvalid = FALSE;
                    ovalid = FALSE;
		    m = (n + WORDSIZE - 1) / WORDSIZE;
                }
                else
                    fprintf(ERRFILE,"g is not defined\n\n");
                minus = FALSE;
                break;

            case '_':   /* complement graph or converse digraph */
                minus = FALSE;
                if ((d = getc(INFILE)) != '_') ungetc((char)d,INFILE);

                if (gvalid)
                {
                    if (d == '_') converse(g,m,n);
                    else          complement(g,m,n);
                    cvalid = FALSE;
                    ovalid = FALSE;
                }
                else
                    fprintf(ERRFILE,"g is not defined\n\n");
                break;

            case '@':   /* copy canong into savedg */
                minus = FALSE;
                if (cvalid)
                {
#if !MAXN
		    DYNALLOC2(graph,savedg,savedg_sz,n,m,"dreadnaut");
		    DYNALLOC1(int,savedlab,savedlab_sz,n,"dreadnaut");
#endif
                    sgn = n;
                    for (li = (long)n * (long)m; --li >= 0;)
                        savedg[li] = canong[li];
                    for (i = n; --i >= 0;) savedlab[i] = lab[i];
                    sgorg = labelorg;
                }
                else
                    fprintf(ERRFILE,"h is not defined\n\n");
                break;

            case '#':   /* compare canong to savedg */
                if ((d = getc(INFILE)) != '#') ungetc((char)d,INFILE);

                if (cvalid)
                {
                    if (sgn > 0)
                    {
                        if (sgn != n)
                            fprintf(OUTFILE,
                                  "h and h' have different sizes.\n");
                        else
                        {
                            for (li = (long)n * (long)m; --li >= 0;)
                                if (savedg[li] != canong[li]) break;
                            if (li >= 0)
                                fprintf(OUTFILE,"h and h' are different.\n");
                            else
                            {
                                fprintf(OUTFILE,
                                   "h and h' are identical.\n");
                                if (d == '#')
                                    putmapping(OUTFILE,savedlab,sgorg,
                                           lab,labelorg,options.linelength,n);
                            }
                        }
                    }
                    else
                        fprintf(ERRFILE,"h' is not defined\n\n");
                }
                else
                    fprintf(ERRFILE,"h is not defined\n\n");
                break;

            case 'j':   /* relabel graph randomly */
                minus = FALSE;
                if (gvalid)
                {
#if !MAXN
		    DYNALLOC2(graph,canong,canong_sz,n,m,"dreadnaut");
#endif
                    ranperm(perm,n);
                    relabel(g,(pvalid ? lab : NULL),perm,canong,m,n);
                    cvalid = FALSE;
                    ovalid = FALSE;
                }
                else
                    fprintf(ERRFILE,"g is not defined\n\n");
                break;

            case 'v':   /* write vertex degrees */
                minus = FALSE;
                if (gvalid)
                    putdegs(OUTFILE,g,options.linelength,m,n);
                else
                    fprintf(ERRFILE,"g is not defined\n\n");
                break;

            case '%':   /* do Mathon doubling operation */
                minus = FALSE;
                if (gvalid)
                {
#if !MAXN
		    if (2L * ((long)n + 1L) > NAUTY_INFINITY-2)
                    {
                        fprintf(ERRFILE,
			     "n can't be more than %d\n\n",NAUTY_INFINITY-2);
                        break;
                    }
#else
                    if (2L * ((long)n + 1L) > MAXN)
                    {
                        fprintf(ERRFILE,"n can't be more than %d\n\n",MAXN);
                        break;
                    }
#endif
                    newn = 2 * (n + 1);
                    newm = (newn + WORDSIZE - 1) / WORDSIZE;
#if !MAXN
		    DYNALLOC2(graph,canong,canong_sz,n,m,"dreadnaut");
#endif

                    for (li = (long)n * (long)m; --li >= 0;)
                        canong[li] = g[li];

#if !MAXN
                    DYNALLOC2(graph,g,g_sz,newn,newm,"dreadnaut");
                    DYNALLOC1(int,lab,lab_sz,newn,"dreadnaut");
                    DYNALLOC1(int,ptn,ptn_sz,newn,"dreadnaut");
		    worksize = 2*WORKSIZE*newm;
                    DYNALLOC1(setword,workspace,workspace_sz,
                                                        worksize,"dreadnaut");
                    DYNALLOC1(int,orbits,orbits_sz,newn,"dreadnaut");
                    DYNALLOC1(permutation,perm,perm_sz,newn,"dreadnaut");
                    DYNALLOC1(set,active,active_sz,newm,"dreadnaut");
#endif
                    mathon(canong,m,n,g,newm,newn);
                    m = newm;
                    n = newn;
                    cvalid = FALSE;
                    ovalid = FALSE;
                    pvalid = FALSE;
                }
                else
                    fprintf(ERRFILE,"g is not defined\n\n");
                break;

            case 's':   /* generate random graph */
                minus = FALSE;
                i = getint(INFILE);
                if (i <= 0) i = 2;
                rangraph(g,options.digraph,i,m,n);
                gvalid = TRUE;
                cvalid = FALSE;
                ovalid = FALSE;
                break;

            case 'q':   /* quit */
                EXIT;
                break;

            case '"':   /* copy comment to output */
                minus = FALSE;
                copycomment(INFILE,OUTFILE,'"');
                break;

            case 'I':   /* do refinement and invariants procedure */
                if (!pvalid)
                    unitptn(lab,ptn,&numcells,n);
                cellstarts(ptn,0,active,m,n);
#ifdef  CPUTIME
                timebefore = CPUTIME;
#endif
                doref(g,lab,ptn,0,&numcells,&qinvar,perm,active,&refcode,
                        options.userrefproc ? options.userrefproc : 
			(m == 1 ? refine1 : refine),
                        options.invarproc,0,0,
                        options.invararg,options.digraph,m,n);
#ifdef  CPUTIME
                timeafter = CPUTIME;
#endif
                fprintf(OUTFILE," %d cell%s; code = %x",
                        SS(numcells,"","s"),refcode);
                if (options.invarproc != NULL)
                    fprintf(OUTFILE," (%s %s)",invarprocname,
                        (qinvar == 2 ? "worked" : "failed"));
#ifdef  CPUTIME
                fprintf(OUTFILE,"; cpu time = %.2f seconds\n",
                        timeafter-timebefore);
#else
                fprintf(OUTFILE,"\n");
#endif
                if (numcells > 1) pvalid = TRUE;
                break;

            case 'i':   /* do refinement */
                if (!pvalid)
                    unitptn(lab,ptn,&numcells,n);
                cellstarts(ptn,0,active,m,n);
		if (options.userrefproc)
		    (*options.userrefproc)
                         (g,lab,ptn,0,&numcells,perm,active,&refcode,m,n);
                else if (m == 1)
                    refine1(g,lab,ptn,0,&numcells,perm,active,&refcode,m,n);
                else
                    refine(g,lab,ptn,0,&numcells,perm,active,&refcode,m,n);
                fprintf(OUTFILE," %d cell%s; code = %x\n",
                        SS(numcells,"","s"),refcode);
                if (numcells > 1) pvalid = TRUE;
                break;

            case 'x':   /* execute nauty */
                minus = FALSE;
                ovalid = FALSE;
                cvalid = FALSE;
                if (!gvalid)
                {
                    fprintf(ERRFILE,"g is not defined\n\n");
                    break;
                }
                if (pvalid)
                {
                    fprintf(OUTFILE,"[fixing partition]\n");
                    options.defaultptn = FALSE;
                }
                else
                    options.defaultptn = TRUE;
                options.outfile = outfile;

                if (options.getcanon)
                {
#if !MAXN
		    DYNALLOC2(graph,canong,canong_sz,n,m,"dreadnaut");
#endif
                }

                firstpath = TRUE;
		options.writeautoms = options_writeautoms;
		options.writemarkers = options_writemarkers;
#ifdef  CPUTIME
                timebefore = CPUTIME;
#endif
		for (i = 0; i < multiplicity; ++i)
		{
                    nauty(g,lab,ptn,NULL,orbits,&options,&stats,workspace,
                         worksize,m,n,canong);
		    options.writeautoms = FALSE;
                    options.writemarkers = FALSE;
		}
#ifdef  CPUTIME
                timeafter = CPUTIME;
#endif
                if (stats.errstatus != 0)
                    fprintf(ERRFILE,
                      "nauty returned error status %d [this can't happen]\n\n",
                       stats.errstatus);
                else
                {
                    if (options.getcanon) cvalid = TRUE;
                    ovalid = TRUE;
                    fprintf(OUTFILE,"%d orbit%s",SS(stats.numorbits,"","s"));
                    if (stats.grpsize2 == 0)
                        fprintf(OUTFILE,"; grpsize=%.0f",stats.grpsize1+0.1);
                    else
                    {
                        while (stats.grpsize1 >= 10.0)
                        {
                            stats.grpsize1 /= 10.0;
                            ++stats.grpsize2;
                        }
                        fprintf(OUTFILE,"; grpsize=%12.10fe%d",
                                   stats.grpsize1,stats.grpsize2);
                    }
                    fprintf(OUTFILE,"; %d gen%s",
                            SS(stats.numgenerators,"","s"));
                    fprintf(OUTFILE,"; %ld node%s",SS(stats.numnodes,"","s"));
                    if (stats.numbadleaves)
                        fprintf(OUTFILE," (%ld bad lea%s)",
                                SS(stats.numbadleaves,"f","ves"));
                    fprintf(OUTFILE,"; maxlev=%d\n", stats.maxlevel);
                    fprintf(OUTFILE,"tctotal=%ld",stats.tctotal);
                    if (options.getcanon)
                        fprintf(OUTFILE,"; canupdates=%ld",stats.canupdates);
#ifdef  CPUTIME
                    fprintf(OUTFILE,multiplicity == 1 ?
			      "; cpu time = %.2f seconds\n" :
			      "; cpu time = %.5f seconds\n",
                            (timeafter-timebefore)/multiplicity);
#else
                    fprintf(OUTFILE,"\n");
#endif
                    if (options.invarproc != NULL &&
                                           options.maxinvarlevel != 0)
                    {
                        fprintf(OUTFILE,"invarproc \"%s\" succeeded %ld/%ld",
                            invarprocname,stats.invsuccesses,stats.invapplics);
                        if (stats.invarsuclevel > 0)
                            fprintf(OUTFILE," beginning at level %d.\n",
                                    stats.invarsuclevel);
                        else
                            fprintf(OUTFILE,".\n");
                    }
                }
                break;

            case 'f':   /* read initial partition */
                if (minus)
                {
                    pvalid = FALSE;
                    minus = FALSE;
                }
                else
                {
                    readptn(INFILE,lab,ptn,&numcells,prompt,n);
                    pvalid = TRUE;
                }
                break;

            case 't':   /* type graph */
                minus = FALSE;
                if (!gvalid)
                    fprintf(ERRFILE,"g is not defined\n\n");
                else
                    putgraph(OUTFILE,g,options.linelength,m,n);
                break;

            case 'T':   /* type graph preceded by n, $ and g commands */
                minus = FALSE;
                if (!gvalid)
                    fprintf(ERRFILE,"g is not defined\n\n");
                else
                {
                    fprintf(OUTFILE,"n=%d $=%d g\n",n,labelorg);
                    putgraph(OUTFILE,g,options.linelength,m,n);
                    fprintf(OUTFILE,"$$\n");
                }
                break;

            case 'u':   /* call user procs */
                if (minus)
                {
                    umask = 0;
                    minus = FALSE;
                }
                else
                {
                    umask = getint(INFILE);
                    if (umask < 0)
                        umask = ~0;
                }
                if (umask & U_NODE)  options.usernodeproc = NODEPROC;
                else                 options.usernodeproc = NULL;
                if (umask & U_AUTOM) options.userautomproc = AUTOMPROC;
                else                 options.userautomproc = NULL;
                if (umask & U_LEVEL) options.userlevelproc = LEVELPROC;
                else                 options.userlevelproc = NULL;
                if (umask & U_TCELL) options.usertcellproc = TCELLPROC;
                else                 options.usertcellproc = NULL;
                if (umask & U_REF)   options.userrefproc = REFPROC;
                else                 options.userrefproc = NULL;
                break;

            case 'o':   /* type orbits */
                minus = FALSE;
                if (ovalid)
                    putorbits(OUTFILE,orbits,options.linelength,n);
                else
                    fprintf(ERRFILE,"orbits are not defined\n\n");
                break;

            case 'b':   /* type canonlab and canong */
                minus = FALSE;
                if (cvalid)
                    putcanon(OUTFILE,lab,canong,options.linelength,m,n);
                else
                    fprintf(ERRFILE,"h is not defined\n\n");
                break;

            case 'z':   /* type hashcode for canong */
                minus = FALSE;
                if (cvalid)
		{
		    zseed = n;
		    for (i = 0, gp = canong; i < n; ++i, gp += m)   
			zseed = sethash(gp,n,zseed,321);
                    fprintf(OUTFILE,"[%7lx",zseed);
		    
                    for (i = 0, gp = canong; i < n; ++i, gp += m) 
                        zseed = sethash(gp,n,zseed,3109);
                    fprintf(OUTFILE," %7lx",zseed);
                    
                    for (i = 0, gp = canong; i < n; ++i, gp += m) 
                        zseed = sethash(gp,n,zseed,4317); 
                    fprintf(OUTFILE," %7lx]\n",zseed);
		}
                else
                    fprintf(ERRFILE,"h is not defined\n\n");
                break;

            case 'c':   /* set getcanon option */
                options.getcanon = !minus;
                minus = FALSE;
                break;

            case 'w':   /* read size of workspace */
                minus = FALSE;
                worksize = getint(INFILE);
#if !MAXN
		DYNALLOC1(setword,workspace,workspace_sz,worksize,"dreadnaut");
#else
                if (worksize > 2*MAXM*WORKSIZE)
                {
                    fprintf(ERRFILE,
                       "too big - setting worksize = %d\n\n", 2*MAXM*WORKSIZE);
                    worksize = 2*MAXM*WORKSIZE;
                }
#endif
                break;

            case 'l':   /* read linelength for output */
                options.linelength = getint(INFILE);
                minus = FALSE;
                break;

            case 'y':   /* set tc_level field of options */
                options.tc_level = getint(INFILE);
                minus = FALSE;
                break;

            case 'M':   /* set multiplicity */
                multiplicity = getint(INFILE);
		if (multiplicity <= 0) multiplicity = 1;
                minus = FALSE;
                break;

            case 'k':   /* set invarlev fields of options */
                options.mininvarlevel = getint(INFILE);
                options.maxinvarlevel = getint(INFILE);
                minus = FALSE;
                break;

            case 'K':   /* set invararg field of options */
                options.invararg = getint(INFILE);
                minus = FALSE;
                break;

            case '*':   /* set invarproc field of options */
                minus = FALSE;
                d = getint(INFILE);
                if (d >= -1 && d <= NUMINVARS-2)
                {
                    options.invarproc = invarproc[d+1].entrypoint;
                    invarprocname = invarproc[d+1].name;
		    if (options.invarproc != NULL)
		    {
			options.mininvarlevel = 0;
			options.maxinvarlevel = 1;
			if (options.invarproc == indsets ||
			    options.invarproc == cliques ||
			    options.invarproc == cellind ||
			    options.invarproc == cellcliq)
				options.invararg = 3;
			else
			    options.invararg = 0;
		    }
                }
                else
                    fprintf(ERRFILE,"no such vertex-invariant\n\n");
                break;

            case 'a':   /* set writeautoms option */
                options_writeautoms = !minus;
                minus = FALSE;
                break;

            case 'm':   /* set writemarkers option */
                options_writemarkers = !minus;
                minus = FALSE;
                break;

            case 'p':   /* set cartesian option */
                options.cartesian = !minus;
                minus = FALSE;
                break;

            case 'd':   /* set digraph option */
                if (options.digraph && minus)
                    gvalid = FALSE;
                options.digraph = !minus;
                minus = FALSE;
                break;

            case '$':   /* set label origin */
                if ((d = getc(INFILE)) == '$')
                    labelorg = oldorg;
                else
                {
                    ungetc((char)d,INFILE);
                    oldorg = labelorg;
                    i = getint(INFILE);
                    if (i < 0)
                        fprintf(ERRFILE,"labelorg must be >= 0\n\n");
                    else
                        labelorg = i;
                }
                break;

            case '?':   /* type options, etc. */
                minus = FALSE;
                fprintf(OUTFILE,"m=%d n=%d labelorg=%d",m,n,labelorg);
                if (!gvalid)
                    fprintf(OUTFILE," g=undef");
                else
                {
                    li = 0;
                    for (i = 0, gp = g; i < n; ++i, gp += m)
                        li += setsize(gp,m);
                    if (options.digraph)
                        fprintf(OUTFILE," arcs=%ld",li);
                    else
                        fprintf(OUTFILE," edges=%ld",li/2);
                }
                fprintf(OUTFILE," options=(%cc%ca%cm%cp%cd",
                            PM(options.getcanon),PM(options_writeautoms),
                            PM(options_writemarkers),PM(options.cartesian),
                            PM(options.digraph));
                if (umask & 31)
                    fprintf(OUTFILE," u=%d",umask&31);
                if (options.tc_level > 0)
                    fprintf(OUTFILE," y=%d",options.tc_level);
                if (options.mininvarlevel != 0 || options.maxinvarlevel != 0)
                    fprintf(OUTFILE," k=(%d,%d)",
                                  options.mininvarlevel,options.maxinvarlevel);
                if (options.invararg > 0)
                    fprintf(OUTFILE," K=%d",options.invararg);
		if (multiplicity > 1) fprintf(OUTFILE," M=%d",multiplicity);
                fprintf(OUTFILE,")\n");
                fprintf(OUTFILE,"linelen=%d worksize=%d input_depth=%d",
                                options.linelength,worksize,curfile);
                if (options.invarproc != NULL)
                    fprintf(OUTFILE," invarproc=%s",invarprocname);
                if (pvalid)
                    fprintf(OUTFILE,"; %d cell%s",SS(numcells,"","s"));
                else
                    fprintf(OUTFILE,"; 1 cell");
                fprintf(OUTFILE,"\n");
                if (OUTFILE != PROMPTFILE)
                    fprintf(PROMPTFILE,"m=%d n=%d depth=%d labelorg=%d\n",
                            m,n,curfile,labelorg);
                break;

            case '&':   /* list the partition and possibly the quotient */
                if ((d = getc(INFILE)) == '&')
                    doquot = TRUE;
                else
                {
                    ungetc((char)d,INFILE);
                    doquot = FALSE;
                }
                minus = FALSE;
                if (pvalid)
                    putptn(OUTFILE,lab,ptn,0,options.linelength,n);
                else
                    fprintf(OUTFILE,"unit partition\n");
                if (doquot)
                {
                    if (!pvalid) unitptn(lab,ptn,&numcells,n);
                    putquotient(OUTFILE,g,lab,ptn,0,options.linelength,m,n);
                }
                break;

            case 'h':   /* type help information */
	    case 'H':
                minus = FALSE;
                help(PROMPTFILE,c == 'H');
                break;

            default:    /* illegal command */
                fprintf(ERRFILE,"'%c' is illegal - type 'h' for help\n\n",c);
                flushline(INFILE);
                if (prompt) fprintf(PROMPTFILE,"> ");
                break;

            }  /* end of switch */

	exit(0);
}

/*****************************************************************************
*                                                                            *
*  help(f,i) writes help information to file f (i = 0,1).                    *
*                                                                            *
*****************************************************************************/

static void
help(FILE *f, int i)
{
#define H(ss) fprintf(f," %s\n",ss);

if (i == 0)
{
H("+- a : write automs        v : write degrees    *=# : select invariant:")
H("   b : write canong      w=# : set worksize")
H("+- c : canonise            x : run nauty         -1 = user-defined")
H("+- d : digraph or loops  y=# : set tc_level       0 = none")
H("   e : edit graph          z : write hashcode     1 = twopaths")
H("-f, f=#, f=[...] : set colours                    2 = adjtriang(K=0,1)")
H("   g : read graph        $=# : set origin         3 = triples")
H(" h,H : help               $$ : restore origin     4 = quadruples")
H("   i : refine              ? : type options       5 = celltrips")
H("   I : refine using invar  _ : compl  __ : conv   6 = cellquads")
H("   j : relabel randomly    % : Mathon doubling    7 = cellquins")
H("k=# # : set invar levels   & : type colouring     8 = distances(K)")
H(" K=# : set invar param    && : + quotient matrix  9 = indsets(K)")
H(" l=# : set line length   >ff : write to file     10 = cliques(K)")
H("+- m : write markers    >>ff : append to file    11 = cellcliq(K)")
H(" n=# : set order          -> : revert to stdout  12 = cellind(K)")
H("   o : write orbits      <ff : read from file    13 = adjacencies")
H("+- p : set autom format    @ : save canong       14 = cellfano")
H("   q : quit                # : canong = savedg?  15 = cellfano2")
H(" r,R : relabel/subgraph   ## : + write mapping")
H(" s=# : random g (p=1/#)  \"...\" : copy comment")
H(" t,T : type graph          ! : ignore line      Type H for more..")
}

if (i == 1)
{
H("Commands for g and e : ")
H("   There is always a \"current vertex\" v, initially first vertex.")
H("   # : add edge v=#       ; : increment v (exit if over limit)")
H("  -# : delete edge v=#   #: : set v := #")
H("   ? : list nbhs of v     . : exit")
H("Syntax for f :  f=[2 3|4:9|10]  (rest in extra cell at right)")
H("               -f same as f=[], f=# same as f=[#]")
H("Syntax for r :  r 2:4 1 5;    (rest appended in order)")
H("Syntax for R :  R 2:4 1 5;   or  -R 0 3 6:10;")
H("Arguments for u : 1=node,2=autom,4=level,8=tcell,16=ref (add them)")
H("Accurate times for easy graphs: M=# selects number of times to run.")
}

}

/*****************************************************************************
*                                                                            *
*  usernode(g,lab,ptn,level,numcells,tc,code,m,n) is a simple version of the *
*  procedure named by options.usernodeproc.                                  *
*                                                                            *
*****************************************************************************/

static void
usernode(graph *g, int *lab, int *ptn, int level, int numcells,
         int tc, int code, int m, int n)
{
        register int i;

        for (i = 0; i < level; ++i) PUTC('.',OUTFILE);
        if (numcells == n)
            fprintf(OUTFILE,"(n/%d)\n",code);
        else if (tc < 0)
            fprintf(OUTFILE,"(%d/%d)\n",numcells,code);
        else
            fprintf(OUTFILE,"(%d/%d/%d)\n",numcells,code,tc);
        if (firstpath) putptn(OUTFILE,lab,ptn,level,options.linelength,n);
        if (numcells == n) firstpath = FALSE;
}

/*****************************************************************************
*                                                                            *
*  userautom(count,perm,orbits,numorbits,stabvertex,n) is a simple           *
*  version of the procedure named by options.userautomproc.                  *
*                                                                            *
*****************************************************************************/

static void
userautom(int count, permutation *perm, int *orbits,
          int numorbits, int stabvertex, int n)
{
        fprintf(OUTFILE,
             "**userautomproc:  count=%d stabvertex=%d numorbits=%d\n",
             count,stabvertex+labelorg,numorbits);
        putorbits(OUTFILE,orbits,options.linelength,n);
}

/*****************************************************************************
*                                                                            *
*  userlevel(lab,ptn,level,orbits,stats,tv,index,tcellsize,numcells,cc,n)    *
*  is a simple version of the procedure named by options.userlevelproc.      *
*                                                                            *
*****************************************************************************/

static void
userlevel(int *lab, int *ptn, int level, int *orbits, statsblk *stats,
          int tv, int index, int tcellsize, int numcells, int cc, int n)
{
      fprintf(OUTFILE,
            "**userlevelproc:  level=%d tv=%d index=%d tcellsize=%d cc=%d\n",
            level,tv+labelorg,index,tcellsize,cc);
      fprintf(OUTFILE,"    nodes=%ld cells=%d orbits=%d generators=%d\n",
            stats->numnodes,numcells,stats->numorbits,stats->numgenerators);
}
