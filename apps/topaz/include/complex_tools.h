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

#pragma once

#include "polymake/Array.h"
#include "polymake/PowerSet.h"
#include "polymake/graph/ShrinkingLattice.h"
#include "polymake/graph/LatticeTools.h"
#include "polymake/list"
#include <string>

/** Tools to treat simplicial complexes.
 *
 *  A simplicial complex is represented as a list of its FACETS, encoded as their vertex sets.
 *  Therefore any container of GenericSet of Int (template <typename Complex>) may be used to
 *  represent a complex. In the following std::list< polymake::Set<Int> >, polymake::PowerSet<Int>
 *  and polymake::Array< polymake::Set<Int> > are used.
 */

namespace polymake { namespace topaz {

   using graph::HasseDiagram_facet_iterator;
   using graph::Lattice;
   using graph::ShrinkingLattice;
   using graph::lattice::BasicDecoration;

// Computes the k_skeleton of a simplicial complex.
template <typename Complex>
PowerSet<Int> k_skeleton(const Complex& C, const Int k);

template <typename Complex, typename TSet>
struct link_helper {
   typedef pm::same_value_container<const TSet&> same_set;
   typedef pm::SelectedContainerPairSubset< const Complex&, same_set, operations::includes >
     selected_facets;
   typedef pm::SelectedContainerPairSubset< const Complex&, same_set,
                                            operations::composed21<operations::includes, std::logical_not<bool> > >
     deletion_type;
   typedef pm::TransformedContainerPair< selected_facets, same_set, operations::sub >
     result_type;
};

// Computes the star of a given face F.
template <typename Complex, typename TSet>
auto star(const Complex& C, const GenericSet<TSet, Int>& F)
{
   using helper = link_helper<Complex, TSet>;
   return typename helper::selected_facets(C, typename helper::same_set(F.top()));
}

// Computes the deletion of a given face F.
template <typename Complex, typename TSet>
auto deletion(const Complex& C, const GenericSet<TSet, Int>& F)
{
   using helper = link_helper<Complex, TSet>;
   return typename helper::deletion_type(C, typename helper::same_set(F.top()));
}

// Computes the link of a given face F.
template <typename Complex, typename TSet>
auto link(const Complex& C, const GenericSet<TSet, Int>& F)
{
   using helper = link_helper<Complex, TSet>;
   return typename helper::result_type(star(C, F), typename helper::same_set(F.top()));
}

struct star_maker {
   typedef HasseDiagram_facet_iterator<Lattice<BasicDecoration>> argument_type;
   typedef const Set<Int>& result_type;

   result_type operator() (const argument_type& it) const { return it.face(); }
};

struct link_maker {
   Int start_face;
   link_maker(Int start_arg = -1) : start_face(start_arg) {}

   typedef HasseDiagram_facet_iterator<Lattice<BasicDecoration>> argument_type;
   typedef pm::LazySet2<const Set<Int>&, const Set<Int>&, pm::set_difference_zipper> result_type;

   result_type operator() (const argument_type& it) const { return it.face()-it.face(start_face); }
};

// Enumerates the star of a face (specified by it's index in the HasseDiagram.
typedef pm::unary_transform_iterator<HasseDiagram_facet_iterator<Lattice<BasicDecoration> >, star_maker> star_enumerator;

inline
star_enumerator star_in_HD(const Lattice<BasicDecoration>& HD, const Int f)
{
   return HasseDiagram_facet_iterator<Lattice<BasicDecoration> >(HD,f);
}

// Enumerates the link of a face (specified by it's index in the HasseDiagram.
typedef pm::unary_transform_iterator<HasseDiagram_facet_iterator<Lattice<BasicDecoration>>, link_maker> link_enumerator;

inline
link_enumerator link_in_HD(const Lattice<BasicDecoration>& HD, const Int f)
{
   return link_enumerator(HasseDiagram_facet_iterator<Lattice<BasicDecoration>>(HD,f), f);
}

// Enumerates the vertex star of a complex represented as a Hasse Diagram and a given vertex v.
inline
star_enumerator vertex_star_in_HD(const Lattice<BasicDecoration>& HD, const Int v)
{
   return star_in_HD(HD, find_vertex_node(HD,v));
}

// Computes the vertex link of a complex represented as a Hasse Diagram and a given vertex v.
inline
link_enumerator vertex_link_in_HD(const Lattice<BasicDecoration>& HD, const Int v)
{
   return link_in_HD(HD, find_vertex_node(HD,v));
}

// Computes the vertex set of the link of the vertex v.
Set<Int> vertices_of_vertex_link(const Lattice<BasicDecoration>& HD, const Int v);

class out_degree_checker {
public:
   typedef void argument_type;
   typedef bool result_type;

   out_degree_checker(Int degree_arg = 0) : degree(degree_arg) { }

