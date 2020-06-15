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

class DoublyConnectedEdgeList {
public:
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
         return incidentEdge;
      }

      void setIncidentEdge(HalfEdge* edge)
      {
         incidentEdge = edge;
      }

      Face* getFace() const
      {
         return face;
      }

      void setFace(Face* new_face)
      {
         face = new_face;
      }
   };

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
         return half_edge;
      }

      void setHalfEdge(HalfEdge* edge)
      {
         half_edge = edge;
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
   };

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
   };

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
   static Int getNumVert(const Array<Array<Int>>& half_edge_list);

   // get the number of triangles corresponding to an DCEL input array
   static Int getNumTriangs(const Array<Array<Int>>& half_edge_list);

   // Construct a DCEL out of a given half edge list.
   // The ith element in dcel_data is [i.head, (i+1).head, i.next, (i+1).next, i.face, (i+1).face].
   // The latter two entries may be omitted if no faces are specified
   explicit DoublyConnectedEdgeList(const Array<Array<Int>>& half_edge_list);

   DoublyConnectedEdgeList(const Array<Array<Int>>& half_edge_list, const Vector<Rational>& coords);

private:
   // set the incidences of an edge (which are two half edges) according to the input
   void setEdgeIncidences(Int halfEdgeId, Int headId, Int twinHeadId, Int nextId, Int twinNextId);

   // set face incidences of an edge
   void setFaceIncidences(Int half_edge_id, Int face_id, Int twin_face_id);

public:
   // return the edges-vertices incidence matrix
   SparseMatrix<Int> EdgeVertexIncidenceMatrix() const;

   // return true if the edge of index 'edgeId' is flippable, the two half edges have id 'edgeId' and 'edgeId'+1
   bool isFlippable(Int edgeId) const;

   // flip half edge and its twin ccw
   void flipHalfEdge(HalfEdge* halfEdge);

   void flipEdgeWithFaces(Int edge_id);

   // flip edge of index 'edgeId'
   void flipEdge(Int edgeId);

   // unflip half edge and its twin ccw
   void unflipHalfEdge(HalfEdge* halfEdge);

   // unflip edge of index 'edgeId'
   void unflipEdge(Int edgeId);

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

   const Vertex* getVertex(const Int id) const
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

   const HalfEdge* getHalfEdge(const Int id) const
   {
      return &edges[id];
   }

   Face* getFace(const Int face_id)
   {
      return &faces[face_id];
   }

   const Face* getFace(const Int face_id) const
   {
      return &faces[face_id];
   }

   Int getFaceId(const Face* face) const
   {
      if (face >= faces.begin() && face < faces.end())
         return face - faces.begin();
      return null_id();
   }

   /* return the indices of the half edges that form a quadrilateral around the half edge of index id

            k
           / \            half edge ik has index id
          / | \
         /  |  \          the output vector gives the id's of the surrounding quad as [ij, jk, kl, il]
       l \  |  / j
          \ | /
           \ /
            i
   */
   std::array<Int, 8> getQuadId(Int id) const;

   // set the lengths of the edges according to the input vector
   void setMetric(const Vector<Rational>& metric);

   // set the A-coordinates of according to the input vector
   void setAcoords(const Vector<Rational>& acoords);

   // return the lengths of the edges
   Vector<Rational> edgeLengths() const;

   // calculte the inequalities that define the secondary cone
   Matrix<Rational> DelaunayInequalities() const;

   // for each valid facet of the secondary cone we collect the indices of those edges whose Delaunay inequalities define that facet
   Array<flip_sequence> flippableEdges(const flip_sequence& list_arg = std::list<Int>()) const;

   Matrix<Rational> coneFacets() const;

   // normalize the Vector in the positive orthant by dividing by its 1-norm
   template <typename TVec>
   static
   auto normalize(const GenericVector<TVec>& v)
   {
      return v / accumulate(v.top(), operations::add());
   }

   // normalize a matrix rowwise
   template <typename TMatrix>
   static
   Matrix<Rational> normalize(const GenericMatrix<TMatrix, Rational>& m)
   {
      Matrix<Rational> result(m);
      for (auto v = entire(rows(result)); !v.at_end(); ++v) {
         *v /= accumulate(*v, operations::add());
      }
      return result;
   }

   Set<Vector<Rational>> coneRays() const;

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
   flip_sequence flipEdges_and_give_flips(const flip_sequence& edgeIds, flip_sequence former_flips = flip_sequence(), bool reverse = false);

   // flip the edges of the given indices and in the given order: false = [left->right], true = [right->left]
   void flipEdges(const flip_sequence& edgeIds, bool reverse = false);

   // we flip those edges whose inequalities are equivalent to the facet normal until the there is no such edge, we only flip through valid facets
   flip_sequence flipThroughFace(const Vector<Rational>& facet_normal, flip_sequence former_flips = flip_sequence());

   // return the index of the first Delaunay inequality matrix that is equivalent to the given inequality "ineq"; return -1 if there is no such row
   Int first_equiv_row(const Vector<Rational>& ineq) const;

   // return true if the two vectors define the same non-degenerate half space
   bool is_equiv(const Vector<Rational>& ineq_a, const Vector<Rational>& ineq_b) const;

   // check if the the edge with index id is Delaunay after scaling the horocycles by the weights
   bool is_Delaunay(Int id, const Vector<Rational>& weights) const;

   // check if the triangulation is Delaunay w.r.t. the given weights, return id of the first edge that is not Delaunay or -1 if the triangulation is Delaunay
   Int is_Delaunay(const Vector<Rational>& weights) const;

   Vector<Int> DelaunayConditions(const Vector<Rational>& weights) const;

   // the flip algorithm, we flip edges that are non-Delaunay w.r.t. the weights as long as there are some
   flip_sequence flipToDelaunayAlt(const Vector<Rational>& weights);

   // return the angle sum of the vertex of index id
   Rational angleSum(Int id) const;

   // return the angle sum vector
   Vector<Rational> angleVector() const;

   // each face gets a new index = face_id + numHalfEdges
   // the triangleMap maps each edge_id to the index of its corresponding face
   const Map<Int, Int> triangleMap() const;

}; // end class DoublyConnectedEdgeList

} // end graph namespace
} // end polymake namespace

#endif // POLYMAKE_GRAPH_DOUBLY_CONNECTED_EDGE_LIST_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
