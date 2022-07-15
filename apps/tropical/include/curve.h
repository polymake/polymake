/* Copyright (c) 1997-2022
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

#include "polymake/Array.h"
#include "polymake/Set.h"
#include <map>
#include "polymake/Map.h"
#include "polymake/Graph.h"
#include "polymake/graph/GraphIso.h"
#include "polymake/IncidenceMatrix.h"

/*

  ** Data structures for tropical curves **

  We have the problem of deciding when two graphs with possibly
  multiple edges and multiple loops between nodes, and with assigned
  edge lengths, are isomorphic. The edge lengths will be modeled by
  colors; this is the job of the three maps

    length_of:        Edges -> Scalar, 
    color_of_length: Scalar -> Int,
    color_of_edge:    Edges -> Int
 
  below.

  Given the interface in GraphIso.h, there are two possible data structures:
  - An IncidenceMatrix allows multiple edges and loops, but not colors 
    on the edges;
  - A Graph<Undirected,Multi> allows multiple edges and loops, but does not
    permit access to unique identifiers of those edges, so we can't consistently
    talk about the color of an edge.

  Therefore, we have to build our own data structure to encode these graphs.
  At the moment, they're realized using the data structure

     InputEdgeIndicesBetween : (node, node) -> Set of original edge indices
  
  which tells the (original, uncontracted) names of the edges between
  two given nodes of the tropical graph.

  In order to solve the isomorphism problem, we build an auxiliary graph
  by modifying the input tropical graph as follows:
  
  (1) marked edges get a new (end-)vertex
  (2) unmarked non-loop edges get subdivided with an additional vertex
  (3) unmarked loop edges get subdivided with two additional vertices.

  The new vertices then get colored according to 
  - whether they're "marked" vertices or "loop" vertices in cases (1) and (3)
  - the color_of_edge in case (2).

  The resulting simple, undirected graph G with node colors is then
  fed into the GraphIso.h interface.
  
  In the code, G is built in the constructor of SubdividedGraph, which
  elaborates some bookkeeping data and then calls build_G, where that
  simple undirected graph is actually constructed.
  
  All this is complicated by the fact that some of the edges of the
  original tropical graph will need to be contracted, which changes
  the connectivity and the resulting simple undirected graph G. We
  have opted to track these connectivity changes at the level of
  InputEdgeIndicesBetween, and to construct G from scratch each time
  all edges have been contracted. (By the way we construct all those
  contracted graphs in the beginning, in fill_cgc(), we actually
  contract these edges one at a time).

  It would also be possible to implement this directly on G, but the
  pay-off seems to be too small at the moment to put effort into this.

  Apart from this, SubdividedGraph also contains more bookkeeping data
  to correctly map non-contracted edges onto original coordinates, and
  to translate automorphisms of G into coordinate permutations of the
  original cone.

  Since we need to repeatedly test rays of the fan for whether the
  corresponding simple undirected graphs are isomorphic, we actually
  construct most of the coloring upon construction (and store the
  result in partial_G_node_colors), and only finish the coloring of
  the nodes corresponding to cases (2) and (3) once those colors
  become available. This happens in induced_node_coloring().

  Finally, the class TropicalGraph takes care of constructing an
  InputEdgeIndicesBetween and a SubdividedGraph from a given
  IncidenceMatrix, marked edges, vertex weights, and set of indices of
  contracted edges. Via auto_group_on_coordinates(), it also gives
  access to the translation of the automorphism group of G to the
  original coordinates.

*/

namespace polymake { namespace tropical {

#if POLYMAKE_DEBUG      
template<typename Container>
void
ensure_permutation(const Container& c)
{
   if (Set<Int>(entire(c)) == sequence(0,c.size()))
      return;
   cerr << "not a permutation: " << c << endl;
   throw std::runtime_error("stop");
}
#endif
      
      
template<typename Scalar>
Map<Scalar,Int>
find_color_of_length(const Map<Int,Scalar>& length_of,
                     const Int verbosity)
{
   Set<Scalar> values;
   for (const auto& is: length_of)
      values += is.second;

   Map<Scalar,Int> color_of_length;
   Int next_new_color(0);
   for (const auto& s: values)
      color_of_length[s] = next_new_color++;

   if (verbosity > 4)
      cerr << "find_color_of_length: length_of " << length_of << " yields " << color_of_length << endl;
   return color_of_length;
}

template<typename Scalar>
Map<Int,Int>
find_color_of_edge(const Map<Int,Scalar>& length_of,
                   const Map<Scalar,Int>& color_of_length,
                   const Int verbosity)
{
   Map<Int,Int> color_of_edge;
   for (const auto& is: length_of)
      color_of_edge[is.first] = color_of_length[is.second];

   if (verbosity > 4)
      cerr << "find_color_of_edge: length_of " << length_of
           << ", color_of_length " << color_of_length
           << " yields color_of_edge " << color_of_edge << endl;
   return color_of_edge;
}

template<typename Scalar>
Set<Int>
zeros_of(const Vector<Scalar>& v)
{
   Set<Int> zero_indices;
   for (auto it = entire<indexed>(v); !it.at_end(); ++it) 
      if (is_zero(*it))
         zero_indices += it.index();
   return zero_indices;
}

template<typename Scalar>
Map<Int,Scalar>
nonzero_lengths_of(const Vector<Scalar>& v)
{
   Map<Int,Scalar> length_of_edge;
   for (auto it = entire<indexed>(v); !it.at_end(); ++it) 
      if (!is_zero(*it))
         length_of_edge[it.index()] = *it;
   return length_of_edge;
}

template<typename Scalar>
Map<Scalar,Int>
multiplicity_of_length(const Map<Int,Scalar>& length_of)
{
   Map<Scalar,Int> mult_of;
   for (const auto& is: length_of)
      mult_of[is.second]++;
   return mult_of;
}

class InputEdgeIndicesBetween : public std::map<std::pair<Int,Int>, Set<Int>> {
   typedef std::map<std::pair<Int,Int>, Set<Int>> super_class;
   
public:
   InputEdgeIndicesBetween() {}

