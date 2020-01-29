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

#ifndef POLYMAKE_GRAPH_DOUBLY_CONNECTED_EDGE_LIST_H
#define POLYMAKE_GRAPH_DOUBLY_CONNECTED_EDGE_LIST_H

#include "polymake/client.h"
#include "polymake/Vector.h"
#include "polymake/Array.h"
#include <vector>
#include "polymake/Matrix.h"
#include "polymake/Integer.h"
#include "polymake/Rational.h"
#include "polymake/SparseMatrix.h"
#include "polymake/Set.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/linalg.h"

namespace polymake { namespace graph {

class HalfEdge;
class Face;

class Vertex {
private:
   HalfEdge* incidentEdge;
   Face* face;

public:
   Vertex()
      : incidentEdge(nullptr)
      , face(nullptr) {};

   HalfEdge* getIncidentEdge() const
   {
      return this->incidentEdge;
   }

   void setIncidentEdge(HalfEdge* edge)
   {
      this->incidentEdge = edge;
   }

   Face* getFace() const
   {
      return this->face;
   }

   void setFace(Face* new_face)
   {
      this->face = new_face;
   }
}; // end class Vertex

class Face {
private:
   HalfEdge* half_edge;
   Vertex* vertex;
   Rational det_coord;

public:
   Face()
      : half_edge(nullptr)
      , vertex(nullptr) {}

   HalfEdge* getHalfEdge() const
   {
      return this->half_edge;
   }

   void setHalfEdge(HalfEdge* edge)
   {
      this->half_edge = edge;
   }

   bool operator==(const Face& other) const
   {
      return half_edge == other.half_edge && vertex == other.vertex;
   }

   const Rational& getDetCoord() const
   {
      return det_coord;
   }

   void setDetCoord(const Rational& new_det_coord)
   {
      det_coord = new_det_coord;
   }
}; // end class Face

class HalfEdge {
protected:
   HalfEdge* twin;
   HalfEdge* next;
   HalfEdge* prev;
   Vertex* head;
   Face* face;
   Rational length;

public:
   HalfEdge()
      : twin(nullptr)
      , next(nullptr)
      , prev(nullptr)
      , head(nullptr)
      , face(nullptr)
      , length(1) {}

   bool operator==(const HalfEdge& other) const
   {
      return twin == other.twin && next == other.next;
   }

   const Rational& getLength() const
   {
      return length;
   }

   void setLength(const Rational& newLength)
   {
      length = newLength;
   }

   HalfEdge* getTwin()
   {
      return twin;
   }
   const HalfEdge* getTwin() const
   {
      return twin;
   }

   void setTwin(HalfEdge* newTwin)
   {
      twin = newTwin;
      newTwin->twin = this;
   }

   HalfEdge* getNext()
   {
      return next;
   }

   const HalfEdge* getNext() const
   {
      return next;
   }

   void setNext(HalfEdge* newNext)
   {
      next = newNext;
      newNext->prev = this;
   }

   HalfEdge* getPrev()
   {
      return prev;
   }

   const HalfEdge* getPrev() const
   {
      return prev;
   }

   void setPrev(HalfEdge* newPrev)
   {
      prev = newPrev;
      newPrev->next = this;
   }

   Vertex* getHead()
   {
      return head;
   }

   const Vertex* getHead() const
   {
      return head;
   }

   void setHead(Vertex* newHead)
   {
      head = newHead;
      newHead->setIncidentEdge(this);
   }

   void setFace(Face* newFace)
   {
      face = newFace;
      newFace->setHalfEdge(this);
   }

   Face* getFace()
   {
      return face;
   }
   const Face* getFace() const
   {
      return face;
   }
}; // end class HalfEdge

class DoublyConnectedEdgeList {
private:
   static constexpr Int null_id() { return std::numeric_limits<Int>::max(); }

   using flip_sequence = std::list<Int>;

protected:
   Array<Vertex> vertices;
   Array<HalfEdge> edges;
   Array<Face> faces;
   bool with_faces;

public:
   DoublyConnectedEdgeList() = default;

