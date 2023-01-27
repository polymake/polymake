/* Copyright (c) 1997-2023
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

// This code was originally written by Silke Moeser and Benjamin Schroeter in 2011.
// It was later modified by Julian Pfeifle and others.

#pragma once

#include "polymake/client.h"
#include "polymake/Array.h"
#include "polymake/Matrix.h"
#include "polymake/pair.h"
#include "polymake/Vector.h"
#include "polymake/ListMatrix.h"
#include "polymake/Graph.h"
#include "polymake/Map.h"
#include "polymake/polytope/solve_LP.h"
#include "polymake/ReverseSearch.h"

namespace polymake { namespace polytope {

namespace {

template<typename Node>
class Logger {
   using Scalar = typename Node::value_type;
   private:
      Int dim;
      Map<Vector<Scalar>, Array<Int>> labelledRays;

   public:

      Logger(Int d) : dim(d) {}

      void log_node(const Node& node){
         labelledRays[node.get_vertex()] = node.get_indices();
      }

      void print_tree() {}
      void tree_add_node(const Node& node) {}
      void tree_add_edge(const Node& n0, const Node& n1) {}
      void exit_node(const Node& node) {}

      std::pair<Matrix<Scalar>, Array<Array<Int>>> get_result() const {
         Matrix<Scalar> result_mat(labelledRays.size(), dim);
         Array<Array<Int>> result_arr(labelledRays.size());
         Int i = 0;
         for(const auto& ray : labelledRays){
            result_mat[i] = ray.first;
            result_arr[i] = ray.second;
            i++;
         }
         return std::pair<Matrix<Scalar>, Array<Array<Int>>>(result_mat, result_arr);
      }

};


template<typename Scalar>
struct AdjacencyOracle {
   private:
      Int k; // number of (input)polytopes 
      Array<Graph<Undirected>> graphs; // stores all Graphs from the input Polytopes P_j
      Array<Matrix<Scalar>> vertices_of_polytope;  // stores matrices s.t. the i-th entry is a discribtion of P_j by vertices
      Array<Int> root_indices; // j-th entry will contain the number of a vertex in P_j  s.t. v = v(1)+v(2)+...+v(k) where v(j)= P_j(comp(j))
      Vector<Scalar> canonical_of_root; // Canonical Vector c*
      Vector<Scalar> canonical_of_root_alt; // alternative for c* (if c is parallel to canonical_of_root)
      Int dim;

   public:

      AdjacencyOracle(const Array<BigObject>& summands):
         k(summands.size()), graphs(summands.size()), vertices_of_polytope(summands.size()),
         root_indices(summands.size())
      {
         // initialize graphs and polytopes:
         Int j = 0;
         for (const BigObject& s : summands) {
            const Matrix<Scalar> m=s.give("VERTICES");
            vertices_of_polytope[j] = m;
            const Graph<Undirected> graph=s.give("GRAPH.ADJACENCY");
            graphs[j] = graph;
            ++j;
         }
         dim = vertices_of_polytope[0].cols();

         // initialize root_indices;
         // we search for an objective function that only selects vertices in all polytopes
         Vector<Scalar> obj = ones_vector<Scalar>(vertices_of_polytope[0].row(0).size()); // LP Vector;
         bool obj_selects_only_vertices(true);
         do {
            obj_selects_only_vertices = true;
            for (Int i = 0; i < k; ++i) {
               const std::pair<bool,Int> unique_and_max_vertex = find_max_vertex(vertices_of_polytope[i], graphs[i], obj);  // solves LP over P_j
               obj_selects_only_vertices = unique_and_max_vertex.first;
               if (!obj_selects_only_vertices)
                  break;
               root_indices[i] = unique_and_max_vertex.second;
            }
            if (!obj_selects_only_vertices) {
               for (Int col = 1; col < dim; ++col) {
                  if (obj[col] < 3001) {
                     obj[col] *= col;
                  } else {
                     obj[col] -= 3000;
                  }
               }
            }
         } while (!obj_selects_only_vertices);

         //initialize canonical_of_root and canonical_of_root_alt (canonical Vectors in N(P,v_root)):
         const auto c_and_eps  = canonical_vector(root_indices);
         canonical_of_root     = c_and_eps.first;
         canonical_of_root_alt = canonical_of_root + c_and_eps.second;
      }

      Array<Int> get_root_indices() const {
         return root_indices;
      }

      Int get_dim() const {
         return dim;
      }

      std::pair<Vector<Scalar>,Vector<Scalar>> canonical_vector(const Array<Int>&) const;
      Vector<Scalar> sum_of_vertices(const Array<Int>& vertex_index_of_polytope) const;
      Vector<Scalar> first_intersected_hyperplane(const Array<Int>&) const;
      std::pair<bool, Array<Int>> verify_child(const Array<Int>&, const std::pair<Int, Int>&) const;
      Array<Int> local_search(const Array<Int>& vertex_index_of_polytope) const;


      Set<std::pair<Int,Int>> get_child_candidates_from_indices(const Array<Int>& indices) const {
         Set<std::pair<Int, Int>> result;
         for (Int j=0; j<k; ++j) {
            for (auto neighbor : graphs[j].adjacent_nodes(indices[j])){
               result += std::pair<Int, Int>(j, neighbor);
            }
         }
         return result;
      }


   private:
      /* 
       * computes the maximal vertex of a polytope with respect to an objective function, 
       * and check whether that vertex is unique
       * (the code is adapted from apps/polytope/src/pseudo_simplex.cc)
       */
      std::pair<bool,Int>
      find_max_vertex(const Matrix<Scalar>& V, const Graph<Undirected>& G, const Vector<Scalar>& objective) {
         NodeMap<Undirected, bool> visited(G,false);
         Int current_vertex(0);
         Scalar opt(objective * V[current_vertex]);
         Int optimal_vertex(current_vertex);
         visited[current_vertex] = true;

         bool
            have_improved(false),
            unique_optimum(true);

         do {
            have_improved = false;
            // steepest ascent/descent
            for (auto v=entire(G.out_edges(current_vertex));  !v.at_end();  ++v) {
               const Int neighbor(v.to_node());
               if (visited[neighbor])  // this neighbor can't be better
                  continue;

               visited[neighbor] = true;
               const Scalar value = objective * V[neighbor];
               if (value > opt) { // this one is better
                  current_vertex = neighbor;
                  opt = value;
                  optimal_vertex = current_vertex;
                  unique_optimum = true;
                  have_improved = true;
               } else if (value == opt) {
                  unique_optimum = false;
               }
            }
         } while (have_improved);

         return std::make_pair(unique_optimum, optimal_vertex);
      }
};


