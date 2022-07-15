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

#include "polymake/client.h"
#include "polymake/vector"
#include "polymake/list"
#include "polymake/linalg.h"
#include "polymake/polytope/solve_LP.h"
#include "polymake/Map.h"
#include "polymake/Graph.h"
#include "polymake/Set.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/common/labels.h"

namespace polymake { namespace polytope {
namespace {

template <typename E, typename Matrix, typename Vector1, typename Vector2>
void assign_facet_through_points(const GenericMatrix<Matrix, E>& M,
                                 const GenericVector<Vector1, E>& V_cut,
                                 GenericVector<Vector2, E>&& f)
{
   f = null_space(M)[0];
   if (f * V_cut > 0) f.negate();
}

} // end anonymous namespace

template <typename Scalar, typename TSet>
BigObject truncation(BigObject p_in, const GenericSet<TSet>& trunc_vertices, OptionSet options)
{
   if (options.exists("cutoff") && options.exists("no_coordinates")) 
      throw std::runtime_error("truncation: cannot specify cutoff and no_coordinates options simultaneously");
   
   const bool pointed = p_in.give("POINTED");
   if (!pointed)
      throw std::runtime_error("truncation: input should be pointed");
      
   const bool noc = options["no_coordinates"],
      relabel = !options["no_labels"];

   const IncidenceMatrix<> VIF=p_in.give("VERTICES_IN_FACETS");
   const Graph<> G=p_in.give("GRAPH.ADJACENCY");

   bool inequalities;

   const Int n_vertices = VIF.cols(), n_facets=VIF.rows();

   if (n_vertices < 2)
      throw std::runtime_error("truncation: cannot truncate polytopes with dimension < 1");

   typedef Map<Int, Int> vertex_map_type;
   vertex_map_type vertex_map;          // truncated vertex => the first of the new vertices

   Int n_vertices_out, n_trunc_vertices;
   if (trunc_vertices.top().empty()) 
      throw std::runtime_error("truncation: no vertices to truncate specified");
   if (trunc_vertices.top().front() < 0 || trunc_vertices.top().back() >= n_vertices)
      throw std::runtime_error("vertex numbers out of range");

   BigObject p_out("Polytope", mlist<Scalar>());
   if (std::is_same<TSet, Set<Int>>::value)
      p_out.set_description() << p_in.name() << " with vertices " << trunc_vertices << " truncated" << endl;

   n_trunc_vertices=trunc_vertices.top().size();
   n_vertices_out=n_vertices-n_trunc_vertices;

   for (auto v = entire(trunc_vertices.top()); !v.at_end(); ++v) {
      vertex_map[*v] = n_vertices_out;
      n_vertices_out += G.degree(*v);
   }

   Int n_facets_out = n_facets+n_trunc_vertices;
   IncidenceMatrix<> VIF_out(n_facets_out, n_vertices_out);

   // first inherit the original facets along with untouched vertices in them
   if (n_trunc_vertices < n_vertices)
      copy_range(entire(rows(VIF.minor(All,~keys(vertex_map)))), rows(VIF_out).begin());

   Int new_facet = n_facets;
   for (const auto& vm : vertex_map) {
      Int new_vertex = vm.second;
      for (auto nb = entire(G.adjacent_nodes(vm.first));  !nb.at_end();  ++nb, ++new_vertex) {
         // the new vertex inherits the ridge from the truncated vertex,
         // and it belongs to the new facet
         (VIF_out.col(new_vertex) = VIF.col(vm.first) * VIF.col(*nb)) += new_facet;
      }
      ++new_facet;
   }

   std::vector<std::string> labels_out;
   if (relabel) {
      const std::vector<std::string> labels = common::read_labels(p_in, "VERTEX_LABELS", n_vertices);
      labels_out.resize(n_vertices_out);
      copy_range(entire(select(labels, ~keys(vertex_map))), labels_out.begin());

      for (const auto& vm : vertex_map) {
         Int new_vertex = vm.second;
         for (auto nb = entire(G.adjacent_nodes(vm.first));  !nb.at_end();  ++nb, ++new_vertex) {
            labels_out[new_vertex]=labels[vm.first] + '-' + labels[*nb];
         }
      }
   }

   if (noc) {
      if (p_in.exists("COMBINATORIAL_DIM")) {
         const Int dim = p_in.give("COMBINATORIAL_DIM");
         p_out.take("COMBINATORIAL_DIM") << dim;
         inequalities = (dim == 1);
      } else {
         inequalities = true;
      }
   } else {
      Scalar cutoff_factor = Scalar(1)/Scalar(2);
      if (options["cutoff"] >> cutoff_factor && (cutoff_factor<=0 || cutoff_factor>1))
         throw std::runtime_error("cutoff factor must be within (0,1]");

      std::vector<Int> renumber_vertices;
      if (cutoff_factor == 1) {
         renumber_vertices.resize(n_vertices);
         copy_range(sequence(0).begin(), select(renumber_vertices, ~keys(vertex_map)).begin());
      }
      const Int dim = p_in.give("CONE_DIM");
      inequalities = (cutoff_factor == 1 || dim == 2);
      
      const Matrix<Scalar> V=p_in.give("VERTICES"),
         F=p_in.give("FACETS"),
         AH=p_in.give("AFFINE_HULL");

      Matrix<Scalar> F_out = F / zero_matrix<Scalar>(n_trunc_vertices, F.cols());
      Matrix<Scalar> orth(AH);
      if (orth.cols() != 0) orth.col(0).fill(0);

      auto new_facet_it = rows(F_out).begin() + n_facets;
      for (const auto& vm : vertex_map) {
         const Int v_cut_off = vm.first;
         Matrix<Scalar> basis(G.out_degree(v_cut_off), V.cols());
         const bool simple_vertex = basis.rows()+AH.rows() == V.cols()-1;

         auto b = rows(basis).begin();
         for (auto nb_v = entire(G.adjacent_nodes(v_cut_off)); !nb_v.at_end(); ++nb_v, ++b) {
            if (vertex_map.exists(*nb_v))
               *b = (1-cutoff_factor/2) * V[v_cut_off] + cutoff_factor/2 * V[*nb_v];
            else
               *b = (1-cutoff_factor) * V[v_cut_off] + cutoff_factor * V[*nb_v];
         }
         if (simple_vertex) {
            // calculate a hyperplane thru the basis points
            assign_facet_through_points(basis/orth, V[v_cut_off], *new_facet_it);
         } else {
            // look for a valid separating hyperplane farthest from the vertex being cut off
            const auto S = solve_LP(basis, orth, V[v_cut_off], false);
            if (S.status != LP_status::valid)
               throw std::runtime_error("truncation: wrong LP");
            *new_facet_it = S.solution;
         }

         if (cutoff_factor==1) {
            // we must take care of coinciding vertices
            Int new_vertex = vm.second;
            b = rows(basis).begin();
            for (auto nb_v = entire(G.adjacent_nodes(v_cut_off)); !nb_v.at_end();  ++nb_v, ++new_vertex) {

               if (!simple_vertex && !is_zero((*new_facet_it)*(*b))) continue;       // doesn't touch this vertex

               Int other_vertex;
               auto otv = vertex_map.find(*nb_v);
               if (!otv.at_end()) {
                  // pairs of coinciding new vertices should not be handled twice
                  if (v_cut_off < *nb_v) continue;
                  other_vertex = otv->second;
                  // number of the opposite new vertex can be found only by enumeration of adjacent_nodes...
                  for (auto other_nb_v = G.adjacent_nodes(*nb_v).begin();
                       *other_nb_v != v_cut_off;  ++other_nb_v, ++other_vertex) ;
               } else {
                  other_vertex=renumber_vertices[*nb_v];
               }
               // the new vertex coincides with the neighbor and disappears
               VIF_out.col(other_vertex) += VIF_out.col(new_vertex).back(); // the new facet
               VIF_out.col(new_vertex).clear();
               if (relabel) labels_out[new_vertex].clear();
            }
         }
         ++new_facet_it;
      }

      // if cutoff is 1 some old facets might not be facets anymore
      // if dim is 1 then vertices are facets. So we truncate facets
      if (inequalities){
        F_out /= unit_vector<Scalar>(F_out.cols(), 0);
        p_out.take("INEQUALITIES")  << F_out;
        p_out.take("EQUATIONS") << AH;
      } else {
        p_out.take("FACETS")  << F_out;
        p_out.take("AFFINE_HULL") << AH;
      }

      if (cutoff_factor==1) {
         VIF_out.squeeze();
         if (relabel) {
            labels_out.resize(std::remove(labels_out.begin()+n_vertices-n_trunc_vertices,
                                          labels_out.end(), std::string())
                              - labels_out.begin());
         }
      }
   }

   p_out.take("N_VERTICES") << VIF_out.cols();
   p_out.take("FEASIBLE") << true;
   
   // if cutoff is 1 some old facets might not be facets anymore
   if (!inequalities){
     p_out.take("VERTICES_IN_FACETS") << VIF_out;
   }

   if (relabel)
      p_out.take("VERTEX_LABELS") << labels_out;
   
   return p_out;
}

template<typename Scalar>
BigObject truncation(BigObject p_in, const pm::all_selector&, OptionSet options)
{
   const Int n_verts = p_in.give("N_VERTICES");
   BigObject p_out = truncation<Scalar>(p_in, sequence(0, n_verts), options);
   p_out.set_description() << p_in.name() << " with all vertices truncated" << endl;
   return p_out;
}

template<typename Scalar>
BigObject truncation(BigObject p_in, Int vertex, OptionSet options)
{
   BigObject p_out = truncation<Scalar>(p_in, scalar2set(vertex), options);
   p_out.set_description() << p_in.name() << " with vertex " << vertex << " truncated" << endl;
   return p_out;
}

template<typename Scalar>
BigObject truncation(BigObject p_in, const Array<Int>& verts, OptionSet options)
{
   Set<Int> trunc_vertices;
   for (const auto& vi : verts)
      trunc_vertices += vi;

   if (verts.size() != trunc_vertices.size())
      throw std::runtime_error("truncation: repeating vertex numbers in the list");
   
   return truncation<Scalar>(p_in, trunc_vertices, options);
}

UserFunctionTemplate4perl("# @category Producing a polytope from polytopes"
                          "# "
                          "# Cut off one or more vertices of a polyhedron."
                          "# "
                          "# The exact location of the cutting hyperplane(s) can be controlled by the"
                          "# option //cutoff//, a rational number between 0 and 1."
                          "# When //cutoff//=0, the hyperplane would go through the chosen vertex, thus cutting off nothing."
                          "# When //cutoff//=1, the hyperplane touches the nearest neighbor vertex of a polyhedron."
                          "# "
                          "# Alternatively, the option //no_coordinates// can be specified to produce a"
                          "# pure combinatorial description of the resulting polytope, which corresponds to"
                          "# the cutoff factor 1/2."
                          "# @param Polytope P"
                          "# @param Set<Int> trunc_vertices the vertex/vertices to be cut off;"
                          "#   A single vertex to be cut off is specified by its number."
                          "#   Several vertices can be passed in a Set or in an anonymous array of indices: [n1,n2,...]"
                          "#   Special keyword __All__ means that all vertices are to be cut off."
                          "# @option Scalar cutoff controls the exact location of the cutting hyperplane(s);"
                          "#   rational number between 0 and 1; default value: 1/2"
                          "# @option Bool no_coordinates produces a pure combinatorial description (in contrast to //cutoff//)"
                          "# @option Bool no_labels Do not copy [[VERTEX_LABELS]] from the original polytope. default: 0"
                          "#   New vertices get labels of the form 'LABEL1-LABEL2', where LABEL1 is the original label"
                          "#   of the truncated vertex, and LABEL2 is the original label of its neighbor."
                          "# @return Polytope"
                          "# @example To truncate the second vertex of the square at 1/4, try this:"
                          "# > $p = truncation(cube(2),2,cutoff=>1/4);"
                          "# > print $p->VERTICES;"
                          "# | 1 -1 -1"
                          "# | 1 1 -1"
                          "# | 1 1 1"
                          "# | 1 -1 1/2"
                          "# | 1 -1/2 1"
                          "# @author Kerstin Fritzsche (initial version)",
                          "truncation<Scalar>(Polytope<Scalar> * {cutoff=>undef, no_coordinates=>undef, no_labels=>0})");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
