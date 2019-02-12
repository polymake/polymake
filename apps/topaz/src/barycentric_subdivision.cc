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
#include "polymake/graph/maximal_chains.h"
#include "polymake/topaz/hasse_diagram.h"
#include "polymake/list"
#include <sstream>
#include <vector>

namespace polymake { namespace topaz {

   using namespace graph;

   // Renumbers node indices as if bottom node (and potentially top node)
   // are removed
   template <typename Decoration, typename SeqType>
      Array<Set<int> > renumber_nodes( const Array<Set<int> >& old_facets,
            const graph::Lattice<Decoration, SeqType>& HD,
            bool ignore_top_node) {
         Array<Set<int> > new_facets(old_facets.size());
         int b_index = HD.bottom_node();
         int t_index = HD.top_node();
         auto o_it = entire(old_facets);
         auto n_it = entire(new_facets);
         for(; !n_it.at_end(); ++o_it, ++n_it) {
            Set<int> nset;
            for(auto n : (*o_it)) {
               nset += (n - (n > b_index) - (ignore_top_node && n > t_index));
            }
            *n_it = nset;
         }
         return new_facets;
      }

   template <typename Decoration, typename SeqType, typename Scalar>
      perl::Object barycentric_subdivision_impl(perl::Object p_in, perl::OptionSet options, bool force_ignore_top_node = false)
      {
         const bool isComplex = p_in.isa("topaz::SimplicialComplex");
         const bool realize = options["geometric_realization"];
         const bool ignore_top_node = options["ignore_top_node"] || force_ignore_top_node;

         perl::ObjectType result_type = realize
            ? perl::ObjectType("GeometricSimplicialComplex", mlist<Scalar>())
            : perl::ObjectType("SimplicialComplex");
         perl::Object p_out(result_type);
         p_out.set_description() << "Barycentric subdivision of " << p_in.description() << endl;

         graph::Lattice<Decoration, SeqType> HD;
         std::string hasse_section = options["pin_hasse_section"];
         if (isComplex)
            hasse_section = "HASSE_DIAGRAM";
         p_in.give(hasse_section) >> HD;

         Set<int> wanted_nodes = sequence(0, HD.nodes()) - HD.bottom_node();
         if(ignore_top_node) wanted_nodes -= HD.top_node();

         Array<Set<int> > barycentric_facets = maximal_chains(HD,true, ignore_top_node);
         p_out.take("FACETS") << renumber_nodes(barycentric_facets, HD, ignore_top_node);
         p_out.take("PURE") << 1;
         p_out.take("DIM") << HD.rank() - 1 - isComplex;

         if (options["relabel"]) {
            Array<std::string> old_labels;
            std::string label_section = options["label_section"];
            if (isComplex)
               label_section = "VERTEX_LABELS";
            p_in.lookup(label_section) >> old_labels;
            p_out.take("VERTEX_LABELS") << select(bs_labels(HD, old_labels, ignore_top_node), wanted_nodes);
         }

         if (realize) {
            std::string coord_section = options["coord_section"];
            if (isComplex)
               coord_section = "COORDINATES";
            Matrix<Scalar> old_coord = p_in.give(coord_section);
            p_out.take("COORDINATES") << bs_geom_real<Scalar, Decoration, SeqType>(old_coord, HD, ignore_top_node).minor(wanted_nodes,All);
         }

         return p_out;
      }

   template <typename Decoration, typename SeqType, typename Scalar>
      perl::Object iterated_barycentric_subdivision_impl(perl::Object p_in, int k, perl::OptionSet options, bool force_ignore_top_node = false)
      {
         if (k<=0) return p_in;
         const perl::Object subd1 = barycentric_subdivision_impl<Decoration, SeqType, Scalar>(p_in, options, force_ignore_top_node);
         perl::Object subd = iterated_barycentric_subdivision_impl<Decoration, graph::lattice::Nonsequential, Scalar>(subd1, k-1, options, true);

         const char num[][5] = { "1st ", "2nd ", "3rd " };
         std::ostringstream desc;
         if (k <= 3) desc << num[k-1];
         else desc << k << "th ";
         desc << "barycentric subdivision of " << p_in.description();
         subd.set_description() << desc.str();
         return subd;
      }

   FunctionTemplate4perl("barycentric_subdivision_impl<Decoration = BasicDecoration, SeqType = Nonsequential, Scalar=Rational>($ { relabel => 1, geometric_realization => 0, ignore_top_node=> 1 })");

   FunctionTemplate4perl("iterated_barycentric_subdivision_impl<Decoration = BasicDecoration, SeqType = Nonsequential, Scalar=Rational>($ $ { relabel => 1, geometric_realization => 0, ignore_top_node=> 1 })");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
