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

#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/linalg.h"
#include "polymake/hash_map"
#include "polymake/Rational.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Set.h"
#include "polymake/FacetList.h"
#include "polymake/ReverseSearch.h"
#include "polymake/group/action.h"

namespace polymake {
namespace fan {
namespace reverse_search_cell_decomposition {

template<typename Scalar>
Vector<Scalar> signature_to_vertex(const Matrix<Scalar>& hyp, const Set<Int>& signature){
   Vector<Scalar> result = ones_vector<Scalar>(hyp.rows());
   result.slice(~signature) *= -1;
   return T(hyp) * result;
}


template<typename Scalar>
class AllCache {
   private:
      mutable Map<Set<Int>, BigObject> signature2Cell;
      const Matrix<Scalar>& hyperplanes, supportIneq, supportEq;

      BigObject& get_cell(const Set<Int>& signature) const {
         if(!signature2Cell.exists(signature)){
            BigObject newcell("Cone", mlist<Scalar>());
            Matrix<Scalar> inequalities(hyperplanes);
            inequalities.minor(~signature, All) *= -1;
            newcell.take("INEQUALITIES") << inequalities / supportIneq;
            newcell.take("EQUATIONS") << supportEq;
            signature2Cell[signature] = newcell;
         }
         return signature2Cell[signature];
      }

   public:
      AllCache(const Matrix<Scalar>& H, const Matrix<Scalar>& SI, const Matrix<Scalar> SE):
         hyperplanes(H), supportIneq(SI), supportEq(SE)
      {}

      Matrix<Scalar> get_facets(const Set<Int>& signature) const {
         BigObject& cell(get_cell(signature));
         return cell.give("FACETS");
      }

      Matrix<Scalar> get_rays(const Set<Int>& signature) const {
         BigObject& cell(get_cell(signature));
         return cell.give("RAYS");
      }
      
      Matrix<Scalar> get_lineality(const Set<Int>& signature) const {
         BigObject& cell(get_cell(signature));
         return cell.give("LINEALITY_SPACE");
      }
};


template<typename Scalar, typename CacheType>
class Node {
   private:
      const Matrix<Scalar>& hyperplanes;
      Set<Int> signature;
      CacheType& cache;
      Vector<Scalar> vertex;
      Map<Vector<Scalar>, Set<Int>> upNeighbors, downNeighbors;

      Set<Int> neighbor_signature_from_facet(const Vector<Scalar>& facet, bool& facet_is_hyperplane){
         // This should be done more efficiently:
         // We could normalize the hyperplanes and check for equality. Although
         // we would have to modify signatures in this case.
         Set<Int> result(signature);
         Int i = 0;
         Matrix<Scalar> tmp(0, facet.dim());
         tmp /= facet;
         for(const auto& f : rows(hyperplanes)){
            if(rank(tmp/f) == 1){
               facet_is_hyperplane = true;
               result ^= i;
            }
            i++;
         }
         return result;
      }

      void populate_neighbors(){
         const Matrix<Scalar> F = cache.get_facets(signature);
         for(const auto& f : rows(F)){
            bool facet_is_hyperplane = false;
            Set<Int> neighborS = neighbor_signature_from_facet(f, facet_is_hyperplane);
            if(facet_is_hyperplane){
               Vector<Scalar> neighborV = signature_to_vertex(hyperplanes, neighborS);
               if(lex_compare(neighborV, vertex) == 1){
                  upNeighbors[neighborV] = neighborS;
               } else {
                  downNeighbors[neighborV] = neighborS;
               }
            }
         }
      }

   public:

      Node& operator=(const Node& in){
         signature = in.signature;
         vertex = in.vertex;
         upNeighbors = in.upNeighbors;
         downNeighbors = in.downNeighbors;
         return *this;
      }

      Node(const Matrix<Scalar>& hyp, const Set<Int>& sig, CacheType& c):
         hyperplanes(hyp), signature(sig), cache(c) {
            vertex = signature_to_vertex(hyperplanes, signature);
            populate_neighbors();
         }

      bool operator==(const Node& other) const {
         return signature == other.signature;
      }

      bool has_jth_child(Int j) const {
         return j < downNeighbors.size();
      }
      
      bool has_predecessor(const Node& pred) const {
         const auto& front = upNeighbors.front();
         return front.first == pred.vertex;
      }

      Node get_jth_child(Int j) const {
         Int i = 0;
         for(const auto& neighbor : downNeighbors){
            if(i == j){
               return Node(hyperplanes, neighbor.second, cache);
            }
            i++;
         }
         return *this;
      }
      
      Node get_predecessor(Int& j) const {
         const auto& front = upNeighbors.front();
         Node result(hyperplanes, front.second, cache);
         j = 0;
         for(const auto& neighbor : result.downNeighbors){
            if(neighbor.second == signature){
               break;
            }
            j++;
         }
         return result;
      }

      Int get_Delta() const {
         return downNeighbors.size();
      }

      bool has_upneighbor() const {
         return upNeighbors.size() > 0;
      }

      Matrix<Scalar> get_rays() const {
         return cache.get_rays(signature);
      }
      