   DoublyConnectedEdgeList(const DoublyConnectedEdgeList& list) = default;

   const Array<Vertex>& getVertices() const
   {
      return vertices;
   }

   const Array<HalfEdge>& getEdges() const
   {
      return edges;
   }

   const Array<Face>& getFaces() const
   {
      return faces;
   }

   // get the number of vertices corresponding to an DCEL input array
   Int getNumVert(const Array<Array<Int>>& half_edge_list)
   {
      Int num_edges = half_edge_list.size();
      Int num_vertices = 0;
      for (Int i = 0; i < num_edges; ++i) {
         if (half_edge_list[i][0] > num_vertices)
            num_vertices = half_edge_list[i][0];
         if (half_edge_list[i][1] > num_vertices )
            num_vertices = half_edge_list[i][1];
      }
      ++num_vertices;
      return num_vertices;
   }

   // get the number of triangles corresponding to an DCEL input array
   Int getNumTriangs(const Array<Array<Int>>& half_edge_list)
   {
      Int num_edges = half_edge_list.size();
      Int num_triangles = 0;
      for (Int i = 0; i < num_edges; ++i) {
         if (half_edge_list[i][4] > num_triangles)
            num_triangles = half_edge_list[i][4];
         if (half_edge_list[i][5] > num_triangles )
            num_triangles = half_edge_list[i][5];
      }
      ++num_triangles;
      return num_triangles;
   }

   // Construct a DCEL out of a given half edge list.
   // The ith element in dcel_data is [i.head, (i+1).head, i.next, (i+1).next, i.face, (i+1).face].
   // The latter two entries may be omitted if no faces are specified
   explicit DoublyConnectedEdgeList(const Array<Array<Int>>& half_edge_list)
   {
      with_faces = 0;
      Int num_edges = half_edge_list.size();
      Int num_vertices = getNumVert(half_edge_list);    
      Int num_faces = 2*num_edges/3;
      vertices = Array<Vertex>(num_vertices);
      edges = Array<HalfEdge>(num_edges*2);
      faces = Array<Face>(num_faces);
      for (Int i = 0; i < half_edge_list.size(); ++i) {
         setEdgeIncidences(i, half_edge_list[i][0], half_edge_list[i][1], half_edge_list[i][2], half_edge_list[i][3]);
         if (half_edge_list[i].size() == 6) {
            setFaceIncidences(i, half_edge_list[i][4], half_edge_list[i][5]);
            with_faces = 1;
         }
      }
   }
   
   DoublyConnectedEdgeList(const Array<Array<Int>>& half_edge_list, const Vector<Rational>& coords)
      : DoublyConnectedEdgeList(half_edge_list)
   {
      if (half_edge_list[0].size() == 4)
         setMetric(coords);
      if (half_edge_list[0].size() == 6)
         setAcoords(coords);
   }

   // set the incidences of an edge (which are two half edges) according to the input
   void setEdgeIncidences(const Int halfEdgeId, const Int headId, const Int twinHeadId, const Int nextId, const Int twinNextId)
   {
      HalfEdge* halfEdge = &edges[2*halfEdgeId];
      halfEdge->setHead(getVertex(headId));
      halfEdge->setNext(getHalfEdge(nextId));

      HalfEdge* twinHalfEdge = &edges[2*halfEdgeId+1];
      twinHalfEdge->setHead(getVertex(twinHeadId));
      twinHalfEdge->setNext(getHalfEdge(twinNextId));

      halfEdge->setTwin(twinHalfEdge);
   }

   // set face incidences of an edge
   void setFaceIncidences(const Int half_edge_id, const Int face_id, const Int twin_face_id)
   {
      Face* face = &faces[face_id];
      Face* twin_face = &faces[twin_face_id];
      HalfEdge* half_edge = &edges[2*half_edge_id];
      HalfEdge* twin_half_edge = &edges[2*half_edge_id+1];

      face->setHalfEdge(half_edge);
      twin_face->setHalfEdge(twin_half_edge);
      half_edge->setFace(getFace(face_id));
      twin_half_edge->setFace(getFace(twin_face_id));
   }

