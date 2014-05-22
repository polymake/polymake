/* Copyright (c) 1997-2014
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

#include "polymake/polytope/minkowski_sum_fukuda.h"

namespace polymake { namespace polytope {

namespace {
   // the following template typedefs are deferred until c++11's "using"
   
   // template<typename E> typedef hash_set<Vector<E>> vertex_list;
   // template<typename E> typedef std::vector<Vector<E>> clist; 
   // template<typename E> typedef Array<Matrix<E>> matrix_list;

   typedef Array<Graph<Undirected> > graph_list;
}

/* converts a list to a matrix
 *
 */
template<typename E>
Matrix<E> list2matrix(const hash_set<Vector<E> >& v) {
   const int dim=(v.begin())->size();
   Matrix<E> A(v.size(),dim);

   int i=0;
   for (typename hash_set<Vector<E> >::const_iterator it=v.begin(); it!=v.end(); ++it, ++i) {
      A.row(i)=*it;
   }
   return A;
}

/* converts a list to a matrix
 *
 */
template<typename E>
Matrix<E> list2matrix(const typename std::vector<Vector<E> >& v) {
   const int dim=(v.begin())->size();
   Matrix<E> A(v.size(),dim);

   int i=0;
   for (typename std::vector<Vector<E> >::const_iterator it=v.begin(); it!=v.end(); ++it, ++i) {
      A.row(i)=*it;
   }
   return A;
}


/* LP solver uses cdd for Rational and Integer, and TOSimplex for other types
 *
 * returns optimal solution
 * called by Adj
 */
template<typename E>
Vector<E> solve_lp(const Matrix<E>& constraints, const Vector<E>& objective) {
   typedef typename choose_solver<E>::solver Solver;
   Solver solver;
   Matrix<E> eqs;
   typename Solver::lp_solution S=solver.solve_lp(constraints, eqs, objective, 1);
   return S.second;
}

/* computes the minkowski sum of the points listed in components (and rows of polytopes)
 *  v = v(1)+v(2)+..+(k) where v(j) = P_j(comp(j))
 *
 * returns the sumvector v
 * called by initial, local_search and addition
 */
template<typename E>
Vector<E> components2vector(const Array<int>& components, const Array<Matrix<E> >& polytopes){
   Vector<E> result(polytopes[0].row(0).size());
   int i=0;
   for(Array<int>::const_iterator it=components.begin(); it!=components.end(); ++it, ++i)	{
      result+=polytopes[i].row(*it);
   }
   result[0]=1;
   return result;
}

/* computes a canonical vector in the relative interior of the normal cone N(v,P)
 * (can also return a vector [v,w], where w is a ...)
 *
 * TODO: comment
 */
template<typename E>
Vector<E> canonical_vector(const int k, const Array<int>& components, const Array<Matrix<E> >& polytopes, const graph_list& graphs, bool setc_st=false) {
   typename std::vector<Vector<E> > c;	// constraints
   for (int j=0; j<k; ++j) 
      for (Entire<Graph<>::adjacent_node_list>::const_iterator it=entire(graphs[j].adjacent_nodes(components[j])); !it.at_end(); ++it) 
         c.push_back(polytopes[j].row(*it)-polytopes[j].row(components[j]));

   // constraints of the form e_j(v_j,i)^T \lambda + \lambda_0 \leq 0
   const Matrix<E> c1=list2matrix(c);
   const int dim=c1.cols();
   const Vector<E> c3=ones_vector<E>(c.size());
   const Matrix<E> c2 = (c1 | -c3);

   // bounds for \lambda_i: -1\leq\lambda_i\leq 1
   const Vector<E> b1(ones_vector<E>(dim));
   const Matrix<E> b2(unit_matrix<E>(dim));
   const Matrix<E> b= ((b1 | b2) / (b1 | -b2));

   const Matrix<E> constraints(c2/b);
   const Vector<E> obj(unit_vector<E>(dim+1,dim));	// objective function
   const Vector<E> r = solve_lp(constraints, obj);

   if (!setc_st)  
      return r.slice(0,r.size()-1);

   const E d(r[r.size()-1]);	// the "minimal distance" of r to the bounding hyperplanes
   const Vector<E> x(r.slice(0,r.size()-1));
   assert(x.size()>=2);

   Vector<E> eps(x.size());
   eps[1] = d/2;
   for (int i=2; i<eps.size(); ++i)
      eps[i] = eps[i-1]/E(-i);

   return r.slice(0,r.size()-1) | r.slice(0,r.size()-1) + eps;
}