template <typename E>
Vector<E> valid_lp_solution(const Matrix<E>& constraints,
                            const Vector<E>& objective)
{
   const auto S = solve_LP(constraints, objective, true);
   if (S.status != LP_status::valid)
      throw std::runtime_error("minkowski_sum_fukuda: wrong LP");
   return S.solution;
}

/* 
 * return the sum of the vertices indexed by vertex_index_of_polytope:
 *   v = P_1[vi_1] + P_2[vi_2] + ... + P_k[vi_k]
 * where vi = vertex_index_of_polytope
 */
template<typename E>
Vector<E> AdjacencyOracle<E>::sum_of_vertices(const Array<Int>& indices) const {
   Vector<E> result(vertices_of_polytope[0].row(0).size());
   for(Int i=0; i<indices.size(); i++){
      result += vertices_of_polytope[i].row(indices[i]);
   }
   result[0] = 1;
   return result;
}

/* 
 * computes a canonical vector x in the relative interior of the outer normal cone N(v,P)
 * and returns it together with a perturbation vector eps.

   The outer normal cone is given by constraints of the form 
   ( neighbor - vertex ) lambda  <= -lambda_0
                        lambda_0 <= K
   and we wish to maximize lambda_0 with this property, along with the constraints 
   -1 <= lambda_i <= 1.

   Therefore (and notice the sign change in the first inequality below), we solve the linear program
  
   max lambda_0  s.t.

   ( vertex - neighbor ) lambda - lambda_0 >= 0
                              K - lambda_0 >= 0
                              1 + lambda_i >= 0
                              1 - lambda_i >= 0

   in terms of coordinates (const, lambda, lambda_0) the inequality matrix reads

   0 vertex-neighbor -1
   K       0         -1
   1    1             0
   1   -1             0
 */
