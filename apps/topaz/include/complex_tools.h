/* Copyright (c) 1997-2015
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

#ifndef POLYMAKE_TOPAZ_COMPLEX_TOOLS_H
#define POLYMAKE_TOPAZ_COMPLEX_TOOLS_H

#include "polymake/Array.h"
#include "polymake/PowerSet.h"
#include "polymake/graph/HasseDiagramTools.h"
#include "polymake/list"
#include <string>

/** Tools to treat simplicial complexes.
 *
 *  A simplicial complex is represented as a list of its FACETS, encoded as their vertex sets.
 *  Therefore any container of GenericSet of int (template <typename Complex>) may be used to
 *  represent a complex. In the following std::list< polymake::Set<int> >, polymake::PowerSet<int>
 *  and polymake::Array< polymake::Set<int> > are used.
 */


namespace polymake { namespace topaz {

using graph::HasseDiagram;
using graph::HasseDiagram_facet_iterator;

// Computes the k_skeleton of a simplicial complex.
template <typename Complex>
PowerSet<int> k_skeleton(const Complex& C, const int k);

template <typename Complex, typename Set>
struct link_helper {
   typedef pm::constant_value_container<const Set&> same_set;
   typedef pm::SelectedContainerPairSubset< const Complex&, same_set, operations::includes >
   selected_facets;
   typedef pm::SelectedContainerPairSubset< const Complex&, same_set,
                                            operations::composed21<operations::includes, std::logical_not<bool> > >
   deletion_type;
   typedef pm::TransformedContainerPair< selected_facets, same_set, operations::sub >
   result_type;
};

// Computes the star of a given face F.
template <typename Complex, typename Set> inline
typename link_helper<Complex,Set>::selected_facets
star(const Complex& C, const GenericSet< Set,int >& F)
{
   return typename link_helper<Complex,Set>::selected_facets(C, F.top());
}

// Computes the deletion of a given face F.
template <typename Complex, typename Set> inline
typename link_helper<Complex,Set>::deletion_type
deletion(const Complex& C, const GenericSet< Set,int >& F)
{
   return typename link_helper<Complex,Set>::deletion_type(C, F.top());
}

// Computes the link of a given face F.
template <typename Complex, typename Set> inline
typename link_helper<Complex,Set>::result_type
link(const Complex& C, const GenericSet< Set,int >& F)
{
   return typename link_helper<Complex,Set>::result_type(star(C,F), F.top());
}

struct star_maker {
   typedef HasseDiagram_facet_iterator argument_type;
   typedef const Set<int>& result_type;

   result_type operator() (const argument_type& it) const { return it.face(); }
};

struct link_maker {
   int start_face;
   link_maker(int start_arg=-1) : start_face(start_arg) {}

   typedef HasseDiagram_facet_iterator argument_type;
   typedef pm::LazySet2<const Set<int>&, const Set<int>&, pm::set_difference_zipper> result_type;

   result_type operator() (const argument_type& it) const { return it.face()-it.face(start_face); }
};

// Enumerates the star of a face (specified by it's index in the HasseDiagram.
typedef pm::unary_transform_iterator<HasseDiagram_facet_iterator, star_maker> star_enumerator;

inline
star_enumerator star_in_HD(const HasseDiagram& HD, const int f)
{
   return HasseDiagram_facet_iterator(HD,f);
}
  
// Enumerates the link of a face (specified by it's index in the HasseDiagram.
typedef pm::unary_transform_iterator<HasseDiagram_facet_iterator, link_maker> link_enumerator;

inline
link_enumerator link_in_HD(const HasseDiagram& HD, const int f)
{
   return link_enumerator(HasseDiagram_facet_iterator(HD,f), f);
}
  
// Enumerates the vertex star of a complex represented as a Hasse Diagram and a given vertex v.
inline
star_enumerator vertex_star_in_HD(const HasseDiagram& HD, const int v)
{
   return star_in_HD(HD, find_vertex_node(HD,v));
}

// Computes the vertex link of a complex represented as a Hasse Diagram and a given vertex v.
inline
link_enumerator vertex_link_in_HD(const HasseDiagram& HD, const int v)
{
   return link_in_HD(HD, find_vertex_node(HD,v));
}

// Computes the vertex set of the link of the vertex v. 
Set<int> vertices_of_vertex_link(const HasseDiagram& HD, const int v);

// Computes the Hasse Diagram of a complex C. The parameter d
// is a lower bound for the dimension of the complex, default is 0.
// The parameter end_dim determines which levels (starting at
// the top) of the Hasse Diagram are computed.
template <typename Complex>
HasseDiagram hasse_diagram(const Complex& C, const int d=0, const int end_dim=0);

// Computes the Hasse Diagram of a PURE complex C.
// The parameter end_dim determines which levels (starting at
// the top) of the Hasse Diagram are computed.
template <typename Complex>
HasseDiagram pure_hasse_diagram(const Complex& C, const int end_dim=0);

class out_degree_checker {
public:
   typedef void argument_type;
   typedef bool result_type;
    
   out_degree_checker(int degree_arg=0) : degree(degree_arg) { }
    