/* checks whether two Vectors are parallel
 *
 * returns boolean
 * called by local_search_compare and Adj
 */
template<typename E>
bool parallel_edges(const Vector<E>& e1, const Vector<E>& e2) {
   const int dim=e1.size();
   int j=1;
   bool quotient_found=0;
   E q(0);		// quotient e2[j]/e1[j] -- should be constant
   while (!quotient_found) {
      if (is_zero(e1[j])) {
         if (e2[j].non_zero())
            return 0;
      }
      else {
         q=e2[j]/e1[j];
         quotient_found=1;
      }
      ++j;

   }
   for (j=1; j<dim; ++j) {
      if ((e1[j]) * q != e2[j])
         return 0;
   }
   return 1;
}

/* computes the normal Vector for the by [c,c_st] first intersectet hyperplane (part of Normalcone N(v,P))
 * direction in which f(v) lies
 *
 * returns the normalvector
 * called by local_search and local_search_compare
 */
template<typename E>
Vector<E> search_direction(const int k, const Vector<E>& c_st1, const Vector<E>& c_st2, const Array<int>& components, const Array<Matrix<E> >& polytopes, const graph_list& graphs) {
   // computes canonical vector for v
   const Vector<E> c = canonical_vector(k, components, polytopes, graphs);
   const Vector<E> c_st = parallel_edges(c_st1, c) ? c_st2 : c_st1;   
   E best_theta(2);				// smallest theta value (so far)
   Vector<E> best_hyperplane;	// the hyperplane intersected first

   // goes through all (potential) neighbours of next
   for (int j=0; j<k; ++j) {
      for (Entire<Graph<>::adjacent_node_list>::const_iterator it=entire(graphs[j].adjacent_nodes(components[j])); !it.at_end(); ++it) {
         const Vector<E> hyperplane = polytopes[j].row(*it)-polytopes[j].row(components[j]); //= e_j(next_j,i)
         // now compute theta such that c+theta(c_st-c) lies on the hyperplane
         const E d = hyperplane * (c-c_st);
         if (is_zero(d)) {
            continue;	// the current hyperplane is parallel to c-c_st -> no intersection
         }
         const E theta = (hyperplane*c) / d;
         if (0<=theta && 1>=theta && theta<best_theta) {
            best_theta=theta;
            best_hyperplane=hyperplane;
         }
      }
   }
   return best_hyperplane;
}

/*
 * TODO: comment
 */
