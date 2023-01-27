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

/* This file models reverse search on a simple polytope. There are some
 * implicit assumptions that have not been documented before: For example, it
 * is assumed that at every vertex there are exactly d hyperplanes assuming
 * equality. An edge is then found by leaving out one of the hyperplanes. This
 * means we cannot have redundant hyperplanes going through vertices, thus we
 * need to assume that we have facets.
 *
 * This reverse search does not go back to the root of the tree, rather it
 * assumes the [[ONE_VERTEX]] of the polytope is the root. It will probably
 * fail if the objective given does not select this vertex.
 */

#include "polymake/client.h"
#include "polymake/Graph.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/linalg.h"
#include "polymake/hash_set"
#include "polymake/hash_map"
#include "polymake/list"
#include "polymake/ReverseSearch.h"

namespace polymake { namespace polytope {

namespace reverse_search_simple_polytope {

template<typename Scalar>
const Vector<Scalar> normalize_leading_1(const Vector<Scalar> &v) {
   Int i=1;
   while (v[i]==0) ++i;
   return Vector<Scalar>(v/v[i]);
}


template<typename Scalar>
class RayLogger {

   private:
      Int n_rays;
      hash_set<Vector<Scalar> > rays;

   public:

      RayLogger() : n_rays(0) {}

      void log_ray(const Vector<Scalar>& direction){
         if ((rays.insert(normalize_leading_1(direction))).second) ++n_rays;
      }

      Int size() {
         return n_rays;
      }

      hash_set<Vector<Scalar>>& get_rays() {
         return rays;
      }

};


// Node
template<typename Scalar>
class Node {
   private:
      const Matrix<Scalar>& inequalities, equations;
      const Vector<Scalar>& objective;
      Array<Int> basis, predecessorBasis;
      Matrix<Scalar> A;
      Vector<Scalar> costs, vertex, b;
      bool isRay;
      RayLogger<Scalar>& logger;
      Int dim;


   public:
      // FIXME 
      // Make constructor using min_vertex?

      Node(const Array<Int>& startBasis, const Vector<Scalar>& objective_in, const Matrix<Scalar>& ineq_in, const Matrix<Scalar>& eq_in, RayLogger<Scalar>& sl, Int d):
         inequalities(ineq_in),
         equations(eq_in),
         objective(objective_in),
         basis(startBasis),
         isRay(false),
         logger(sl), dim(d)
   {
      std::sort(basis.begin(), basis.end());
      if(basis.size() > 0){
         A = inequalities.minor(basis, All)/equations;
      } else {
         A = equations;
      }
      b = A.col(0);
      costs=lin_solve(T(-A.minor(All, range_from(1))), objective);
      vertex=null_space(A).row(0);
      Scalar t = 1/vertex[0];
      vertex *= t;
   }

      Node& operator=(const Node& rsn) {
         basis = rsn.basis;
         vertex = rsn.vertex;
         costs = rsn.costs;
         A = rsn.A;
         b = rsn.b;
         isRay = rsn.isRay;
         dim = rsn.dim;
         return *this;
      }

      Node(const Node& rsn):
         inequalities(rsn.inequalities),
         equations(rsn.equations),
         objective(rsn.objective),
         basis(rsn.basis), 
         A(rsn.A),
         costs(rsn.costs), 
         vertex(rsn.vertex),
         b(rsn.b), isRay(rsn.isRay),
         logger(rsn.logger), dim(rsn.dim) {}

      void step_in_jth_direction(const Int j){
         Vector<Scalar> direction = get_jth_direction(j);
         Int inequalityIndex=0;
         isRay = true;
         Scalar lambda(get_direction_factor(isRay, inequalityIndex, direction));
         if (!isRay && lambda==0) throw std::runtime_error("Inequalities not in general position.");
         if(!isRay){
            basis[j] = inequalityIndex;
            std::sort(basis.begin(), basis.end());
            A = inequalities.minor(basis, All)/equations;
            b = A.col(0);
            costs = lin_solve(T(-A.minor(All, range_from(1))),objective);
            vertex = vertex + lambda*direction;
         }
      }

