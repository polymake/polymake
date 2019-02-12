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
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/Set.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/linalg.h"
#include "polymake/graph/LatticeTools.h"
#include "polymake/polytope/hasse_diagram.h"

namespace polymake { namespace polytope {

namespace {

template <typename Scalar, typename Cmp>
Set<int> violated_rows(const Matrix<Scalar>& A, const Vector<Scalar>& q, Cmp cmp)
{
   Set<int> vr;
   for (auto rit = entire<indexed>(rows(A)); !rit.at_end(); ++rit)
      if (cmp(*rit * q)) vr += rit.index();
   return vr;
}

template<typename Scalar>
inline
void fudge_min_face(Set<int>& min_face, const Matrix<Scalar>&, const Vector<Scalar>&, const Scalar&, const double&)
{
}

template<>
inline
void fudge_min_face(Set<int>& min_face,
                    const Matrix<double>& V,
                    const Vector<double>& lin_obj,
                    const double& min_val,
                    const double& epsilon)
{
   for (int i=0; i<V.rows(); ++i)
      if (fabs(V[i] * lin_obj - min_val) < epsilon)
         min_face += i;
}

Lattice<BasicDecoration, Sequential>
ridges_and_facets(const IncidenceMatrix<>& VIF,
                  const int cone_dim)
{
   using graph::lattice::BasicDecorator;
   using graph::lattice::BasicClosureOperator;
   using graph::lattice::RankCut;
      
   const int total(VIF.rows()), boundary_dim(cone_dim-2);
   BasicClosureOperator<> cop(total,T(VIF));
   auto cut_above = RankCut<BasicDecoration,graph::lattice::RankCutType::GreaterEqual>(boundary_dim);
   BasicDecorator<> dec(VIF.cols(), cone_dim, scalar2set(-1));

   Lattice<BasicDecoration, Sequential> init_lattice;
   return graph::lattice_builder::compute_lattice_from_closure<BasicDecoration>(cop, cut_above, dec, 1, graph::lattice_builder::Dual(), init_lattice);
}

} // end anonymous namespace

      
template<typename Scalar>
Set<int> violations(const perl::Object P, const Vector<Scalar>& q, perl::OptionSet options)
{
   const std::string section = options["section"];
   const int violating_criterion = options["violating_criterion"];
   const Matrix<Scalar> A = P.give(section);

   return

      (section == "INEQUALITIES" ||
       section == "FACETS" ||
       violating_criterion == -1)
      ? violated_rows(A, q, pm::operations::negative<Scalar>())

      : ((section == "EQUATIONS" ||
          section == "AFFINE_HULL" ||
          violating_criterion == 0)
         ? violated_rows(A, q, pm::operations::non_zero<Scalar>())

         : violated_rows(A, q, pm::operations::positive<Scalar>()));
}

template<typename Scalar>
Set<int>
visible_facet_indices(const perl::Object P, const Vector<Scalar>& q)
{
   const Matrix<Scalar> F = P.give("FACETS");
   return violated_rows(F, q, pm::operations::negative<Scalar>());
}

template <typename Scalar>
Set<int>
visible_face_indices(const perl::Object P, const Vector<Scalar>& q)
{
   const graph::Lattice<graph::lattice::BasicDecoration, graph::lattice::Sequential>& HD = P.give("HASSE_DIAGRAM");
   const IncidenceMatrix<> VIF = P.give("RAYS_IN_FACETS");
   Set<int> facets_in_HD;
   for (int i: visible_facet_indices(P,q))
      facets_in_HD += graph::find_facet_node(HD, VIF[i]);
   return graph::order_ideal<graph::Down>(facets_in_HD, HD);
}

template <typename Scalar>
Set<int>
containing_normal_cone(const perl::Object P, Vector<Scalar> q)
{
   Matrix<Scalar>          R   = P.give("RAYS");
   const Matrix<Scalar>    F   = P.give("FACETS");
   const Matrix<Scalar>    H   = P.give("AFFINE_HULL");
   const IncidenceMatrix<> VIF = P.give("VERTICES_IN_FACETS");

   // normalize rays and q
   for (auto rit = entire(rows(R)); !rit.at_end(); ++rit)
      rit->dehomogenize();
   q.dehomogenize();

   // we optimize over the polar polytope.
   // first we calculate the transformations that will center P
   Vector<Scalar> barycenter = ones_vector<Scalar>(R.rows()) * R;
   barycenter.dehomogenize();

   SparseMatrix<Scalar> tau=unit_matrix<Scalar>(R.cols());
   tau[0].slice(range_from(1))=-barycenter.slice(range_from(1));
   const auto inv_tau (inv(tau));

   // then we polarize the centered P
   perl::Object LP("LinearProgram", mlist<Scalar>());
   const Vector<Scalar> lin_obj(q - barycenter);
   LP.take("LINEAR_OBJECTIVE") << lin_obj;
   perl::Object Q("Polytope", mlist<Scalar>());
   Q.take("FACETS") << R * tau;

   const Matrix<Scalar> Ftrans(F * T(inv_tau));
   Q.take("VERTICES") << Ftrans; // also transform the facets so we get the correct numbering and don't have to compute a convex hull again
   Q.take("AFFINE_HULL") << H * T(inv_tau);
   Q.take("LP") << LP;

   // then we optimize over the polar polytope.
   // We use minimization because the polar polytope is mirrored around the origin
   Set<int> min_face = Q.give("LP.MINIMAL_FACE");
   const Scalar min_val = Q.give("LP.MINIMAL_VALUE");

   // for floating-point polytopes, also add vertices within epsilon of the minimum
   fudge_min_face(min_face, Ftrans, lin_obj, min_val, 1e-7);

   // finally, we intersect all facets of P that generate the normal cone containing q
   Set<int> normal_cone_gen(sequence(0,R.rows()));
   for (int i: min_face)
      normal_cone_gen *= VIF[i];

   return normal_cone_gen;
}

template <typename Scalar>
Set<int>
containing_outer_cone(const perl::Object P, Vector<Scalar> q)
{
   const Matrix<Scalar>    V   = P.give("VERTICES");
   const Matrix<Scalar>    H   = P.give("AFFINE_HULL");
   const IncidenceMatrix<> VIF = P.give("VERTICES_IN_FACETS");
   const int               d   = P.give("COMBINATORIAL_DIM");
   const Vector<Scalar>    b   = P.give("VERTEX_BARYCENTER");
   
   const auto raf(ridges_and_facets(VIF, d));
   Set<int> containing_outer_face_cone(sequence(0, V.rows()));
   for (const auto f: raf.nodes_of_rank(d-1)) {
      bool good_facet(true);
      for (const auto r: raf.in_adjacent_nodes(f)) {
         Vector<Scalar> ker(null_space(V.minor(raf.face(r),All) / b / H)[0]);
         if (ker * V[ (raf.face(f) - raf.face(r)).front() ] <= 0)
            ker = -ker;
         if (ker * q < 0) {
            good_facet = false; break;
         }
      }
      if (good_facet)
         containing_outer_face_cone *= raf.face(f);
   }
   return containing_outer_face_cone;
}

UserFunctionTemplate4perl("# @category Geometry"
                          "# Check which relations, if any, are violated by a point."
                          "# @param Cone P"
                          "# @param Vector q"
                          "# @option String section Which section of P to test against q"
                          "# @option Int violating_criterion has the options: +1 (positive values violate; this is the default), 0 (*non*zero values violate), -1 (negative values violate)"
                          "# @return Set"
                          "# @example This calculates and prints the violated equations defining a square with the origin as its center and side length 2 with respect to a certain point:"
                          "# > $p = cube(2);"
                          "# > $v = new Vector([1,2,2]);"
                          "# > $S = violations($p,$v);"
                          "# > print $S;"
                          "# | {1 3}",
                          "violations<Scalar> (Cone<Scalar> Vector<Scalar> { section => 'FACETS', violating_criterion => 1 } )");

UserFunctionTemplate4perl("# @category Geometry"
                          "# Return the indices of all facets that are visible from a point //q//."
                          "# @param Cone P"
                          "# @param Vector q"
                          "# @return Set"
                          "# @example This prints the facets of a square with the origin as its center and side length 2 that are visible from a certain point:"
                          "# > $p = cube(2);"
                          "# > $v = new Vector([1,2,2]);"
                          "# > map { print $p->VERTICES_IN_FACETS->[$_], \"\\n\" } @{visible_facet_indices($p,$v)};"
                          "# | {1 3}"
                          "# | {2 3}",
                          "visible_facet_indices<Scalar> (Cone<Scalar> Vector<Scalar>)");

UserFunctionTemplate4perl("# @category Geometry"
                          "# Return the indices (in the HASSE_DIAGRAM) of all faces that are visible from a point //q//."
                          "# @param Cone P"
                          "# @param Vector q"
                          "# @return Set"
                          "# @example This prints the faces of a square with the origin as its center and side length 2 that are visible from a certain point:"
                          "# > $p = cube(2);"
                          "# > $v = new Vector([1,2,2]);"
                          "# > map { print $p->HASSE_DIAGRAM->FACES->[$_], \"\\n\" } @{visible_face_indices($p,$v)};"
                          "# | {}"
                          "# | {1}"
                          "# | {2}"
                          "# | {3}"
                          "# | {1 3}"
                          "# | {2 3}",
                          "visible_face_indices<Scalar> (Cone<Scalar> Vector<Scalar>)");

UserFunctionTemplate4perl("# @category Geometry"
                          "# Return the vertices of the face of P whose normal cone contains a point //q//."
                          "# @param Cone P"
                          "# @param Vector q"
                          "# @return Set"
                          "# @example To find the face whose normal cone contains a given point, type"
                          "# > $p = new Polytope(VERTICES=>[[1,-1,0],[1,0,-1],[1,0,1],[1,100,0]]);"
                          "# > print containing_normal_cone($p, new Vector([1,1,2]));"
                          "# | {2 3}",
                          "containing_normal_cone<Scalar>(Cone<Scalar>, Vector<Scalar>)");

UserFunctionTemplate4perl("# @category Geometry"
                          "# Return the vertices of the face of P whose outer cone contains a point //q//."
                          "# @param Polytope P"
                          "# @param Vector q"
                          "# @return Set"
                          "# @example To find the face whose outer cone contains a given point, type"
                          "# > print containing_outer_cone(cube(3), new Vector([1,2,2,2]));"
                          "# | {7}",
                          "containing_outer_cone<Scalar>(Polytope<Scalar>, Vector<Scalar>)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