   void
   add(const Int a,
       const Int b,
       const Int value) {
      const auto key(std::make_pair(std::min(a,b), std::max(a,b)));
      auto find_it = find(key);
      if (find_it == end())
         emplace(std::make_pair(key, scalar2set(value)));
      else
         find_it->second += value;
   }

   void
   erase(const Int a,
         const Int b) {
      const auto key(std::make_pair(std::min(a,b), std::max(a,b)));
      super_class::erase(key);
   }

   // let's not confuse ourselves and error out if we try to access the map directly
   auto operator[](const std::pair<Int,Int>&) = delete;

};


class SubdividedGraph {
protected:
   Int n_unsqueezed_edges;
   Int verbosity;
   Int next_available_color;

   Set<Int> loop_edges;
   Set<Int> non_loop_edges;
   Set<Int> squeezed_marked_edge_vertices;
   Map<Int,Int> non_loop_squeezed_edge_vertex_of_edge;
   Map<Int,std::pair<Int,Int>> loop_squeezed_edge_vertices_of_edge;
   Graph<Undirected> G;
   Set<Int> contracted_edges;
   Map<Int,Int> input_edge_index_of_squeezed_edge_node;
   Array<Int> original_node_of_squeezed_node;
   Map<Int,Int> squeezed_node_of_original_node;
   Array<Int> partial_G_node_colors;

   void build_G(const InputEdgeIndicesBetween& input_edge_indices_between) {
      for (const auto& eib: input_edge_indices_between) {
         const Int
            cons_a(squeezed_node_of_original_node[eib.first.first ]),
            cons_b(squeezed_node_of_original_node[eib.first.second]);
         if (cons_a == cons_b) { // these are loop edges
            for (const Int i: eib.second) {
               const auto& new_pair(loop_squeezed_edge_vertices_of_edge[i]);
               const Int cons_c(new_pair.first);
               const Int cons_d(new_pair.second);
               G.edge(cons_a, cons_c);
               G.edge(cons_c, cons_d);
               G.edge(cons_d, cons_a);
            }
         } else { // these are non-loop edges
            for (const Int i: eib.second) {
               const Int cons_j(non_loop_squeezed_edge_vertex_of_edge[i]);
               if (cons_a != cons_j) // special cases for marked vertices
                  G.edge(cons_a, cons_j);
               if (cons_j != cons_b)
                  G.edge(cons_j, cons_b);
            }
         }
      }
      if (verbosity > 4)
         cerr << "G:\n" << G << endl;
   }
   
   void initialize_non_edge_colors(const Array<Int>& contraction_image_of_node,
                                   const Array<Int>& vertex_weights) {
      next_available_color = 0;
      partial_G_node_colors.resize(G.nodes());
      
      for (Int i: squeezed_marked_edge_vertices) 
         partial_G_node_colors[i] = next_available_color++;
      
      Map<Int,Int> color_of_weight;
      const Set<Int> contraction_images(entire(contraction_image_of_node));
      for (auto vit = entire(select(vertex_weights, contraction_images)); !vit.at_end(); ++vit)
         if (!color_of_weight.exists(*vit))
            color_of_weight[*vit] = next_available_color++;
      if (verbosity > 4)
         cerr << "color_of_weight: " << color_of_weight << endl;

      for (Int i: contraction_images)
         partial_G_node_colors[squeezed_node_of_original_node[i]] = color_of_weight[vertex_weights[i]];
   }
   
public:
   SubdividedGraph() {}

