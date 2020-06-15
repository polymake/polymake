/* Copyright (c) 1997-2020
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
// This code was originally written by Silke Moeser in 2011.
// It was later modified by Benjamin Schroeter, Julian Pfeifle and others.

#include "polymake/client.h"
#include "polymake/Array.h"
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/ListMatrix.h"
#include "polymake/Graph.h"
#include "polymake/hash_set"
#include "polymake/hash_map"
#include "polymake/list"
#include "polymake/polytope/solve_LP.h"

namespace polymake { namespace polytope {

template <typename E>
using VertexList = hash_set<Vector<E>>;
template <typename E>
using MatrixList = Array<Matrix<E>>;
using GraphList = Array<Graph<Undirected>>;

template <typename E>
Matrix<E> list2matrix(const VertexList<E>& v)
{
   auto v_it = v.begin();
   return Matrix<E>(v.size(), v_it->dim(), v_it);
}

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

Vector<E> sum_of_vertices(const MatrixList<E>& vertices_of_polytope,
                          const Array<Int>& vertex_index_of_polytope) {
   Vector<E> result(vertices_of_polytope[0].row(0).size());
   Int i=0;
   for(auto it=vertex_index_of_polytope.begin(); it!=vertex_index_of_polytope.end(); ++it, ++i)       {
      result += vertices_of_polytope[i].row(*it);
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
canonical_vector(const Int k,
                 const Array<Int>& vertex_index_of_polytope,
                 const MatrixList<E>& vertices_of_polytope,
                 const GraphList& graphs) {
   ListMatrix<Vector<E>> c;  // constraints
   for (Int j=0; j<k; ++j) {
      for (auto it=entire(graphs[j].adjacent_nodes(vertex_index_of_polytope[j])); !it.at_end(); ++it)
         c /=
            vertices_of_polytope[j].row(vertex_index_of_polytope[j]) -
            vertices_of_polytope[j].row(*it);
   }
   const E max_coef(accumulate(attach_operation(concat_rows(c), operations::abs_value()), operations::max()));

   // constraints of the form e_j(v_j,i)^T \lambda + \lambda_0 \leq 0
   const Int dim(c.cols());
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

/* computes the normal vector for the hyperplane that is first intersected by the segment [c,canonical_of_root], first intersectet hyperplane (part of Normalcone N(v,P))
 * direction in which f(v) lies
 *
 * returns the normalvector
 * called by local_search and local_search_compare
 */
