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
#include "polymake/polytope/cayley_embedding.h"
#include "polymake/Set.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Vector.h"
#include "polymake/hash_map"

namespace polymake { namespace fan {

template <typename Scalar, typename TVector>
perl::Object mixed_subdivision(int m,
                               const perl::Object cayley_embedding,
                               const Array<Set<int> >& vertices_in_cells,
                               const GenericVector<TVector>& t)
{
   // input sanity checks
   if (!m)
      throw std::runtime_error("mixed_subdivision: empty array given.");

   const Matrix<Scalar> V = cayley_embedding.give("VERTICES|POINTS");
   const int d = V.cols() - m - 1; // common dimension of the input polytopes

   const Matrix<Scalar> slice_eqs = (-t) / accumulate(t.top(), operations::add())
                                  | zero_matrix<Scalar>(m,d)
                                  | unit_matrix<Scalar>(m);

   hash_map<Vector<Scalar>, int> index_of_point;
   int n_points(0);

   perl::ObjectType poly_type("Polytope", mlist<Scalar>());
   std::vector<Set<int>> vif_vector;
   for (auto vicit = entire(vertices_in_cells); !vicit.at_end(); ++vicit) {

      perl::Object cayley_cell(poly_type);
      cayley_cell.take("POINTS") << V.minor(*vicit, All);
      const Matrix<Scalar> F = cayley_cell.give("FACETS");

      perl::Object cayley_slice(poly_type);
      cayley_slice.take("INEQUALITIES") << F;
      cayley_slice.take("EQUATIONS") << slice_eqs;
      const Matrix<Scalar> P = cayley_slice.give("VERTICES");
      if (!P.rows())
         throw std::runtime_error("mixed_subdivision: unexpectedly empty slice polytope");

      Set<int> vif;
      for (auto rit = entire(rows(P)); !rit.at_end(); ++rit) {
         const Vector<Scalar> pt(*rit);
         if (!index_of_point.exists(pt))
            index_of_point[pt] = n_points++;
         vif += index_of_point[pt];
      }
      vif_vector.push_back(vif);
   }

   Matrix<Scalar> V_out(index_of_point.size(), d+1);
   for (auto hit = entire(index_of_point); !hit.at_end(); ++hit)
      V_out[hit->second] = hit->first.slice(sequence(0, d+1));

   perl::Object p_out("PolyhedralComplex", mlist<Scalar>());
   p_out.take("VERTICES") << V_out;
   p_out.take("MAXIMAL_POLYTOPES") << IncidenceMatrix<>(vif_vector);
   return p_out;
}

template <typename Scalar, typename TVector>
perl::Object mixed_subdivision(const Array<perl::Object>& p_array,
                               const Array<Set<int>>& vertices_in_cells,
                               const GenericVector<TVector>& t,
                               perl::OptionSet options)
{
   return mixed_subdivision<Scalar>(p_array.size(),
                                    polytope::cayley_embedding(p_array, Vector<Scalar>(), options),
                                    vertices_in_cells,
                                    t);
}

template <typename Scalar>
perl::Object mixed_subdivision(const perl::Object& p_in1, const perl::Object& p_in2,
                               const Array<Set<int>>& vertices_in_cells,
                               const Scalar& t, const Scalar& t_prime,
                               perl::OptionSet options)
{
   const Array<perl::Object> p_array{ p_in1, p_in2 };
   const Vector<Scalar> t_vec{ t, t_prime };

   return mixed_subdivision<Scalar>(p_array, vertices_in_cells, t_vec, options);
}


UserFunctionTemplate4perl("# @category Producing a polyhedral complex"
                          "# Create a weighted mixed subdivision of the Minkowski sum of two polytopes, using the Cayley trick."
                          "# The polytopes must have the same dimension, at least one of them must be pointed. "
                          "# The vertices of the first polytope //P_0// are weighted with //t_0//,"
                          "# and the vertices of the second polytope //P_1// with //t_1//."
                          "# "
                          "# Default values are //t_0//=//t_1//=1."
                          "# @param Polytope P_0 the first polytope"
                          "# @param Polytope P_1 the second polytope"
                          "# @param Array<Set> VIF the indices of the vertices of the mixed cells"
                          "# @param Scalar t_0 the weight for the vertices of //P_0//; default 1"
                          "# @param Scalar t_1 the weight for the vertices of //P_1//; default 1"
                          "# @option Bool no_labels Do not copy [[VERTEX_LABELS]] from the original polytopes. default: 0"
                          "# @return PolyhedralComplex",
                          "mixed_subdivision<Scalar>(Polytope<type_upgrade<Scalar>>, Polytope<type_upgrade<Scalar>>, Array<Set>; "
                          "                          type_upgrade<Scalar>=1, type_upgrade<Scalar>=1, {no_labels => 0 })");

UserFunctionTemplate4perl("# @category Producing a polyhedral complex"
                          "# Create a weighted mixed subdivision of a Cayley embedding of a sequence of polytopes. "
                          "# Each vertex //v// of the //i//-th polytope is weighted with //t_i//, "
                          "# the //i//-th entry of the optional array //t//. "
                          "# @param Int m the number of polytopes giving rise to the Cayley embedding"
                          "# @param Polytope C the Cayley embedding of the input polytopes"
                          "# @param Array<Set> a triangulation of C"
                          "# @option Vector<Scalar> t scaling for the Cayley embedding; defaults to the all-1 vector"
                          "# @option Bool no_labels Do not copy [[VERTEX_LABELS]] from the original polytopes. default: 0"
                          "# @return PolyhedralComplex",
                          "mixed_subdivision<Scalar>($, Polytope<type_upgrade<Scalar>>, Array<Set>; "
                          "                          Vector<type_upgrade<Scalar>>=(ones_vector<Scalar>($_[0])))");


UserFunctionTemplate4perl("# @category Producing a polyhedral complex"
                          "# Create a weighted mixed subdivision of a sequence (P1,...,Pm) of polytopes, using the Cayley trick. "
                          "# All polytopes must have the same dimension, at least one of them must be pointed. "
                          "# Each vertex //v// of the //i//-th polytope is weighted with //t_i//, "
                          "# the //i//-th entry of the optional array //t//. "
                          "# @param Array<Polytope> A the input polytopes"
                          "# @option Vector<Scalar> t scaling for the Cayley embedding; defaults to the all-1 vector"
                          "# @option Bool no_labels Do not copy [[VERTEX_LABELS]] from the original polytopes. default: 0"
                          "# @return PolyhedralComplex",
                          "mixed_subdivision<Scalar>(Polytope<type_upgrade<Scalar>>+, Array<Set>; "
                          "                          Vector<type_upgrade<Scalar>>=(ones_vector<Scalar>(scalar(@{$_[0]}))), { no_labels => 0 })");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
