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

#include "polymake/tropical/curve.h"
#include "polymake/FacetList.h"

namespace polymake { namespace tropical {

      
template<typename Scalar>      
BigObject
moduli_space(const Array<BigObject>& graph_objects, OptionSet options)
{
   const Int verbosity = options["verbosity"];
   constexpr Int silent(0);
   
   std::vector<std::string> vertex_labels;
   std::ostringstream os;

   const Int n(graph_objects.size());
   std::vector<Curve>     tg_array; tg_array.reserve(n);
   Array<Matrix<Scalar>>  coordinates_of_cell(n);
   Array<Array<Set<Int>>> facets_of_cell(n);
   Array<Int>             n_vertices_of(n);
   Array<ContractedGraphCollection> cgcs(n);

   if (verbosity > 1)
      cerr << "moduli_space: preparing cells" << endl;

   Int global_dim(-1);
   
   for (Int i=0; i<n; ++i) {
      const IncidenceMatrix<> etv = graph_objects[i].give("EDGES_THROUGH_VERTICES");
      Set<Int> marked_edges;
      graph_objects[i].lookup("MARKED_EDGES") >> marked_edges;
      
      Array<Int> vertex_weights;
      if (!(graph_objects[i].lookup("VERTEX_WEIGHTS") >> vertex_weights))
         vertex_weights = Array<Int>(etv.rows());
      
      tg_array.emplace_back(Curve(etv, marked_edges, vertex_weights, silent));
      fill_contracted_graph_collection(cgcs[i], tg_array[i]);

      const Matrix<Scalar> coos = graph_objects[i].give("MODULI_CELL.COORDINATES"); 
      coordinates_of_cell[i] = coos;
      if (0 == i)
         global_dim = coos.cols();
      else if (global_dim != coos.cols())
         throw std::runtime_error("moduli_space: differing dimensions of moduli cells");
      
      const Array<Set<Int>> fc  = graph_objects[i].give("MODULI_CELL.FACETS");
      facets_of_cell[i] = fc;

      const Int nv              = graph_objects[i].give("MODULI_CELL.N_VERTICES");
      n_vertices_of[i] = nv;
   }

   if (verbosity > 1)
      cerr << "moduli_space: done preparing cells" << endl;

   Int                          next_new_global_index_of_vertex(0);
   std::vector<Int>             graph_containing_index_of_vertex;

   Array<Array<Int>>            global_vertex_of(n);
   Array<std::vector<Int>>      original_vertex_of(n);
   std::vector<Map<Int,Scalar>> nonzero_lengths_of_vertex;
   std::vector<Set<Int>>        zeros_of_support_of_vertex;
   Map<Array<Scalar>, Set<Int>>   possibly_isomorphic_to_vertex;

   for (Int i=0; i<n; ++i) {
      if (verbosity)
         cerr << "moduli_space: cell " << i << ", " << n_vertices_of[i] << " vertices" << endl;

      UniqueRepFinderFromArray<Scalar>(next_new_global_index_of_vertex, global_vertex_of[i], original_vertex_of[i], coordinates_of_cell[i], nonzero_lengths_of_vertex, zeros_of_support_of_vertex, possibly_isomorphic_to_vertex, vertex_labels, cgcs[i], n_vertices_of[i], os, verbosity, i, cgcs, graph_containing_index_of_vertex).find_unique_reps();
   }

   FacetList total_space;

   for (Int i=0; i<n; ++i) {
      for (const auto& F: facets_of_cell[i]) {
         Set<Int> reindexed_facet;
         for (const Int j: F)
            reindexed_facet += global_vertex_of[i][j];
         total_space.insertMax(reindexed_facet);
      }
   }

   return BigObject("topaz::SimplicialComplex",
                    "FACETS", total_space,
                    "VERTEX_LABELS", Array<std::string>(vertex_labels.size(), entire(vertex_labels)));
}
      
UserFunctionTemplate4perl("# @category Symmetry"
                          "# Produces the stacky moduli space corresponding to the tropical curves G<sub>1</sub>,G<sub>2</sub>,...,G<sub>n</sub>."
                          "# @param Curve<Scalar> G1 first tropical curve"
                          "# @param Curve<Scalar> G2 second tropical curve"
                          "# @param Curve<Scalar> Gn last tropical curve"
                          "# @option Int verbosity 0 (default) .. 5"
                          "# @return topaz::SimplicialComplex the gluing of the individual moduli cells",
                          "moduli_space<Scalar>(Curve<Scalar> + { verbosity=>0 })");

} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
