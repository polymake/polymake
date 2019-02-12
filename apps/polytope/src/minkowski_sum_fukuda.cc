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
// This code was originally written by Silke Moeser in 2011.
// It was later modified by Benjamin Schroeter and others.

#include "polymake/client.h"
#include "polymake/Array.h"
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/ListMatrix.h"
#include "polymake/Graph.h"
#include "polymake/hash_set"
#include "polymake/list"
#include "polymake/polytope/solve_LP.h"

namespace polymake { namespace polytope {

template <typename E>
using vertex_list = hash_set<Vector<E>>;
template <typename E>
using matrix_list = Array<Matrix<E>>;
using graph_list = Array<Graph<Undirected>> ;

template <typename E>
Matrix<E> list2matrix(const vertex_list<E>& v)
{
   auto v_it = v.begin();
   return Matrix<E>(v.size(), v_it->dim(), v_it);
}

/*
 * returns optimal solution
 * called by Adj
 */
template <typename E>
Vector<E> solve_lp(const Matrix<E>& constraints, const Vector<E>& objective)
{
   const auto S = solve_LP(constraints, objective, true);
   if (S.status != LP_status::valid)
      throw std::runtime_error("minkowski_sum_fukuda: wrong LP");
   return S.solution;
}

/* computes the minkowski sum of the points listed in components (and rows of polytopes)
 *  v = v(1)+v(2)+..+(k) where v(j) = P_j(comp(j))
 *
 * returns the sumvector v
 * called by initial, local_search and addition
 */
template<typename E>
Vector<E> components2vector(const Array<int>& components, const matrix_list<E>& polytopes){
   Vector<E> result(polytopes[0].row(0).size());
   int i=0;
   for(Array<int>::const_iterator it=components.begin(); it!=components.end(); ++it, ++i)       {
      result+=polytopes[i].row(*it);
   }
   result[0]=1;
   return result;
}

/* computes a canonical vector in the relative interior of the normal cone N(v,P)
 * (can also return a vector [v,w], where w is a slightly perturbed variant of v)
 */
template <typename E>
Vector<E> canonical_vector(const int k, const Array<int>& components, const matrix_list<E>& polytopes, const graph_list& graphs) {
   ListMatrix<Vector<E>> c;  // constraints
   for (int j=0; j<k; ++j) {
      for (auto it=entire(graphs[j].adjacent_nodes(components[j])); !it.at_end(); ++it)
         c /= polytopes[j].row(*it)-polytopes[j].row(components[j]);
   }
   const E max_coef(accumulate(attach_operation(concat_rows(c), operations::abs_value()), operations::max()));

   // constraints of the form e_j(v_j,i)^T \lambda + \lambda_0 \leq 0
   const int dim=c.cols();
   c |= -ones_vector<E>(c.rows());

   // bounds for \lambda_i: -1\leq\lambda_i\leq 1
   const Matrix<E> b= ((ones_vector<E>(dim) | unit_matrix<E>(dim)) /
                       (ones_vector<E>(dim) | -unit_matrix<E>(dim)));

   const Matrix<E> constraints(c/b);
   const Vector<E> obj(unit_vector<E>(dim+1,dim));      // objective function
   const Vector<E> r = solve_lp(constraints, obj);

   const E d(r[r.size()-1]);    // the "minimal distance" of r to the bounding hyperplanes
   const Vector<E> x(r.slice(sequence(0, r.size()-1)));
   assert(x.size()>=2);

   Vector<E> eps(x.size());
   // as d is the minimal distance, we need to factor out the maximal
   // coefficient of the contraints times the dimension, to make sure
   // the perturbation is small enough.
   eps[1] = d/(2*dim*max_coef);
   for (int i=2; i<eps.size(); ++i)
      eps[i] = eps[i-1]/E(-i);

   return r.slice(sequence(0, r.size()-1)) | eps;
}

/* checks whether two Vectors are parallel
 *
 * returns boolean
 * called by local_search_compare and Adj
 */
template <typename E>
bool parallel_edges(const Vector<E>& e1, const Vector<E>& e2)
{
   const int dim = e1.dim();
   E q(0);              // quotient e2[j]/e1[j] -- should be constant
   int j = 1;
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

/* computes the normal Vector for the by [c,c_st] first intersectet hyperplane (part of Normalcone N(v,P))
 * direction in which f(v) lies
 *
 * returns the normalvector
 * called by local_search and local_search_compare
 */
template<typename E>
Vector<E> search_direction(const int k, const Vector<E>& c_st1, const Vector<E>& c_st2, const Array<int>& components, const matrix_list<E>& polytopes, const graph_list& graphs)
{
   // computes canonical vector for v
   const Vector<E> c_eps = canonical_vector(k, components, polytopes, graphs);
   const int size(c_eps.size()/2);
   Vector<E> c = c_eps.slice(sequence(0, size));
   const Vector<E> eps = c_eps.slice(sequence(size, size));
   const Vector<E> c_st = parallel_edges(c_st2, c) ? c_st1 : c_st2;
   E best_theta(2);                             // smallest theta value (so far)
   Vector<E> best_hyperplane;   // the hyperplane intersected first

   // goes through all (potential) neighbours of next
   int hyperplane_counter = 0;
   int restart_count = 0;
   for (int j=0; j<k; ++j) {
      for (auto it=entire(graphs[j].adjacent_nodes(components[j])); !it.at_end(); ++it) {
         const Vector<E> hyperplane = polytopes[j].row(*it)-polytopes[j].row(components[j]); //= e_j(next_j,i)
         // now compute theta such that c+theta(c_st-c) lies on the hyperplane
         const E d = hyperplane * (c-c_st);
         if (is_zero(d)) {
            continue;   // the current hyperplane is parallel to c-c_st -> no intersection
         }
         const E theta = (hyperplane*c) / d;
         if (0<=theta && 1>=theta){
            if (theta == best_theta && !parallel_edges(hyperplane,best_hyperplane)) {
               hyperplane_counter++;
            } else if (theta<best_theta) {
               hyperplane_counter=1;
               best_theta=theta;
               best_hyperplane=hyperplane;
            }
         }
      }
      if (j==k-1 && hyperplane_counter > 1) {
         restart_count++;
         j=-1;
         c = c - eps/(2*restart_count);
         c[restart_count%size] += eps[restart_count%size]/restart_count;
         best_theta = 2;
      }
   }
   return best_hyperplane;
}


template <class E>
Vector<E> local_search(const int k, const Vector<E>& c_st, const Vector<E>& c_st2, Array<int>& components, const matrix_list<E>& polytopes, const graph_list& graphs)
{
   //search normal Vector for the first intersected hyperplane (part of Normalcone)
   const Vector<E> hyperplane=search_direction(k, c_st, c_st2, components, polytopes, graphs);

   // now find the summands of f(v)
   for (int j=0; j<k; ++j) {
      for (auto it = entire(graphs[j].adjacent_nodes(components[j])); !it.at_end(); ++it) {
         const Vector<E> w (polytopes[j].row(*it) - polytopes[j].row(components[j]));
         if (parallel_edges(hyperplane, w)) {
            components[j] = *it;
            break;                                                      // e_j(v_j,i) is in the right direction, so no other edge in P_j could be
         }
      }
   }
   return components2vector(components, polytopes);
}

/* checks whether f(next)==v, quicker variant of localsearch
 *
 * subroutins: parallel_edges and search_hyperplane
 * called by addition
 * TODO: kann verbessert werden in dem die richtung von next-v genutzt wird um ein erstes theta vorzugeben
 */
template <typename E>
bool local_search_compare(const int k, const Vector<E>& v_st, const Vector<E>& c_st, const Vector<E>& c_st2, const Vector<E>& v, const Vector<E>& next, const Array<int>& next_components, const matrix_list<E>& polytopes, const graph_list& graphs)
{
   if (next==v_st) return 0;

   //search normal Vector for the first intersected hyperplane (part of Normalcone)
   const Vector<E> hyperplane = search_direction(k, c_st, c_st2, next_components, polytopes, graphs);
   // now v == f(next) iff it lies in the right direction
   return parallel_edges(hyperplane, Vector<E>(next-v));
}

/* searchs a adjacent edge of v in G(P) in direction e_s(v_s,r)
 *
 * returns 0 if there is no edge or the index (s,r) is not the smallest for this direction
 * subroutines: parallel_edges and solve_lp
 * called by addition
 */
template <typename E>
bool Adj(const int k, Array<int>& next_components, const Array<int>& components, const int s, const int r, const graph_list& graphs, matrix_list<E>& polytopes)
{
   const Vector<E> e_s = polytopes[s].row(r) - polytopes[s].row(components[s]); // =e_s(v_s,r)
   Vector<E> e_j;                      // = e_j(v_j,i)
   ListMatrix<Vector<E>> ineq_list;    // list of ineq. (3.6) e_j(v_j,i)^T lambda >=0
   ineq_list /= -e_s;                  // and e_s(v_s,r)^T lambda < 0
   next_components=components;
   for (int j=0; j<k; ++j) {
      for (auto it = entire(graphs[j].adjacent_nodes(components[j])); !it.at_end(); ++it) {
         e_j = polytopes[j].row(*it) - polytopes[j].row(components[j]);
         if (parallel_edges(e_s, e_j)) {
            // returns 0 if there is an index j smaller than s in same direction
            if (j<s) return 0;
            next_components[j]=*it;
         } else {
            ineq_list /= e_j;
         }
      }
   }
   // add another variable mu in the first inequality
   // (the ineq will be strict iff mu is positive)
   const Matrix<E> m( ineq_list | -unit_vector<E>(ineq_list.rows(), 0) );
   const int l=m.cols()-1;
   // one more ineq to bound lambda from above
   Vector<E> b = unit_vector<E>(m.cols(),0);
   b[l] = -1;
   const Matrix<E> d (m / b);
   // maximize lambda
   const Vector<E> obj = unit_vector<E>(m.cols(),l);
   const Vector<E> opt = solve_lp(d, obj);
   return !is_zero(opt[l]);
}

/* Reverse Search Algorithm according to Komei Fukuda
 * (see also MinkowskiAddition in 'From the Zonotope Construction to the Minkowski Addition of Convex Polytopes')
 *
 * subroutins: Adj, local_search, local_search_compare and components2vector
 * called by minkowski_sum_fukuda
 */
template <typename E>
vertex_list<E> addition(const int k, const Vector<E>& v_st, const Vector<E>& c_st, const Vector<E>& c_st2, Array<int>& components, const graph_list& graphs, matrix_list<E>& polytopes)
{
   vertex_list<E> vertices; //return value
   Vector<E> next;
   Array<int> next_components(k), u_components(k);
   Vector<E> v = v_st;
   int j=0;                     // iterates through all input-polytopes
   auto it=entire(graphs[j].adjacent_nodes(components[j]));      //iterates through all neighbors in P_j
   vertices.insert(v_st);

   while (j!=k or v!=v_st) {
      while (j<k) {
         while (!it.at_end()) {
            if (Adj(k, next_components, components, j, *it, graphs, polytopes)) {      // (r1)
               next=components2vector(next_components, polytopes);
               if (local_search_compare(k, v_st, c_st, c_st2, v, next, next_components, polytopes, graphs)){// (r2), f(next)==v
                  //reverse traverse:
                  components = next_components;
                  v = next;
                  j=0;
                  it=entire(graphs[j].adjacent_nodes(components[j]));
                  --it;
                  vertices.insert(v);
               }
            }
            ++it;
         }
         ++j;
         if (j!=k) {
            it=entire(graphs[j].adjacent_nodes(components[j]));
         }
      }
      if (v!=v_st) {
         //forward traverse:
         u_components=components;       //(f1)
         v=local_search(k, c_st, c_st2, components, polytopes, graphs); //v=f(v)
         //restore (j,it) s.t. Adj(v,(j,*it))=u:        //(f2)
         j=0;
         while (polytopes[j].row(u_components[j])==polytopes[j].row(components[j])) {
            ++j;
         }
         it=entire(graphs[j].adjacent_nodes(components[j]));
         while (*it!=u_components[j]) {
            ++it;
         }
         if (it.at_end()) {
            ++j;
            if(j!=k) {
               it=entire(graphs[j].adjacent_nodes(components[j]));
            }
         } else {
            ++it;
         }
      }
   }
   return vertices;
}

/* computes the maximal face of polytope
 * (the code is essentially borrowed from apps/polytope/src/pseudo_simplex.cc)
 */
template<typename E>
Set<int> find_max_face(const Matrix<E>& V, const Graph<Undirected>& G, const Vector<E>& objective) {
   NodeMap<Undirected, bool> visited(G,false);

   int current_vertex=0;

   E opt=objective * V[current_vertex];
   Set<int> optimal_face=scalar2set(current_vertex);
   visited[current_vertex]=true;

   bool better;

   do {
      better = false;
      // steepest ascent/descent
      for (auto v=entire(G.out_edges(current_vertex));  !v.at_end();  ++v) {
         const int neighbor=v.to_node();
         if (visited[neighbor])  // this neighbor can't be better
            continue;

         visited[neighbor]=true;
         const E value = objective * V[neighbor];
         if (value > opt) { // this one is better
            current_vertex = neighbor;
            opt = value;
            optimal_face = scalar2set(current_vertex);
            better = true;
         } else if (value==opt) { // also belongs to optimal_face
            optimal_face += neighbor;
         }
      }
   } while (better);

   std::list<int> optimal_vertices(optimal_face.begin(), optimal_face.end());

   while (!optimal_vertices.empty()) {
      current_vertex = optimal_vertices.front();
      optimal_vertices.pop_front();

      for (auto v=entire(G.out_edges(current_vertex));  !v.at_end();  ++v) {
         const int neighbor=v.to_node();
         if (!visited[neighbor]) {
            visited[neighbor]=true;
            if (is_zero(V(neighbor,0)) && objective * V[neighbor] == opt) {
               optimal_face += neighbor;
               optimal_vertices.push_back(neighbor);
            }
         }
      }
   }

   return optimal_face;
}

/* decides whether row a is larger than row b, compared by lexicographical order
 *
 * returns a or b
 * called by initial
 */
template <typename E>
int lex_max(const int a, const int b, const Matrix<E>& mat)
{
   return operations::cmp()(mat.row(a), mat.row(b)) >= pm::cmp_eq ? a : b;
}

/* initialize graphs, polytopes, components, v_st and c_st (c_st2)
 * using Array<perl::Object> summands (and k)
 *
 * and subroutines find_max_face, lex_max, components2vector and canonical_vector
 * called by minkowski_sum_fukuda
 */
template <typename E>
void initialize(const Array<perl::Object>& summands, const int k, graph_list& graphs,
                matrix_list<E>& polytopes, Array<int>& components, Vector<E>& v_st, Vector<E>& c_st, Vector<E>& c_st2)
{
   // initialize graphs and polytopes:
   int j = 0;
   for (const perl::Object& s : summands) {
      const Matrix<E> m=s.give("VERTICES");
      polytopes[j] = m;
      const Graph<Undirected> graph=s.give("GRAPH.ADJACENCY");
      graphs[j] = graph;
      ++j;
   }
   const int dim = polytopes[0].cols();

   // initialize components and v_st:
   Vector<E> obj = ones_vector<E>(polytopes[0].row(0).size()); // LP Vector;
   bool find_new_direction = false;
   for (int i = 0; i < k; ++i) {
      Set<int> max_face = find_max_face(polytopes[i], graphs[i], obj);  // solves LP over P_j
      if (max_face.size()>1) {
         if (find_new_direction) {
            for (int col = 1; col < dim; ++col) {
               if (obj[col] < 3001) {
                  obj[col] *= col;
               } else {
                  obj[col] -= 3000;
               }
            }
            i = 0;
            max_face = find_max_face(polytopes[0], graphs[0], obj);
            if (max_face.size() == 1)
               find_new_direction = false;
         } else {
            find_new_direction = true;
         }
         while (max_face.size() > 1) {
            auto SetIterator = max_face.begin();
            ++SetIterator;
            max_face.erase(lex_max(*(max_face.begin()),*SetIterator, polytopes[i])); //delete some, chosen by lex. order)
         }
      }
      components[i]=*(max_face.begin());        //to find v_st
   }
   v_st=components2vector(components, polytopes);


   //initialize c_st and c_st2 (canonical Vectors in N(P,v_st)):
   const Vector<E> c = canonical_vector(k, components, polytopes, graphs);
   const int size = c.size()/2;
   c_st = c.slice(sequence(0, size));
   c_st2= c_st + c.slice(sequence(size, size));
}

template <typename E>
Matrix<E> minkowski_sum_vertices_fukuda(const Array<perl::Object>& summands)
{
   const int k = summands.size();       // number of (input)polytopes
   Vector<E> v_st;      // Root Vertex v*
   Vector<E> c_st;      // Canonical Vector c*
   Vector<E> c_st2;     // alternative for c* (if c is parallel to c_st)
   Array<int> components(k);    // j-th entry will contain the number of a vertex in P_j  s.t. v = v(1)+v(2)+...+v(k) where v(j)= P_j(comp(j))
   graph_list graphs(k);                // stores all Graphs from the input Polytopes P_j
   matrix_list<E> polytopes(k);      // stores matrices s.t. the i-th entry is a discribtion of P_j by vertices

   initialize(summands, k, graphs, polytopes, components, v_st, c_st, c_st2);
   vertex_list<E> vertices = addition(k, v_st, c_st, c_st2, components, graphs, polytopes);
   // Output = listed coordinates of the vertices of P, (the first vertex will be v*)
   return list2matrix(vertices);
}

template <typename E>
perl::Object minkowski_sum_fukuda(const Array<perl::Object>& summands)
{
   const Matrix<E> vertices = minkowski_sum_vertices_fukuda<E>(summands);
   perl::Object p("Polytope", mlist<E>());
   p.take("VERTICES") << vertices;
   return p;
}

template <typename E>
Matrix<E> zonotope_vertices_fukuda(const Matrix<E>& Z, perl::OptionSet options)
{
   const int n = Z.rows();
   const int d = Z.cols();
   Array<perl::Object> summands(perl::ObjectType("Polytope", mlist<E>()), n);
   const bool centered_zonotope = options["centered_zonotope"];

   Graph<Undirected> G(2);
   G.edge(0,1);

   Vector<E> point, opposite = unit_vector<E>(d, 0);
   for (int i=0; i<n; ++i) {
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
                          "# Computes the ([[VERTICES]] of the) __Minkowski sum__ of a list of polytopes using the algorithm by Fukuda described in"
                          "#\t   Komei Fukuda, From the zonotope construction to the Minkowski addition of convex polytopes, J. Symbolic Comput., 38(4):1261-1272, 2004."
                          "# @param Array<Polytope<Scalar>> summands"
                          "# @return Polytope<Scalar>"
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
                          "# @param Matrix<Scalar> M"
                          "# @option Bool centered_zonotope default 1"
                          "# @return Matrix<E>"
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