   SubdividedGraph(const InputEdgeIndicesBetween& input_edge_indices_between,
                   const Map<Int,Int>& marked_edge_vertex_of_input_edge,
                   const Array<Int>& contraction_image_of_node,
                   const Array<Int>& vertex_weights,
                   const Int _n_unsqueezed_edges,
                   const Int _verbosity)
      : n_unsqueezed_edges(_n_unsqueezed_edges)
      , verbosity(_verbosity)
   {
      Set<Int> original_nodes_with_holes;
      for (const auto& eib: input_edge_indices_between) {
         if (eib.first.first == eib.first.second)
            loop_edges += eib.second;
         else 
            non_loop_edges += eib.second;
         original_nodes_with_holes += eib.first.first;
         original_nodes_with_holes += eib.first.second;
      }
      original_node_of_squeezed_node = Array<Int>(original_nodes_with_holes.size(), entire(original_nodes_with_holes));

      Int next_squeezed_node_index(0);
      for (const Int i: original_nodes_with_holes) 
         squeezed_node_of_original_node[i] = next_squeezed_node_index++;

      for (const auto& mit: marked_edge_vertex_of_input_edge)
         squeezed_marked_edge_vertices += squeezed_node_of_original_node[mit.second];
      
      if (verbosity > 4)
         cerr << "SubdividedGraph(): original_node_of_squeezed_node: " << original_node_of_squeezed_node
              << ", squeezed_node_of_original_node: " << squeezed_node_of_original_node 
              << ", loop_edges: " << loop_edges
              << ", non_loop_edges: " << non_loop_edges
              << ", marked_edge_vertex_of_input_edge: " << marked_edge_vertex_of_input_edge
              << ", squeezed_marked_edge_vertices: " << squeezed_marked_edge_vertices
              << ", n_unsqueezed_edges: " << n_unsqueezed_edges
              << endl;

      G = Graph<Undirected>(next_squeezed_node_index
                            + non_loop_edges.size()
                            + 2 * loop_edges.size()
                            - marked_edge_vertex_of_input_edge.size());

      // the node nodes will be indexed with the original node indices
      // next come the non_loop_edges nodes
      // then the loop nodes
      for (const Int i: non_loop_edges)
         non_loop_squeezed_edge_vertex_of_edge[i] = marked_edge_vertex_of_input_edge.exists(i)
            ? squeezed_node_of_original_node[marked_edge_vertex_of_input_edge[i]]
            : next_squeezed_node_index++;

      for (const Int i: loop_edges) {
         loop_squeezed_edge_vertices_of_edge[i] = std::make_pair(next_squeezed_node_index, next_squeezed_node_index+1);
         next_squeezed_node_index += 2;
      }

      if (verbosity > 4)
         cerr << "non_loop_squeezed_edge_vertex_of_edge: " << non_loop_squeezed_edge_vertex_of_edge << endl
              << "loop_squeezed_edge_vertices_of_edge: " << loop_squeezed_edge_vertices_of_edge << endl;
      
      for (const auto& nle_it: non_loop_squeezed_edge_vertex_of_edge) 
         input_edge_index_of_squeezed_edge_node[nle_it.second] = nle_it.first;
      for (const auto& le_it: loop_squeezed_edge_vertices_of_edge)
         input_edge_index_of_squeezed_edge_node[le_it.second.first] =
            input_edge_index_of_squeezed_edge_node[le_it.second.second] = le_it.first;
      if (verbosity > 4)
         cerr << "input_edge_index_of_squeezed_edge_node: " << input_edge_index_of_squeezed_edge_node << endl;

      build_G(input_edge_indices_between);
      if (G.nodes())
         initialize_non_edge_colors(contraction_image_of_node, vertex_weights);
   }

   const Graph<Undirected>& the_graph() const { return G; }
   
protected:
   Array<Array<Int>> convert_to_unsqueezed_edge_perms(const Array<Array<Int>>& SD_graph_autos) const {
      if (!SD_graph_autos.size())
         return Array<Array<Int>>();

      // some graph_autos might just interchange the auxiliary loop edges,
      // and we won't need those. So right now we don't know how many autos there will be
      std::vector<Array<Int>> edge_perms;
      edge_perms.reserve(SD_graph_autos.size());

      const Array<Int> id(sequence(0, n_unsqueezed_edges));
      
      for (const auto& graph_auto: SD_graph_autos) {
         Array<Int> edge_perm(id);
         for (auto it = entire<indexed>(graph_auto); !it.at_end(); ++it) {
            if (input_edge_index_of_squeezed_edge_node.exists(it.index()))
               edge_perm[input_edge_index_of_squeezed_edge_node[it.index()]] = input_edge_index_of_squeezed_edge_node[*it];
         }
         if (edge_perm != id)
            edge_perms.push_back(edge_perm);
      }
      if (verbosity > 4)
         cerr << "edge_perms:\n" << edge_perms
              << "input_edge_index_of_squeezed_edge_node: " << input_edge_index_of_squeezed_edge_node
              << endl;

#if POLYMAKE_DEBUG
      for (const auto& g: edge_perms)
         ensure_permutation(g);
#endif
      
      return Array<Array<Int>>(edge_perms.size(), entire(edge_perms));
   }

   void color_non_loop_edge_nodes(Array<Int>& final_G_node_colors,
                                  const bool trivial_lengths,
                                  const Map<Int,Int>& color_of_edge,
                                  const std::map<Int,Int>& coordinate_of_input_edge) const {
      for (const auto& nle_it: non_loop_squeezed_edge_vertex_of_edge) {
         if (squeezed_marked_edge_vertices.contains(nle_it.second))
            continue;
         else if (trivial_lengths)
            final_G_node_colors[nle_it.second] = next_available_color;
         else {
            const auto find_it = coordinate_of_input_edge.find(nle_it.first);
            if (find_it == coordinate_of_input_edge.end())
               throw std::runtime_error("induced_node_coloring (color_non_loop_edge_nodes): could not find coordinate of edge");
            final_G_node_colors[nle_it.second] = next_available_color + color_of_edge[find_it->second];
         }
      }
   }