template<typename E>
Vector<E>
first_intersected_hyperplane(const Array<Int>& vertex_index_of_polytope,
                             const Int k,
                             const Vector<E>& canonical_of_root1,
                             const Vector<E>& canonical_of_root_alt,
                             const MatrixList<E>& vertices_of_polytope,
                             const GraphList& graphs)
{
   // computes canonical vector for v
   const auto c_and_eps = canonical_vector(k, vertex_index_of_polytope, vertices_of_polytope, graphs);
   Vector<E> c(c_and_eps.first);
   const Vector<E>& eps(c_and_eps.second);
   const Int dim(c.size());
   const Vector<E>& canonical_of_root = are_parallel(canonical_of_root_alt, c) ? canonical_of_root1 : canonical_of_root_alt;
   E best_theta(2);                             // smallest theta value (so far)
   Vector<E> first_intersected_hyperplane;

   // goes through all (potential) neighbours of next
   Int hyperplane_counter = 0;
   Int restart_count = 0;
   for (Int j=0; j<k; ++j) {
      for (auto it=entire(graphs[j].adjacent_nodes(vertex_index_of_polytope[j])); !it.at_end(); ++it) {
         const Vector<E> hyperplane (vertices_of_polytope[j].row(*it) -
                                     vertices_of_polytope[j].row(vertex_index_of_polytope[j])); //= e_j(next_j,i)
         // now compute theta such that c+theta(canonical_of_root-c) lies on the hyperplane
         const E d = hyperplane * (c-canonical_of_root);
         if (is_zero(d)) {
            continue;   // the current hyperplane is parallel to c-canonical_of_root -> no intersection
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


/* checks whether f(next)==v, quicker variant of localsearch
 *
 * subroutins: are_parallel and search_hyperplane
 * called by minkowski_addition
 * TODO: kann verbessert werden in dem die richtung von next-v genutzt wird um ein erstes theta vorzugeben
 */
template <typename E>
bool
is_f_of_next_equalto_v(const Vector<E>& next,
                       const Array<Int>& next_vertex_index_of_polytope,
                       const Vector<E>& v,
                       const Int k,
                       const Vector<E>& v_root,
                       const Vector<E>& canonical_of_root,
                       const Vector<E>& canonical_of_root_alt,
                       const MatrixList<E>& vertices_of_polytope,
                       const GraphList& graphs)
{
   if (next == v_root) {
      return false;
   }

   //search normal Vector for the first intersected hyperplane (part of Normalcone)
   const Vector<E> hyperplane = first_intersected_hyperplane(next_vertex_index_of_polytope, k,
                                                             canonical_of_root, canonical_of_root_alt, vertices_of_polytope, graphs);
   // now v == f(next) iff it lies in the right direction
   return are_parallel(hyperplane, Vector<E>(next-v));
}

/* searches an adjacent edge of v in G(P) in direction e_s(v_s,r).
 * 
 * This is the adjacency oracle in the paper. The translation is:
 * When the paper says 
 *    Adj(v,(s,r)) = v^hat,
 * the code says
 *    adjacency_oracle(...,vertex_index_of_polytope,...,s,r,...) = (true, adjacent_vertex_index_of_polytope).
 *
 * returns false if there is no edge or the index (s,r) is not the smallest for this direction
 * subroutines: are_parallel and valid_lp_solution
 * called by minkowski_addition
 */
template <typename E>
std::pair<bool, Array<Int>>
adjacency_oracle(const Int k,
                 const Array<Int>& vertex_index_of_polytope,
                 const Int s,
                 const Int r,
                 const GraphList& graphs,
                 const MatrixList<E>& vertices_of_polytope)
{
   const Vector<E> e_s(vertices_of_polytope[s].row(r) -
                       vertices_of_polytope[s].row(vertex_index_of_polytope[s])); // =e_s(v_s,r)
   Vector<E> e_j;                      // = e_j(v_j,i)
   ListMatrix<Vector<E>> ineq_list;    // list of ineq. (3.6) e_j(v_j,i)^T lambda >=0
   ineq_list /= -e_s;                  // and e_s(v_s,r)^T lambda < 0
   Array<Int> adjacent_vertex_index_of_polytope(vertex_index_of_polytope);
   for (Int j=0; j<k; ++j) {
      for (auto neighbor_it = entire(graphs[j].adjacent_nodes(vertex_index_of_polytope[j])); !neighbor_it.at_end(); ++neighbor_it) {
         e_j =
            vertices_of_polytope[j].row(*neighbor_it) -
            vertices_of_polytope[j].row(vertex_index_of_polytope[j]);
         if (are_parallel(e_s, e_j)) {
            // returns false if there is an index j smaller than s in same direction
            if (j<s)
               return std::make_pair(false, Array<Int>());
            adjacent_vertex_index_of_polytope[j] = *neighbor_it;
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


/* Reverse Search Algorithm according to Komei Fukuda
 * (see also MinkowskiAddition in 'From the Zonotope Construction to the Minkowski Addition of Convex Polytopes')
 
 The code as written in the paper is somewhat confusing, as it uses invalid iterators and undefined values of iterators.
 Conceptually, it first finds an initial vertex v = v_1 + ... + v_k of the Minkowski sum P via a generic objective function,
 then does a depth-first search on the implicitly given graph of P:
 for each neighbor next of v, given by the Adj function, check if f(next)=v, where f is the local search function.
 If yes, recursively do depth first search from next.

 */

template<typename E>
void
DFS(const Vector<E>& v,
    const Array<Int>& vertex_index_of_polytope,
    const Int k,
    const GraphList& graphs,
    const MatrixList<E>& vertices_of_polytope,
    const Vector<E>& v_root,
    const Vector<E>& canonical_of_root,
    const Vector<E>& canonical_of_root_alt,
          VertexList<E>& sum_vertices)
{
   bool Adj_oracle_succeeded;
   Array<Int> incoming_edge_index_of_polytope;
   for (Int j=0; j<k; ++j) {
      for (auto neighbor_it = entire(graphs[j].adjacent_nodes(vertex_index_of_polytope[j])); !neighbor_it.at_end(); ++neighbor_it) {
         std::tie(Adj_oracle_succeeded, incoming_edge_index_of_polytope) =
            adjacency_oracle(k, vertex_index_of_polytope, j, *neighbor_it, graphs, vertices_of_polytope);
         if (Adj_oracle_succeeded) {
            const Vector<E> incoming_edge_vector(sum_of_vertices(vertices_of_polytope, incoming_edge_index_of_polytope));
            if (is_f_of_next_equalto_v(incoming_edge_vector, incoming_edge_index_of_polytope, v,
                                       k, v_root, canonical_of_root, canonical_of_root_alt, vertices_of_polytope, graphs)) {
               sum_vertices.insert(incoming_edge_vector);

               DFS(incoming_edge_vector, incoming_edge_index_of_polytope,
                   k, graphs, vertices_of_polytope, v_root, canonical_of_root, canonical_of_root_alt, sum_vertices);
            }
         }
      }
   }
}

template <typename E>
VertexList<E>
minkowski_addition(const Int k,
                   const Array<Int>& vertex_index_of_polytope,
                   const GraphList& graphs,
                   const MatrixList<E>& vertices_of_polytope,
                   const Vector<E>& canonical_of_root,
                   const Vector<E>& canonical_of_root_alt)
{
   VertexList<E> sum_vertices;
   
   const Vector<E> v_root = sum_of_vertices(vertices_of_polytope, vertex_index_of_polytope);
   sum_vertices.insert(v_root);
   
   DFS(v_root, vertex_index_of_polytope, k, graphs, vertices_of_polytope, v_root, canonical_of_root, canonical_of_root_alt, sum_vertices);

   return sum_vertices;
}

/* 
 * computes the maximal vertex of a polytope with respect to an objective function, 
 * and check whether that vertex is unique
 * (the code is adapted from apps/polytope/src/pseudo_simplex.cc)
 */
template<typename E>
std::pair<bool,Int>
find_max_vertex(const Matrix<E>& V, const Graph<Undirected>& G, const Vector<E>& objective) {
   NodeMap<Undirected, bool> visited(G,false);
   Int current_vertex(0);
   E opt(objective * V[current_vertex]);
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
         const E value = objective * V[neighbor];
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

/* decides whether row a is larger than row b, compared by lexicographical order
 *
 * returns a or b
 * called by initial
 */
template <typename E>
Int lex_max(const Int a, const Int b, const Matrix<E>& mat)
{
   return operations::cmp()(mat.row(a), mat.row(b)) >= pm::cmp_eq ? a : b;
}

/* initialize graphs, polytopes, vertex_index_of_polytope, v_root and canonical_of_root (canonical_of_root_alt)
 * using Array<BigObject> summands (and k)
 *
 * and subroutines find_max_vertex, lex_max, sum_of_vertices and canonical_vector
 * called by minkowski_sum_fukuda
 */
template <typename E>
void initialize(const Array<BigObject>& summands,
                const Int k,
                      GraphList& graphs,
                      MatrixList<E>& vertices_of_polytope,
                      Array<Int>& vertex_index_of_polytope,
                      Vector<E>& canonical_of_root,
                      Vector<E>& canonical_of_root_alt)
{
   // initialize graphs and polytopes:
   Int j = 0;
   for (const BigObject& s : summands) {
      const Matrix<E> m=s.give("VERTICES");
      vertices_of_polytope[j] = m;
      const Graph<Undirected> graph=s.give("GRAPH.ADJACENCY");
      graphs[j] = graph;
      ++j;
   }
   const Int dim = vertices_of_polytope[0].cols();

   // initialize vertex_index_of_polytope and v_root:
   // we search for an objective function that only selects vertices in all polytopes
   Vector<E> obj = ones_vector<E>(vertices_of_polytope[0].row(0).size()); // LP Vector;
   bool obj_selects_only_vertices(true);
   do {
      obj_selects_only_vertices = true;
      for (Int i = 0; i < k; ++i) {
         const std::pair<bool,Int> unique_and_max_vertex = find_max_vertex(vertices_of_polytope[i], graphs[i], obj);  // solves LP over P_j
         obj_selects_only_vertices = unique_and_max_vertex.first;
         if (!obj_selects_only_vertices)
            break;
         vertex_index_of_polytope[i] = unique_and_max_vertex.second;
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
   const auto c_and_eps  = canonical_vector(k, vertex_index_of_polytope, vertices_of_polytope, graphs);
   canonical_of_root     = c_and_eps.first;
   canonical_of_root_alt = canonical_of_root + c_and_eps.second;
}

template <typename E>
Matrix<E> minkowski_sum_vertices_fukuda(const Array<BigObject>& summands)
{
   const Int k = summands.size();       // number of (input)polytopes
   Vector<E> canonical_of_root;      // Canonical Vector c*
   Vector<E> canonical_of_root_alt;     // alternative for c* (if c is parallel to canonical_of_root)
   Array<Int> vertex_index_of_polytope(k);    // j-th entry will contain the number of a vertex in P_j  s.t. v = v(1)+v(2)+...+v(k) where v(j)= P_j(comp(j))
   GraphList graphs(k);                // stores all Graphs from the input Polytopes P_j
   MatrixList<E> vertices_of_polytope(k);      // stores matrices s.t. the i-th entry is a discribtion of P_j by vertices

   initialize(summands, k, graphs, vertices_of_polytope, vertex_index_of_polytope, canonical_of_root, canonical_of_root_alt);
   const VertexList<E> vertices = minkowski_addition(k, vertex_index_of_polytope, graphs, vertices_of_polytope, canonical_of_root, canonical_of_root_alt);
   // Output = listed coordinates of the vertices of P
   return list2matrix(vertices);
}

template <typename E>
BigObject minkowski_sum_fukuda(const Array<BigObject>& summands)
{
   return BigObject("Polytope", mlist<E>(),
                    "VERTICES", minkowski_sum_vertices_fukuda<E>(summands));
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

   return minkowski_sum_vertices_fukuda<E>(summands);
}

UserFunctionTemplate4perl("# @category Producing a polytope from polytopes"
                          "# Computes the ([[Polytope::VERTICES]] of the) __Minkowski sum__ of a list of polytopes using the algorithm by Fukuda described in"
                          "#\t   Komei Fukuda, From the zonotope construction to the Minkowski addition of convex polytopes, J. Symbolic Comput., 38(4):1261-1272, 2004."
                          "# @param Array<Polytope> summands"
                          "# @return Polytope"
                          "# @example [nocompare] > $p = minkowski_sum_fukuda([cube(2),simplex(2),cross(2)]);"
                          "# > print $p->VERTICES;"
                          "# | 1 3 -1"
                          "# | 1 3 1"
                          "# | 1 -1 -2"
                          "# | 1 1 3"
                          "# | 1 -1 3"
                          "# | 1 2 -2"
                          "# | 1 -2 2"
                          "# | 1 -2 -1",
                          "minkowski_sum_fukuda<E>(Polytope<type_upgrade<E>> +)");

UserFunctionTemplate4perl("# @category Producing a polytope from scratch"
                          "# Create the vertices of a zonotope from a matrix whose rows are input points or vectors."
                          "# @param Matrix M"
                          "# @option Bool centered_zonotope default 1"
                          "# @return Matrix"
                          "# @example [nocompare]"
                          "# The following stores the vertices of a parallelogram with the origin as its vertex barycenter and prints them:"
                          "# > $M = new Matrix([[1,1,0],[1,1,1]]);"
                          "# > print zonotope_vertices_fukuda($M);"
                          "# | 1 0 -1/2"
                          "# | 1 0 1/2"
                          "# | 1 -1 -1/2"
                          "# | 1 1 1/2",
                          "zonotope_vertices_fukuda<E>(Matrix<E> { centered_zonotope => 1 })");

FunctionTemplate4perl("minkowski_sum_vertices_fukuda<E>(Polytope<type_upgrade<E>> +)");

} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