   template <typename Iterator>
   result_type operator() (const Iterator& node_it) const
   {
      return node_it.out_degree()==degree;
   }
protected:
   Int degree;
};

// Computes the boundary complex (= ridges contained in one facet only)
// of a PSEUDO_MANIFOLD. The complex is encoded as a Hasse Diagrams.

template<typename SeqType>      
inline
auto boundary_of_pseudo_manifold(const Lattice<BasicDecoration, SeqType>& PM)
{
   return attach_selector(select(PM.decoration(), PM.nodes_of_rank(PM.rank()-2)), out_degree_checker(1));
}

// Removes the vertex star of v from a complex C, represented as a Hasse Diagram.
void remove_vertex_star(ShrinkingLattice<BasicDecoration>& HD, const Int v);

// Removes a facet F from a simplicial complex, represented as a Hasse Diagram.
template <typename TSet>
void remove_facet(ShrinkingLattice<BasicDecoration>& HD, const GenericSet<TSet, Int>& F);

// Checks whether a 1-complex (graph) is a 1-ball or 1-sphere.
// return values: 1=true, 0=false, -1=undef (does not occur here)
template <typename Complex, typename VertexSet>
Int is_ball_or_sphere(const Complex& C, const GenericSet<VertexSet>& V, int_constant<1>);

// Checks whether a 2-complex is a 2-ball or 2-sphere.
// return values: 1=true, 0=false, -1=undef (does not occur here)
template <typename Complex, typename VertexSet>
Int is_ball_or_sphere(const Complex& C, const GenericSet<VertexSet>& V, int_constant<2>);

// Checks whether a 1-complex (graph) is a manifold.
// return values: 1=true, 0=false, -1=undef (does not occur here)
template <typename Complex, typename VertexSet>
Int is_manifold(const Complex& C, const GenericSet<VertexSet>& V, int_constant<1>, Int* bad_link_p = nullptr);

// Heuristic check whether a complex of arbitrary dimension d is a manifold.
// If not, *bad_link_p = vertex whose link is neither a ball nor a sphere
// return values: 1=true, 0=false, -1=undef
template <typename Complex, typename VertexSet, int d>
Int is_manifold(const Complex& C, const GenericSet<VertexSet>& V, int_constant<d>, Int* bad_link_p = nullptr);

// heuristic sphere checking
// return values: 1=true, 0=false, -1=undef
template <typename Complex, int d>
Int is_ball_or_sphere(const Complex& C, int_constant<d>);

// The same for a trusted complex (pure, without gaps in vertex numbering)
// return values: 1=true, 0=false, -1=undef
template <typename Complex, int d>
Int is_ball_or_sphere(const Complex& C, Int n_vertices, int_constant<d>)
{
   return is_ball_or_sphere(C, sequence(0,n_vertices), int_constant<d>());
}

template <typename Complex, int d>
// return values: 1=true, 0=false, -1=undef
Int is_manifold(const Complex& C, int_constant<d>, Int* bad_link_p = nullptr);

// The same for a trusted complex (pure, without gaps in vertex numbering)
// return values: 1=true, 0=false, -1=undef
template <typename Complex, int d>
Int is_manifold(const Complex& C, Int n_vertices, int_constant<d>, Int* bad_link_p = nullptr)
{
   return is_manifold(C, sequence(0,n_vertices), int_constant<d>(), bad_link_p);
}

/// Adjusts the vertex numbers to 0..n-1.
/// @retval true if the numbering has been adjusted
template <typename Complex, typename Set>
bool adj_numbering(Complex& C, const Set& V);

// Checks whether a complex, represented as a Hasse Diagram, is a closed pseudo manifold.
bool is_closed_pseudo_manifold(const Lattice<BasicDecoration>& HD, bool known_pure);

// Checks whether a complex, represented as a Hasse Diagram, is a pseudo manifold
// and computes its boundary.
template <typename OutputIterator>
bool is_pseudo_manifold(const Lattice<BasicDecoration>& HD, bool known_pure, OutputIterator boundary_consumer, Int* bad_face_p = nullptr);

// Checks whether a complex, represented as a Hasse Diagram, is a pseudo manifold.
inline
bool is_pseudo_manifold(const Lattice<BasicDecoration>& HD, bool known_pure, Int* bad_face_p = nullptr)
{
   return is_pseudo_manifold(HD, known_pure, black_hole<Set<Int>>(), bad_face_p);
}

bool is_pure(const Lattice<BasicDecoration>& HD);

// checks whether a flag of faces, represented by a Set<Set<Int>>,
// lies completely in the boundary of a complex, which is represented by an IncidenceMatrix
inline
bool on_boundary(const Set<Set<Int>>& label, const IncidenceMatrix<>& VIF)
{
   Set<Int> face;
   for (auto lit = entire(label); !lit.at_end(); ++lit)
      face += *lit;

   for (auto rit = entire(rows(VIF)); !rit.at_end(); ++rit) 
      if (!(face - *rit).size())
         return true; // it's contained in the boundary

   return false;
}
      
// The torus.
Array<Set<Int>> torus_facets();

// The real projective plane.
Array<Set<Int>> real_projective_plane_facets();

// The complex projective plane.
Array<Set<Int>> complex_projective_plane_facets();

} }

#include "polymake/topaz/complex_tools.tcc"
#include "polymake/topaz/subcomplex_tools.tcc"
#include "polymake/topaz/1D_tools.tcc"
#include "polymake/topaz/2D_tools.tcc"


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
