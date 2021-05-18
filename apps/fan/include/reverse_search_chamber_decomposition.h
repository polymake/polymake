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

#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/ReverseSearch.h"
#include "polymake/group/action.h"
#include "polymake/Set.h"

namespace polymake {
namespace fan {
namespace reverse_search_chamber_decomposition {

template<typename Scalar>
Vector<Scalar> signature_to_vertex(const Matrix<Scalar>& hyp, const Bitset& signature){
   Vector<Scalar> result = ones_vector<Scalar>(hyp.rows());
   result.slice(~signature) *= -1;
   return T(hyp) * result;
}

template<typename Scalar, typename CacheType>
class Node {
   private:
      const Matrix<Scalar>& hyperplanes;
      Bitset signature;
      CacheType& cache;
      Vector<Scalar> vertex;
      Map<Vector<Scalar>, Bitset> upNeighbors, downNeighbors;

      Bitset neighbor_signature_from_facet(const Vector<Scalar>& facet, bool& facet_is_hyperplane){
         // This should be done more efficiently:
         // We could normalize the hyperplanes and check for equality. Although
         // we would have to modify signatures in this case.
         Bitset result(signature);
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
            if(!cache.facet_belongs_to_support(f)){
               bool facet_is_hyperplane = false;
               Bitset neighborS = neighbor_signature_from_facet(f, facet_is_hyperplane);
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
      }

   public:

      Node& operator=(const Node& in){
         signature = in.signature;
         vertex = in.vertex;
         upNeighbors = in.upNeighbors;
         downNeighbors = in.downNeighbors;
         return *this;
      }

      Node(const Matrix<Scalar>& hyp, const Bitset& sig, CacheType& c):
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
      
      const Bitset& get_signature() const {
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
Bitset point_to_signature(const Vector<Scalar>& point, const Matrix<Scalar>& hyp, BigObject support){
   Bitset signature;   
   const Int n = hyp.rows();
   for( Int i = 0; i < n; ++i){
      if(hyp[i] * point > 0){
         signature.insert(i);
      }
   }
   return signature;   
}


} // namespace reverse_search_chamber_decomposition
} // namespace fan
} // namespace polymake