template <typename E>
std::pair<Vector<E>,Vector<E>>
AdjacencyOracle<E>::canonical_vector(const Array<Int>& vertex_index_of_polytope) const {
   ListMatrix<Vector<E>> c;  // constraints
   for (Int j=0; j<k; ++j) {
      for (auto i :graphs[j].adjacent_nodes(vertex_index_of_polytope[j]))
         c /=
            vertices_of_polytope[j].row(vertex_index_of_polytope[j]) -
            vertices_of_polytope[j].row(i);
   }
   const E max_coef(accumulate(attach_operation(concat_rows(c), operations::abs_value()), operations::max()));

   // constraints of the form e_j(v_j,i)^T \lambda + \lambda_0 \leq 0
   c |= -ones_vector<E>(c.rows());

   // bounds for \lambda_i: -1\leq\lambda_i\leq 1
   // Because dim is the ambient dimension + 1,
   // this also includes inequalities -1 <= lambda_0 <= 1,
   // but that's OK for us; the '<= 1' part says that K=1,
   // and we don't care about the condition lambda_0 >= -1
   const Matrix<E> constraints(c/
                               (ones_vector<E>(dim) |  unit_matrix<E>(dim)) /
                               (ones_vector<E>(dim) | -unit_matrix<E>(dim)));
   const Vector<E> obj(unit_vector<E>(dim+1,dim));      // objective function
   const Vector<E> r = valid_lp_solution(constraints, obj);

   const E d(r[r.size()-1]);    // the "minimal distance" of r to the bounding hyperplanes
   if (is_zero(d)) {
      throw std::runtime_error("minkowski_sum_fukuda: unexpected zero distance to bounding hyperplane");
   }
   const Vector<E> x(r.slice(sequence(0, r.size()-1)));
   assert(x.size()>=2);

   Vector<E> eps(x.size());
   // as d is the minimal distance, we need to factor out the maximal
   // coefficient of the constraints times the dimension, to make sure
   // the perturbation is small enough.
   eps[1] = d/(2*dim*max_coef);
   for (Int i = 2; i < eps.size(); ++i)
      eps[i] = eps[i-1]/E(-i);

   return std::make_pair(x, eps);
}

/* checks whether two Vectors are parallel
 *
 * returns boolean
 * called by local_search_compare and Adj
 */
template <typename E>
bool are_parallel(const Vector<E>& e1, const Vector<E>& e2)
{
   const Int dim = e1.dim();
   assert(dim == e2.dim());
   E q(0);              // quotient e2[j]/e1[j] -- should be constant
   Int j = 1;
   for (; j < dim; ++j) {
      if (is_zero(e1[j])) {
         if (!is_zero(e2[j]))
            return false;
      } else {
         q = e2[j] / e1[j];
         break;
      }
   }
   while (++j < dim) {
      if (e1[j] * q != e2[j])
         return false;
   }
   return true;
}

/*
   This is the local search function f from the paper.
   For a vertex v given by vertex_index_of_polytope,
   it returns the array of indices that determines the vector f(v).
*/
template <class E>
Array<Int>
AdjacencyOracle<E>::local_search(const Array<Int>& vertex_index_of_polytope) const
{
   // cerr << "\n  local search from " << vertex_index_of_polytope << endl;
   //search normal vector for the first intersected hyperplane (part of Normalcone)
   const Vector<E> hyperplane = first_intersected_hyperplane(vertex_index_of_polytope);
   // cerr << "  best hyperplane: " << hyperplane << endl;
   // now find the summands of f(v)
   Array<Int> image_index_of_polytope(vertex_index_of_polytope);
   for (Int j=0; j<k; ++j) {
      for (auto it = entire(graphs[j].adjacent_nodes(vertex_index_of_polytope[j])); !it.at_end(); ++it) {
         const Vector<E> w (vertices_of_polytope[j].row(*it) - vertices_of_polytope[j].row(vertex_index_of_polytope[j]));
         if (are_parallel(hyperplane, w)) {
            image_index_of_polytope[j] = *it;
            // e_j(v_j,i) is in the right direction, so no other edge in P_j could be
         }
      }
   }
   // cerr << "  f(v) = " << image_index_of_polytope << endl;
   return image_index_of_polytope;
}

/* computes the normal vector for the hyperplane that is first intersected by the segment [c,canonical_of_root], first intersectet hyperplane (part of Normalcone N(v,P))
 * direction in which f(v) lies
 *
 * returns the normalvector
 * called by local_search and local_search_compare
 */