      const Array<Int>& get_basis() const{
         return basis;
      }

      const Vector<Scalar>& get_costs() const {
         return costs;
      }

      const Vector<Scalar>& get_vertex() const {
         return vertex;
      }

      Vector<Scalar> get_jth_direction(Int j) const {
         if(j > A.rows() || j < 0){
            cout << "j: " << j << " something went wrong" << endl;
         }
         Matrix<Scalar> tmp(A);
         tmp.col(0) = -unit_vector<Scalar>(tmp.rows(), j);
         Vector<Scalar> v = null_space(tmp).row(0);
         Scalar t = 1/v[0];
         v *= t;
         v[0] = 0;
         return v;
      }

      Int get_predecessor_index() const {
         Int i=0;
         while (costs[i]<=0) ++i;
         return i;
      }

      Node get_jth_child(Int j) const {
         Node result(*this);
         result.step_in_jth_direction(j);
         return result;
      }

      Scalar get_direction_factor(bool& ray, Int& inequalityIndex, const Vector<Scalar>& direction) const {
         Scalar lambda;
         Vector<Scalar> directionValues=-inequalities*direction;
         Vector<Scalar> vertexValues=inequalities*vertex;
         for (Int k=0;k<inequalities.rows();++k){
            if (directionValues[k]>0) {
               if (ray) {
                  ray=false;
                  lambda=vertexValues[k]/directionValues[k];
                  inequalityIndex=k;
               } else {
                  const Scalar lambda_new=vertexValues[k]/directionValues[k];
                  if (lambda_new<lambda) {
                     inequalityIndex=k;
                     lambda=lambda_new;
                  }
               }
            }
         }
         return lambda;
      }

      bool has_predecessor(const Node& predecessorCandidate){
         // This is a hack, since we just assume that these nodes were already
         // neighbors.
         Int j = get_predecessor_index();
         Node pred = get_predecessor(j);
         return pred == predecessorCandidate;
      }

      void print() const {
         cout << "Basis " << basis << " Vertex " << vertex << " Costs " << costs << endl;
      }

      bool operator==(const Node& rsn){
         return basis == rsn.basis;
      }

      Node get_predecessor(Int& j){
         Int basisIndex = get_predecessor_index();
         Node result(get_jth_child(basisIndex));
         Set<Int> sBasis(basis), sPredBasis(result.basis);
         sPredBasis -= sBasis;
         Int predecessorFacet = sPredBasis.front();
         j=0;
         while(result.basis[j] != predecessorFacet){ j++; }
         return result;
      }

      bool is_ray() const {
         return isRay;
      }

      bool has_jth_child(Int j) {
         if(costs[j] < 0){
            Node jthNeighbor(get_jth_child(j));
            if(jthNeighbor.is_ray()){
               Vector<Scalar> direction = get_jth_direction(j);
               logger.log_ray(direction);
               return false;
            } else {
               return true;
            }
         } else {
            return false;
         }
      }

      Int get_Delta() {
         return dim-1;
      }
};


template<typename Scalar>
class Logger {
   using VectorType = Vector<Scalar>;
   using Edge = std::pair<VectorType, VectorType >;
   using NodeType = Node<Scalar>;

   private:
   BigObject p;
   Matrix<Scalar> inequalities, equations;
   VectorType objective;
   Int dim, n_vertices;
   //used to construct the graph at the end
   hash_map<VectorType, Int> v_map;
   RayLogger<Scalar> rl;
   ListMatrix<Vector <Scalar> > vertices;
   std::list<Edge> edges;
   Array<Int> start_B;

   public:

   Logger(BigObject p_in, const VectorType& min_vertex, OptionSet options):
      p(p_in), n_vertices(0)
   {
      // p.give("FACETS|INEQUALITIES") >> inequalities;
      p.give("FACETS") >> inequalities;
      dim=inequalities.cols()-1;
      equations.resize(0, dim+1);
      // p.lookup("AFFINE_HULL|EQUATIONS") >> equations;
      p.lookup("AFFINE_HULL") >> equations;

      //select initial basis !
      start_B.resize(rank(inequalities.minor(All, range_from(1))));
      copy_range(entire(indices(attach_selector(inequalities*min_vertex, operations::is_zero()))), 
            start_B.begin());
      //the last rows of A are equations, so they has to be in each basis

      if (options.exists("objective")) {
         options["objective"] >> objective;
         objective=objective.slice(range_from(1));
      } else {
         // if no objective function is given we artificially construct one that minimizes min_vertex
         objective = -T(-inequalities.minor(start_B, range_from(1)) / equations.minor(All, range_from(1))) * ones_vector<Scalar>(dim);
      }
   }


   void log_node(NodeType rsn){
      vertices/=(rsn.get_vertex());
      v_map[rsn.get_vertex()]=n_vertices++;  
   }

   void tree_add_node(NodeType rsn){}

   void tree_add_edge(NodeType source, NodeType target){
      edges.push_back(Edge(source.get_vertex(), target.get_vertex()));
   }

   NodeType get_initial() {
      return NodeType(start_B, objective, inequalities, equations, rl, dim);
   }

   void exit_node(const NodeType& rsn){
      // MTS stuff would go here
   }

   void print_tree(){}

   Vector<Scalar> get_objective() {
      return zero_vector<Scalar>(1) | objective;
   }

   Matrix<Scalar> get_vertices() {
      if(rl.size() == 0){
         return vertices;
      } else {
         return ListMatrix<VectorType>(rl.size()+vertices.rows(), dim+1, concatenate(rows(vertices),rl.get_rays()).begin());
      }
   }

   Set<Int> get_far_face() {
      if(rl.size() != 0){
         return sequence(n_vertices,rl.size());
      } else {
         return Set<Int>();
      }
   }

   Graph<Directed> get_bounded_graph() {
      Graph<Directed> graph(sequence(0,n_vertices), n_vertices+rl.size());
      for (auto i=entire(edges); !i.at_end(); ++i)
         graph.edge(v_map[i->first],v_map[i->second]);
      return graph;
   }

};

template<typename Scalar>
ListReturn generic(BigObject p, const Vector<Scalar>& min_vertex, OptionSet options){
   Logger<Scalar> SL(p, min_vertex, options);
   ReverseSearchTemplate<Node<Scalar>, Logger<Scalar>, true, true> RS(SL);
   Node<Scalar> initial(SL.get_initial());
   RS.auto_reverse_search(initial);
   ListReturn result;
   result << SL.get_far_face();
   result << SL.get_vertices();
   result << SL.get_bounded_graph();
   result << SL.get_objective();
   return result;
}

} // end namespace reverse_search_simple_polytope

template<typename Scalar>
ListReturn simple_polytope_vertices_rs(BigObject p, const Vector<Scalar>& min_vertex, OptionSet options){
   return reverse_search_simple_polytope::generic(p, min_vertex, options);
}

UserFunctionTemplate4perl("# @category Geometry"
                          "# Use reverse search method to find the vertices of a polyhedron."
                          "# While applying this method, also collect the directed graph of"
                          "# cost optimization with respect to a (optionally) provided"
                          "# objective. If no objective is provided, one will be selected"
                          "# that cuts of [[ONE_VERTEX]]"
                          "# The input polytope must be [[SIMPLE]] and [[POINTED]], these"
                          "# properties are not checked by the algorithm."
                          "# @param Polytope<Scalar> P"
                          "# @param Vector<Scalar> min_vertex"
                          "# @return List (Set<Int> far face, Matrix<Scalar> vertices, Graph<Directed> directed bounded graph, Vector<Scalar> objective) ",
   "simple_polytope_vertices_rs<Scalar>(Polytope<Scalar>,$,{ objective => undef })"); 

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