      const Set<Int>& get_signature() const {
         return signature;
      }

};

// Get a generic point in the [[SUPPORT]] of a HyperplaneArrangement, i.e. a
// point that does not lie on any of the [[HYPERPLANES]].
template<typename Scalar>
Vector<Scalar> get_generic_point(const Matrix<Scalar>& hyp, BigObject support){
   // TODO: Check whether some of the hyperplanes contain support and ignore these.
   // Make sure that generic point is not contained in one of the hyperplanes.
   Matrix<Scalar> rays = support.give("RAYS | INPUT_RAYS");
   Matrix<Scalar> lineality = support.give("LINEALITY_SPACE | INPUT_LINEALITY");
   Matrix<Scalar> gens(rays / lineality);
   Vector<Scalar> result(gens.cols());
   for(const auto& row : rows(gens)){
      result += rand() * row;
   }
   return result;
}

template<typename Scalar>
Set<Int> point_to_signature(const Vector<Scalar>& point, const Matrix<Scalar>& hyp, BigObject support){
   Set<Int> signature;   
   const Int n = hyp.rows();
   for( Int i = 0; i < n; ++i){
      if(hyp[i] * point > 0){
         signature.insert(i);
      }
   }
   return signature;   
}




template<typename Scalar, typename NodeType>
class Logger {
   private:
      Int n, currentRayIndex;
      Map<Vector<Scalar>, Int> rayIndices;
      Set<Set<Int>> maximalCones;
      Map<Set<Int>, Set<Int>> cone2signature;
      
      Int log_ray(const Vector<Scalar>& r){
         if(!rayIndices.exists(r)){
            rayIndices[r] = currentRayIndex;
            currentRayIndex++;
         }
         return rayIndices[r];
      }

   public:
      Logger(){
         currentRayIndex = 0;
         n = 0;
      }

      void log_node(const NodeType& cell){
         Set<Int> mc;
         Matrix<Scalar> rays = cell.get_rays();
         for(const auto& r : rows(rays)){
            mc += log_ray(r);
         }
         maximalCones += mc;
         cone2signature[mc] = cell.get_signature();
         n++;
      }
      void exit_node(const NodeType& cell){}
      void tree_add_node(const NodeType& cell){}
      void tree_add_edge(const NodeType& cell0, const NodeType& cell1){}
      void print_tree(){}

      Matrix<Scalar> get_ray_matrix() const {
         Matrix<Scalar> result(rayIndices.size(), rayIndices.front().first.dim());
         Int i = 0;
         for(const auto& entry : rayIndices){
            result.row(i) = entry.first;
            i++;
         }
         return result;
      }

      // TODO
      // Should probably return an IncidenceMatrix?
      Array<Set<Int>> get_maximal_cones() const {
         Array<Int> perm(rayIndices.size());
         Int i = 0;
         for(const auto& entry : rayIndices){
            perm[i] = entry.second;
            i++;
         }
         Array<Set<Int>> result(maximalCones.size());
         i = 0;
         for(const auto& s : maximalCones){
            result[i] = group::action_inv<group::on_elements>(perm,s);
            i++;
         }
         return result;
      }

      Array<Set<Int>> get_cell_signatures() const {
         Array<Set<Int>> result(maximalCones.size());
         Int i = 0;
         for(const auto& s : maximalCones){
            result[i] = cone2signature[s];
            i++;
         }
         return result;
      }
};


template<typename Scalar>
ListReturn generic(BigObject HA) {
   using CacheType = AllCache<Scalar>;
   using NodeType = Node<Scalar, CacheType>;
   using LoggerType = Logger<Scalar, NodeType>;

   // Read input data.
   const Matrix<Scalar> hyp = HA.give("HYPERPLANES");
   BigObject support = HA.give("SUPPORT");
   const Matrix<Scalar> supportIneq = support.give("FACETS | INEQUALITIES");
   const Matrix<Scalar> supportEq = support.give("LINEAR_SPAN | EQUATIONS");
   AllCache<Scalar> AC(hyp, supportIneq, supportEq);
   
   // Find initial cell.
   Set<Int> sig;
   Vector<Scalar> gen = get_generic_point(hyp, support);
   sig = point_to_signature(gen, hyp, support);
   NodeType initial(hyp, sig, AC);
   // Walk back to root of reverse search tree.
   Int i = 0;
   while(initial.has_upneighbor()){
      initial = initial.get_predecessor(i);
   }
   
   // Actual reverse search part.
   LoggerType CL;
   ReverseSearchTemplate<NodeType, LoggerType, false, false> RST(CL);
   RST.auto_reverse_search(initial);

   // Write output.
   ListReturn result;
   result << CL.get_ray_matrix();
   result << CL.get_maximal_cones();
   result << CL.get_cell_signatures();
   result << AC.get_lineality(sig);
   return result;
}

} // namespace reverse_search_cell_decomposition

template<typename Scalar>
ListReturn cell_decomposition_rs(BigObject HA) {
   return reverse_search_cell_decomposition::generic<Scalar>(HA);
}

UserFunctionTemplate4perl("# @category Producing a fan"
                          "# Produce the cell decomposition induced by a hyperplane arrangement",
                          "cell_decomposition_rs<Scalar>(HyperplaneArrangement<type_upgrade<Scalar>>)");

} // namespace fan
} // namespace polymake


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