   void color_loop_edge_nodes(Array<Int>& final_G_node_colors,
                              const bool trivial_lengths,
                              const Map<Int,Int>& color_of_edge,
                              const std::map<Int,Int>& coordinate_of_input_edge) const {
      for (const auto& le_it: loop_squeezed_edge_vertices_of_edge) {
         if (squeezed_marked_edge_vertices.contains(le_it.second))
            continue;
         else if (trivial_lengths)
            final_G_node_colors[le_it.second.second] =
               final_G_node_colors[le_it.second.first] = next_available_color + 1;
         else {
            const auto find_it = coordinate_of_input_edge.find(le_it.first);
            if (find_it == coordinate_of_input_edge.end())
               throw std::runtime_error("induced_node_coloring (color_loop_edge_nodes): could not find coordinate of edge");
            final_G_node_colors[le_it.second.second] =
               final_G_node_colors[le_it.second.first] = next_available_color + color_of_edge[find_it->second];
         }
      }
   }
   
public:
   Array<Int> induced_node_coloring(const Array<Int>& vertex_weights,
                                    const Map<Int, Int>& color_of_edge,
                                    const std::map<Int,Int>& coordinate_of_input_edge) const
   {
      if (verbosity > 4)
         cerr << "\ninduced_node_coloring: vertex_weights " << vertex_weights
              << ", color_of_edge " << color_of_edge << endl;
      
      const bool trivial_lengths(0 == color_of_edge.size());
      Array<Int> final_G_node_colors(partial_G_node_colors);
      
      color_non_loop_edge_nodes(final_G_node_colors, trivial_lengths, color_of_edge, coordinate_of_input_edge);
      color_loop_edge_nodes    (final_G_node_colors, trivial_lengths, color_of_edge, coordinate_of_input_edge);

      if (verbosity > 4)
         cerr << "final_G_node_colors: " << final_G_node_colors << endl;
      
      return final_G_node_colors;
   }


   template<typename Scalar>
   Array<Array<Int>> edge_autos(const Array<Int>& vertex_weights,
                                const Map<Int,Scalar>& length_of_edge,
                                const std::map<Int,Int>& coordinate_of_input_edge) const {
      if (verbosity > 4)
         cerr << "edge_autos with vertex_weights " << vertex_weights
              << ", length_of_edge " << length_of_edge << ". "
              << endl;

      const Map<Int,Int> color_of_edge = find_color_of_edge(length_of_edge, find_color_of_length(length_of_edge, verbosity), verbosity);
      const Array<Array<Int>> SD_graph_autos = graph::automorphisms(G, induced_node_coloring(vertex_weights, color_of_edge, coordinate_of_input_edge));
      if (verbosity > 4)
         cerr << "SD_graph_autos:\n" << SD_graph_autos << endl;
      
      return convert_to_unsqueezed_edge_perms(SD_graph_autos);
   }

};
      
class Curve {
protected:
   const IncidenceMatrix<>& original_etv;
   Array<Int> vertex_weights;
   Int verbosity;
   Set<Int> marked_edges;
   Array<Int> edge_index_of_coordinate;
   Array<Int> unmarked_edges;
   Set<Int> contracted_edges;
   Int original_n_edges;
   Array<Int> node_contracts_to;
   
   InputEdgeIndicesBetween input_edge_indices_between;
   std::map<Int,Int> coordinate_of_input_edge;
   Map<Int,Int> marked_edge_vertex_of_input_edge;
   
private:
   SubdividedGraph SD_graph;

protected:
   struct ToModify {
      std::vector<std::pair<Int,Int>> to_erase;
      std::vector<std::array<Int,3>> to_add;
   };

   ToModify
   indices_to_modify(const Int a,
                     const Int b) const {

      // in input_edge_indices_between, map the indices of
      // (b,x) -> { i1, i2, ... } and (x,b) -> { j1, j2, ... }
      // to (a,x) and (x,a), respectively

      assert(a != b);

      ToModify to_modify;
      for (auto mit = input_edge_indices_between.begin(); mit != input_edge_indices_between.end(); ++mit) {
         const bool
            b_first(mit->first.first == b),
            b_second(mit->first.second == b);
         if (b_first && b_second) {
            for (const Int j: mit->second)
               to_modify.to_add.push_back({{a,a,j}});
            to_modify.to_erase.push_back({b,b});
         } else if (b_first) {
            const Int c(mit->first.second);
            for (const Int j: mit->second)
               to_modify.to_add.push_back({{a,c,j}});
            to_modify.to_erase.push_back({b,c});
         } else if (b_second) {
            const Int c(mit->first.first);
            for (const Int j: mit->second)
               to_modify.to_add.push_back({{a,c,j}});
            to_modify.to_erase.push_back({c,b});
         }
      }
      return to_modify;
   }
   
   void contract_edge(const Int edge_index) {
      if (verbosity > 4)
         cerr << "contracting edge " << edge_index
              << " with vertex weights being " << vertex_weights
              << endl;
      
      if (marked_edges.contains(edge_index))
         throw std::runtime_error("cannot contract marked edge");
          
      // we will do as little erasing and reindexing as possible,
   
      auto ie_it = input_edge_indices_between.begin();
      for (; ie_it != input_edge_indices_between.end(); ++ie_it) {
         if (ie_it->second.contains(edge_index)) {
            ie_it->second -= edge_index;
            break;
         }
      }
      if (ie_it == input_edge_indices_between.end())
         throw std::runtime_error("could not find edge to remove");

      if (verbosity > 4)
         cerr << "removed " << edge_index << " from " << ie_it->first << " -> " << ie_it->second << endl;
      
      const Int
         a(ie_it->first.first),
         b(ie_it->first.second);

      if (a >= vertex_weights.size() ||
          b >= vertex_weights.size())
         throw std::runtime_error("contract_edge: illegal index");
      
      // now a is the valid node
      node_contracts_to[b] = a;
      
      if (a==b) {
         vertex_weights[a]++;
         if (!ie_it->second.size())
            input_edge_indices_between.erase(a,a);
      } else { // a != b
         vertex_weights[a] += vertex_weights[b];
         const ToModify itm = indices_to_modify(a,b);
         for (const auto& p: itm.to_erase)
            input_edge_indices_between.erase(p.first, p.second);
         for (const auto& t: itm.to_add)
            input_edge_indices_between.add(t[0], t[1], t[2]);
      }      
      if (verbosity > 4) {
         cerr << "after erasing edge " << edge_index << ", input_edge_indices_between:" << endl;
         for (const auto& p: input_edge_indices_between)
            cerr << p.first << " -> " << p.second << endl;
         cerr << "coordinate_of_input_edge: ";
         for (const auto& it: coordinate_of_input_edge)
            cerr << "(" << it.first << " -> " << it.second << ") ";
         cerr << ", edge_index_of_coordinate: " << edge_index_of_coordinate 
              << ", vertex_weights: " << vertex_weights << endl;
      }
   }      

