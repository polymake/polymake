/* Copyright (c) 1997-2014
   Ewgenij Gawrilow, Michael Joswig (Technische Universitaet Berlin, Germany)
   http://www.polymake.org

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version: http://www.gnu.org/licenses/gpl.txt.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
--------------------------------------------------------------------------------
*/

#include "polymake/client.h"
#include "polymake/Set.h"
#include "polymake/Graph.h"
#include "polymake/Array.h"
#include "polymake/Matrix.h"
/* #include "Singular/mod2.h"
   #include "kernel/structs.h"
   #include "kernel/ring.h"
   #include "kernel/febase.h"
   #include "kernel/ideals.h"
   #include "kernel/matpol.h"
   #include "Singular/MinorInterface.h"
*/

namespace polymake { namespace tropical {
   
   template <typename Coord>
   Array< Array < Set <int> > > types(const Matrix<Coord>& points, const Matrix<Coord>& generators)
   {
      const int m(points.rows()), n(generators.rows()), d(generators.cols());
      
      // This code was produced by Hans Schoenemann and Katja Kulas for integrating singular functions to polymake.
      // A ring is defined and printed out.
      /*    feInitResources("/home/katja/Singular-svn/Singular/libsingular.so");
            char** var=(char**)malloc(n*d*sizeof(char*));
            for (int i=0; i< n;i++)
            for (int j=0; j< d;j++)
            {
            char* c = (char*)malloc(23);
            var[i*d+j]=c;
            sprintf(c,"x%d_%d",i,j);
            }
            
            int *o=(int*)malloc(2*sizeof(int)); o[0]=ringorder_wp; o[1]=0;
            int *b0=(int*)malloc(2*sizeof(int)); b0[0]=1;
            int *b1=(int*)malloc(2*sizeof(int)); b1[0]=n*d;
            
            int *weights=(int*)malloc(n*d*sizeof(int));
            for (int i=0; i< n;i++)
            for (int j=0; j< d;j++)
            {
            weights[i*d+j]=points[i][j];
            }
            
            ring r=(ring)malloc(sizeof(*r));
            memset(r,0,sizeof(*r));
            r->ch=0;
            r->N=n*d;
            r->names=var;
            r->order=o;
            r->block0=b0;
            r->block1=b1;
            r->OrdSgn=1; //global ring
            r->wvhdl=(int**)malloc(2*sizeof(int*));
            r->wvhdl[0]=weights;
            rComplete(r,1);
            rWrite(r);
            rChangeCurrRing(r);
            
            matrix M=mpNew(n,d);
            ideal I=idMaxIdeal(1);
            for(int i=0;i<n*d;i++) { M->m[i]=I->m[i]; I->m[i]=NULL; }
            idDelete(&I);
            I=idMinors(M,2);
            char *output_s=iiStringMatrix((matrix)I,1,' ');
            cout<< output_s<<endl;
      */
      Array< Array < Set<int> > > types(m);
      
      for (int j=0; j<m; ++j) { // determine the type of each point
         Array< Set<int> > this_type(d);
         
         for (int i=0; i<n; ++i) { // iterate over all generators
            Vector<Coord> diff = generators[i]-points[j];
            Coord m = accumulate(diff, operations::min());
            for (int k=0; k<d; ++k)
               if (diff[k]==m) this_type[k] += i;
         }
         types[j] = this_type;
      }
      
      return Array< Array< Set <int> > >(types);
   }
   
   template <typename Coord>
   Array< Array<int> >  coarse_types(const Matrix<Coord>& points, const Matrix<Coord>& generators)
   {
      const int m(points.rows()), n(generators.rows()), d(generators.cols());
      Array< Array<int> > coarse_types(m);
      
      for (int j=0; j<m; ++j) { // determine the coarse type of each point
         Array<int> this_coarse_type(d);
         
         for (int i=0; i<n; ++i) { // iterate over all generators
            Vector<Coord> diff = generators[i]-points[j];
            Coord m = accumulate(diff, operations::min());
            for (int k=0; k<d; ++k)
               if (diff[k]==m) ++this_coarse_type[k];
         }
         
         coarse_types[j] = this_coarse_type;
      }
      
      return Array< Array<int> >(coarse_types);
   }
   
   UserFunctionTemplate4perl("# @category Other"
                             "# Compute the fine types of the //points// set relative to a set of //generators//."
                             "#  The following are two typical cases:"
                             "#    (1) //points// = [[TropicalPolytope::VERTICES|VERTICES]] and //generators// = [[TropicalPolytope::VERTICES|VERTICES]]"
                             "#    (2) //points// = [[TropicalPolytope::POINTS|POINTS]]  and //generators// = [[TropicalPolytope::PSEUDOVERTICES|PSEUDOVERTICES]]"
                             "# @param Matrix points"
                             "# @param Matrix generators"
                             "# @return Array<Array<Set>>",
                             "types<Coord>(Matrix<Coord> Matrix<Coord>)");
   
   UserFunctionTemplate4perl("# @category Other"
                             "# Compute the coarse types of the //points// set relative to a set of //generators//."
                             "#  The following are two typical cases:"
                             "#    (1) //points// = [[TropicalPolytope::VERTICES|VERTICES]] and //generators// = [[TropicalPolytope::VERTICES|VERTICES]]"
                             "#    (2) //points// = [[TropicalPolytope::POINTS|POINTS]]  and //generators// = [[TropicalPolytope::PSEUDOVERTICES|PSEUDOVERTICES]]"
                             "# @param Matrix points"
                             "# @param Matrix generators"
                             "# @return Array< Array<int>>",
                             "coarse_types<Coord>(Matrix<Coord> Matrix<Coord>)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
