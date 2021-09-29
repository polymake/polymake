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

#include "polymake/client.h"
#include "polymake/Graph.h"
#include "polymake/Set.h"
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/graph/DoublyConnectedEdgeList.h"
#include "polymake/Polynomial.h"
#include "polymake/hash_map"

namespace polymake { namespace topaz {

using DoublyConnectedEdgeList = graph::dcel::DoublyConnectedEdgeList;
using HalfEdge = graph::dcel::HalfEdge;

using halfedge_variables = Array<Polynomial<Rational, Int>>;
using flip_sequence = std::list<Int>;

// return the outitude polynomial for edge of index edge_id (half edges: 2*edge_id, 2*edge_id+1)
Polynomial<Rational, Int> getOutitudePolynomial(const Matrix<Int>& dcel_data, Int edge_id)
{
   const auto var = Polynomial<Rational, Int>::monomial;
   DoublyConnectedEdgeList dcel(dcel_data);
   Int dim = 4*dcel.getNumHalfEdges()/3;  
   Int e = 2*edge_id;
   const HalfEdge* halfEdge = dcel.getHalfEdge(e);
   Int a = dcel.getHalfEdgeId( halfEdge->getNext() );
   Int b = dcel.getHalfEdgeId( halfEdge->getPrev()->getTwin() );
   Int c = dcel.getHalfEdgeId( halfEdge->getTwin()->getNext() );
   Int d = dcel.getHalfEdgeId( halfEdge->getTwin()->getPrev()->getTwin() );
   Int f = dcel.getHalfEdgeId( halfEdge->getTwin() );
   auto triangle_map = dcel.triangleMap(); 
   //auto triangle_map = triangleMap(dcel_data);
   return var(triangle_map[f],dim)*( var(e,dim)*var(a,dim) + var(f,dim)*var(b,dim) - var(e,dim)*var(f,dim) ) +
          var(triangle_map[e],dim)*( var(e,dim)*var(d,dim) + var(f,dim)*var(c,dim) - var(e,dim)*var(f,dim) );
}

// given dcel data, return all outitudes, one for each edge
Array<Polynomial<Rational,Int>> outitudePolynomials(const Matrix<Int>& dcel_data)
{
   DoublyConnectedEdgeList dcel(dcel_data);
   auto outs = Array<Polynomial<Rational, Int>>(dcel.getNumEdges());
   for (Int i = 0; i < dcel.getNumEdges(); ++i) {
      outs[i] = getOutitudePolynomial(dcel_data,i);
   }
   return outs;
}

UserFunction4perl("# @category Other"
                  "# Given a triangulation of a punctured surface this calculates all the outitude polynomials.\n" 
                  "# The first e = #{oriented edges} monomials correspond to A-coordinates of the oriented edges, labeled as in the input.\n" 
                  "# The last t = #{triangles} monomials correspond to A-coordinates of the triangles."
                  "# @param Matrix<Int> dcel_data the data for the doubly connected edge list representing the triangulation."
                  "# @return Array<Polynomial<Rational,Int>> an array containing the outitudes in order of the input." 
                  "# @example We may calculate the outitude polynomials of a thrice punctured sphere."
                  "# Here the first six monomials x_0, ... , x_5 are associated to the six oriented edges, x_6 and x_7 are associated to the triangles enclosed by the oriented edges 0,2,4 and 1,3,5 respectively."
                  "# > $S3 = new Matrix<Int>([[1,0,2,5,0,1],[2,1,4,1,0,1],[0,2,0,3,0,1]]);;"
                  "# > print outitudePolynomials($S3);"
                  "# | - x_0*x_1*x_6 - x_0*x_1*x_7 + x_0*x_2*x_6 + x_0*x_2*x_7 + x_1*x_5*x_6 + x_1*x_5*x_7 x_1*x_3*x_6 + x_1*x_3*x_7 - x_2*x_3*x_6 - x_2*x_3*x_7 + x_2*x_4*x_6 + x_2*x_4*x_7 x_0*x_4*x_6 + x_0*x_4*x_7 + x_3*x_5*x_6 + x_3*x_5*x_7 - x_4*x_5*x_6 - x_4*x_5*x_7",
                  &outitudePolynomials,"outitudePolynomials( Matrix<Int> )");



// return all outitudes, one for each edge 
/*Array<Polynomial<Rational,Int>> outitudePolynomialsFromString(const std::string& surface)
{
   const char s(surface[0]);
   Int n;
   std::istringstream is (surface.substr(1));
   is >> n;
   switch(s){
      case 'S':
      switch(n){
         case 3:
            return outitudePolynomials( {{1,0,2,5,0,1},{2,1,4,1,0,1},{0,2,0,3,0,1}} );
         //case 4:
         //   return outitudePolynomials( {{1,0,2,6},{2,1,4,9},{0,2,0,11},{3,0,8,5},{1,3,1,10},{2,3,3,7}} );
         default:
            throw std::runtime_error("Sorry, so far we cannot handle spheres with more than three punctures.");
      }

      case 'T':
      switch(n){
         case 1:
            return outitudePolynomials( {{0,0,2,3,0,1},{0,0,4,5,0,1},{0,0,0,1,0,1}} );
         case 2:
            return outitudePolynomials( {{0,0,6,5},{0,0,1,10},{0,0,8,2},{1,0,11,4},{1,0,7,3},{1,0,9,0}} );
         case 3:
            return outitudePolynomials( {{1,0,2,17},{2,1,4,14},{0,2,0,6},{1,2,8,16},{0,1,5,10},{2,1,12,1},{0,2,9,3},{0,1,13,7},{0,2,15,11}} );
         
         default:
            throw std::runtime_error("Sorry, so far we cannot handle a torus with more than one puncture.");
      }
      
      case 'D':
      switch(n){
         case 1:
            return outitudePolynomials( {{0,0,2,6},{0,0,4,8},{0,0,0,1},{0,0,5,3},{0,0,7,10},{0,0,16,12},{0,0,14,15},{0,0,11,17},{0,0,9,13}} ); 
         default:
            throw std::runtime_error("Sorry, so far we cannot handle a double torus with more than one puncture. Soon we will.");
      }
      default:
         throw std::runtime_error("Did not recognize your surface.");
   }

}
UserFunction4perl("# @category Other"
                  "# Given a punctured surface by a string from the list below, this calculates all the outitude polynomials.\n" 
                  "# Choose among: S3, S4 (punctured spheres) and T1, T2, T3 (punctured tori) and D1 (punctured double torus).\n" 
                  "# A triangulation of the surface will be chosen for you.\n"
                  "# The first e = #{oriented edges} monomials correspond to A-coordinates of the oriented edges.\n" 
                  "# The last t = #{triangles} monomials correspond to A-coordinates of the triangles."
                  "# @param String surface the surface name."
                  "# @return Array<Polynomial<Rational,Int>> an array containing the outitudes." ,
                  &outitudePolynomialsFromString,"outitudePolynomialsFromString( String )");
*/


// return the dual outitude polynomial for edge of index edge_id (half edges: 2*edge_id, 2*edge_id+1)
Polynomial<Rational,Int> getDualOutitudePolynomial(const Matrix<Int>& dcel_data, Int edge_id)
{
   const auto var = Polynomial<Rational, Int>::monomial;
   
   DoublyConnectedEdgeList dcel(dcel_data);
   Int dim = 4*dcel.getNumHalfEdges()/3;  
   Int e = 2*edge_id;
   const HalfEdge* halfEdge = dcel.getHalfEdge(e);
   Int a = dcel.getHalfEdgeId( halfEdge->getNext() );
   Int aa = dcel.getHalfEdgeId( halfEdge->getNext()->getTwin() );
   Int b = dcel.getHalfEdgeId( halfEdge->getPrev()->getTwin() );
   Int bb = dcel.getHalfEdgeId( halfEdge->getPrev() );
   Int c = dcel.getHalfEdgeId( halfEdge->getTwin()->getNext() );
   Int cc = dcel.getHalfEdgeId( halfEdge->getTwin()->getNext()->getTwin() );
   Int d = dcel.getHalfEdgeId( halfEdge->getTwin()->getPrev()->getTwin() );
   Int dd = dcel.getHalfEdgeId( halfEdge->getTwin()->getPrev() );
   Int f = dcel.getHalfEdgeId( halfEdge->getTwin() );
   
   auto triangle_map = dcel.triangleMap();
   Int A = triangle_map[e];
   Int B = triangle_map[f];
   
   return var(A,dim)*( var(e,dim)*var(cc,dim)*var(d,dim) + var(f,dim)*var(c,dim)*var(dd,dim) )*( var(f,dim)*var(aa,dim) + var(e,dim)*var(bb,dim) - var(e,dim)*var(f,dim) ) +
          var(B,dim)*( var(e,dim)*var(a,dim)*var(bb,dim) + var(f,dim)*var(aa,dim)*var(b,dim) )*( var(f,dim)*var(dd,dim) + var(e,dim)*var(cc,dim) - var(e,dim)*var(f,dim) );
}
//Function4perl(&getDualOutitudePolynomial,"getDualOutitudePolynomial( $$ )");



// return all  dual outitudes, one for each edge
Array<Polynomial<Rational,Int>> dualOutitudePolynomials(const Matrix<Int>& dcel_data)
{
   DoublyConnectedEdgeList dcel(dcel_data);
   auto outs = Array<Polynomial<Rational, Int>>(dcel.getNumEdges());
   for (Int i = 0; i < dcel.getNumEdges(); i++){
      outs[i] = getDualOutitudePolynomial(dcel_data,i);
   }
   return outs;
}
UserFunction4perl("# @category Other"
                  "# Given a triangulation of a punctured surface this calculates all the outitude polynomials of the dual structure.\n" 
                  "# The first e = #{oriented edges} monomials correspond to A-coordinates of the oriented edges of the primal structure , labeled as in the input.\n" 
                  "# The last t = #{triangles} monomials correspond to A-coordinates of the triangles of the primal structure."
                  "# @param Matrix<Int> dcel_data the data for the doubly connected edge list representing the triangulation."
                  "# @return Array<Polynomial<Rational,Int>> an array containing the dual outitudes in order of the input.",
                  &dualOutitudePolynomials,"dualOutitudePolynomials( $ )");




Vector<Rational> outitudes_from_dcel(const DoublyConnectedEdgeList& dcel)
{
   Vector<Rational> out_vec(dcel.getNumEdges());
   for (Int i = 0; i < dcel.getNumEdges(); ++i) {
      const HalfEdge* e_edge = dcel.getHalfEdge(2*i);
      const HalfEdge* e_twin = e_edge->getTwin();
      const Rational& e_plus = e_edge->getLength();
      const Rational& e_minus = e_twin->getLength();
      const Rational& a = e_edge->getNext()->getLength();
      const Rational& b = e_edge->getPrev()->getTwin()->getLength();
      const Rational& c = e_twin->getNext()->getLength();
      const Rational& d = e_twin->getPrev()->getTwin()->getLength();
      const Rational& A = e_twin->getFace()->getDetCoord();
      const Rational& B = e_edge->getFace()->getDetCoord();
      out_vec[i] = A*(e_plus*a+e_minus*b-e_plus*e_minus) + B*(e_plus*d+e_minus*c-e_plus*e_minus);
   }
   return out_vec;
}


std::pair<Set<Int>, Set<Int>> is_canonical(const DoublyConnectedEdgeList& dcel)
{
   Set<Int> concave_edges;
   Set<Int> flat_edges;

   Vector<Rational> out_vec = outitudes_from_dcel(dcel);

   for (Int i = 0; i < out_vec.size(); ++i) {
      Rational out_i = out_vec[i];
      if (out_i < 0)
         concave_edges += i;
      else if (out_i == 0)
         flat_edges += i;
   }
   return { concave_edges, flat_edges };
}



Vector<Rational> flip_coords(DoublyConnectedEdgeList& dcel, Vector<Rational> coords, Int edge_id)
{
   Vector<Rational> new_coords(coords);
   Int e_plus_id = 2*edge_id;
   Int e_minus_id = 2*edge_id+1;
   const HalfEdge* halfEdge = dcel.getHalfEdge(e_plus_id);
   Int A_id = dcel.getFaceId(halfEdge->getFace());
   Int B_id = dcel.getFaceId(halfEdge->getTwin()->getFace());
   Int a_plus_id = dcel.getHalfEdgeId(halfEdge->getNext());
   Int a_minus_id = dcel.getHalfEdgeId(halfEdge->getNext()->getTwin());
   Int b_plus_id = dcel.getHalfEdgeId(halfEdge->getNext()->getNext());
   Int b_minus_id = dcel.getHalfEdgeId(halfEdge->getNext()->getNext()->getTwin());
   Int c_minus_id = dcel.getHalfEdgeId(halfEdge->getTwin()->getNext());
   Int c_plus_id = dcel.getHalfEdgeId(halfEdge->getTwin()->getNext()->getTwin());
   Int d_minus_id = dcel.getHalfEdgeId(halfEdge->getTwin()->getNext()->getNext());
   Int d_plus_id = dcel.getHalfEdgeId(halfEdge->getTwin()->getNext()->getNext()->getTwin());
    
   Rational C = (coords[A_id]*coords[c_minus_id] + coords[B_id]*coords[b_minus_id])/coords[e_plus_id];
   Rational D = (coords[A_id]*coords[d_plus_id] + coords[B_id]*coords[a_plus_id])/coords[e_minus_id];
   Rational f_plus = (C*coords[d_minus_id] + D*coords[c_plus_id])/coords[B_id];
   Rational f_minus = (C*coords[a_minus_id] + D*coords[b_plus_id])/coords[A_id];

   new_coords[e_plus_id] = f_plus;
   new_coords[e_minus_id] = f_minus;
   new_coords[A_id] = C;
   new_coords[B_id] = D;

	//halfEdge->setLength(f_plus);
	//dcel.getHalfEdge(e_minus_id).setLength(f_minus);
	//dcel.getFaces()[A_id].setDetCoord(C);
	//dcel.getFaces()[B_id].setDetCoord(D);
	return new_coords;
}



std::pair<flip_sequence, Set<Int>> flips_to_canonical_triangulation(const Matrix<Int>& dcel_data, Vector<Rational>& a_coords)
{  
   DoublyConnectedEdgeList dcel(dcel_data,a_coords);
   Vector<Rational> curr_a_coords(a_coords);

   flip_sequence flips;
   auto stuff = is_canonical(dcel);
   auto concave_edges = stuff.first;
   auto flat_edges = stuff.second;
   while (!concave_edges.empty()) {
      Int bad_edge = concave_edges.front();
      Vector<Rational> new_coords = flip_coords(dcel,curr_a_coords,bad_edge);
      dcel.flipEdgeWithFaces(bad_edge);
      flips.push_back(bad_edge);
      stuff = is_canonical(dcel);
      concave_edges = stuff.first;
      flat_edges = stuff.second;
   }
   return { flips, flat_edges };
}

UserFunction4perl("# @category Other\n"
                  "# Computes a flip sequence to a canonical triangulation (first list)."
                  "# The second output is a list of flat edges in the canonical triangulation."
                  "# @param Matrix<Int> DCEL_data"
                  "# @param Vector<Rational> A_coords"
                  "# @return Pair<List<Int>,Set<Int>>"
                  "# @example In the following example only edge 2 has negative outitude, so the flip sequence should start with 2. After performing this flip, the triangulation thus obtained is canonical."
                  "# > $T1 = new Matrix<Int>([[0,0,2,3,0,1],[0,0,4,5,0,1],[0,0,0,1,0,1]]);"
                  "# > print flips_to_canonical_triangulation($T1,[1,2,3,4,5,6,1,2]);"
                  "# | {2} {}",
                  &flips_to_canonical_triangulation,"flips_to_canonical_triangulation($$)");


Rational out(Matrix<Int> dcel_data, Vector<Rational> a_coords, Int edge_id)
{
   DoublyConnectedEdgeList dcel(dcel_data, a_coords);	
   const HalfEdge* e_edge = dcel.getHalfEdge(2*edge_id);
   const HalfEdge* e_twin = e_edge->getTwin();
   Rational e_plus = e_edge->getLength();
   Rational e_minus = e_twin->getLength();
   Rational a = e_edge->getNext()->getLength();
   Rational b = e_edge->getPrev()->getTwin()->getLength();
   Rational c = e_twin->getNext()->getLength();
   Rational d = e_twin->getPrev()->getTwin()->getLength();
   Rational A = e_twin->getFace()->getDetCoord();
   Rational B = e_edge->getFace()->getDetCoord();
   return A*(e_plus*a+e_minus*b-e_plus*e_minus) + B*(e_plus*d+e_minus*c-e_plus*e_minus);
}

Vector<Rational> outitudes(Matrix<Int> dcel_data, Vector<Rational> a_coords)
{
   Vector<Rational> out_vec(dcel_data.rows());
   for (Int i = 0; i < dcel_data.rows(); ++i) {
      out_vec[i] = out(dcel_data,a_coords, i);
   }
   return out_vec;
}

UserFunction4perl("# @category Producing other objects\n"
                  "# Computes the outitudes along edges."
                  "# @param Matrix<Int> DCEL_data"
                  "# @param Vector<Rational> A_coords"
                  "# @return Vector<Rational>"
                  "# @example In the following example only edge 2 has negative outitude."
                  "# > $T1 = new Matrix<Int>([[0,0,2,3,0,1],[0,0,4,5,0,1],[0,0,0,1,0,1]]);"
                  "# > print outitudes($T1,[1,2,3,4,5,6,1,2]);"
                  "# | 37 37 -5",
&outitudes, "outitudes($ $)");



} //end topaz namespace
} //end polymake namespace

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
