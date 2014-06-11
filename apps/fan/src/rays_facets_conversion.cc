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
#include "polymake/Rational.h"
#include "polymake/linalg.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/SparseMatrix.h"
#include "polymake/SparseVector.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Set.h"
#include "polymake/PowerSet.h"

namespace polymake { namespace fan {
namespace {

template <typename Coord>
int signCheck(const Vector<Coord>& v) 
{
   int sgn = 0;
   for(typename Entire< Vector< Coord > >::const_iterator d=entire(v); !d.at_end(); ++d)
   {
      int s = sign(*d);
      if (s != 0) 
      {
         if (s*sgn >= 0)
            sgn = s;
         else 
            return 0;
      }
   }
   return sgn;
}
  
}
  
template <typename Coord>
void raysToFacetNormals(perl::Object f)
{
   const int ambientDim = f.give("FAN_AMBIENT_DIM");
   const Matrix<Coord> rays = f.give("RAYS");
   const IncidenceMatrix<> incidence = f.give("MAXIMAL_CONES");
   const Matrix<Coord> linealitySpace = f.lookup("LINEALITY_SPACE|INPUT_LINEALITY");
    
   Matrix<Coord> facets(0,ambientDim);
   Matrix<Coord> linearSpan(0,ambientDim);
   RestrictedSparseMatrix<int> facetIndices(incidence.rows());
   RestrictedIncidenceMatrix<only_rows> linearSpanIndices(incidence.rows());
    
   int coneNum = 0;
   int linealityDim = rank(linealitySpace);
   int fanDim = rank(rays/linealitySpace);
    
   Matrix<Coord> fanLinearSpan;
   Set<int> fanLinearSpanIndices;
    
   // find linear span of the whole fan
   if (fanDim < ambientDim)
   {
      if (linealityDim > 0)
         fanLinearSpan = null_space(rays/linealitySpace);
      else {
         if (rays.rows() > 0)
            fanLinearSpan = null_space(rays);
         else
            fanLinearSpan = unit_matrix<Coord>(ambientDim);
      }
      
      linearSpan /= fanLinearSpan;
      fanLinearSpanIndices += sequence(0,fanLinearSpan.rows());
   }
    
   // iterate cones
   for(Entire< Rows< IncidenceMatrix<> > >::const_iterator cone=entire(rows(incidence)); !cone.at_end(); ++cone) 
   {
      Set<int> coneSet(*cone);
      Matrix<Coord> coneRays = rays.minor(coneSet,All);
      if (linealityDim > 0)
         coneRays /= linealitySpace / (-linealitySpace);
      
      int coneDim = rank(coneRays);
      int diff = fanDim - coneDim;
      
      linearSpanIndices.row(coneNum) += fanLinearSpanIndices;
      coneRays /= fanLinearSpan;

      // add linear span of this cone if neccessary
      if (diff > 0) 
      {
         for(int linearSpanIndex = (ambientDim-fanDim); linearSpanIndex < linearSpan.rows(); ++linearSpanIndex)
         {
            if (coneRays * (linearSpan.row(linearSpanIndex)) == zero_vector<Coord>(coneRays.rows()))
            {
               linearSpanIndices.row(coneNum) +=linearSpanIndex;
               coneRays /= linearSpan.row(linearSpanIndex);
               diff--;
            }
         }
         if (diff > 0)
         {
            Matrix<Coord> coneLinearSpan = null_space(coneRays);
            linearSpanIndices.row(coneNum) += sequence(linearSpan.rows(), coneLinearSpan.rows());
            linearSpan /= coneLinearSpan;
            diff -= coneLinearSpan.rows();
         }
      }
      
      if (coneDim < 1 || coneSet.size() == 0)
      {
         coneNum++;
         continue;
      }
      
      // iterate possible facets of the current cone
      for (Entire< Subsets_of_k<const Set<int>&> >::const_iterator subset = entire(all_subsets_of_k(coneSet,coneDim-1-linealityDim)); !subset.at_end(); ++subset)
      {
         Matrix<Coord> facetRays = rays.minor(*subset,All);
         if (linealityDim > 0) 
            facetRays /= linealitySpace / (-linealitySpace);
        
         if (rank(facetRays) != coneDim - 1)
            continue;
        
         Vector<Coord> facet;
         if (ambientDim == 1)
            facet = unit_vector<Coord>(1,0);
         else if (coneDim == ambientDim)
            facet = null_space(facetRays)[0];
         else 
            facet = null_space(facetRays / linearSpan.minor(linearSpanIndices.row(coneNum),All))[0];
        
         // check if the cone is on one side
         Vector<Coord> dist = rays.minor((*cone)-(*subset),All) * facet;
         int sgn = signCheck(dist);
        
         if (sgn != 0) 
         {
            // search if this facet normal already exists
            int facetNum = -1;
            typename Entire< Rows< Matrix<Coord> > >::iterator currFacet=entire(rows(facets));
            do 
            {
               facetNum++;
               if(currFacet.at_end())
               {
                  facetNum = facets.rows();
                  facets /= facet;
                  break;
               }
            } while((*(currFacet++)) != facet);
          
            facetIndices(coneNum,facetNum) = sgn;
         }
      }
      
      coneNum++;
   }
    
   // create matrices
   SparseMatrix<int> facetIndicesMatrix(facetIndices);
   IncidenceMatrix<NonSymmetric> linearSpanIndicesMatrix(linearSpanIndices);
    
   f.take("FACET_NORMALS") << facets;
   f.take("MAXIMAL_CONES_FACETS") << facetIndicesMatrix;
   f.take("LINEAR_SPAN_NORMALS") << linearSpan;
   f.take("MAXIMAL_CONES_LINEAR_SPAN_NORMALS") << linearSpanIndicesMatrix;
}

FunctionTemplate4perl("raysToFacetNormals<Coord> (PolyhedralFan<Coord>) : void");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