   template <typename Iterator>
   result_type operator() (const Iterator& node_it) const
   {
      return node_it.out_degree()==degree;
   }
protected:
   int degree;
};
  
// Computes the boundary complex (= ridges contained in one facet only) 
// of a PSEUDO_MANIFOLD. The complex is encoded as a Hasse Diagrams.
typedef pm::IndexedSubset<const HasseDiagram::faces_map_type&, HasseDiagram::nodes_of_dim_set> faces_of_dim_set;
typedef pm::SelectedSubset<faces_of_dim_set, out_degree_checker> Boundary_of_PseudoManifold;
  
inline
Boundary_of_PseudoManifold boundary_of_pseudo_manifold(const HasseDiagram& PM)
{
   return Boundary_of_PseudoManifold(faces_of_dim_set(PM.faces(), PM.nodes_of_dim(-2)), out_degree_checker(1));
}
  
// Removes the vertex star of v from a complex C, represented as a Hasse Diagram.
void remove_vertex_star(HasseDiagram& HD, const int v);

// Removes a facet F from a simplicial complex, represented as a Hasse Diagram.
template <typename Set>
void remove_facet(HasseDiagram& HD, const GenericSet< Set,int >& F);

// Checks whether a 1-complex (graph) is a 1-ball or 1-sphere.
// return values: 1=true, 0=false, -1=undef (does not occur here)
template <typename Complex, typename VertexSet>
int is_ball_or_sphere(const Complex& C, const GenericSet<VertexSet>& V, int2type<1>);

// Checks whether a 2-complex is a 2-ball or 2-sphere.
// return values: 1=true, 0=false, -1=undef (does not occur here)
template <typename Complex, typename VertexSet>
int is_ball_or_sphere(const Complex& C, const GenericSet<VertexSet>& V, int2type<2>);

// Checks whether a 1-complex (graph) is a manifold.
// return values: 1=true, 0=false, -1=undef (does not occur here)
template <typename Complex, typename VertexSet>
int is_manifold(const Complex& C, const GenericSet<VertexSet>& V, int2type<1>, int *bad_link_p=0);

// Heuristic check whether a complex of arbitrary dimension d is a manifold.
// If not, *bad_link_p = vertex whose link is neither a ball nor a sphere
// return values: 1=true, 0=false, -1=undef
template <typename Complex, typename VertexSet, int d>
int is_manifold(const Complex& C, const GenericSet<VertexSet>& V, int2type<d>, int *bad_link_p=0);

// heuristic sphere checking
// return values: 1=true, 0=false, -1=undef
template <typename Complex, int d>
int is_ball_or_sphere(const Complex& C, int2type<d>);

// The same for a trusted complex (pure, without gaps in vertex numbering)
// return values: 1=true, 0=false, -1=undef
template <typename Complex, int d> inline
int is_ball_or_sphere(const Complex& C, int n_vertices, int2type<d>)
{
   return is_ball_or_sphere(C, sequence(0,n_vertices), int2type<d>());
}

template <typename Complex, int d>
// return values: 1=true, 0=false, -1=undef
int is_manifold(const Complex& C, int2type<d>, int *bad_link_p=0);

// The same for a trusted complex (pure, without gaps in vertex numbering)
// return values: 1=true, 0=false, -1=undef
template <typename Complex, int d> inline
int is_manifold(const Complex& C, int n_vertices, int2type<d>, int *bad_link_p=0)
{
   return is_manifold(C, sequence(0,n_vertices), int2type<d>(), bad_link_p);
}

/// Adjusts the vertex numbers to 0..n-1.
/// @retval true if the numbering has been adjusted
template <typename Complex, typename Set>
bool adj_numbering(Complex& C, const Set& V);

// Checks whether a complex, represented as a Hasse Diagram, is a closed pseudo manifold.
bool is_closed_pseudo_manifold(const HasseDiagram& HD, bool known_pure);

// Checks whether a complex, represented as a Hasse Diagram, is a pseudo manifold
// and computes its boundary.
template <typename OutputIterator>
bool is_pseudo_manifold(const HasseDiagram& HD, bool known_pure, OutputIterator boundary_consumer, int *bad_face_p=0);

// Checks whether a complex, represented as a Hasse Diagram, is a pseudo manifold.
inline
bool is_pseudo_manifold(const HasseDiagram& HD, bool known_pure, int *bad_face_p=0)
{
   return is_pseudo_manifold(HD, known_pure, black_hole< Set<int> >(), bad_face_p);
}

bool is_pure(const HasseDiagram& HD);

// The torus.
Array< Set<int> > torus_facets();

// The real projective plane.
Array< Set<int> > real_projective_plane_facets();

// The complex projective plane.
Array< Set<int> > complex_projective_plane_facets();

} }

#include "polymake/topaz/complex_tools.tcc"
#include "polymake/topaz/subcomplex_tools.tcc"
#include "polymake/topaz/1D_tools.tcc"
#include "polymake/topaz/2D_tools.tcc"

#endif // POLYMAKE_TOPAZ_COMPLEX_TOOLS_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
