/* Copyright (c) 1997-2023
   Ewgenij Gawrilow, Michael Joswig, and the polymake team
   Technische Universität Berlin, Germany
   https://polymake.org

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
#include "polymake/Matrix.h"
#include "polymake/linalg.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/common/labels.h"

namespace polymake { namespace polytope {

typedef Rational Scalar; // to prepare complete templatification

namespace {

template <typename PointsSubset, typename Hyperplane>
void lift_to_hyperplane(PointsSubset&& P, const GenericVector<Hyperplane>& U)
{
   const Int d = U.dim()-1;
   for (auto p = entire(P); !p.at_end(); ++p) {
      auto p_i = p->begin();
      auto u_i = U.top().begin();
      typename Hyperplane::element_type x(0);
      /*
         Find the (d+1)st coordinate of the lifted point hat(P).
         hat(P) must lie on the hyperplane U, with equation (u_0,...,u_d),
         thus we need the p_d such that
               <  (u_0,...,u_d), (p_0,...,p_d) > == 0 ;
         Therefore, 
               p_d = - < (u_0,...,u_{d-1}), (p_0,...,p_{d-1}) > / u_d
       */
      for (Int i = 0; i < d; ++i, ++p_i, ++u_i)
         x -= (*p_i) * (*u_i);
      (*p_i) = x / (*u_i);
   }
}

   //template<typename Scalar>
Matrix<Scalar>
wedge_coord(const Matrix<Scalar>& V,          // input vertices
            const Vector<Scalar>& z0,         // interior point
            const IncidenceMatrix<>& VIF,       // VERTICES_IN_FACETS
            Int wedge_facet,
            const Scalar& z,
            const Scalar& z_prime)
{
   const Int d = V.cols(),
      n_vertices = VIF.cols(),
      n_vertices_edge = VIF[wedge_facet].size(),
      n_vertices_out =  2*n_vertices-n_vertices_edge;

   Matrix<Scalar> V_out=(V / V.minor(~VIF[wedge_facet],All)) | zero_vector<Scalar>();

   // top (or bottom) facet goes thru the barycenter projection and the edge facet,
   // so we calculate a basis for the orthogonal complement of the new facet.
   const Matrix<Scalar> facet=
      null_space( ( z0                          | (z_prime != 0 ? z_prime : z)) /
                  (V.minor(VIF[wedge_facet],All) | zero_vector<Scalar>()) );

   // find a row of facet that has non-zero last coordinate.
   auto rit = entire(rows(facet));
   while (!rit.at_end() && is_zero(rit->back()))
      ++rit;
   if (rit.at_end()) throw std::runtime_error("Encountered a problem with the new facet.");

   if (z_prime != 0) {
      lift_to_hyperplane(select(rows(V_out), range(n_vertices, n_vertices_out-1)), *rit);

      if (z != 0)    // bottom side facet has to be inclined too
         V_out.col(d).slice(sequence(0, n_vertices)-VIF[wedge_facet]) =
            z/z_prime * V_out.col(d).slice(range(n_vertices,n_vertices_out-1));
   } else {
      lift_to_hyperplane(select(rows(V_out), sequence(0,n_vertices)-VIF[wedge_facet]), *rit);
   }

   return V_out;
}

} // end unnamed namespace