template<typename E>
Vector<E> AdjacencyOracle<E>::first_intersected_hyperplane(const Array<Int>& vertex_index_of_polytope) const {
   // computes canonical vector for v
   const auto c_and_eps = canonical_vector(vertex_index_of_polytope);
   Vector<E> c(c_and_eps.first);
   const Vector<E>& eps(c_and_eps.second);
   const Vector<E>& canonical_of_root_used = are_parallel(canonical_of_root_alt, c) ? canonical_of_root : canonical_of_root_alt;
   E best_theta(2);                             // smallest theta value (so far)
   Vector<E> first_intersected_hyperplane;

   // goes through all (potential) neighbours of next
   Int hyperplane_counter = 0;
   Int restart_count = 0;
   for (Int j=0; j<k; ++j) {
      for (auto i : graphs[j].adjacent_nodes(vertex_index_of_polytope[j])){
         const Vector<E> hyperplane (vertices_of_polytope[j].row(i) -
                                     vertices_of_polytope[j].row(vertex_index_of_polytope[j])); //= e_j(next_j,i)
         // now compute theta such that c+theta(canonical_of_root_used-c) lies on the hyperplane
         const E d = hyperplane * (c-canonical_of_root_used);
         if (is_zero(d)) {
            continue;   // the current hyperplane is parallel to c-canonical_of_root_used -> no intersection
         }
         const E theta = (hyperplane*c) / d;
         if (0 <= theta && theta <= 1){
            if (theta == best_theta &&
                !are_parallel(hyperplane,first_intersected_hyperplane)) {
               hyperplane_counter++;
            } else if (theta<best_theta) {
               hyperplane_counter=1;
               best_theta=theta;
               first_intersected_hyperplane=hyperplane;
            }
         }
      }
      if (j==k-1 && hyperplane_counter > 1) {
         restart_count++;
         j = -1;
         c = c - eps/(2*restart_count);
         c[restart_count % dim] += eps[restart_count % dim]/restart_count;
         best_theta = 2;
      }
   }
   return first_intersected_hyperplane;
}



/* searches an adjacent edge of v in G(P) in direction e_s(v_s,r).
 * 
 * This is the adjacency oracle in the paper. The translation is:
 
 *    Adj(v,(s,r)) = v^hat,
 * the code says
 *    verify_child(...,vertex_index_of_polytope,...,s,r,...) = (true, adjacent_vertex_index_of_polytope).
 *
 * returns false if there is no edge or the index (s,r) is not the smallest for this direction
 * subroutines: are_parallel and valid_lp_solution
 * called by minkowski_addition
 */
template <typename E>
std::pair<bool, Array<Int>>
AdjacencyOracle<E>::verify_child(
                 const Array<Int>& vertex_index_of_polytope,
                 const std::pair<Int, Int>& p) const
{
   Int s = p.first, r = p.second;
   const Vector<E> e_s(vertices_of_polytope[s].row(r) -
                       vertices_of_polytope[s].row(vertex_index_of_polytope[s])); // =e_s(v_s,r)
   Vector<E> e_j;                      // = e_j(v_j,i)
   ListMatrix<Vector<E>> ineq_list;    // list of ineq. (3.6) e_j(v_j,i)^T lambda >=0
   ineq_list /= -e_s;                  // and e_s(v_s,r)^T lambda < 0
   Array<Int> adjacent_vertex_index_of_polytope(vertex_index_of_polytope);
   for (Int j=0; j<k; ++j) {
      for (auto neighbor : graphs[j].adjacent_nodes(vertex_index_of_polytope[j])){
         e_j =
            vertices_of_polytope[j].row(neighbor) -
            vertices_of_polytope[j].row(vertex_index_of_polytope[j]);
         if (are_parallel(e_s, e_j)) {
            // returns false if there is an index j smaller than s in same direction
            if (j<s)
               return std::make_pair(false, Array<Int>());
            adjacent_vertex_index_of_polytope[j] = neighbor;
         } else {
            ineq_list /= e_j;
         }
      }
   }
   // add another variable mu in the first inequality
   // (the ineq will be strict iff mu is positive)
   const Matrix<E> m( ineq_list | -unit_vector<E>(ineq_list.rows(), 0) );
   const Int l(m.cols()-1);
   // one more ineq to bound lambda from above
   Vector<E> b = unit_vector<E>(m.cols(),0);
   b[l] = -1;
   const Matrix<E> d = m / b;
   // maximize lambda
   const Vector<E> obj = unit_vector<E>(m.cols(),l);
   const Vector<E> opt = valid_lp_solution(d, obj);
   const bool success = !is_zero(opt[l]);
   return std::make_pair(success, adjacent_vertex_index_of_polytope);
}



template<typename Scalar>
class Node {
   private:
      const AdjacencyOracle<Scalar>& AO;
      Array<Int> indices;
      Vector<Scalar> vertex;
      Array<std::pair<Int, Int>> childCandidates;

      // Workaround for not wasting too much information
      mutable Array<Int> jth_child_indices;
      mutable Int childj;

      void compute_childCandidates() {
         Set<std::pair<Int, Int>> result(AO.get_child_candidates_from_indices(indices));
         childCandidates = Array<std::pair<Int, Int>>(result);
      }

   public:
      using value_type = Scalar;

