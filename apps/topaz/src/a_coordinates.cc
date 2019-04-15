/* Copyright (c) 1998-2016
Ewgenij Gawrilow, Michael Joswig (Technische Universitaet Berlin, Germainy)
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

#include "polymake/client.h"
#include "polymake/Graph.h"
#include "polymake/Set.h"
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/graph/DoublyConnectedEdgeList.h"
#include "polymake/Polynomial.h"
#include "polymake/hash_map"

namespace polymake { namespace topaz {

using namespace graph;
typedef Array<Polynomial<Rational,int>> halfedge_variables;

// set up triangle list from DCEL data
// this should be part of the dcel data structure...
Map<int,int> triangleMap(const Array<Array<int>>& dcel_data){
   DoublyConnectedEdgeList dcel(dcel_data);
   int dim = dcel.getNumHalfEdges();     
   Map<int,int> triangle_map;
   Set<int> curr_set(sequence(0,dim));
   int triangle_id = dim;
   while( !curr_set.empty() ){
      int edge_id = curr_set.front();
      const HalfEdge* halfEdge = &( dcel.getEdges()[edge_id] );
      int next_id = dcel.getHalfEdgeId( halfEdge->getNext() );
      int next_next_id = dcel.getHalfEdgeId( halfEdge->getNext()->getNext() );      
      
      triangle_map[ edge_id ] = triangle_id;
      triangle_map[ next_id ] = triangle_id;
      triangle_map[ next_next_id ] = triangle_id;
      curr_set -= Set<int>({edge_id, next_id, next_next_id}); 
      triangle_id++;
   }
   return triangle_map;
}
Function4perl(&triangleMap,"triangleMap( $ )");





// return the outitude polynomial for edge of index edge_id (half edges: 2*edge_id, 2*edge_id+1)
Polynomial<Rational,int> getOutitude(const Array<Array<int>>& dcel_data, int edge_id){
   const auto var = Polynomial<Rational,int>::monomial;
   
   DoublyConnectedEdgeList dcel(dcel_data);
   int dim = 4*dcel.getNumHalfEdges()/3;  
   int e = 2*edge_id;
   const HalfEdge* halfEdge = &( dcel.getEdges()[e] );
   int a = dcel.getHalfEdgeId( halfEdge->getNext() );
   int b = dcel.getHalfEdgeId( halfEdge->getPrev()->getTwin() );
   int c = dcel.getHalfEdgeId( halfEdge->getTwin()->getNext() );
   int d = dcel.getHalfEdgeId( halfEdge->getTwin()->getPrev()->getTwin() );
   int f = dcel.getHalfEdgeId( halfEdge->getTwin() );
   
   auto triangle_map = triangleMap(dcel_data);
   
   return var(triangle_map[f],dim)*( var(e,dim)*var(a,dim) + var(f,dim)*var(b,dim) - var(e,dim)*var(f,dim) ) +
          var(triangle_map[e],dim)*( var(e,dim)*var(d,dim) + var(f,dim)*var(c,dim) - var(e,dim)*var(f,dim) );
}
Function4perl(&getOutitude,"getOutitude( $$ )");


// given dcel data, return all outitudes, one for each edge
Array<Polynomial<Rational,int>> outitudes(const Array<Array<int>>& dcel_data){
   DoublyConnectedEdgeList dcel(dcel_data);
   auto outs = Array<Polynomial<Rational,int>>(dcel.getNumEdges());
   for(int i = 0; i < dcel.getNumEdges(); i++){
      outs[i] = getOutitude(dcel_data,i);
   }
   return outs;
}

UserFunction4perl("# @category Other"
                  "# Given a triangulation of a punctured surface this calculates all the outitude polynomials.\n" 
                  "# The first e = #{oriented edges} monomials correspond to A-coordinates of the oriented edges, labeled as in the input.\n" 
                  "# The last t = #{triangles} monomials correspond to A-coordinates of the triangles."
                  "# @param Array<Array<Int>> dcel_data the data for the doubly connected edge list representing the triangulation."
                  "# @return Array<Polynomial<Rational,Int>> an array containing the outitudes in order of the input." 
                  "# @example We may calculate the outitude polynomials of a once punctured torus by typing:"
                  "# > $T1 = new Array<Array<Int>>([[0,0,2,3],[0,0,4,5],[0,0,0,1]]);"
                  "# > print outitudes($T1);"
                  "# | - x_0*x_1*x_6 - x_0*x_1*x_7 + x_0*x_2*x_7 + x_0*x_4*x_6 + x_1*x_3*x_6 + x_1*x_5*x_7 x_0*x_2*x_6 + x_1*x_3*x_7 - x_2*x_3*x_6 - x_2*x_3*x_7 + x_2*x_4*x_7 + x_3*x_5*x_6 x_0*x_4*x_7 + x_1*x_5*x_6 + x_2*x_4*x_6 + x_3*x_5*x_7 - x_4*x_5*x_6 - x_4*x_5*x_7",
                  &outitudes,"outitudes( $ )");



   
  

// return all outitudes, one for each edge 
Array<Polynomial<Rational,int>> outitudes_string(const std::string& surface){
   const char s(surface[0]);
   int n;
   std::istringstream is (surface.substr(1));
   is >> n;
   switch(s){
      case 'S':
      switch(n){
         case 3:
            return outitudes( {{1,0,2,5},{2,1,4,1},{0,2,0,3}} );
         case 4:
            return outitudes( {{1,0,2,6},{2,1,4,9},{0,2,0,11},{3,0,8,5},{1,3,1,10},{2,3,3,7}} );
         default:
            throw std::runtime_error("Sorry, so far we cannot handle spheres with more than four punctures. Soon we will.");
      }

      case 'T':
      switch(n){
         case 1:
            return outitudes( {{0,0,2,3},{0,0,4,5},{0,0,0,1}} );
         case 2:
            return outitudes( {{0,0,6,5},{0,0,1,10},{0,0,8,2},{1,0,11,4},{1,0,7,3},{1,0,9,0}} );
         case 3:
            return outitudes( {{1,0,2,17},{2,1,4,14},{0,2,0,6},{1,2,8,16},{0,1,5,10},{2,1,12,1},{0,2,9,3},{0,1,13,7},{0,2,15,11}} );
         default:
            throw std::runtime_error("Sorry, so far we cannot handle a torus with more than three punctures. Soon we will.");
      }
      
      case 'D':
      switch(n){
         case 1:
            return outitudes( {{0,0,2,6},{0,0,4,8},{0,0,0,1},{0,0,5,3},{0,0,7,10},{0,0,16,12},{0,0,14,15},{0,0,11,17},{0,0,9,13}} ); 
         default:
            throw std::runtime_error("Sorry, so far we cannot handle a double torus with more than one puncture. Soon we will.");
      }
      default:
         throw std::runtime_error("Did not recognize your surface.");
   }

}
UserFunction4perl("# @category Other"
                  "# Given a punctured surface by a string from the list below, this calculates all the outitude polynomials.\n" 
                  "# Choose among: S3, S4 (ipunctured spheres) and T1, T2, T3 (punctured tori) and D1 (punctured double torus).\n" 
                  "# A triangulation of the surface will be chosen for you.\n"
                  "# The first e = #{oriented edges} monomials correspond to A-coordinates of the oriented edges.\n" 
                  "# The last t = #{triangles} monomials correspond to A-coordinates of the triangles."
                  "# @param String surface the surface name."
                  "# @return Array<Polynomial<Rational,Int>> an array containing the outitudes." ,
                  &outitudes_string,"outitudes( String )");










// print all outitudes
void print_outitudes(const Array<Array<int>>& dcel_data){
   DoublyConnectedEdgeList dcel(dcel_data);
   for(int i = 0; i < dcel.getNumEdges(); i++){
      cout << "Outitude " << i << ": " << getOutitude(dcel_data,i) << endl;
   }
}

Function4perl(&print_outitudes,"print_outitudes( $ )");





// return the dual outitude polynomial for edge of index edge_id (half edges: 2*edge_id, 2*edge_id+1)
Polynomial<Rational,int> getDualOutitude(const Array<Array<int>>& dcel_data, int edge_id){
   const auto var = Polynomial<Rational,int>::monomial;
   
   DoublyConnectedEdgeList dcel(dcel_data);
   int dim = 4*dcel.getNumHalfEdges()/3;  
   int e = 2*edge_id;
   const HalfEdge* halfEdge = &( dcel.getEdges()[e] );
   int a = dcel.getHalfEdgeId( halfEdge->getNext() );
   int aa = dcel.getHalfEdgeId( halfEdge->getNext()->getTwin() );
   int b = dcel.getHalfEdgeId( halfEdge->getPrev()->getTwin() );
   int bb = dcel.getHalfEdgeId( halfEdge->getPrev() );
   int c = dcel.getHalfEdgeId( halfEdge->getTwin()->getNext() );
   int cc = dcel.getHalfEdgeId( halfEdge->getTwin()->getNext()->getTwin() );
   int d = dcel.getHalfEdgeId( halfEdge->getTwin()->getPrev()->getTwin() );
   int dd = dcel.getHalfEdgeId( halfEdge->getTwin()->getPrev() );
   int f = dcel.getHalfEdgeId( halfEdge->getTwin() );
   
   auto triangle_map = triangleMap(dcel_data);
   int A = triangle_map[e];
   int B = triangle_map[f];
   
   return var(A,dim)*( var(e,dim)*var(cc,dim)*var(d,dim) + var(f,dim)*var(c,dim)*var(dd,dim) )*( var(f,dim)*var(aa,dim) + var(e,dim)*var(bb,dim) - var(e,dim)*var(f,dim) ) +
          var(B,dim)*( var(e,dim)*var(a,dim)*var(bb,dim) + var(f,dim)*var(aa,dim)*var(b,dim) )*( var(f,dim)*var(dd,dim) + var(e,dim)*var(cc,dim) - var(e,dim)*var(f,dim) );
}

Function4perl(&getDualOutitude,"getDualOutitude( $$ )");


// return all  dual outitudes, one for each edge
Array<Polynomial<Rational,int>> dualOutitudes(const Array<Array<int>>& dcel_data){
   DoublyConnectedEdgeList dcel(dcel_data);
   auto outs = Array<Polynomial<Rational,int>>(dcel.getNumEdges());
   for(int i = 0; i < dcel.getNumEdges(); i++){
      outs[i] = getDualOutitude(dcel_data,i);
   }
   return outs;
}

UserFunction4perl("# @category Other"
                  "# Given a triangulation of a punctured surface this calculates all the outitude polynomials of the dual structure.\n" 
                  "# The first e = #{oriented edges} monomials correspond to A-coordinates of the oriented edges of the primal structure , labeled as in the input.\n" 
                  "# The last t = #{triangles} monomials correspond to A-coordinates of the triangles of the primal structure."
                  "# @param Array<Array<Int>> dcel_data the data for the doubly connected edge list representing the triangulation."
                  "# @return Array<Polynomial<Rational,Int>> an array containing the dual outitudes in order of the input.",
                  &dualOutitudes,"dualOutitudes( $ )");





// print all dual outitudes
void print_dual_outitudes(const Array<Array<int>>& dcel_data){
   DoublyConnectedEdgeList dcel(dcel_data);
   for(int i = 0; i < dcel.getNumEdges(); i++){
      cout << "Dual Outitude " << i << ": " << getDualOutitude(dcel_data,i) << endl;
   }
}
Function4perl(&print_dual_outitudes,"print_dual_outitudes( $ )");






/*
class ProjectiveSurface{

friend class DoublyConnectedEdgeList;

private:

   // the triangulation of the surface, here we define the A-coordinates
   Array<Array<int>> triangulation;

   // A-coordinates for the half edges
   halfedge_variables edge_coords;


public:

ProjectiveSurface(Array<Array<int>> dcel_data)
   : triangulation(dcel_data)
{
   edge_coords = edgeCoordsFromDcel(dcel_data);
}


}; // end class ProjectiveSurface 
*/






} //end topaz namespace
} //end polymake namespace

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:

