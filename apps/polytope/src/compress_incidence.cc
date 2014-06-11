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
#include "polymake/linalg.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Set.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/FacetList.h"
#include "polymake/polytope/compress_incidence.h"
#include <sstream>

namespace polymake { namespace polytope {


// this client may fail if POINTS has been modified by canonicalize_coord
// we cannot catch this!
template <typename Scalar>
void compress_incidence_primal(perl::Object p)
{
   Matrix<Scalar> V=p.give("INPUT_RAYS");
   IncidenceMatrix<> VIF=p.give("INPUT_RAYS_IN_FACETS");
   Matrix<Scalar> L=p.lookup("INPUT_LINEALITY");

   cols(VIF).resize(V.rows());     // for the exotic case of the last point(s) being strict inner points
                                   // note that this might still happen as an incidence matrix defined in perl determines its number of columns from the input

   // FacetList does not like empty sets, so we have to take care of this case separately
   if ( VIF.rows() > 1 ) {
      const std::pair< Set<int>, Set<int> > non_vertices=compress_incidence(T(VIF));

      if (non_vertices.second.size() == VIF.cols() ) {
         std::ostringstream err_msg;
         wrap(err_msg) << "illegal point section: pointed part of polyhedron must be specified even for affine spaces";
         throw std::runtime_error(err_msg.str());
      }

      L /= V.minor(non_vertices.second,All); // add points contained in all facets to input lineality, taken care of later
      
      if (!non_vertices.first.empty()) {
         VIF=VIF.minor(All, ~non_vertices.first);
         V=V.minor(~non_vertices.first,All);
      }

   } else {
      if ( VIF.rows() == 1 ) {
         // we have a single point
         // NOTE: we cannot specify an empty polytope via POINTS, so we don't have to catch this case here
         // in that case  the one facet does not meet the point
         if (VIF.row(0).size() == VIF.cols() ) {
            std::ostringstream err_msg;
            wrap(err_msg) << "illegal point section: pointed part of polyhedron must be specified even for affine spaces";
            throw std::runtime_error(err_msg.str());
         }
         
         L /= V.minor(VIF.row(0),All);   // add points contained in the one facet to input lineality, taken care of later
         const int c = (sequence(0,VIF.cols())-VIF.row(0)).front(); // pick any of the remaining points to span the pointed part
         V = V.minor(scalar2set(c),All);                            // FIXME selection of the first zero entry of VIF.row(0) should be easier?
         VIF.resize(1,1); VIF(0,0)=0;
      } else {
         // we have no facet. In this case our object must be a cone, and this cone defines a linear space

         Matrix<Scalar> empty;
         V = empty; // this cone has no rays
      }
   }
   
   // finally take care of the lineality space
   Set<int> b=basis_rows(L);    // compute an affine hull
   if (b.size() < L.rows()) L=L.minor(b,All);
   
   p.take("RAYS") << V;
   p.take("RAYS_IN_FACETS") << VIF;
   p.take("LINEALITY_SPACE") << L;
}

// this client assumes that the the number of rows of the incidence matrix 
// and the number of inequalities coincides
// this requires to specify the far face explicitely in INEQUALITIES and RAYS_IN_INEQUALITIES, 
// otherwise canonicalize_coord adds it to INEQUALITIES but not to RAYS_IN_INEQUALITIES
template <typename Scalar>
void compress_incidence_dual(perl::Object p)
{
   Matrix<Scalar> F=p.give("INEQUALITIES");
   IncidenceMatrix<> VIF=p.give("RAYS_IN_INEQUALITIES");
   Matrix<Scalar> E=p.lookup("EQUATIONS");

   if (VIF.rows() != F.rows() ) {
      std::ostringstream err_msg;
      wrap(err_msg) << "dimension mismatch. Note: the far hyperplane must always be specified explicitly in INEQUALITIES and RAYS_IN_INEQUALITIES";
      throw std::runtime_error(err_msg.str());
   }

   if (VIF.cols() == 0 ) {  // empty polytope
      E /= F;                      // we still have to keep the linear span: otherwise dimension is wrong
      Set<int> b=basis_rows(E);    // compute an affine hull
      if (b.size() < E.rows()) E=E.minor(b,All);
      VIF.resize(0,0);
      F.resize(0,0);
   } else {
      const std::pair< Set<int>, Set<int> > non_facets=compress_incidence(VIF);
      E /= F.minor(non_facets.second,All);
      Set<int> b=basis_rows(E);    // compute an affine hull
      if (b.size() < E.rows()) E=E.minor(b,All);

      if ( VIF.cols() > 1 ) { 
         if (!non_facets.first.empty()) {
            VIF=VIF.minor(~non_facets.first, All);
            F=F.minor(~non_facets.first, All);
         }
      } else {
         // we have a single ray or point. 
         // in that case  the one facet does not meet the ray or point
         // FacetList does not like empty sets, so we have to take care of this separately
         if (p.isa("Polytope"))   // polytopes need the spacial facet [1,0,....]
            F = vector2row(unit_vector<Scalar>(F.cols(),0));
         else {
            const int c = (sequence(0,VIF.rows())-VIF.col(0)).front(); // pick any of the inequalities not containing the ray
            F = F.minor(scalar2set(c),All); 
         }
         VIF.resize(1,1); VIF(0,0)=0;
      }
   }
   
   p.take("FACETS") << F;
   p.take("LINEAR_SPAN") << E;
   p.take("RAYS_IN_FACETS") << VIF;
}

FunctionTemplate4perl("compress_incidence_primal<Scalar> (Cone<Scalar>) : void");
FunctionTemplate4perl("compress_incidence_dual<Scalar> (Cone<Scalar>) : void");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
