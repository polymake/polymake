/* Copyright (c) 1997-2019
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

#ifndef COMPACTIFICATION
#define COMPACTIFICATION

#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/Map.h"
#include "polymake/Set.h"
#include "polymake/linalg.h"
#include "polymake/graph/Decoration.h"
#include "polymake/graph/Lattice.h"
#include "polymake/fan/hasse_diagram.h"

namespace polymake { namespace fan{
namespace compactification {
	
   using graph::Lattice;
   using namespace graph::lattice;
   using namespace fan::lattice;
   

   struct IteratorWrap {
      FacetList groundSet;
      FacetList::const_iterator state;

      IteratorWrap(FacetList&& gs): groundSet(gs){
         state = groundSet.begin();
      }

      Set<int> operator*(){
         return *state;
      }

      void operator++(){
         state++;
      }

      bool at_end(){
         return state == groundSet.end();
      }
   };

   

   template<typename DecorationType, typename Scalar>
   class CellularClosureOperator {
      private:
         FaceMap<> face_index_map;
         Map<int, Set<int>> int2vertices;
         Map<Set<int>, int> vertices2int;
         int nVertices;
         Set<int> farVertices;
         Matrix<Scalar> vertices;
         Lattice<BasicDecoration, Nonsequential> oldHasseDiagram;

      public:
         typedef Set<int> ClosureData;

         CellularClosureOperator(perl::Object pc) {
            pc.give("FAR_VERTICES") >> farVertices;
            pc.give("VERTICES") >> vertices;
            pc.give("HASSE_DIAGRAM") >> oldHasseDiagram;
            nVertices = vertices.rows();
            Set<int> topNode; topNode += -1;
            int i = 0;
            // Build new vertices
            for(const auto& f : oldHasseDiagram.decoration()){
               if(f.face != topNode) { 
                  int faceDim = f.rank-1;
                  int tailDim = rank(vertices.minor(f.face * farVertices, All));
                  if(faceDim == tailDim){
                     int2vertices[i] = f.face;
                     vertices2int[f.face] = i;
                     i++;
                  }
               }
            }
         }
         
         Set<int> old_closure(const Set<int>& a) const {
            // We find a closure of a face in the old Hasse diagram by starting
            // at the top node and then descending into lower nodes whenever
            // they contain the given set of vertices. If no further descent is
            // possible, we terminate.
            int currentNode = oldHasseDiagram.top_node();
            const Graph<Directed>& G(oldHasseDiagram.graph());
            bool found = true;
            while(found){
               found = false;
               for(const auto& p: G.in_adjacent_nodes(currentNode)){
                const BasicDecoration& decor = oldHasseDiagram.decoration(p);
                  if(incl(a, decor.face) <= 0){
                     found = true;
                     currentNode = p;
                     break;
                  }
               }
            }
            return oldHasseDiagram.decoration(currentNode).face;
         }

         Set<int> closure(const Set<int> a) const {
            Set<int> originalRealisation;
            for(const auto i:a){
               originalRealisation += int2vertices[i];
            }
            Set<int> originalClosure = old_closure(originalRealisation);
            Set<int> commonRays = originalRealisation * farVertices;
            for(const auto i : a){
               commonRays = commonRays * int2vertices[i];
            }
            Set<int> result;
            for(const auto& v:vertices2int){
               if(incl(commonRays, v.first)<=0 && incl(v.first, originalClosure)<=0){
                  result += v.second;
               }
            }
            return result;
         }
         
         Set<int> closure_of_empty_set(){
            Set<int> empty;
            return empty;
         }

         FaceIndexingData get_indexing_data(const ClosureData& data)
         {
            int& fi = face_index_map[data];
            return FaceIndexingData(fi, fi == -1, fi == -2);
         }

         Set<int> compute_closure_data(const DecorationType& bd) const {
            return bd.face;
         }

         IteratorWrap get_closure_iterator(const Set<int>& face) const {
            Set<int> all = pm::range(0,int2vertices.size()-1);
            Set<int> toadd = all-face;
            FacetList result;
            for(auto i:toadd){
               result.insertMin(closure(face+i));
            }
            return IteratorWrap(std::move(result));
         }

         const Map<int, Set<int>>& get_int2vertices() const{
            return int2vertices;
         }

         const Set<int>& get_farVertices() const {
            return farVertices;
         }
   };
   
   
   struct SedentarityDecoration : public GenericStruct<SedentarityDecoration> {
     DeclSTRUCT( DeclFIELD(face, Set<int>)
                 DeclFIELD(rank,int)
                 DeclFIELD(realisation, Set<int>) 
                 DeclFIELD(sedentarity, Set<int>) );

     SedentarityDecoration() {}
     SedentarityDecoration(const Set<int>& f, int r, const Set<int>& re, const Set<int>& se) : face(f), rank(r), realisation(re), sedentarity(se) {}
   };
   
   class SedentarityDecorator {
      private:
         const Map<int, Set<int>>& int2vertices;
         const Set<int>& farVertices;

         Set<int> realisation(const Set<int> face) const {
            Set<int> result;
            for(const auto& e:face){
               result += int2vertices[e];
            }
            return result;
         }

         Set<int> sedentarity(const Set<int>& face) const {
            if(face.size() == 0){
               return Set<int>();
            }
            Set<int> result(farVertices);
            for(const auto& e:face){
               result *= int2vertices[e];
            }
            return result;
         }
      
      public:
         typedef SedentarityDecoration DecorationType;
         SedentarityDecorator(const Map<int, Set<int>>& i2v, const Set<int>& fv): int2vertices(i2v), farVertices(fv){}

         SedentarityDecoration compute_initial_decoration(const Set<int>& face) const {
            return SedentarityDecoration(face, 0, realisation(face), sedentarity(face));
         }

         SedentarityDecoration compute_decoration(const Set<int>& face, const SedentarityDecoration& bd) const {
            return SedentarityDecoration(face, bd.rank+1, realisation(face), sedentarity(face));
         }

         SedentarityDecoration compute_artificial_decoration(const NodeMap<Directed, SedentarityDecoration>& decor, const std::list<int>& max_faces) const {
            Set<int> D;
            Set<int> empty;
            D += -1;
            int rank = 0;
            for(const auto& mf : max_faces){
               if(rank < decor[mf].rank){ rank = decor[mf].rank; }
            }
            rank++;
            return SedentarityDecoration(D, rank, D, empty);
         }
         
   };

   


} // namespace compactification
} // namespace fan
} // namespace polymake
#endif