template <class E>
Vector<E> local_search(const int k, const Vector<E>& c_st, const Vector<E>& c_st2, Array<int>& components, const Array<Matrix<E> >& polytopes, const graph_list& graphs) 
{
   //search normal Vector for the first intersected hyperplane (part of Normalcone)
   const Vector<E> hyperplane=search_direction(k, c_st, c_st2, components, polytopes, graphs);

   // now find the summands of f(v)
   for (int j=0; j<k; ++j) {
      for (Entire<Graph<>::adjacent_node_list>::const_iterator it=entire(graphs[j].adjacent_nodes(components[j])); !it.at_end(); ++it) {
         const Vector<E> w (polytopes[j].row(*it) - polytopes[j].row(components[j]));
         if (parallel_edges(hyperplane, w)) {
            components[j] = *it;
            break;							// e_j(v_j,i) is in the right direction, so no other edge in P_j could be
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
template<typename E>
bool local_search_compare(const int k, const Vector<E>& v_st, const Vector<E>& c_st, const Vector<E>& c_st2, const Vector<E>& v, const Vector<E>& next, const Array<int>& next_components, const Array<Matrix<E> >& polytopes, const graph_list& graphs) 
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
 * subroutins: parallel_edges, list2matrix and solve_lp
 * called by addition
 */
template<typename E>
bool Adj(const int k, Array<int>& next_components, const Array<int>& components, const int s, const int r, const graph_list& graphs, Array<Matrix<E> >& polytopes)
{
   const Vector<E> e_s = polytopes[s].row(r) - polytopes[s].row(components[s]); // =e_s(v_s,r)
   Vector<E> e_j;	           // = e_j(v_j,i)
   std::vector<Vector<E> > ineq_list;	           // list of ineq. (3.6) e_j(v_j,i)^T lambda >=0
   ineq_list.push_back(-e_s);	   // and e_s(v_s,r)^T lambda < 0
   next_components=components;
   for(int j=0;j<k;++j) {
      for (Entire<Graph<>::adjacent_node_list>::const_iterator it = entire(graphs[j].adjacent_nodes(components[j])); !it.at_end(); ++it) {
         e_j = polytopes[j].row(*it) - polytopes[j].row(components[j]);
         if (parallel_edges(e_s, e_j)) {
            // returns 0 if there is an index j smaller than s in same direction
            if (j<s) return 0;
            next_components[j]=*it;
         } else {
            ineq_list.push_back(e_j);
         }
      }
   }
   // add another variable mu in the first inequality
   // (the ineq will be strict iff mu is positive)
   const Matrix<E> m = ( (list2matrix(ineq_list) ) | -unit_vector<E>(ineq_list.size(), 0) );
   const int l=m.cols()-1;
   // one more ineq to bound lambda from above
   Vector<E> b = unit_vector<E>(m.cols(),0);
   b[l] = -1;
   const Matrix<E> d (m / b);
   // maximize lambda
   const Vector<E> obj = unit_vector<E>(m.cols(),l);
   const Vector<E> opt = solve_lp(d, obj);
   return opt[l].non_zero();
}

/* Reverse Search Algorithm according to Komei Fukuda
 * (see also MinkowskiAddition in 'From the Zonotope Construction to the Minkowski Addition of Convex Polytopes')
 *
 * subroutins: Adj, local_search, local_search_compare and components2vector
 * called by minkowski_sum_fukuda
 */
template<typename E>
void addition(const int k, const Vector<E>& v_st, const Vector<E>& c_st, const Vector<E>& c_st2, hash_set<Vector<E> >& vertices, Array<int>& components, const graph_list& graphs, Array<Matrix<E> >& polytopes){
   Vector<E> next;
   Array<int> next_components(k), u_components(k);
   Vector<E> v = v_st;
   int j=0;  			// iterates through all input-polytopes
   Entire<Graph<>::adjacent_node_list>::const_iterator it=entire(graphs[j].adjacent_nodes(components[j]));	//iterates through all neighbors in P_j
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
         u_components=components;	//(f1)
         v=local_search(k, c_st, c_st2, components, polytopes, graphs);	//v=f(v)
         //restore (j,it) s.t. Adj(v,(j,*it))=u:	//(f2)
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
}

/* computes the maximal face of polytope
 * (the code is essentially borrowed from apps/polytope/src/pseudo_simplex.cc)
 *
 * TODO: comment
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
      for (Entire<Graph<>::out_edge_list>::const_iterator v=entire(G.out_edges(current_vertex));  !v.at_end();  ++v) {
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

      for (Entire<Graph<>::out_edge_list>::const_iterator v=entire(G.out_edges(current_vertex));  !v.at_end();  ++v) {
         const int neighbor=v.to_node();
         if (!visited[neighbor]) {
            visited[neighbor]=true;
            if (V(neighbor,0).non_zero() && objective * V[neighbor] == opt) {
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
template<typename E>
int lex_max(const int a, const int b, const Matrix<E>& mat){
   const Vector<E> v = mat.row(a)-mat.row(b);
   for (typename Vector<E>::const_iterator it=v.begin(); it!=v.end(); ++it) {
      if (*it>0)
         return a;
      if (*it<0)
         return b;
   }
   assert(0);
   return a; //unreachable
}

/* initialize graphs, polytopes, components, v_st and c_st (c_st2)
 * using Array<perl::Object> summands (and k)
 *
 * and subroutines find_max_face, lex_max, components2vector and canonical_vector
 * called by minkowski_sum_fukuda
 */
template<typename E>
void initialize(const Array<perl::Object>& summands, const int k, graph_list& graphs, Array<Matrix<E> >& polytopes, Array<int>& components, Vector<E>& v_st, Vector<E>& c_st, Vector<E>& c_st2)
{
   //initialize graphs and polytopes:
   int j=0;
   for (Array<perl::Object>::const_iterator it=summands.begin(); it!=summands.end(); ++it, ++j) {
      const Matrix<E> m=it->give("VERTICES");
      polytopes[j]=m;
      const Graph<Undirected> graph=it->give("GRAPH.ADJACENCY");
      graphs[j]=graph;
   }

   //initialize components and v_st:
   const Vector<E> obj = ones_vector<E>(polytopes[0].row(0).size()); //LP Vector; TODO: good direction
   for (int i=0; i<k; ++i) {
      Set<int> max_face = find_max_face(polytopes[i], graphs[i], obj);	//solves LP over P_j
      while (max_face.size()>1) {
         Set<int>::iterator SetIterator=max_face.begin();
         ++SetIterator;
         max_face.erase(lex_max(*(max_face.begin()),*SetIterator, polytopes[i])); //delete some, chosen by lex. order)
      }
      components[i]=*(max_face.begin());	//to find v_st
   }
   v_st=components2vector(components, polytopes);

   //initialize c_st and c_st2 (canonical Vectors in N(P,v_st)):
   const Vector<E> c = canonical_vector(k, components, polytopes, graphs, true);
   const int size = c.size()/2;
   c_st = c.slice(0,size);
   c_st2= c.slice(size,size);
}

template <typename E>
Matrix<E> minkowski_sum_vertices_fukuda(const Array<perl::Object>& summands)
{
   const int k = summands.size();	// number of (input)polytopes
   Vector<E> v_st;  	// Root Vertex v*
   Vector<E> c_st;  	// Canonical Vector c*
   Vector<E> c_st2; 	// alternative for c* (if c is parallel to c_st)
   hash_set<Vector<E> > vertices;		// Output = listed coordinates of the vertices of P, (the first vertex will be v*)
   Array<int> components(k);	// j-th entry will contain the number of a vertex in P_j  s.t. v = v(1)+v(2)+...+v(k) where v(j)= P_j(comp(j))
   graph_list graphs(k);		// stores all Graphs from the input Polytopes P_j
   Array<Matrix<E> > polytopes(k);	// stores matrices s.t. the i-th entry is a discribtion of P_j by vertices

   initialize(summands, k, graphs, polytopes, components, v_st, c_st, c_st2);
   addition(k, v_st, c_st, c_st2, vertices, components, graphs, polytopes);
   return list2matrix(vertices);
}

template <typename E>
perl::Object minkowski_sum_fukuda(const Array<perl::Object>& summands)
{
   const Matrix<E> vertices = minkowski_sum_vertices_fukuda<E>(summands);  
   perl::Object p(perl::ObjectType::construct<E>("Polytope"));
   p.take("VERTICES") << vertices;
   return p;
}

template <typename E>
Matrix<E> zonotope_vertices_fukuda(const Matrix<E>& Z, perl::OptionSet options)
{
   const int 
      n = Z.rows(),
      d = Z.cols();
   Array<perl::Object> summands(n);
   const bool centered_zonotope = options["centered_zonotope"];

   Graph<Undirected> G(2);
   G.edge(0,1);

   Vector<E> 
      point, 
      opposite = unit_vector<E>(d, 0);
   for (int i=0; i<n; ++i) {
      point = Z.row(i);
      if (centered_zonotope) {
         point[0] *= 2;  // make the segment half as long to compensate for the addition of opposite
         opposite = -point;
         opposite[0].negate();
      } 

      perl::Object summand(perl::ObjectType::construct<E>("Polytope"));
      summand.take("VERTICES") << Matrix<E>(point/opposite);
      summand.take("GRAPH.ADJACENCY") << G;
      summands[i] = summand;
   }

   return minkowski_sum_vertices_fukuda<E>(summands);
}

UserFunctionTemplate4perl("# @category Producing a polytope from polytopes", "minkowski_sum_fukuda<E>(Polytope<E> +)");

UserFunctionTemplate4perl("# @category Producing a polytope from scratch", "zonotope_vertices_fukuda<E>(Matrix<E> { centered_zonotope => 1 })");

}}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
