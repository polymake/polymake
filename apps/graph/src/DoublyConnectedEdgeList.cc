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

#include "polymake/graph/DoublyConnectedEdgeList.h"
#include "polymake/Map.h"
#include "polymake/Set.h"
#include "polymake/pair.h"

namespace polymake { 
namespace graph {
namespace dcel {

Int DoublyConnectedEdgeList::getNumVert(const Matrix<Int>& half_edge_list)
{
   Int num_vertices = 0;
   for (const auto& half_edge : rows(half_edge_list)) {
      assign_max(num_vertices, half_edge[0]);
      assign_max(num_vertices, half_edge[1]);
   }
   return num_vertices+1;
}

Int DoublyConnectedEdgeList::getNumTriangs(const Matrix<Int>& half_edge_list)
{
   Int num_triangles = 0;
   for (const auto& half_edge : rows(half_edge_list)) {
      assign_max(num_triangles, half_edge[4]);
      assign_max(num_triangles, half_edge[5]);
   }
   return num_triangles+1;
}

void DoublyConnectedEdgeList::verifyHalfedge(Int& counter, const std::pair<Int, Int>& key, Map<std::pair<Int, Int>, Int>& existing_edges){
   if(!existing_edges.exists(key)){
      std::pair<Int, Int> twin_key(key.second, key.first);
      existing_edges[key] = counter++;
      existing_edges[twin_key] = counter++;
      halfedges[counter-2].setTwin(&halfedges[counter-1]);
      halfedges[counter-2].setHead(&vertices[key.second]);
      halfedges[counter-1].setHead(&vertices[twin_key.second]);
   }
}

void DoublyConnectedEdgeList::insert_container(){
   for(auto& halfedge : halfedges){
      halfedge.setContainer(this);
   }
   for(auto& vertex : vertices){
      vertex.setContainer(this);
   }
   if(with_faces){
      for(auto& face : faces){
         face.setContainer(this);
      }
   }
}


DoublyConnectedEdgeList::DoublyConnectedEdgeList(const Array<Array<Int>>& vif_cyclic_normals) : with_faces(true)
{
   // Prune these
   Int n_halfedges = 0;
   Set<Int> vertices_tmp;
   for(const auto& cycle : vif_cyclic_normals){
      n_halfedges += cycle.size();
      vertices_tmp += Set<Int>(cycle);
   }
   Int n_vertices = vertices_tmp.size();
   Int n_facets = vif_cyclic_normals.size();

   resize(n_vertices, n_halfedges, n_facets);

   Map<std::pair<Int, Int>, Int> halfedge_indices;
   Int halfedge_counter = 0;
   for(Int i = 0; i<n_facets; i++){
      const Array<Int>& cycle = vif_cyclic_normals[i];
      for(Int j = 0; j<cycle.size(); j++){
         Int a = cycle[(j-1+cycle.size()) % cycle.size()];
         Int b = cycle[j % cycle.size()];
         Int c = cycle[(j+1) % cycle.size()];
         Int d = cycle[(j+2) % cycle.size()];
         
         std::pair<Int, Int> prev_key(a,b);
         std::pair<Int, Int> key(b,c);
         std::pair<Int, Int> next_key(c,d);
         std::pair<Int, Int> twin_key(c,b);
         verifyHalfedge(halfedge_counter, prev_key, halfedge_indices);
         verifyHalfedge(halfedge_counter, key, halfedge_indices);
         verifyHalfedge(halfedge_counter, next_key, halfedge_indices);
         verifyHalfedge(halfedge_counter, twin_key, halfedge_indices);
         halfedges[halfedge_indices[key]].setPrev(&halfedges[halfedge_indices[prev_key]]);
         halfedges[halfedge_indices[key]].setNext(&halfedges[halfedge_indices[next_key]]);
         halfedges[halfedge_indices[key]].setFace(&faces[i]);
         if(j == cycle.size()-1){
            faces[i].setHalfEdge(&halfedges[halfedge_indices[next_key]]);
         }
      }
   }

}


DoublyConnectedEdgeList::DoublyConnectedEdgeList(const Matrix<Int>& half_edge_list)
   : with_faces(false)
{
   matrix_representation = half_edge_list;
   resize();
   populate();
}

void DoublyConnectedEdgeList::populate(){
   populate(matrix_representation);
}

void DoublyConnectedEdgeList::resize(){
   const Int num_edges = matrix_representation.rows();
   const Int num_vertices = getNumVert(matrix_representation);    
   if(matrix_representation.cols() == 6){
      Set<Int> tmp;
      for (const auto& half_edge : rows(matrix_representation)) {
         tmp += half_edge[4];
         tmp += half_edge[5];
      }
      if(tmp != sequence(0,tmp.size())) throw std::runtime_error("Faces are not labelled consequetively");
      resize(num_vertices, num_edges*2, tmp.size());
   } else {
      resize(num_vertices, num_edges*2);
   }
}

void DoublyConnectedEdgeList::resize(Int nvert, Int nhe, Int nfaces){
   vertices.resize(nvert);
   halfedges.resize(nhe);
   faces.resize(nfaces);
   with_faces = true;
   insert_container();
}

void DoublyConnectedEdgeList::resize(Int nvert, Int nhe){
   vertices.resize(nvert);
   halfedges.resize(nhe);
   insert_container();
}

void DoublyConnectedEdgeList::populate(const Matrix<Int>& half_edge_list){
   if(half_edge_list.rows() == 0){
      return;
   }

   Int i = 0;
   for (const auto& half_edge : rows(half_edge_list)) {
      setEdgeIncidences(i, half_edge[0], half_edge[1], half_edge[2], half_edge[3]);
      if (half_edge.size() == 6) {
         setFaceIncidences(i, half_edge[4], half_edge[5]);
      }
      ++i;
   }
}

DoublyConnectedEdgeList::DoublyConnectedEdgeList(const Matrix<Int>& half_edge_list, const Vector<Rational>& coords)
      : DoublyConnectedEdgeList(half_edge_list)
{
   if (half_edge_list[0].size() == 4)
      setMetric(coords);
   if (half_edge_list[0].size() == 6)
      setAcoords(coords);
}

const Matrix<Int>& DoublyConnectedEdgeList::toMatrixInt() const {
   // Do the opposite of the constructor
   Int n = getNumHalfEdges();
   Int nrows = n/2;
   Int ncols = with_faces ? 6 : 4;
   matrix_representation = Matrix<Int>(nrows, ncols);
   for(Int i=0; i<nrows; i++){
      // Do the opposite of DoublyConnectedEdgeList::setEdgeIncidences
      const HalfEdge* halfEdge = &halfedges[2*i];
      matrix_representation(i,0) = halfEdge->getHead()->getID();
      matrix_representation(i,1) = halfEdge->getTwin()->getHead()->getID();
      matrix_representation(i,2) = halfEdge->getNext()->getID();
      matrix_representation(i,3) = halfEdge->getTwin()->getNext()->getID();
      if(with_faces){
         matrix_representation(i,4) = halfEdge->getFace()->getID();
         matrix_representation(i,5) = halfEdge->getTwin()->getFace()->getID();
      }
   }
   return matrix_representation;
}

Array<Array<Int>> DoublyConnectedEdgeList::faces_as_cycles() const {
   if(!with_faces) throw std::runtime_error("This is not a DCEL with faces.");
   Int n = getNumFaces();
   Array<Array<Int>> result(n);
   for(Int i=0; i<n; i++){
      const Face& face = faces[i];
      std::vector<Int> tmp;
      HalfEdge* he = face.getHalfEdge();
      Int first = he->getPrev()->getHead()->getID();
      tmp.push_back(first);
      Int current_id = he->getHead()->getID();
      while(first != current_id){
         tmp.push_back(current_id);
         if(he->getNext() != nullptr){
            he = he->getNext();
         } else {
            throw std::runtime_error("No cycle around face in DCEL");
         }
         current_id = he->getHead()->getID();
      }
      result[i] = Array<Int>(tmp.begin(), tmp.end());
   }
   return result;
}


void DoublyConnectedEdgeList::setEdgeIncidences(const Int halfEdgeId, const Int headId, const Int twinHeadId, const Int nextId, const Int twinNextId)
{
   HalfEdge& halfEdge = halfedges[2*halfEdgeId];
   halfEdge.setHead(&vertices[headId]);
   halfEdge.setNext(&halfedges[nextId]);

   HalfEdge& twinHalfEdge = halfedges[2*halfEdgeId+1];
   twinHalfEdge.setHead(&vertices[twinHeadId]);
   twinHalfEdge.setNext(&halfedges[twinNextId]);

   halfEdge.setTwin(&twinHalfEdge);
}

void DoublyConnectedEdgeList::setFaceIncidences(const Int half_edge_id, const Int face_id, const Int twin_face_id)
{
   Face& face = faces[face_id];
   Face& twin_face = faces[twin_face_id];
   HalfEdge& half_edge = halfedges[2*half_edge_id];
   HalfEdge& twin_half_edge = halfedges[2*half_edge_id+1];

   face.setHalfEdge(&half_edge);
   twin_face.setHalfEdge(&twin_half_edge);
   half_edge.setFace(&faces[face_id]);
   twin_half_edge.setFace(&faces[twin_face_id]);
}

SparseMatrix<Int> DoublyConnectedEdgeList::EdgeVertexIncidenceMatrix() const
{
   const Int numEdges = getNumEdges();
   SparseMatrix<Int> M(numEdges, vertices.size());

   for (Int i = 0 ; i < numEdges; ++i) {
      const HalfEdge& halfEdge = halfedges[2*i];
      M(i, getVertexId(halfEdge.getHead())) = 1;
      M(i, getVertexId(halfEdge.getTwin()->getHead())) = 1;
   }

   return M;
}

bool DoublyConnectedEdgeList::isFlippable(const Int edgeId) const
{
   const HalfEdge* halfEdge = &halfedges[2*edgeId];
   return halfEdge != halfEdge->getNext()
       && halfEdge != halfEdge->getNext()->getNext()
       && halfEdge != halfEdge->getNext()->getTwin()
       && halfEdge != halfEdge->getNext()->getNext()->getTwin();
}

void DoublyConnectedEdgeList::flipHalfEdge(HalfEdge* const halfEdge)
{
   HalfEdge* twin = halfEdge->getTwin();
   HalfEdge* a = halfEdge->getNext();
   HalfEdge* b = a->getNext();
   HalfEdge* c = twin->getNext();
   HalfEdge* d = c->getNext();

   if (halfEdge->getHead()->getIncidentEdge() == halfEdge)
      halfEdge->getHead()->setIncidentEdge(d);
   if (twin->getHead()->getIncidentEdge() == twin)
      twin->getHead()->setIncidentEdge(b);

   const Rational newLength = ((a->getLength() * c->getLength()) + (b->getLength() * d->getLength())) / halfEdge->getLength();
   halfEdge->setLength(newLength);
   twin->setLength(newLength);
   halfEdge->setHead(a->getHead());
   halfEdge->setNext(b);
   b->setNext(c);
   c->setNext(halfEdge);
   twin->setHead(c->getHead());
   twin->setNext(d);
   d->setNext(a);
   a->setNext(twin);
}

void DoublyConnectedEdgeList::flipEdgeWithFaces(const Int edge_id)
{
   HalfEdge* halfEdge = &halfedges[2*edge_id];
   HalfEdge* twin = halfEdge->getTwin();
   HalfEdge* a_edge = halfEdge->getNext();
   HalfEdge* b_edge = a_edge->getNext();
   HalfEdge* c_edge = twin->getNext();
   HalfEdge* d_edge = c_edge->getNext();
   Face* A_face = halfEdge->getFace();
   Face* B_face = twin->getFace();
   const Rational& A = A_face->getDetCoord();
   const Rational& B = B_face->getDetCoord();
   const Rational& a_plus = a_edge->getLength();
   const Rational& a_minus = a_edge->getTwin()->getLength();
   const Rational& b_plus = b_edge->getLength();
   const Rational& b_minus = b_edge->getTwin()->getLength();
   const Rational& c_minus = c_edge->getLength();
   const Rational& c_plus = c_edge->getTwin()->getLength();
   const Rational& d_minus = d_edge->getLength();
   const Rational& d_plus = d_edge->getTwin()->getLength();
   const Rational& e_plus = halfEdge->getLength();
   const Rational& e_minus = twin->getLength();

   if (halfEdge->getHead()->getIncidentEdge() == halfEdge)
      halfEdge->getHead()->setIncidentEdge(d_edge);
   if (twin->getHead()->getIncidentEdge() == twin)
      twin->getHead()->setIncidentEdge(b_edge);

   A_face->setHalfEdge(halfEdge);
   B_face->setHalfEdge(twin);
   a_edge->setFace(B_face);
   c_edge->setFace(A_face);
   halfEdge->setHead(a_edge->getHead());
   halfEdge->setNext(b_edge);
   b_edge->setNext(c_edge);
   c_edge->setNext(halfEdge);
   twin->setHead(c_edge->getHead());
   twin->setNext(d_edge);
   d_edge->setNext(a_edge);
   a_edge->setNext(twin);

   const Rational C = ( A*c_minus + B*b_minus ) / e_plus;
   const Rational D = ( A*d_plus + B*a_plus ) / e_minus;
   const Rational f_plus = ( C*d_minus + D*c_plus ) / B;
   const Rational f_minus = ( C*a_minus + D*b_plus ) / A;

   halfEdge->setLength(f_plus);
   twin->setLength(f_minus);
   A_face->setDetCoord(C);
   B_face->setDetCoord(D);
}

void DoublyConnectedEdgeList::flipEdge(const Int edgeId)
{
   HalfEdge* halfEdge = &halfedges[2*edgeId];
   if (halfEdge != halfEdge->getNext()
       && halfEdge != halfEdge->getNext()->getNext()
       && halfEdge != halfEdge->getNext()->getTwin()
       && halfEdge != halfEdge->getNext()->getNext()->getTwin())
      flipHalfEdge(halfEdge);
}

void DoublyConnectedEdgeList::unflipHalfEdge(HalfEdge* const halfEdge)
{
   HalfEdge* twin = halfEdge->getTwin();
   HalfEdge* a = halfEdge->getNext();
   HalfEdge* b = a->getNext();
   HalfEdge* c = twin->getNext();
   HalfEdge* d = c->getNext();

   if (halfEdge->getHead()->getIncidentEdge() == halfEdge)
      halfEdge->getHead()->setIncidentEdge(d);
   if (twin->getHead()->getIncidentEdge() == twin)
      twin->getHead()->setIncidentEdge(b);

   const Rational newLength = ((a->getLength() * c->getLength()) + (b->getLength() * d->getLength())) / halfEdge->getLength();
   halfEdge->setLength(newLength);
   twin->setLength(newLength);

   halfEdge->setHead(c->getHead());
   halfEdge->setNext(d);
   d->setNext(a);
   a->setNext(halfEdge);

   twin->setHead(a->getHead());
   twin->setNext(b);
   b->setNext(c);
   c->setNext(twin);
}

void DoublyConnectedEdgeList::unflipEdge(const Int edgeId)
{
   HalfEdge* halfEdge = &halfedges[2*edgeId];
   if (halfEdge != halfEdge->getNext()
       && halfEdge != halfEdge->getNext()->getNext()
       && halfEdge != halfEdge->getNext()->getTwin()
       && halfEdge != halfEdge->getNext()->getNext()->getTwin())
      unflipHalfEdge(halfEdge);
}

std::array<Int, 8> DoublyConnectedEdgeList::getQuadId(const Int id) const
{
   const HalfEdge* halfEdge = &halfedges[id];

   Int kl = getHalfEdgeId(halfEdge->getNext());
   Int il = getHalfEdgeId(halfEdge->getNext()->getNext());
   Int ij = getHalfEdgeId(halfEdge->getTwin()->getNext());
   Int jk = getHalfEdgeId(halfEdge->getTwin()->getNext()->getNext());

   Int i = getVertexId(halfEdge->getTwin()->getHead());
   Int j = getVertexId(halfEdge->getTwin()->getNext()->getHead());
   Int k = getVertexId(halfEdge->getHead());
   Int l = getVertexId(halfEdge->getNext()->getHead());

   return {{i, ij, j, jk, k, kl, l, il}};
}

void DoublyConnectedEdgeList::setMetric(const Vector<Rational>& metric)
{
   for (Int i = 0, end = getNumEdges(); i < end; ++i) {
      halfedges[2*i].setLength(metric[i]);
      halfedges[2*i+1].setLength(metric[i]);
   }
}

void DoublyConnectedEdgeList::setAcoords(const Vector<Rational>& acoords)
{
   const Int numHalfEdges = getNumHalfEdges();
   const Int numFaces = getNumFaces();
   for (Int i = 0; i < numHalfEdges; ++i) {
      halfedges[i].setLength(acoords[i]);
   }
   for (Int j = 0; j < numFaces; ++j) {
      faces[j].setDetCoord(acoords[numHalfEdges+j]);
   }
}

Vector<Rational> DoublyConnectedEdgeList::edgeLengths() const
{
   const Int numEdges = getNumEdges();
   Vector<Rational> metric(numEdges);
   for (Int i = 0; i < numEdges ; ++i)
      metric[i] = halfedges[2*i].getLength();
   return metric;
}

Matrix<Rational> DoublyConnectedEdgeList::DelaunayInequalities() const
{
   const Int numVertices = getNumVertices();
   const Int numEdges = getNumEdges();
   Matrix<Rational> M(numEdges+numVertices, numVertices+1);
   for (Int a = 0; a < numEdges; ++a) {
      const auto quadId = getQuadId(2*a);

      const Rational& ik = halfedges[2*a].getLength();
      const Rational& kl = halfedges[quadId[5]].getLength();
      const Rational& il = halfedges[quadId[7]].getLength();
      const Rational& ij = halfedges[quadId[1]].getLength();
      const Rational& jk = halfedges[quadId[3]].getLength();

      const Int i = quadId[0];
      const Int j = quadId[2];
      const Int k = quadId[4];
      const Int l = quadId[6];

      M(a, i+1) += kl/(il*ik) + jk/(ij*ik);
      M(a, k+1) += il/(ik*kl) + ij/(ik*jk);
      M(a, j+1) += -ik/(ij*jk);
      M(a, l+1) += -ik/(il*kl);
   }

   for (Int j = 0; j < numVertices; ++j) {
      M(numEdges+j, j+1) = 1;
   }
   return remove_zero_rows(M);
}

Array<DoublyConnectedEdgeList::flip_sequence> DoublyConnectedEdgeList::flippableEdges(const flip_sequence& list_arg) const
{
   const Matrix<Rational> M = DelaunayInequalities();
   BigObject p("polytope::Polytope<Rational>", "INEQUALITIES", M);
   const Matrix<Rational> facets = p.give("FACETS");

   const Int numFacets = facets.rows()-1;  // -1 for the far face [1:0: ... :0]

   Array<flip_sequence> flipList(numFacets);

   for (Int i = 0; i < numFacets; ++i) {               
      // let's hope the far facet is always the last one...
      if (validFacet(facets.row(i))) {
         flip_sequence active_edges_at_facet_i = list_arg;

         for (Int j = 0; j < M.rows(); ++j) {
            if (is_equiv(M.row(j), facets.row(i)))
               active_edges_at_facet_i.push_back(j);
         }

         flipList[i] = active_edges_at_facet_i;
      }
   }

   return flipList;
}

Matrix<Rational> DoublyConnectedEdgeList::coneFacets() const
{
   BigObject p("polytope::Polytope<Rational>", "INEQUALITIES", DelaunayInequalities());
   return p.give("FACETS");
}

Set<Vector<Rational>> DoublyConnectedEdgeList::coneRays() const
{
   Set<Vector<Rational>> ray_set;
   BigObject p("polytope::Polytope<Rational>", "INEQUALITIES", DelaunayInequalities());
   Matrix<Rational> rays = p.give("VERTICES");

   for (auto&& row : rows(rays))
      ray_set += normalize(row);

   return ray_set;
}

DoublyConnectedEdgeList::flip_sequence DoublyConnectedEdgeList::flipEdges_and_give_flips(const flip_sequence& edgeIds, flip_sequence former_flips, bool reverse)
{
   if (!reverse) {
      for (const Int edgeId : edgeIds) {
         flipEdge(edgeId);
         former_flips.push_back(edgeId);
      }
   } else {
      for (auto rit = edgeIds.rbegin(), rend = edgeIds.rend(); rit != rend; ++rit) {
         unflipEdge(*rit);
         former_flips.push_back(*rit);
      }
   }
   return former_flips;
}

void DoublyConnectedEdgeList::flipEdges(const flip_sequence& edgeIds, bool reverse)
{
   if (!reverse) {
      for (const Int edgeId : edgeIds) {
         flipEdge(edgeId);
      }
   } else {
      for (auto rit = edgeIds.rbegin(), rend = edgeIds.rend(); rit != rend; ++rit) {
         unflipEdge(*rit);
      }
   }
}

DoublyConnectedEdgeList::flip_sequence DoublyConnectedEdgeList::flipThroughFace(const Vector<Rational>& facet_normal, flip_sequence former_flips)
{
   if (validFacet(facet_normal)) {
      // we perform at most this many flips to get through this facet
      Int upper_flip_bound = 10 * facet_normal.size();

      Int row_id = first_equiv_row(facet_normal);
      Int counter = 0;

      while (row_id != -1) {
         // this does not change the actual former_flips, we work on a copy instead
         former_flips.push_back(row_id);
         flipEdge(row_id);
         ++counter;
         row_id = first_equiv_row(facet_normal);
         if (counter > upper_flip_bound) {
            row_id = -1;
            cout << "DoublyConnectedEdgeList->FlipThroughFace:" <<
                    "suggested number of flips exceeded 'upper flip bound'" << endl;
         }
      }
      const Vector<Rational> neighbor_facet = -facet_normal;
      if (first_equiv_row(neighbor_facet) == -1)
         cout << "DoublyConnectedEdgeList->FlipThroughFace: new cone is not facet-neighbor" << endl;
      return former_flips;
   }
   flip_sequence no_flips = flip_sequence();
   return no_flips;
}

Int DoublyConnectedEdgeList::first_equiv_row(const Vector<Rational>& ineq) const
{
   for (auto it = entire<indexed>(rows(DelaunayInequalities())); !it.at_end(); ++it) {
      if (is_equiv(ineq, *it))
         return it.index();
   }
   return -1;
}

bool DoublyConnectedEdgeList::is_equiv(const Vector<Rational>& ineq_a, const Vector<Rational>& ineq_b) const
{
   if (rank(vector2row(ineq_a) / ineq_b) == 1) {
      for (Int i = 0 ; i < ineq_a.dim(); ++i) {
         if (ineq_a[i] != 0)
            return ineq_b[i]/ineq_a[i] > 0;
      }
   }
   return false;
}

bool DoublyConnectedEdgeList::is_Delaunay(const Int id, const Vector<Rational>& weights) const
{
   const auto quadId = getQuadId(2 * id);

   const Rational& ik = halfedges[2 * id].getLength();
   const Rational& kl = halfedges[quadId[5]].getLength();
   const Rational& il = halfedges[quadId[7]].getLength();
   const Rational& ij = halfedges[quadId[1]].getLength();
   const Rational& jk = halfedges[quadId[3]].getLength();
   const Int i = quadId[0];
   const Int j = quadId[2];
   const Int k = quadId[4];
   const Int l = quadId[6];

   // the +1 is because wstill have this 0 at the 0th position of every ray...
   return weights[i+1] * (kl/(il*ik) + jk/(ij*ik)) + weights[k+1] * ( il/(ik*kl) + ij/(ik*jk) )
       >= weights[j+1] * (ik/(ij*jk)) + weights[l+1]*(ik/(il*kl));
}

Int DoublyConnectedEdgeList::is_Delaunay(const Vector<Rational>& weights) const
{
   for (Int i = 0, end = getNumEdges(); i < end; ++i) {
      if (!is_Delaunay(i, weights))
         return i;
   }
   return -1;
}

Vector<Int> DoublyConnectedEdgeList::DelaunayConditions(const Vector<Rational>& weights) const
{
   const Int numEdges = getNumEdges();
   Vector<Int> condition_vector(numEdges);
   for (Int id = 0 ; id < numEdges; ++id) {
      const auto quadId = getQuadId(2*id);

      const Rational& ik = halfedges[2 * id].getLength();
      const Rational& kl = halfedges[quadId[5]].getLength();
      const Rational& il = halfedges[quadId[7]].getLength();
      const Rational& ij = halfedges[quadId[1]].getLength();
      const Rational& jk = halfedges[quadId[3]].getLength();
      const Int i = quadId[0];
      const Int j = quadId[2];
      const Int k = quadId[4];
      const Int l = quadId[6];
      const Rational left = weights[i+1] * (kl/(il*ik) + jk/(ij*ik)) + weights[k+1] * (il/(ik*kl) + ij/(ik*jk));
      const Rational right = weights[j+1] * (ik/(ij*jk)) + weights[l+1] * (ik/(il*kl));
      if (left > right)
         condition_vector[id] = 1;
      else if (left == right)
         condition_vector[id] = 0;
      else
         condition_vector[id] = -1;
   }
   return condition_vector;
}

DoublyConnectedEdgeList::flip_sequence DoublyConnectedEdgeList::flipToDelaunayAlt(const Vector<Rational>& weights)
{
   flip_sequence flip_ids{};
   Int non_delaunay = is_Delaunay(weights);
   while (non_delaunay != -1) {
      flipEdge(non_delaunay);
      flip_ids.push_back(non_delaunay);
      non_delaunay = is_Delaunay(weights);
   }
   return flip_ids;
}

Rational DoublyConnectedEdgeList::angleSum(const Int id) const
{
   const Vertex* v = &vertices[id];
   HalfEdge* e = v->getIncidentEdge();
   HalfEdge* a = e->getTwin();
   HalfEdge* b = a->getNext();
   HalfEdge* c = b->getNext();
   Rational sum = b->getLength() / (a->getLength() * c->getLength());
   while (getHalfEdgeId(c) != getHalfEdgeId(e)) {
      a = c->getTwin();
      b = a->getNext();
      c = b->getNext();
      sum += b->getLength() / (a->getLength() * c->getLength());
   }
   return sum;
}

Vector<Rational> DoublyConnectedEdgeList::angleVector() const
{
   const Int numVert = vertices.size();
   Vector<Rational> angleVec(numVert);
   for (Int i = 0; i < numVert; ++i) {
      angleVec[i] = angleSum(i);
   }
   return angleVec;
}

const Map<Int, Int> DoublyConnectedEdgeList::triangleMap() const
{
   Map<Int, Int> triangle_map;
   const Int numHalfEdges = getNumHalfEdges();
   for (Int i = 0; i < numHalfEdges; ++i) {
      const HalfEdge& halfEdge = halfedges[i];
      triangle_map[i] = numHalfEdges + getFaceId(halfEdge.getFace());
   }
   return triangle_map;
}

DoublyConnectedEdgeList DoublyConnectedEdgeList::dual() const
{
   if(!with_faces) throw std::runtime_error("Only DCELs with faces can be dualized");
   DoublyConnectedEdgeList result;
   result.resize(faces.size(), halfedges.size(), vertices.size());
   Map<std::pair<Int, Int>, Int> halfedge_indices;
   Int halfedge_counter = 0;
   for(const auto& vertex: vertices){
      HalfEdge* current_edge = vertex.getIncidentEdge();
      Face& current_face = result.faces[vertex.getID()];
      std::pair<Int, Int> start_key(current_edge->getFace()->getID(), current_edge->getTwin()->getFace()->getID());
      std::pair<Int, Int> key(start_key), prev_key(start_key);
      result.verifyHalfedge(halfedge_counter, key, halfedge_indices);
      current_face.setHalfEdge(&result.halfedges[halfedge_indices[key]]);
      do {
         std::pair<Int, Int> twin_key(key.second, key.first);
         current_edge = current_edge->getTwin()->getPrev();
         prev_key = key;
         key = std::pair<Int, Int>(current_edge->getFace()->getID(), current_edge->getTwin()->getFace()->getID());
         result.verifyHalfedge(halfedge_counter, key, halfedge_indices);
         result.halfedges[halfedge_indices[key]].setFace(&current_face);
         result.connect_halfedges(&result.halfedges[halfedge_indices[prev_key]], &result.halfedges[halfedge_indices[key]]);
      } while (start_key != key);
   }
   return result;
}

} // namespace dcel
} // namespace graph 
} // namespace polytope

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
