/* Copyright (c) 1997-2021
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
#include "polymake/fan/stacky_fan.h"
#include "polymake/topaz/complex_tools.h"
#include "polymake/topaz/boundary_tools.h"
#include "polymake/group/orbit.h"
#include "polymake/graph/GraphIso.h"
#include "polymake/tropical/curve.h"

/*

  ** Overview of the code **

  Calculating the moduli cell of a tropical curve proceeds in three steps:

  - calculate a fundamental domain inside the first barycentric subdivision,
    which is done in the function stacky_fundamental_domain() elsewhere.
    In this file, we are given the result of this calculation as input.

  - subdivide the fundamental domain once more to ensure regularity of
    the resulting simplicial complex, which happens in the function
    subdivide_fundamental_domain().

  - find the correct identifications along the boundary.

  The last step is by far the hardest. It works as follows:

  The coordinates of the ambient space of the subdivided cone encode
  the non-marked edges in the tropical curve. Thus, the i-th
  coordinate of each ray in the subdivision is the length of the
  corresponding non-marked edge in the graph.  (Marked edges are
  considered fixed, of infinite length, and do not participate in any
  permutations of the edges.)

  The boundary of the subdivided cone consists of graphs where some
  edges have length 0. Whenever two such graphs are isomorphic, the
  corresponding rays must be identified.

  The big picture happens inside stacky_moduli_cell(). This function 

  - reads EDGES_THROUGH_VERTICES, MARKED_EDGES, COORDINATES,
    HASSE_DIAGRAM and a sanity check property IS_FUNDAMENTAL_DOMAIN.
  
  - calls subdivide_fundamental_domain() to get at the second
    barycentric subdivision of the fundamental domain

  - constructs a map 
        ContractedGraphCollection: Set<Int> -> Curve
    that assembles all auxiliary data for a given set of contracted
    edges and stores it for future access.

  - calls an instance of UniqueRepFinder (defined in curve.h) 
    to find unique representatives of the rays modulo symmetry.

 */      

namespace polymake { namespace tropical {