   Array<Int> make_contraction_image_of_node() const {
      Array<Int> contraction_image_of_node(node_contracts_to.size());
      for (Int i=0; i<node_contracts_to.size(); ++i) {
         Int j(i);
         while(node_contracts_to[j] != j) 
            j = node_contracts_to[j];
         contraction_image_of_node[i] = j;
      }
      if (verbosity > 4)
         cerr << "node_contracts_to " << node_contracts_to
              << ", contraction_image_of_node " << contraction_image_of_node << endl;
      return contraction_image_of_node;
   }
   
public:

   Curve() = delete;

   Curve(const IncidenceMatrix<>& _etv,
                 const Set<Int>& _marked_edges,
                 const Array<Int>& _vertex_weights,
                 const Set<Int>& _contracted_edges,
                 const Int _verbosity)
      : original_etv(_etv)
      , vertex_weights(_vertex_weights)
      , verbosity(_verbosity)
      , marked_edges(_marked_edges)
      , edge_index_of_coordinate(_etv.cols() - _marked_edges.size())
      , unmarked_edges(_etv.cols() - _marked_edges.size(), entire(Set<Int>(sequence(0, _etv.cols())) - marked_edges))
      , contracted_edges(_contracted_edges)
      , original_n_edges(_etv.cols())
      , node_contracts_to(Array<Int>(_etv.rows(), entire(sequence(0, _etv.rows()))))
   {
      if (_vertex_weights.size() &&
          _vertex_weights.size() != _etv.rows())
         throw std::runtime_error("Curve: Unexpected length of vertex weights");

      if (verbosity > 4)
         cerr << "Curve constructor with etv\n" << original_etv
              << "original_n_edges = " << original_n_edges 
              << ", vertex weights " << vertex_weights << endl;
      
      Int max_marked_index_processed(0);
      Int unmarked_edge_counter(0);
      for (auto cit = entire<indexed>(cols(_etv)); !cit.at_end(); ++cit) {
         const Set<Int> verts(*cit);
         if (1 != verts.size() &&
             2 != verts.size())
            throw std::runtime_error("Curve: unexpected size of edge");

         const bool edge_is_marked(marked_edges.contains(cit.index()));
                                   
         const Int 
            a(verts.front()),
            b((1 == verts.size() && edge_is_marked)
              ? (_etv.rows() + max_marked_index_processed++)
              : verts.back());
         assert(a <= b);
         if (verbosity > 4)
            cerr << "edge " << cit.index() << ": verts " << verts << " -> connects a=" << a << " and b=" << b
                 << "; max_marked_index_processed " << max_marked_index_processed << endl;
         
         // the next data structure, input_edge_indices_between, implements the graph,
         // because there seems to be no way of getting a consistent edge_id, contractions, and automorphism groups with Graph<UndirectedMulti>
         // the edge indices come from the column indices of the incidence matrix
         input_edge_indices_between.add(a,b,cit.index()); 
         if (!edge_is_marked) {
            coordinate_of_input_edge[cit.index()] = unmarked_edge_counter;
            edge_index_of_coordinate[unmarked_edge_counter] = cit.index();
            ++unmarked_edge_counter;
         } else
            marked_edge_vertex_of_input_edge[cit.index()] = b;
         if (verbosity > 4)
            cerr << "unmarked_edge_counter now " << unmarked_edge_counter << endl;
      }
      if (verbosity > 4) {
         cerr << "coordinate_of_input_edge: ";
         for (const auto& it: coordinate_of_input_edge)
            cerr << "(" << it.first << " -> " << it.second << ") ";

         cerr << ", edge_index_of_coordinate: " << edge_index_of_coordinate << endl
              << "input_edge_indices_between:\n";
         for (const auto& p: input_edge_indices_between)
            cerr << p.first << " -> " << p.second << endl;
      }

      for (const Int i: _contracted_edges)
         contract_edge(i);
      
      SD_graph = SubdividedGraph(input_edge_indices_between, marked_edge_vertex_of_input_edge, make_contraction_image_of_node(), vertex_weights, original_n_edges, verbosity);
   }

   Curve(const IncidenceMatrix<>& _etv,
         const Int _verbosity)
      : Curve(_etv,
              Set<Int>(), // marked_edges
              Array<Int>(_etv.rows(), 1), // vertex_weights
              Set<Int>(), // contracted_edges
              _verbosity)
   {}

