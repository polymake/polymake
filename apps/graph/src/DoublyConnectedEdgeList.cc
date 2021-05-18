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

namespace polymake { namespace graph {

Int DoublyConnectedEdgeList::getNumVert(const Array<Array<Int>>& half_edge_list)
{
   Int num_vertices = 0;
   for (const auto& half_edge : half_edge_list) {
      assign_max(num_vertices, half_edge[0]);
      assign_max(num_vertices, half_edge[1]);
   }
   return num_vertices+1;
}

Int DoublyConnectedEdgeList::getNumTriangs(const Array<Array<Int>>& half_edge_list)
{
   Int num_triangles = 0;
   for (const auto& half_edge : half_edge_list) {
      assign_max(num_triangles, half_edge[4]);
      assign_max(num_triangles, half_edge[5]);
   }
   return num_triangles+1;
}

DoublyConnectedEdgeList::DoublyConnectedEdgeList(const Array<Array<Int>>& half_edge_list)
   : with_faces(false)
{
   const Int num_edges = half_edge_list.size();
   const Int num_vertices = getNumVert(half_edge_list);    
   const Int num_faces = 2*num_edges/3;
   vertices.resize(num_vertices);
   edges.resize(num_edges*2);
   faces.resize(num_faces);

   Int i = 0;
   for (const auto& half_edge : half_edge_list) {
      setEdgeIncidences(i, half_edge[0], half_edge[1], half_edge[2], half_edge[3]);
      if (half_edge.size() == 6) {
         setFaceIncidences(i, half_edge[4], half_edge[5]);
         with_faces = true;
      }
      ++i;
   }
}

DoublyConnectedEdgeList::DoublyConnectedEdgeList(const Array<Array<Int>>& half_edge_list, const Vector<Rational>& coords)
      : DoublyConnectedEdgeList(half_edge_list)
{
   if (half_edge_list[0].size() == 4)
      setMetric(coords);
   if (half_edge_list[0].size() == 6)
      setAcoords(coords);
}

void DoublyConnectedEdgeList::setEdgeIncidences(const Int halfEdgeId, const Int headId, const Int twinHeadId, const Int nextId, const Int twinNextId)
{
   HalfEdge& halfEdge = edges[2*halfEdgeId];
   halfEdge.setHead(getVertex(headId));
   halfEdge.setNext(getHalfEdge(nextId));

   HalfEdge& twinHalfEdge = edges[2*halfEdgeId+1];
   twinHalfEdge.setHead(getVertex(twinHeadId));
   twinHalfEdge.setNext(getHalfEdge(twinNextId));

   halfEdge.setTwin(&twinHalfEdge);
}

void DoublyConnectedEdgeList::setFaceIncidences(const Int half_edge_id, const Int face_id, const Int twin_face_id)
{
   Face& face = faces[face_id];
   Face& twin_face = faces[twin_face_id];
   HalfEdge& half_edge = edges[2*half_edge_id];
   HalfEdge& twin_half_edge = edges[2*half_edge_id+1];

   face.setHalfEdge(&half_edge);
   twin_face.setHalfEdge(&twin_half_edge);
   half_edge.setFace(getFace(face_id));
   twin_half_edge.setFace(getFace(twin_face_id));
}

SparseMatrix<Int> DoublyConnectedEdgeList::EdgeVertexIncidenceMatrix() const
{
   const Int numEdges = getNumEdges();
   SparseMatrix<Int> M(numEdges, vertices.size());

   for (Int i = 0 ; i < numEdges; ++i) {
      const HalfEdge& halfEdge = edges[2*i];
      M(i, getVertexId(halfEdge.getHead())) = 1;
      M(i, getVertexId(halfEdge.getTwin()->getHead())) = 1;
   }

   return M;
}

bool DoublyConnectedEdgeList::isFlippable(const Int edgeId) const
{
   const HalfEdge* halfEdge = &edges[2*edgeId];
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
   HalfEdge* halfEdge = &edges[2*edge_id];
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
   HalfEdge* halfEdge = &edges[2*edgeId];
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
   HalfEdge* halfEdge = &edges[2*edgeId];
   if (halfEdge != halfEdge->getNext()
       && halfEdge != halfEdge->getNext()->getNext()
       && halfEdge != halfEdge->getNext()->getTwin()
       && halfEdge != halfEdge->getNext()->getNext()->getTwin())
      unflipHalfEdge(halfEdge);
}

std::array<Int, 8> DoublyConnectedEdgeList::getQuadId(const Int id) const
{
   const HalfEdge* halfEdge = &edges[id];

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
      edges[2*i].setLength(metric[i]);
      edges[2*i+1].setLength(metric[i]);
   }
}

void DoublyConnectedEdgeList::setAcoords(const Vector<Rational>& acoords)
{
   const Int numHalfEdges = getNumHalfEdges();
   const Int numFaces = getNumFaces();
   for (Int i = 0; i < numHalfEdges; ++i) {
      edges[i].setLength(acoords[i]);
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
      metric[i] = edges[2*i].getLength();
   return metric;
}

Matrix<Rational> DoublyConnectedEdgeList::DelaunayInequalities() const
{
   const Int numVertices = getNumVertices();
   const Int numEdges = getNumEdges();
   Matrix<Rational> M(numEdges+numVertices, numVertices+1);
   for (Int a = 0; a < numEdges; ++a) {
      const auto quadId = getQuadId(2*a);

      const Rational& ik = edges[2*a].getLength();
      const Rational& kl = edges[quadId[5]].getLength();
      const Rational& il = edges[quadId[7]].getLength();
      const Rational& ij = edges[quadId[1]].getLength();
      const Rational& jk = edges[quadId[3]].getLength();

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

   const Rational& ik = edges[2 * id].getLength();
   const Rational& kl = edges[quadId[5]].getLength();
   const Rational& il = edges[quadId[7]].getLength();
   const Rational& ij = edges[quadId[1]].getLength();
   const Rational& jk = edges[quadId[3]].getLength();
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

      const Rational& ik = edges[2 * id].getLength();
      const Rational& kl = edges[quadId[5]].getLength();
      const Rational& il = edges[quadId[7]].getLength();
      const Rational& ij = edges[quadId[1]].getLength();
      const Rational& jk = edges[quadId[3]].getLength();
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
   const Vertex* v = getVertex(id);
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
      const HalfEdge& halfEdge = edges[i];
      triangle_map[i] = numHalfEdges + getFaceId(halfEdge.getFace());
   }
   return triangle_map;
}

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