      Node& operator=(const Node& other){
         indices = other.indices;
         vertex = other.vertex;
         childCandidates = other.childCandidates;
         jth_child_indices = other.jth_child_indices;
         childj = other.childj;
         return *this;
      }
      
      Node(const AdjacencyOracle<Scalar>& AO_in): AO(AO_in), indices(AO_in.get_root_indices()){
         childj = -1;
         vertex = AO.sum_of_vertices(indices);
         compute_childCandidates();
      }

      Node(const AdjacencyOracle<Scalar>& AO_in, Array<Int> i): AO(AO_in), indices(i){
         childj = -1;
         vertex = AO.sum_of_vertices(indices);
         compute_childCandidates();
      }

      bool has_jth_child(Int j) const {
         if(j >= childCandidates.size()){
            return false;
            // std::cout << j << " delta: " << get_Delta() << std::endl;
            // throw std::runtime_error("Too high index!");
         }
         childj = j;
         bool result = false;
         std::tie(result, jth_child_indices) = AO.verify_child(indices, childCandidates[j]);
         return result;
      }

      Node get_jth_child(Int j) const {
         if(j != childj){
            if(!has_jth_child(j)){
               throw std::runtime_error("Neighbor calculation failed!");
            }
         }
         return Node(AO, jth_child_indices);
      }

      bool has_predecessor(const Node& node) const {
         // we need to avoid running into the root here as the local search
         // function is not well defined in that case
         if (indices == AO.get_root_indices())
            return false;
         const auto tmp = AO.local_search(indices);
         return node.indices == tmp;
      }

      Int get_Delta() const {
         return childCandidates.size();
      }

      const Vector<Scalar>& get_vertex() const {
         return vertex;
      }

      const Array<Int>& get_indices() const {
         return indices;
      }

      Node get_predecessor(Int& i) const {
         Node result(AO, AO.local_search(indices));
         for(i=0; i<result.get_Delta(); i++){
            if(result.has_jth_child(i)){
               if(indices == result.get_jth_child(i).indices){
                  return result;
               }
            }
         }
         throw std::runtime_error("No predecessor!");
      }

};


template <typename E>
// std::pair<VertexList<E>,Labels<E>>
std::pair<Matrix<E>, Array<Array<Int>>>
minkowski_addition(const AdjacencyOracle<E>& AO)
{
   Logger<Node<E>> logger(AO.get_dim());

   Node<E> initial(AO, AO.get_root_indices());
   ReverseSearchTemplate<Node<E>, Logger<Node<E>>, false> RST(logger);
   RST.auto_reverse_search(initial);
   return logger.get_result();
}


} // end namespace

template <typename E>
std::pair<Matrix<E>,Array<Array<Int>>> minkowski_sum_vertices_fukuda(const Array<BigObject>& summands)
{
   AdjacencyOracle<E> AO(summands);
   return minkowski_addition(AO);
}

template <typename E>
BigObject minkowski_sum_fukuda(const Array<BigObject>& summands)
{
	Matrix<E> V; //const
   	Array<Array<Int>> L; //const
     	std::tie(V, L) = minkowski_sum_vertices_fukuda<E>(summands);


	BigObject p = BigObject("Polytope", mlist<E>(),"VERTICES", V);
	p.attach("Minkowski_Labels") << L;

	return p;
}

template <typename E>
Matrix<E> zonotope_vertices_fukuda(const Matrix<E>& Z, OptionSet options)
{
   const Int n = Z.rows();
   const Int d = Z.cols();
   Array<BigObject> summands(BigObjectType("Polytope", mlist<E>()), n);
   const bool centered_zonotope = options["centered_zonotope"];

   Graph<Undirected> G(2);
   G.edge(0,1);

   Vector<E> point, opposite = unit_vector<E>(d, 0);
   for (Int i = 0; i < n; ++i) {
      point = Z.row(i);

      if (centered_zonotope) {
         point[0] *= 2;  // make the segment half as long to compensate for the addition of opposite
         opposite = -point;
         opposite[0].negate();
      }

      // take care of the origin
      if (is_zero(point.slice(range_from(1)))) {
         summands[i].take("VERTICES") << Matrix<E>(vector2row(point));
         summands[i].take("GRAPH.ADJACENCY") << Graph<Undirected>(1);
      } else {
         summands[i].take("VERTICES") << Matrix<E>(vector2row(point)/opposite);
         summands[i].take("GRAPH.ADJACENCY") << G;
      }
   }

   return minkowski_sum_vertices_fukuda<E>(summands).first;
}

} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