   Curve(const IncidenceMatrix<>& _etv,
         const Set<Int>& _contracted_edges,
         const Int _verbosity)
      : Curve(_etv,
              Set<Int>(), // marked_edges
              Array<Int>(_etv.rows()), // vertex_weights
              _contracted_edges, // contracted_edges
              _verbosity)
   {}

   Curve(const IncidenceMatrix<>& _etv,
         const Set<Int>& _marked_edges,
         const Array<Int>& _vertex_weights,
         const Int _verbosity)
      : Curve(_etv,
              _marked_edges,
              _vertex_weights,
              Set<Int>(), // contracted_edges
              _verbosity)
   {}

   Curve(const IncidenceMatrix<>& _etv,
         const Array<Int>& _vertex_weights,
         const Set<Int>& _contracted_edges,
         const Int _verbosity)
      : Curve(_etv,
              Set<Int>(), // marked_edges
              _vertex_weights,
              _contracted_edges,
              _verbosity)
   {}
   
   Curve(const Curve& tg,
         const Set<Int>& additionally_contracted_edges)
      : Curve(tg)
   {
      if (verbosity > 3)
         cerr << "Curve: additionally contracting from graph with already contracted edges " << tg.contracted_edges << endl;
         
      for (const Int i: additionally_contracted_edges - tg.contracted_edges)
            contract_edge(i);
      
      SD_graph = SubdividedGraph(input_edge_indices_between, marked_edge_vertex_of_input_edge, make_contraction_image_of_node(), vertex_weights, original_n_edges, verbosity);
   }

   Curve(const Curve& tg,
         const Int additionally_contracted_edge)
      : Curve(tg)
   {
      if (verbosity > 3)
         cerr << "Curve: additionally contracting edge " << additionally_contracted_edge << " from graph with already contracted edges " << tg.contracted_edges << endl;
         
      contract_edge(additionally_contracted_edge);
      contracted_edges += additionally_contracted_edge;
      
      SD_graph = SubdividedGraph(input_edge_indices_between, marked_edge_vertex_of_input_edge, make_contraction_image_of_node(), vertex_weights, original_n_edges, verbosity);
   }
   
   const Int get_coordinate_of_input_edge(const Int edge_index) const {
      if (contracted_edges.contains(edge_index))
         throw std::runtime_error("getting coordinate of contracted edge");
      const auto find_it = coordinate_of_input_edge.find(edge_index);
      if (find_it == coordinate_of_input_edge.end())
         throw std::runtime_error("get_coordinate_of_input_edge: edge_index not found");
      return find_it->second;
   }
   const std::map<Int,Int>& get_coordinate_of_unmarked_edge() const { return coordinate_of_input_edge; }
   const Set<Int>& get_marked_edges() const { return marked_edges; }
   const Array<Int>& get_unmarked_edges() const { return unmarked_edges; }
   const Int get_edge_index_of_coordinate(const Int coordinate) const {
      return edge_index_of_coordinate[coordinate];
   }
   const Array<Int>& get_vertex_weights() const { return vertex_weights; }
   const Set<Int>& get_contracted_edges() const { return contracted_edges; }
   const Int get_verbosity() const { return verbosity; }

   const InputEdgeIndicesBetween& get_edge_indices_between() const { return input_edge_indices_between; }
   const Graph<Undirected>& subdivided_graph() const { return SD_graph.the_graph(); }
   const Set<Int> get_participating_node_indices() const {
      Set<Int> node_indices;
      for (const auto& eib: input_edge_indices_between) {
         node_indices += eib.first.first;
         node_indices += eib.first.second;
      }
      return node_indices;
   }
   const Set<Int> get_participating_edge_indices() const {
      Set<Int> edge_indices;
      for (const auto& eib: input_edge_indices_between) 
         edge_indices += eib.second;
      return edge_indices;
   }
   
   const IncidenceMatrix<> get_incidence_matrix() const {
      const Set<Int>
         node_indices = get_participating_node_indices(),
         edge_indices = get_participating_edge_indices();

      if (verbosity)
         cerr << "get_incidence_matrix: participating nodes " << node_indices
              << ", participating edges " << edge_indices
              << endl;
      Map<Int,Int> edge_index_of, node_index_of;
      Int next_edge_index(0), next_node_index(0);
      for (const Int i: edge_indices)
         edge_index_of[i] = next_edge_index++;
      for (const Int i: node_indices)
         node_index_of[i] = next_node_index++;
      
      IncidenceMatrix<> inc(node_indices.size(), edge_indices.size());
      for (const auto& eib: input_edge_indices_between) 
         for (const Int j: eib.second) 
            inc(node_index_of[eib.first.first], edge_index_of[j]) =
               inc(node_index_of[eib.first.second], edge_index_of[j]) = true;

      return inc;
   }

   Array<Int> induced_node_coloring(const Map<Int, Int>& color_of_edge) const {
      return SD_graph.induced_node_coloring(vertex_weights, color_of_edge, coordinate_of_input_edge);
   }

