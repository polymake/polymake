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

#include "polymake/fan/reverse_search_chamber_decomposition.h"
#include "polymake/linalg.h"
#include "polymake/IncidenceMatrix.h"

namespace polymake {
namespace fan {
namespace reverse_search_chamber_decomposition {


template<typename Scalar>
class AllCache {
   private:
      mutable Map<Bitset, BigObject> signature2Cell;
      const Matrix<Scalar>& hyperplanes, supportIneq, supportEq;

      BigObject& get_chamber(const Bitset& signature) const {
         if(!signature2Cell.exists(signature)){
            BigObject newchamber("Cone", mlist<Scalar>());
            Matrix<Scalar> inequalities(hyperplanes);
            inequalities.minor(~signature, All) *= -1;
            newchamber.take("INEQUALITIES") << inequalities / supportIneq;
            newchamber.take("EQUATIONS") << supportEq;
            signature2Cell[signature] = newchamber;
         }
         return signature2Cell[signature];
      }

   public:
      AllCache(const Matrix<Scalar>& H, const Matrix<Scalar>& SI, const Matrix<Scalar> SE):
         hyperplanes(H), supportIneq(SI), supportEq(SE)
      {}

      Matrix<Scalar> get_facets(const Bitset& signature) const {
         BigObject& chamber(get_chamber(signature));
         return chamber.give("FACETS");
      }

      Matrix<Scalar> get_rays(const Bitset& signature) const {
         BigObject& chamber(get_chamber(signature));
         return chamber.give("RAYS");
      }
      
      Matrix<Scalar> get_lineality(const Bitset& signature) const {
         BigObject& chamber(get_chamber(signature));
         return chamber.give("LINEALITY_SPACE");
      }

      bool facet_belongs_to_support(const Vector<Scalar>& facet) const {
         Matrix<Scalar> tmp(0, facet.dim());
         tmp /= facet;
         for(const auto& f : rows(supportIneq)){
            if(rank(tmp/f) == 1){
               return true;
            }
         }
         for(const auto& f : rows(supportEq)){
            if(rank(tmp/f) == 1){
               return true;
            }
         }
         return false;
      }
};


template<typename Scalar, typename NodeType>
class Logger {
   private:
      Int n, currentRayIndex, nHyperplanes;
      Matrix<Scalar> hyperplanes;
      Array<Set<Int>> rih;
      Map<Vector<Scalar>, Int> rayIndices;
      Set<Set<Int>> maximalCones;
      Map<Set<Int>, Bitset> cone2signature;
      
      Int log_ray(const Vector<Scalar>& r){
         if(!rayIndices.exists(r)){
            rayIndices[r] = currentRayIndex;
            Vector<Scalar> prods(hyperplanes*r);
            for (int i=0; i < prods.size(); i++) {
               if (prods[i]==0) {
                  rih[i].collect(currentRayIndex);
               }
            }
            currentRayIndex++;
         }
         return rayIndices[r];
      }

   public:
      Logger(const Matrix<Scalar> hyp):rih(hyp.rows()){
         currentRayIndex = 0;
         n = 0;
         hyperplanes = hyp;
         nHyperplanes = hyp.rows();
      }

      void log_node(const NodeType& chamber){
         Set<Int> mc;
         Matrix<Scalar> rays = chamber.get_rays();
         for(const auto& r : rows(rays)){
            mc += log_ray(r);
         }
         maximalCones += mc;
         cone2signature[mc] = chamber.get_signature();
         n++;
      }
      void exit_node(const NodeType& chamber){}
      void tree_add_node(const NodeType& chamber){}
      void tree_add_edge(const NodeType& chamber0, const NodeType& chamber1){}
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
      
      Array<Int> get_rays_perm() const {
         Array<Int> perm(rayIndices.size());
         Int i = 0;
         for(const auto& entry : rayIndices){
            perm[i] = entry.second;
            i++;
         }
         return perm;
      }

      Array<Set<Int>> get_maximal_cones() const {
         auto perm = get_rays_perm();
         Array<Set<Int>> result(maximalCones.size());
         Int i = 0;
         for(const auto& s : maximalCones){
            result[i] = group::action_inv<group::on_elements>(perm,s);
            i++;
         }
         return result;
      }

      IncidenceMatrix<NonSymmetric> get_chamber_signatures() const {
         IncidenceMatrix<NonSymmetric> result(maximalCones.size(), nHyperplanes);
         Int i = 0;
         for(const auto& s : maximalCones){
            result[i] = cone2signature[s];
            i++;
         }
         return result;
      }

      Array<Set<Int>> get_rays_in_hyperplanes() const {
         auto perm = get_rays_perm();
         Array<Set<Int>> result(rih.size());
         Int i = 0;
         for(const auto& s : rih){
            result[i] = group::action_inv<group::on_elements>(perm,s);
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
   
   // Find initial chamber.
   Bitset sig;
   Vector<Scalar> gen = get_generic_point(hyp, support);
   sig = point_to_signature(gen, hyp, support);
   NodeType initial(hyp, sig, AC);
   // Walk back to root of reverse search tree.
   Int i = 0;
   while(initial.has_upneighbor()){
      initial = initial.get_predecessor(i);
   }
   
   // Actual reverse search part.
   LoggerType CL(hyp);
   ReverseSearchTemplate<NodeType, LoggerType, false, false> RST(CL);
   RST.auto_reverse_search(initial);

   // Write output.
   ListReturn result;
   result << CL.get_ray_matrix();
   result << CL.get_maximal_cones();
   result << CL.get_chamber_signatures();
   result << AC.get_lineality(sig);
   result << CL.get_rays_in_hyperplanes();
   return result;
}

} // namespace reverse_search_chamber_decomposition

template<typename Scalar>
ListReturn chamber_decomposition_rs(BigObject HA) {
   return reverse_search_chamber_decomposition::generic<Scalar>(HA);
}

UserFunctionTemplate4perl("# @category Producing a fan"
                          "# Produce the chamber decomposition induced by a hyperplane arrangement",
                          "chamber_decomposition_rs<Scalar>(HyperplaneArrangement<type_upgrade<Scalar>>)");

} // namespace fan
} // namespace polymake


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
