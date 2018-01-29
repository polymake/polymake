/* Copyright (c) 1997-2018
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
#include "polymake/list"
#include "polymake/Matrix.h"
#include "polymake/Graph.h"
#include "polymake/Array.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/linalg.h"

namespace polymake { namespace polytope {

/*  Convert the combinatorial description of a 2-d or 3-d polytope
 *  to the form suitable for visualization tools.
 *
 *  The rows of the vertex-facet incidence and the facet neighborhood matrices are rearranged
 *  in the facet boundary counterclockwise traversal order, if seen from the outside of the polytope.
 *
 *  In dual mode, the notions of facets and vertices are interchanged.
 */

namespace {

typedef Array< std::list<int> > CycleList;

template <typename VMatrix>
bool reverse_edge(const GenericMatrix<VMatrix>& V, const Array<int>& pts)
{
   return det(V.minor(pts, All)) > 0;
}

template <typename VMatrix>
bool reverse_edge(const GenericMatrix<VMatrix>& V, const GenericMatrix<VMatrix>& AH, const Array<int>& pts)
{
   VMatrix AHc=AH;
   AHc.col(0).fill(0);
   AHc+=repeat_row(V[pts[0]],AH.rows());
   return det(V.minor(pts, All) / AHc) > 0;
}

template <typename VMatrix, typename IMatrix>
void compute_cycles(int dim,
                    const GenericMatrix<VMatrix>& V,
                    const GenericMatrix<VMatrix>& AH,
                    const GenericIncidenceMatrix<IMatrix>& VIF,
                    const Graph<>& DG,
                    CycleList& VIF_cyclic, CycleList& DG_cyclic)
{
   if (dim==4) { // 3-d case

      const int n_facets=VIF.rows();
      VIF_cyclic.resize(n_facets);
      DG_cyclic.resize(n_facets);

      int nf=DG.adjacent_nodes(0).front();              // starting neighbor facet
      {
         // in the start facet, the right orientation must be chosen using vertex coordinates
         const Set<int> ridge=VIF[0] * VIF[nf];
         int v1=ridge.front(), v2=ridge.back();
         const int p1=(VIF[0] - VIF[nf]).front();       // some vertex on the facet 0 but NOT on the ridge
         const int p2=(~VIF)[0].front();                // some vertex NOT on the facet 0
         Array<int> pts={ v1, v2, p1, p2 };

         if (AH.rows()) {
            if (reverse_edge(V, AH, pts)) std::swap(v1,v2);
         } else {
            if (reverse_edge(V, pts)) std::swap(v1,v2);
         }
         VIF_cyclic[0].push_back(v1);
         VIF_cyclic[0].push_back(v2);
      }
      DG_cyclic[0].push_back(nf);
      std::list<int> Q;
      Q.push_back(0);

      while (!Q.empty()) {
         const int cf=Q.front(); Q.pop_front();
         const int v0=VIF_cyclic[cf].front();
         int v=VIF_cyclic[cf].back();
         nf=DG_cyclic[cf].front();

         while (true) {
            nf=(VIF.col(v) * DG.adjacent_nodes(cf) - nf).front();
            DG_cyclic[cf].push_back(nf);
            const int v2=(VIF[nf] * VIF[cf] - v).front();
            if (DG_cyclic[nf].empty()) {
               // this ridge has the opposite direction in the neighbor facet border!
               VIF_cyclic[nf].push_back(v2); VIF_cyclic[nf].push_back(v);
               DG_cyclic[nf].push_back(cf);
               Q.push_back(nf);
            }
            if (v2==v0) break;
            VIF_cyclic[cf].push_back(v2);  v=v2;
         }
      }

   } else if (dim==3) { // 2-d case

      VIF_cyclic.resize(1);

      int v0=VIF[0].back(), v=VIF[0].front();
      if (V.cols()==3) {
         // if the n-gon is embedded in R^3, then it can be seen from both sides anyway
         int v2=(~VIF)[0].front();      // some vertex not on the facet 0
         if (det( V.minor(VIF[0],All) / V[v2] ) > 0)
            std::swap(v,v0);
      }
      int f=0;
      while (true) {
         VIF_cyclic[0].push_back(v);
         if (v==v0) break;
         f=(VIF.col(v) - f).front();
         v=(VIF[f] - v).front();
      }

   } else {
      throw std::runtime_error("only 2-d and 3-d polytopes allowed");
   }
}

} // end unnamed namespace

template <typename Scalar>
void neighbors_cyclic_normal_primal(perl::Object p)
{
   const Matrix<Scalar> V=p.give("RAYS"), AH=p.give("LINEAR_SPAN");
   const IncidenceMatrix<> VIF=p.give("RAYS_IN_FACETS");
   const Graph<> DG=p.give("DUAL_GRAPH.ADJACENCY");
   const int dim=p.give("CONE_DIM");

   CycleList VIF_cyclic, DG_cyclic;
   compute_cycles(dim,V,AH,VIF,DG,VIF_cyclic,DG_cyclic);
   p.take("RIF_CYCLIC_NORMAL", perl::temporary) << VIF_cyclic;
   p.take("NEIGHBOR_FACETS_CYCLIC_NORMAL", perl::temporary) << DG_cyclic;
}

template <typename Scalar>
void neighbors_cyclic_normal_dual(perl::Object p)
{
   const Matrix<Scalar> F=p.give("FACETS"), AH;
   const IncidenceMatrix<> VIF=p.give("RAYS_IN_FACETS");
   const Graph<> G=p.give("GRAPH.ADJACENCY");
   const int dim=p.give("CONE_DIM");

   CycleList VIF_cyclic, DG_cyclic;
   compute_cycles(dim,F,AH,T(VIF),G,VIF_cyclic,DG_cyclic);
   p.take("FTR_CYCLIC_NORMAL", perl::temporary) << VIF_cyclic;
   p.take("NEIGHBOR_RAYS_CYCLIC_NORMAL", perl::temporary) << DG_cyclic;
}

FunctionTemplate4perl("neighbors_cyclic_normal_primal<Scalar> (Cone<Scalar>) : void");
FunctionTemplate4perl("neighbors_cyclic_normal_dual<Scalar> (Cone<Scalar>) : void");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