   protected:
   Array<Array<Int>> convert_to_action_on_coordinates(const Array<Array<Int>>& auto_group_on_unsqueezed_edges) const
   {
      if (verbosity > 4) {
         cerr << "convert_to_action_on_coordinates"
              << ", coordinate_of_input_edge: {";
         for (const auto& it: coordinate_of_input_edge)
            cerr << "(" << it.first << " " << it.second << ") ";
         cerr << "}" << endl;
      }
      
      Array<Array<Int>> auto_group_on_coordinates(auto_group_on_unsqueezed_edges.size());
      auto orig_it = entire(auto_group_on_unsqueezed_edges);
      auto target_it = entire(auto_group_on_coordinates);
      while (!orig_it.at_end()) {
         Array<Int> g(sequence(0, coordinate_of_input_edge.size()));
         if (verbosity > 4)
            cerr << "orig_it: " << *orig_it << ", initial g: " << g << endl;
         for (const auto& ec: coordinate_of_input_edge) {
            if (ec.first >= orig_it->size()) // a high-index input edge got deleted, so that index stays constant
               continue;
            const auto find_target_coo = coordinate_of_input_edge.find((*orig_it)[ec.first]);
            if (find_target_coo == coordinate_of_input_edge.end()) {
               cerr << "ec: (" << ec.first << "," << ec.second << ")" << endl;
               throw std::runtime_error("convert_to_action_on_coordinates: could not find target coordinate");
            }
            if (ec.second >= g.size())
               throw std::runtime_error("convert_to_action_on_coordinates: illegal coordinate");
            g[ec.second] = find_target_coo->second;
         }
         *target_it = g;
         ++orig_it; ++target_it;
      }
      if (verbosity > 2)
         cerr << "auto_group_on_coordinates (" << auto_group_on_coordinates.size() << "):\n" << auto_group_on_coordinates;

#if POLYMAKE_DEBUG      
      for (const auto& g: auto_group_on_coordinates)
         ensure_permutation(g);
#endif
      
      return auto_group_on_coordinates;
   }

   public:
   template<typename Scalar>
   Array<Array<Int>> auto_group_on_coordinates(const Map<Int,Scalar>& length_of_edge) const {
      return convert_to_action_on_coordinates(SD_graph.edge_autos(vertex_weights, length_of_edge, coordinate_of_input_edge));
   }

   Array<Array<Int>> auto_group_on_coordinates() const {
      Map<Int,Int> dummy_map;
      return convert_to_action_on_coordinates(SD_graph.edge_autos(vertex_weights, dummy_map, coordinate_of_input_edge));
   }
   
   template<typename Output>
   friend Output& operator<< (GenericOutput<Output>& os, const Curve& tg) {
      os.top() << "Curve with"
               << " vertex_weights " << tg.vertex_weights
               << ", marked_edges " << tg.marked_edges
               << ", edge_index_of_coordinate " << tg.edge_index_of_coordinate
               << ", unmarked_edges " << tg.unmarked_edges
               << ", coordinate_of_input_edge: {";
      for (const auto& it: tg.coordinate_of_input_edge)
         os.top() << "(" << it.first << " " << it.second << ")";
      
      return
         os.top() << "}, graph\n" << tg.SD_graph.the_graph();
   }

};
      

bool
isomorphic_curves_impl(const Curve& ct_v,
                       const Curve& ct_w,
                       const Array<Int>& coloring_v,
                       const Map<Int,Int>& color_of_edge_for_w,
                       const Int verbosity);

typedef std::map<Set<Int>, Curve> ContractedGraphCollection;

void
fill_contracted_graph_collection(ContractedGraphCollection& cgc,
                                 const Curve& tg);

template<typename Scalar>
class UniqueRepFinder {
protected:

   Int& next_new_global_index_of_vertex;
   Array<Int>& global_vertex_of;
   std::vector<Int>& original_vertex_of;
   const Matrix<Scalar>& coordinates;
   std::vector<Map<Int,Scalar>>& nonzero_lengths_of_vertex;
   std::vector<Set<Int>>& zeros_of_support_of_vertex;
   Map<Array<Scalar>, Set<Int>>& possibly_isomorphic_to_vertex;
   std::vector<std::string>& vertex_labels;
   const ContractedGraphCollection& cgc;
   const Int n_vertices;
   std::ostringstream& os;
   const Int verbosity;

   virtual
   const Curve& find_curve_of(const Int k) {
      const auto find_w_it = cgc.find(zeros_of_support_of_vertex[k]);
      assert (find_w_it != cgc.end());
      return find_w_it->second;      
   }

   virtual
   void notify_found(const Int v_index,
                     const Vector<Scalar>& v,
                     const Int k) const {
      if (verbosity)
         cerr << "moduli_space: vertex " << v_index << " = " << v
              << " is isomorphic to previous vertex " << k
              << " with label " << vertex_labels[k]
              << endl;
   }
   
   virtual
   void post_processing(const Vector<Scalar>& v) {
      os.str("");
      wrap(os) << v;
      vertex_labels.push_back(os.str());
   }
   
public:
   UniqueRepFinder() = delete;
   UniqueRepFinder(Int& _next_new_global_index_of_vertex,
                   Array<Int>& _global_vertex_of,
                   std::vector<Int>& _original_vertex_of,
                   const Matrix<Scalar>& _coordinates,
                   std::vector<Map<Int,Scalar>>& _nonzero_lengths_of_vertex,
                   std::vector<Set<Int>>& _zeros_of_support_of_vertex,
                   Map<Array<Scalar>, Set<Int>>& _possibly_isomorphic_to_vertex,
                   std::vector<std::string>& _vertex_labels,
                   const ContractedGraphCollection& _cgc,
                   const Int _n_vertices,
                   std::ostringstream& _os,
                   const Int _verbosity)
   : next_new_global_index_of_vertex(_next_new_global_index_of_vertex)
   , global_vertex_of(_global_vertex_of)
   , original_vertex_of(_original_vertex_of)
   , coordinates(_coordinates)
   , nonzero_lengths_of_vertex(_nonzero_lengths_of_vertex)
   , zeros_of_support_of_vertex(_zeros_of_support_of_vertex)
   , possibly_isomorphic_to_vertex(_possibly_isomorphic_to_vertex)
   , vertex_labels(_vertex_labels)
   , cgc(_cgc)
   , n_vertices(_n_vertices)
   , os(_os)
   , verbosity(_verbosity)
   {
      global_vertex_of.resize(n_vertices);
   }