   // return the edges-vertices incidence matrix
   SparseMatrix<Int> EdgeVertexIncidenceMatrix() const
   {
      SparseMatrix<Int> Matrix(getNumEdges(), vertices.size());

      for (Int i = 0 ; i < getNumEdges(); ++i) {
         const HalfEdge* halfEdge = &edges[2 * i];
         Matrix(i, getVertexId(halfEdge->getHead())) = 1;
         Matrix(i, getVertexId(halfEdge->getTwin()->getHead())) = 1;
      }

      return Matrix;
   }

   // return true if the edge of index 'edgeId' is flippable, the two half edges have id 'edgeId' and 'edgeId'+1
   bool isFlippable(const Int edgeId) const
   {
      const HalfEdge* halfEdge = &edges[2 * edgeId];
      return halfEdge != halfEdge->getNext()
         && halfEdge != halfEdge->getNext()->getNext()
         && halfEdge != halfEdge->getNext()->getTwin()
         && halfEdge != halfEdge->getNext()->getNext()->getTwin();
   }

   // flip half edge and its twin ccw
   void flipHalfEdge(HalfEdge* const halfEdge)
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
                
   void flipEdgeWithFaces(const Int edge_id)
   {
      HalfEdge* halfEdge = &edges[2 * edge_id];
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

   // flip edge of index 'edgeId'
   void flipEdge(const Int edgeId)
   {
      HalfEdge* halfEdge = &edges[2*edgeId];
      if (halfEdge != halfEdge->getNext()
          && halfEdge != halfEdge->getNext()->getNext()
          && halfEdge != halfEdge->getNext()->getTwin()
          && halfEdge != halfEdge->getNext()->getNext()->getTwin())
         flipHalfEdge(halfEdge);
   }

   // unflip half edge and its twin ccw
   void unflipHalfEdge(HalfEdge* const halfEdge)
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

   // unflip edge of index 'edgeId'
   void unflipEdge(const Int edgeId)
   {
      HalfEdge* halfEdge = &edges[2*edgeId];
      if (halfEdge != halfEdge->getNext()
          && halfEdge != halfEdge->getNext()->getNext()
          && halfEdge != halfEdge->getNext()->getTwin()
          && halfEdge != halfEdge->getNext()->getNext()->getTwin())
         unflipHalfEdge(halfEdge);
   }

   // return the total number of vertices
   Int getNumVertices() const
   {
      return this->vertices.size();
   }

   // returns the number of half edges
   Int getNumHalfEdges() const
   {
      return this->edges.size();
   }

   // returns the number of edges
   Int getNumEdges() const
   {
      return getNumHalfEdges()/2;
   }

   // returns the number of faces
   Int getNumFaces() const
   {
      return faces.size(); 
   }

   // return the index of the given vertex
   Int getVertexId(const Vertex* vertex) const
   {
      if (vertex >= vertices.begin() && vertex < vertices.end())
         return vertex - vertices.begin();
      return null_id();
   }

   // return a pointer to the vertex with the given id
   Vertex* getVertex(const Int id)
   {
      return &vertices[id];
   }

   // return the index of the given halfedge
   Int getHalfEdgeId(const HalfEdge* halfEdge) const
   {
      if (halfEdge >= edges.begin() && halfEdge < edges.end())
         return halfEdge - edges.begin();
      return null_id();
   }

   // return a pointer to the half edge with the given id
   HalfEdge* getHalfEdge(const Int id)
   {
      return &edges[id];
   }

   Face* getFace(const Int face_id)
   {
      return &faces[face_id];
   }

   Int getFaceId(const Face* face) const
   {
      if (face >= faces.begin() && face < faces.end())
         return face - faces.begin();
      return null_id(); 
   }

   // return the indices of the half edges that form a quadrilateral around the half edge of index id
   /*       k
           / \            half edge ik has index id
          / | \
         /  |  \          the output vector gives the id's of the surrounding quad as [ij, jk, kl, il]
       l \  |  / j
          \ | /
           \ /
            i
   */
   std::array<Int, 8> getQuadId(const Int id) const
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

   // set the lengths of the edges according to the input vector
   void setMetric(const Vector<Rational>& metric)
   {
      for (Int i = 0, end = getNumEdges(); i < end; ++i) {
         edges[2*i].setLength(metric[i]);
         edges[2*i+1].setLength(metric[i]);
      }
   }

   // set the A-coordinates of according to the input vector
   void setAcoords(const Vector<Rational>& acoords)
   {
      Int count = 0;
      for (Int i = 0, end = getNumHalfEdges(); i < end; ++i) {
         edges[i].setLength(acoords[i]);
         ++count;
      }
      for (Int j = 0, end = getNumFaces(); j < end; ++j) {
         faces[j].setDetCoord(acoords[count+j]);
      }
   }

   // return the lengths of the edges
   Vector<Rational> edgeLengths() const
   {
      Vector<Rational> metric(getNumEdges());
      for (Int i = 0, end = getNumEdges(); i < end ; ++i)
         metric[i] = edges[2 * i].getLength();
      return metric;
   }

   // calcutate the inequalities that define the secondary cone
   Matrix<Rational> DelaunayInequalities() const
   {
      Matrix<Rational> M(getNumEdges()+getNumVertices(), getNumVertices()+1);
      const Int numEdges = getNumEdges();
      for (Int a = 0; a < numEdges; ++a) {
         const auto quadId = getQuadId(2 * a);

         const Rational& ik = edges[2 * a].getLength();
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

      for (Int j = 0, end = getNumVertices(); j < end; ++j) {
         M(numEdges+j, j+1) = 1;
      }
      return remove_zero_rows(M);
   }

   // for each valid facet of the secondary cone we collect the indices of those edges whose Delaunay inequalities define that facet
   Array<flip_sequence> flippableEdges(const flip_sequence& list_arg = std::list<Int>()) const
   {
      BigObject p("polytope::Polytope<Rational>");
      Matrix<Rational> M = DelaunayInequalities();
      p.take("INEQUALITIES") << M;
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

   Matrix<Rational> coneFacets() const
   {
      BigObject p("polytope::Polytope<Rational>");
      p.take("INEQUALITIES") << DelaunayInequalities();
      return p.give("FACETS");
   }

   // return a set that contains the rays of the secondary cone, we normalize the rays
   Set<Vector<Rational>> coneRays() const
   {
      Set<Vector<Rational>> ray_set;
      BigObject p("polytope::Polytope<Rational>");
      Matrix<Rational> M = DelaunayInequalities();
      p.take("INEQUALITIES") << M;
      Matrix<Rational> rays = p.give("VERTICES");

      for (Int i = 0 ; i < rays.rows(); ++i)
         ray_set += normalize(rays.row(i));

      return ray_set;
   }

   // normalize the Vector in the positive orthant by dividing by its 1-norm
   Vector<Rational> normalize(const Vector<Rational>& v) const
   {
      return v / accumulate(v, operations::add());
   }

   // check if the facet is a potential candidate to flip the corresponding edges in the triangulation of the surface
   // we exclude the far facet ( 1 : 0 : ... : 0 ) and the coordinate hyperplanes ( 0 : ... : 1 : 0 : ... : 0 ) as well as ( 0 : ... : 0 )
   template <typename TVec>
   static bool validFacet(const GenericVector<TVec, Rational>& facet_normal)
   {
      return nonZeros(facet_normal) > 1;
   }

   template <typename TVec>
   static Int nonZeros(const GenericVector<TVec>& facet_normal)
   {
      Int non_zeros = 0;
      for (auto it = entire(facet_normal.top()); !it.at_end(); ++it)
         if (!is_zero(*it)) ++non_zeros;
      return non_zeros;
   }

   // flip the edges of the given indices and in the given order: false = [left->right], true = [right->left]
   // return the flip_sequence, where possible former flips are included if given as optional input
   flip_sequence flipEdges_and_give_flips(const flip_sequence& edgeIds, flip_sequence former_flips = flip_sequence(), bool reverse = false)
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

   // flip the edges of the given indices and in the given order: false = [left->right], true = [right->left]
   void flipEdges(const flip_sequence& edgeIds, bool reverse = false)
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

   // we flip those edges whose inequalities are equivalent to the facet normal until the there is no such edge, we only flip through valid facets
   flip_sequence flipThroughFace(const Vector<Rational>& facet_normal, flip_sequence former_flips = flip_sequence())
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

   // return the index of the first Delaunay inequality matrix that is equivalent to the given inequality "ineq"; return -1 if there is no such row
   Int first_equiv_row(const Vector<Rational>& ineq) const
   {
      for (auto it = entire<indexed>(rows(DelaunayInequalities())); !it.at_end(); ++it) {
         if (is_equiv(ineq, *it))
            return it.index();
      }
      return -1;
   }

   // return true if the two vectors define the same non-degenerate half space
   bool is_equiv(const Vector<Rational>& ineq_a, const Vector<Rational>& ineq_b) const
   {
      if (rank(vector2row(ineq_a) / ineq_b) == 1) {
         for (Int i = 0 ; i < ineq_a.dim(); ++i) {
            if (ineq_a[i] != 0) return ineq_b[i]/ineq_a[i] > 0;
         }
      }
      return false;
   }

   // check if the the edge with index id is Delaunay after scaling the horocycles by the weights
   bool is_Delaunay(const Int id, const Vector<Rational>& weights)
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

   // check if the triangulation is Delaunay w.r.t. the given weights, return id of the first edge that is not Delaunay or -1 if the triangulation is Delaunay
   Int is_Delaunay(const Vector<Rational>& weights)
   {
      for (Int i = 0, end = getNumEdges(); i < end; ++i) {
         if (!is_Delaunay(i, weights)) return i;
      }
      return -1;
   }

   Vector<Int> DelaunayConditions(const Vector<Rational>& weights)
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

   // the flip algorithm, we flip edges that are non-Delaunay w.r.t. the weights as long as there are some
   flip_sequence flipToDelaunayAlt(const Vector<Rational>& weights)
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

   // return the angle sum of the vertex of index id
   Rational angleSum(const Int id)
   {
      Rational sum;
      Vertex* v = getVertex(id);
      HalfEdge* e = v->getIncidentEdge();
      HalfEdge* a = e->getTwin();
      HalfEdge* b = a->getNext();
      HalfEdge* c = b->getNext();
      Rational angle = b->getLength() / (a->getLength() * c->getLength());
      sum = angle;
      while (getHalfEdgeId(c) != getHalfEdgeId(e)) {
         a = c->getTwin();
         b = a->getNext();
         c = b->getNext();
         angle = b->getLength() / (a->getLength() * c->getLength());
         sum += angle;
      }
      return sum;
   }

   // return the angle sum vector
   Vector<Rational> angleVector()
   {
      const Int numVert = vertices.size();
      Vector<Rational> angleVec(numVert);
      for (Int i = 0; i < numVert; ++i) {
         angleVec[i] = angleSum(i);
      }
      return angleVec;
   }

   // each face gets a new index = face_id + numHalfEdges
   // the triangleMap maps each edge_id to the index of its corresponding face
   const Map<Int, Int> triangleMap() const
   {
      Map<Int, Int> triangle_map;
      const Int numHalfEdges = getNumHalfEdges();
      for (Int i = 0; i < numHalfEdges; ++i) {
         const HalfEdge* halfEdge = &edges[i];
         triangle_map[i] = numHalfEdges + getFaceId(halfEdge->getFace());
      }
      return triangle_map;
   }

}; // end class DoublyConnectedEdgeList

} // end graph namespace
} // end polymake namespace

#endif // POLYMAKE_GRAPH_DOUBLY_CONNECTED_EDGE_LIST_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