   using graph::Lattice;
   using graph::lattice::BasicDecoration;
   using graph::lattice::Sequential;

template<typename Scalar>   
fan::SubdivisionData<Scalar>   
subdivide_fundamental_domain(const Lattice<BasicDecoration>& FD_HD,
                             const Matrix<Scalar>& fd_rays,
                             const Int verbosity)
{
   /*
     The fundamental domain is a subcomplex of the first barycentric subdivision
     of the input cone.
     Therefore, subdividing once more yields the second barycentric subdivision
     of the fundamental domain.
   */
   
   fan::SubdivisionData<Scalar> sdd;   
   sdd.fal = topaz::first_barycentric_subdivision(FD_HD, true);
   sdd.subdiv_rays = fan::subdivision_rays(fd_rays, sdd.fal.labels, verbosity);
   if (verbosity > 4)
      cerr << "facets (" << sdd.fal.facets.size() << "):\n" << sdd.fal.facets << endl << endl;
   return sdd;
}
      

template<typename Scalar>
BigObject
moduli_cell_of_curve(BigObject FundamentalDomain,
                     BigObject curve,
                     OptionSet options)
{
   const Int verbosity = options["verbosity"];
   const IncidenceMatrix<> etv = curve.give("EDGES_THROUGH_VERTICES");

   Array<Int> vertex_weights;
   if (curve.lookup("VERTEX_WEIGHTS") >> vertex_weights) {
      if(vertex_weights.size() != etv.rows())
         throw std::runtime_error("size of vertex_weights array must equal the number of rows of the incidence matrix");
   } else
      vertex_weights.resize(etv.rows());

   Set<Int> marked_edges;
   curve.lookup("MARKED_EDGES") >> marked_edges;

   const Curve original_tg(etv, marked_edges, vertex_weights, verbosity);

   if (verbosity) {
      cerr << "\n*** entered moduli_cell_of_curve ***" << endl;
   }
   if (verbosity > 3) {
      cerr << "input curve " << original_tg << endl << endl;
   }

   bool is_fundamental_domain (false);
   if (!(FundamentalDomain.get_attachment("IS_FUNDAMENTAL_DOMAIN") >> is_fundamental_domain)
       || !is_fundamental_domain)
      throw std::runtime_error("moduli_cell_of_curve: need a fundamental domain as input, as provided by stacky_fundamental_domain()");
   
   const Matrix<Scalar> fd_rays = FundamentalDomain.give("COORDINATES");
   const Lattice<BasicDecoration> FD_HD = FundamentalDomain.give("HASSE_DIAGRAM");      
   
   Array<Set<Set<Int>>> original_labels;
   if (!(FundamentalDomain.get_attachment("VERTEX_LABELS_AS_SETS") >> original_labels))
      throw std::runtime_error("moduli_cell_of_curve: need attachment VERTEX_LABELS_AS_SETS");

   const fan::SubdivisionData<Scalar> sdd = subdivide_fundamental_domain(FD_HD, fd_rays, verbosity);

   // prepare the contractions of the curves
   ContractedGraphCollection cgc;
   fill_contracted_graph_collection(cgc, original_tg);
   
   // the following data is filled out by UniqueRepFinder
   Int                          next_new_global_index_of_vertex(0);
   std::vector<Int>             graph_containing_index_of_vertex;
   Array<Int>                   global_vertex_of;
   std::vector<Int>             original_vertex_of;
   std::vector<Map<Int,Scalar>> nonzero_lengths_of_vertex;
   std::vector<Set<Int>>        zeros_of_support_of_vertex;
   Map<Array<Scalar>, Set<Int>> possibly_isomorphic_to_vertex;
   std::vector<std::string>     vertex_labels;
   std::ostringstream os;

   UniqueRepFinder<Scalar>(next_new_global_index_of_vertex, global_vertex_of, original_vertex_of, sdd.subdiv_rays, nonzero_lengths_of_vertex, zeros_of_support_of_vertex, possibly_isomorphic_to_vertex, vertex_labels, cgc, sdd.subdiv_rays.rows(), os, verbosity).find_unique_reps();

   // We are done. Now package the results for output.
   
   Set<Set<Int>> total_space;
   for (const auto& F: sdd.fal.facets) {
      Set<Int> reindexed_facet;
      for (const Int j: F)
         reindexed_facet += global_vertex_of[j];
      total_space += reindexed_facet;
   }

   Array<Set<Set<Int>>> substituted_labels(original_vertex_of.size());
   auto sl_it = entire(substituted_labels);
   for (const Set<Set<Int>>& ssi: select(sdd.fal.labels, original_vertex_of)) {
      for (const Int i: ssi.front())
         (*sl_it) += original_labels[i];
      ++sl_it;
   }

   return BigObject("topaz::GeometricSimplicialComplex",
                    "FACETS", total_space,
                    "COORDINATES", sdd.subdiv_rays.minor(original_vertex_of, All),
                    "VERTEX_LABELS", fan::make_strings(substituted_labels));
}

template<typename Scalar>
Array<Array<Int>>
auto_group_on_coordinates(BigObject curve,
                          OptionSet options)
{
   const IncidenceMatrix<> original_inc = curve.give("EDGES_THROUGH_VERTICES");
   const Int verbosity = options["verbosity"];
   
   Set<Int> marked_edges;
   curve.lookup("MARKED_EDGES") >> marked_edges;

   Array<Int> vertex_weights;
   if (curve.lookup("VERTEX_WEIGHTS") >> vertex_weights) {
      if (vertex_weights.size() != original_inc.rows())
         throw std::runtime_error("size of vertex_weights array must equal the number of rows of the incidence matrix");
   } else
      vertex_weights.resize(original_inc.rows());
   
   Curve tg(original_inc, marked_edges, vertex_weights, verbosity);
   return tg.auto_group_on_coordinates();
}


std::pair<IncidenceMatrix<>,Array<Int>>
contracted_edge_incidence_matrix(const IncidenceMatrix<>& original_inc,
                                 const Set<Int>& contracted_edges,
                                 OptionSet options)
{
   const Int verbosity = options["verbosity"];
   Curve tg(original_inc, contracted_edges, verbosity);

   if (verbosity)
      cerr << "contracted graph:\n" << tg << endl;

   const Array<Int> weights(tg.get_vertex_weights());
   const Set<Int> node_indices(tg.get_participating_node_indices());
   const Array<Int> nodes(select(weights, node_indices));

   return std::make_pair(tg.get_incidence_matrix(), nodes);
                     
}

template<typename Scalar>
bool
isomorphic_curves(const IncidenceMatrix<>& etv,
                  const Array<Int>& vertex_weights,
                  const Vector<Scalar>& v,
                  const Vector<Scalar>& w,
                  OptionSet options)
{
   const Int verbosity = options["verbosity"];
   const Curve
      tg_v(etv, vertex_weights, zeros_of(v), verbosity),
      tg_w(etv, vertex_weights, zeros_of(w), verbosity);

   const Map<Int, Scalar> nzl_v(nonzero_lengths_of(v));
   const Map<Scalar, Int> mult_v(multiplicity_of_length(nzl_v));
   const Map<Scalar, Int> col_v(find_color_of_length(nzl_v, verbosity));
   const Map<Int,Int> color_of_edge_for_v(find_color_of_edge(nzl_v, col_v, verbosity));
   const Array<Int> coloring_v(tg_v.induced_node_coloring(color_of_edge_for_v));
   const Map<Int, Scalar> nzl_w(nonzero_lengths_of(w));
   const Map<Int,Int> color_of_edge_for_w(find_color_of_edge(nzl_w, col_v, verbosity));
   
   return isomorphic_curves_impl(tg_v, tg_w, coloring_v, color_of_edge_for_w, verbosity);
}

template<typename Scalar>
Curve
Object2Curve(BigObject G, const Int verbosity)
{
   const IncidenceMatrix<> inc = G.give("EDGES_THROUGH_VERTICES");
   
   Set<Int> marked_edges;
   G.lookup("MARKED_EDGES") >> marked_edges;

   Array<Int> vertex_weights;
   if (G.lookup("VERTEX_WEIGHTS") >> vertex_weights) {
      if (vertex_weights.size() != inc.rows())
         throw std::runtime_error("size of vertex_weights array must equal the number of rows of the incidence matrix");
   } else
      vertex_weights.resize(inc.rows());

   Vector<Scalar> v;
   Set<Int> zeros;
   if (G.lookup("EDGE_LENGTHS") >> v)
      zeros = zeros_of(v);
   
   return Curve(inc, marked_edges, vertex_weights, zeros, verbosity);
}

template<typename Scalar>
bool
isomorphic_curves(BigObject G, BigObject H, OptionSet options)
{
   const Int verbosity = options["verbosity"];

   const Curve
      tg(Object2Curve<Scalar>(G, verbosity)),
      th(Object2Curve<Scalar>(H, verbosity));

   if (tg.get_vertex_weights() != th.get_vertex_weights())
      return false;

   Vector<Scalar> v,w;
   G.lookup("EDGE_LENGTHS") >> v;
   H.lookup("EDGE_LENGTHS") >> w;
   if (!v.size() && !w.size())
      return graph::isomorphic(tg.subdivided_graph(), th.subdivided_graph());

   const Map<Int, Scalar> nzl_v(nonzero_lengths_of(v));
   const Map<Scalar, Int> mult_v(multiplicity_of_length(nzl_v));
   const Map<Scalar, Int> col_v(find_color_of_length(nzl_v, verbosity));
   const Map<Int,Int>     color_of_edge_for_v(find_color_of_edge(nzl_v, col_v, verbosity));
   const Array<Int>       coloring_v(tg.induced_node_coloring(color_of_edge_for_v));
   const Map<Int, Scalar> nzl_w(nonzero_lengths_of(w));
   const Map<Int,Int>     color_of_edge_for_w(find_color_of_edge(nzl_w, col_v, verbosity));

   if (verbosity)
      cerr << "isomorphic_curves: v = " << v
           << ", nzl_v = " << nzl_v
           << ", mult_v = " << mult_v
           << ", col_v = " << col_v
           << ", color_of_edge_for_v = " << color_of_edge_for_v
           << ", coloring_v = " << coloring_v << endl
           << "w = " << w
           << ", color_of_edge_for_w = " << color_of_edge_for_w << endl;

   
   return isomorphic_curves_impl(tg, th, coloring_v, color_of_edge_for_w, verbosity);
}

UserFunctionTemplate4perl("# @category Symmetry",
                          "moduli_cell_of_curve<Scalar>(topaz::GeometricSimplicialComplex<Scalar>, Curve<Scalar>, { verbosity => 0 })");
                          
UserFunctionTemplate4perl("# @category Symmetry"
                          "# @param IncidenceMatrix etv"
                          "# @param Set<Int> marked_edges"
                          "# @option Int verbosity default 0"
                          "# @return Array<Array<Int>>",
                          "auto_group_on_coordinates<Scalar>(Curve<Scalar>, { verbosity=>0 })");

UserFunction4perl("# @category Symmetry"
                  "# @param IncidenceMatrix etv"
                  "# @param Set<Int> contracted_edges"
                  "# @option Int verbosity default 0"
                  "# @return Pair<IncidenceMatrix,Array<Int>>",
                  &contracted_edge_incidence_matrix,
                  "contracted_edge_incidence_matrix(IncidenceMatrix, Set<Int>, { verbosity => 0})");

UserFunctionTemplate4perl("# @category Symmetry"
                          "# @param IncidenceMatrix etv"
                          "# @param Array<Int> vertex_weights"
                          "# @param Vector<Scalar> v first edge lengths"
                          "# @param Vector<Scalar> w second edge lengths"
                          "# @option Int verbosity default 0"
                          "# @return Bool",
                          "isomorphic_curves<Scalar>(IncidenceMatrix, Array<Int>, Vector<Scalar>, Vector<Scalar>, { verbosity => 0 })");

UserFunctionTemplate4perl("# @category Symmetry"
                          "# @param Curve<Scalar> G"
                          "# @param Curve<Scalar> H"
                          "# @option Int verbosity default 0"
                          "# @return Bool",
                          "isomorphic_curves<Scalar>(Curve<Scalar>, Curve<Scalar>, { verbosity => 0 })");
                          
      
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
