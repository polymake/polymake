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
#include "polymake/graph/lattice_builder.h"
#include "polymake/topaz/hasse_diagram.h"

namespace polymake { namespace topaz {

   using namespace graph;
   using namespace graph::lattice;

   Lattice<BasicDecoration> hasse_diagram_from_facets( const Array<Set<int> >&facets, const RankRestriction& rr) {
      const Set<int> all_vertices = accumulate( facets, operations::add());
      const int n_vertices = accumulate(all_vertices, operations::max())+1;
      const IncidenceMatrix<> maximal_cells(facets.size(), n_vertices, entire(facets));
      
      const TrivialCut<BasicDecoration> trivial_cut;
      const auto cut_above = lattice::RankCut<lattice::BasicDecoration,lattice::RankCutType::GreaterEqual>(rr.boundary_rank);
      if(rr.rank_restricted && rr.rank_restriction_type == RankCutType::LesserEqual)
         throw std::runtime_error("Hasse diagram of SimplicialComplex is always built dually.");
      const Set<int> artificial_set = scalar2set(-1);
      int top_rank = 0;
      if(facets.size() > 0) {
         for(auto f : facets) top_rank = std::max(top_rank, f.size());
         top_rank++;
      }
      SimplicialClosure<BasicDecoration> closure(maximal_cells);
      SimplicialDecorator decorator(top_rank, artificial_set);
      if(!rr.rank_restricted)
         return lattice_builder::compute_lattice_from_closure<BasicDecoration>(
               closure, trivial_cut, decorator, false,lattice_builder::Dual());
      else
         return lattice_builder::compute_lattice_from_closure<BasicDecoration>(
               closure, cut_above, decorator, rr.boundary_rank > 0,lattice_builder::Dual());
   }


   perl::Object hasse_diagram_caller(perl::Object complex, const RankRestriction& rr) {
      const Array<Set<int> >& facets = complex.give("FACETS");
      return hasse_diagram_from_facets(facets, rr).makeObject();
   }

   perl::Object hasse_diagram(perl::Object complex) {
		return hasse_diagram_caller(complex, RankRestriction());
	}

	perl::Object upper_hasse_diagram(perl::Object complex, int boundary_rank) {
		return hasse_diagram_caller(complex, RankRestriction(true, RankCutType::GreaterEqual, boundary_rank));
	}

	Function4perl(&hasse_diagram, "hasse_diagram(SimplicialComplex)");
	Function4perl(&upper_hasse_diagram, "upper_hasse_diagram(SimplicialComplex, $)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