//template<typename Scalar>
BigObject wedge(BigObject p_in, const Int wedge_facet, const Scalar& z, const Scalar& z_prime, OptionSet options)
{
   if (!options["no_coordinates"] && z==z_prime)
      throw std::runtime_error("wedge: z and z' must not be equal");

   const IncidenceMatrix<> VIF = p_in.give("VERTICES_IN_FACETS");
   const Int 
      n_vertices = VIF.cols(), 
      n_facets   = VIF.rows();
  
   if (wedge_facet < 0 || wedge_facet >= n_facets)
      throw std::runtime_error("wedge: edge facet number out of range");
  
   const Int 
      n_vertices_edge = VIF[wedge_facet].size(),
      n_vertices_out  = 2*n_vertices - n_vertices_edge;

   IncidenceMatrix<> VIF_out=
      (VIF | VIF.minor(All, ~VIF[wedge_facet])) // side facet = original facet + its clone - intersection with the edge
      / range(0,n_vertices-1);            // bottom facet = original polytope

   // top facet = edge facet + all clones
   VIF_out[wedge_facet]+=range(n_vertices,n_vertices_out-1);

   BigObject p_out("Polytope", mlist<Scalar>());
   p_out.set_description() << "wedge over " << p_in.name() << "; edge facet " << wedge_facet << endl;

   if (options["no_coordinates"]) {
      if (p_in.exists("COMBINATORIAL_DIM")) {
         const Int dim = p_in.give("COMBINATORIAL_DIM");
         p_out.take("COMBINATORIAL_DIM") << dim+1;
      }
   }

   p_out.take("N_VERTICES") << VIF_out.cols(); 
   p_out.take("VERTICES_IN_FACETS") << VIF_out;

   if (!options["no_labels"]) {
      std::vector<std::string> labels(n_vertices_out);
      common::read_labels(p_in, "VERTEX_LABELS", select(labels, sequence(0, n_vertices)));
      const std::string tick="'";

      copy_range(entire(attach_operation(select(labels, sequence(0,n_vertices)-VIF[wedge_facet]),
                                         same_value(tick), operations::add())),
                 labels.begin()+n_vertices);

      p_out.take("VERTEX_LABELS") << labels;
   }

   if (!options["no_coordinates"]) {
      const bool bounded=p_in.give("BOUNDED");
      if (!bounded)
         throw std::runtime_error("wedge: input polyhedron must be bounded");

      const Matrix<Scalar> V=p_in.give("VERTICES");
      const Vector<Scalar> z0=p_in.give("VERTEX_BARYCENTER");
      const Matrix<Scalar> V_out=wedge_coord(V,z0,VIF,wedge_facet,z,z_prime);
      p_out.take("VERTICES") << V_out;
      p_out.take("BOUNDED") << true;
   }
   return p_out;    
}

// TODO: replace Rational with Scalar in @params when templatification is accomplished
UserFunction4perl("# @category Producing a polytope from polytopes"
                  "# Make a wedge from a polytope over the given //facet//.  The polytope must be bounded."
                  "# The inclination of the bottom and top side facet is controlled by //z// and //z_prime//,"
                  "# which are heights of the projection of the old vertex barycenter"
                  "# on the bottom and top side facet respectively."
                  "# @param Polytope P, must be bounded"
                  "# @param Int facet the `cutting edge'."
                  "# @param Rational z default value is 0."
                  "# @param Rational z_prime default value is -//z//, or 1 if //z//==0."
                  "# @option Bool no_coordinates don't compute coordinates, pure combinatorial description is produced."
                  "# @option Bool no_labels Do not copy [[VERTEX_LABELS]] from the original polytopes. default: 0"
                  "#  By default, the vertices get labelled as follows:"
                  "#  The bottom facet vertices obtain the labels from the original polytope;"
                  "#  the labels of their clones in the top facet get a tick (') appended."
                  "# @return Polytope"
                  "# @author Kerstin Fritzsche (initial version)"
                  "# @example This produces the wedge from a square (over the facet 0), which yields a prism over a triangle:"
                  "# > $p = wedge(cube(2),0);"
                  "# > print $p->VERTICES;"
                  "# | 1 -1 -1 0"
                  "# | 1 1 -1 0"
                  "# | 1 -1 1 0"
                  "# | 1 1 1 0"
                  "# | 1 1 -1 2"
                  "# | 1 1 1 2",
                  &wedge,
                  "wedge(Polytope, $; $=0, $=($_[2]==0 ? 1 : -$_[2]), { no_coordinates => undef, no_labels => 0})");

//                "wedge<Scalar>(Polytope<type_upgrade<Scalar>>, Int; type_upgrade<Scalar>=0, type_upgrade<Scalar>=($_[2]==0 ? 1 : -$_[2]), { no_coordinates => undef, relabel => undef})");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
