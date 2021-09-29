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
#include "polymake/graph/maximal_chains.h"
#include "polymake/topaz/hasse_diagram.h"
#include "polymake/topaz/barycentric_subdivision.h"
#include "polymake/list"
#include <sstream>
#include <vector>

namespace polymake { namespace topaz {

using graph::lattice::BasicDecoration;
using graph::lattice::Nonsequential;

namespace {
         
// extract data from BigObject into the output parameters, listed last
template<typename Scalar, typename Decoration, typename SeqType>
void
bs_Object2data(BigObject& p_in,
               const OptionSet& options,
               const bool ignore_top_node,
               const bool isComplex,
               const bool realize,
               const bool relabel,
               graph::Lattice<Decoration, SeqType>& HD,
               Array<std::string>& old_labels,
               Matrix<Scalar>& old_coord)
{
   std::string hasse_section("HASSE_DIAGRAM");

   if (!isComplex) {
      const std::string hs = options["pin_hasse_section"];
      hasse_section = hs;
   }

   p_in.give(hasse_section) >> HD;

   if (relabel) {
      std::string label_section("VERTEX_LABELS");
      if (!isComplex) {
         const std::string ls = options["label_section"];
         label_section = ls;
      }
      p_in.lookup(label_section) >> old_labels;
   }

   if (realize) {
      std::string coord_section("COORDINATES");
      if (!isComplex) {
         const std::string cs = options["coord_section"];
         coord_section = cs;
      }
      const Matrix<Scalar> oc = p_in.give(coord_section);
      old_coord = oc;
   }
}

// do the actual barycentric subdivision on c++ data structures
// return result into the output parameters, listed last
template<typename Decoration, typename SeqType, typename Scalar>
void
bs_barycentric_subdivision_on_data(const graph::Lattice<Decoration, SeqType>& HD,
                                   const bool ignore_top_node,
                                   const bool isComplex,
                                   const bool relabel,
                                   const bool realize,
                                   const Matrix<Scalar>& old_coord,
                                   const Array<std::string>& old_labels,
                                   Array<Set<Int>>& barycentric_facets,
                                   Int& dim,
                                   Array<std::string>& vertex_labels,
                                   Matrix<Scalar>& coordinates)
{
   barycentric_facets = maximal_chains(HD, true, ignore_top_node);
   bs_renumber_nodes(barycentric_facets, HD, ignore_top_node);
   
   dim = HD.rank() - 1 - isComplex;

   Set<Int> wanted_nodes(sequence(0, HD.nodes()) - HD.bottom_node());
   if (ignore_top_node) wanted_nodes -= HD.top_node();
   
   if (relabel) {
      vertex_labels = select(bs_labels(HD, old_labels, ignore_top_node), wanted_nodes);
   }

   if (realize) {
      coordinates = graph::bs_geom_real(old_coord, HD, ignore_top_node).minor(wanted_nodes,All);
   }
}

// put c++ data into BigObject
template<typename Scalar>
BigObject
bs_data2Object(const bool realize,
               const Array<Set<Int>>& barycentric_facets,
               const Int& dim,
               const Array<std::string>& vertex_labels,
               const Matrix<Scalar>& coordinates,
               const std::string& description,
               const Int k)
{
   BigObjectType result_type = realize
      ? BigObjectType("GeometricSimplicialComplex", mlist<Scalar>())
      : BigObjectType("SimplicialComplex");
   
   BigObject p_out(result_type);
   p_out.take("FACETS") << barycentric_facets;
   p_out.take("PURE") << true;
   p_out.take("DIM") << dim;
   
   if (vertex_labels.size()) {
      p_out.take("VERTEX_LABELS") << vertex_labels;
   }
   
   if (realize && coordinates.rows())
      p_out.take("COORDINATES") << coordinates;

   const char num[][5] = { "1st ", "2nd ", "3rd " };
   std::ostringstream desc;
   if (k <= 3)
      desc << num[k-1];
   else
      desc << k << "th ";
   desc << "barycentric subdivision of " << description;
   if (description.back() != '\n')
      desc << endl;
   p_out.set_description() << desc.str();
   return p_out;
}
   
} // end anonymous namespace

template <typename Decoration, typename SeqType, typename Scalar>
BigObject iterated_barycentric_subdivision_impl(BigObject p_in, Int k, OptionSet options, bool force_ignore_top_node = false)
{
   if (k<=0) return p_in;

   // process options
   const bool realize = options["geometric_realization"];
   const bool relabel = options["relabel"];
   const bool ignore_top_node = options["ignore_top_node"] || force_ignore_top_node;
   const bool isComplex = p_in.isa("topaz::SimplicialComplex");

   // bs_Object2data extracts the following data
   graph::Lattice<Decoration, SeqType> HD;
   Array<std::string> old_labels;
   Matrix<Scalar> old_coord;
   topaz::bs_Object2data<Scalar>(p_in, options, ignore_top_node, isComplex, realize, relabel, 
                          HD, old_labels, old_coord);
   
   // bs_barycentric_subdivision_on_data calculates the following data
   Array<Set<Int>> barycentric_facets;
   Int dim(-1);
   Array<std::string> vertex_labels;
   Matrix<Scalar> coordinates;
   bs_barycentric_subdivision_on_data(HD, ignore_top_node, isComplex, relabel, realize, old_coord, old_labels,
                                      barycentric_facets, dim, vertex_labels, coordinates);
      
   // now iterate
   for (Int i=1; i<k; ++i) {
      Array<Set<Int>> next_bc_facets;
      Int next_dim(-1);
      Array<std::string> next_vertex_labels;
      Matrix<Scalar> next_coordinates;
      
      const graph::Lattice<BasicDecoration> next_HD(hasse_diagram_from_facets(barycentric_facets));
         
      // The Hasse diagram next_HD built by hasse_diagram_from_facets has a fake top node that we don't need,
      // so we set ignore_top_node to true
      bs_barycentric_subdivision_on_data(next_HD, true, isComplex, relabel, realize, coordinates, vertex_labels,
                                         next_bc_facets, next_dim, next_vertex_labels, next_coordinates);
         
      if (dim != next_dim - !isComplex) {
         cerr << "dim: " << dim << ", next_dim: " << next_dim << endl;
         throw std::runtime_error("iterated_barycentric_subdivision_impl: unexpected difference in dimension");
      }
      
      barycentric_facets = next_bc_facets;
      vertex_labels = next_vertex_labels;
      coordinates = next_coordinates;
   }

   return bs_data2Object(realize, barycentric_facets, dim, vertex_labels, coordinates, p_in.description(), k);
}

template <typename Decoration, typename SeqType, typename Scalar>
BigObject barycentric_subdivision_impl(BigObject p_in, OptionSet options, bool force_ignore_top_node = false)
{
   return iterated_barycentric_subdivision_impl<Decoration, SeqType, Scalar>(p_in, 1, options, force_ignore_top_node);
}
      

FunctionTemplate4perl("barycentric_subdivision_impl<Decoration = BasicDecoration, SeqType = Nonsequential, Scalar=Rational>($ { relabel => 1, geometric_realization => 0, ignore_top_node=> 1 })");

FunctionTemplate4perl("iterated_barycentric_subdivision_impl<Decoration = BasicDecoration, SeqType = Nonsequential, Scalar=Rational>($ $ { relabel => 1, geometric_realization => 0, ignore_top_node=> 1 })");

FunctionTemplate4perl("first_barycentric_subdivision<SeqType>(Lattice<BasicDecoration,SeqType>; $=0)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