   void find_unique_reps() {
      for (Int v_index=0; v_index < n_vertices; ++v_index) {
         const Vector<Scalar> v(coordinates[v_index]);
         const Map<Int, Scalar> nzl_v(nonzero_lengths_of(v));
         const Map<Scalar, Int> mult_v(multiplicity_of_length(nzl_v));
         const Map<Scalar, Int> col_v(find_color_of_length(nzl_v, verbosity));
         const Map<Int,Int> color_of_edge_for_v(find_color_of_edge(nzl_v, col_v, verbosity));
         const Set<Int> zeros_v(zeros_of(v));
         const auto find_v_it = cgc.find(zeros_v);
         assert (find_v_it != cgc.end());

         const Array<Int> coloring_v(find_v_it->second.induced_node_coloring(color_of_edge_for_v));

         bool found_isomorphic_previous_vertex(false);
         Array<Scalar> v_sorted(v.dim(), entire(v));
         std::sort(v_sorted.begin(), v_sorted.end());
         for (const Int k: possibly_isomorphic_to_vertex[v_sorted]) {            
            const Map<Int,Int> color_of_edge_for_w(find_color_of_edge(nonzero_lengths_of_vertex[k], col_v, verbosity));
            
            const Curve& tg_w = find_curve_of(k);
            
            if (isomorphic_curves_impl(find_v_it->second, tg_w, coloring_v, color_of_edge_for_w, verbosity)) {
               global_vertex_of[v_index] = k;
               found_isomorphic_previous_vertex = true;
               notify_found(v_index, v, k);
               break;
            }
         }
         
         if (found_isomorphic_previous_vertex) 
            continue;

         nonzero_lengths_of_vertex.push_back(nzl_v);
         zeros_of_support_of_vertex.push_back(zeros_v);
         global_vertex_of[v_index] = next_new_global_index_of_vertex;
         original_vertex_of.push_back(v_index);
         possibly_isomorphic_to_vertex[v_sorted] += next_new_global_index_of_vertex;

         post_processing(v);
         ++next_new_global_index_of_vertex;
      }
      
   }   
        
};

template<typename Scalar>
class UniqueRepFinderFromArray : public UniqueRepFinder<Scalar> {
protected:

   const Int i;
   const Array<ContractedGraphCollection>& cgcs;
   std::vector<Int>& graph_containing_index_of_vertex;
   Int ell;
   
public:
   UniqueRepFinderFromArray() = delete;
   UniqueRepFinderFromArray(Int& _next_new_global_index_of_vertex,
                            Array<Int>& _global_vertex_of,
                            std::vector<Int>& _original_vertex_of,
                            const Matrix<Scalar>& _coordinates,
                            std::vector<Map<Int,Scalar>>& _nonzero_lengths_of_vertex,
                            std::vector<Set<Int>>& _zeros_of_support_of_vertex,
                            Map<Array<Scalar>, Set<Int>>& _possibly_isomorphic_to_vertex,
                            std::vector<std::string>& _vertex_labels,
                            const ContractedGraphCollection& _cgc,
                            const Int _n_vertices,
                            std::ostringstream& _os,
                            const Int _verbosity,
                            const Int _i,
                            const Array<ContractedGraphCollection>& _cgcs,
                            std::vector<Int>& _graph_containing_index_of_vertex)
   : UniqueRepFinder<Scalar>(_next_new_global_index_of_vertex, _global_vertex_of, _original_vertex_of, _coordinates, _nonzero_lengths_of_vertex, _zeros_of_support_of_vertex, _possibly_isomorphic_to_vertex, _vertex_labels, _cgc, _n_vertices, _os, _verbosity)
   , i(_i)
   , cgcs(_cgcs)
   , graph_containing_index_of_vertex(_graph_containing_index_of_vertex)
   , ell(0)
   {}

   const Curve& find_curve_of(const Int k) {
      ell = graph_containing_index_of_vertex[k];
      const auto find_w_it = cgcs[ell].find(this->zeros_of_support_of_vertex[k]);
      assert (find_w_it != cgcs[ell].end());
      return find_w_it->second;      
   }

   void notify_found(const Int v_index,
                     const Vector<Scalar>& v,
                     const Int k) const {
      if (this->verbosity)
         cerr << "moduli_space: vertex " << v_index << " = " << v
              << " is isomorphic to previous vertex " << k
              << " from graph " << ell
              << " with label " << this->vertex_labels[k]
              << endl;
   }
   
   void post_processing(const Vector<Scalar>& v) {
      this->os.str("");
      wrap(this->os) << i << ": " << v;
      this->vertex_labels.push_back(this->os.str());
      graph_containing_index_of_vertex.push_back(i);
   }

};



  } }
// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
    
