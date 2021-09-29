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
#include "polymake/Vector.h"
#include "polymake/Array.h"
#include "polymake/Matrix.h"
#include "polymake/Integer.h"
#include "polymake/Rational.h"
#include "polymake/SparseMatrix.h"
#include "polymake/Set.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/linalg.h"



namespace polymake {
namespace graph {
namespace dcel {
class DoublyConnectedEdgeList;
}

using DoublyConnectedEdgeList = dcel::DoublyConnectedEdgeList;

DoublyConnectedEdgeList conway_ambo_impl(const DoublyConnectedEdgeList& old);
DoublyConnectedEdgeList conway_kis_impl(const DoublyConnectedEdgeList& old);
DoublyConnectedEdgeList conway_snub_impl(const DoublyConnectedEdgeList& old);
namespace dcel {
template<typename Container> class HalfEdgeTemplate;
template<typename Container> class VertexTemplate;
template<typename Container> class FaceTemplate;
using HalfEdge = HalfEdgeTemplate<DoublyConnectedEdgeList>;
using Face = FaceTemplate<DoublyConnectedEdgeList>;
using Vertex = VertexTemplate<DoublyConnectedEdgeList>;

template<typename Container>
class VertexTemplate {
private:
   const Container* container;
   HalfEdgeTemplate<Container>* incidentEdge;

   friend Container;

public:
   VertexTemplate()
      : container(nullptr)
      , incidentEdge(nullptr) {};

   HalfEdgeTemplate<Container>* getIncidentEdge() const
   {
      return incidentEdge;
   }

   void setContainer(const Container* c){
      container = c;
   }

   Int getID() const {
      return container->getVertexId(this);
   }

   void setIncidentEdge(HalfEdgeTemplate<Container>* edge)
   {
      incidentEdge = edge;
   }
};


template<typename Container>
class FaceTemplate {
private:
   const Container* container;
   HalfEdge* half_edge;
   Rational det_coord;

   friend Container;

public:
   FaceTemplate()
      : container(nullptr)
      , half_edge(nullptr) {}

   HalfEdge* getHalfEdge() const
   {
      return half_edge;
   }

   void setContainer(const Container* c){
      container = c;
   }

   Int getID() const {
      return container->getFaceId(this);
   }

   void setHalfEdge(HalfEdge* edge)
   {
      half_edge = edge;
   }

   bool operator==(const FaceTemplate& other) const
   {
      return half_edge == other.half_edge;
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

template<typename Container>
class HalfEdgeTemplate {
private:
   const Container* container;
   HalfEdgeTemplate* twin;
   HalfEdgeTemplate* next;
   HalfEdgeTemplate* prev;
   Vertex* head;
   Face* face;
   Rational length;

   friend Container;

public:
   HalfEdgeTemplate()
      : container(nullptr)
      , twin(nullptr)
      , next(nullptr)
      , prev(nullptr)
      , head(nullptr)
      , face(nullptr)
      , length(1) {}

   bool operator==(const HalfEdgeTemplate& other) const
   {
      return twin == other.twin && next == other.next;
   }

   const Rational& getLength() const
   {
      return length;
   }

   void setContainer(const Container* c){
      container = c;
   }

   Int getID() const {
      return container->getHalfEdgeId(this);
   }

   void setLength(const Rational& newLength)
   {
      length = newLength;
   }

   HalfEdgeTemplate* getTwin()
   {
      return twin;
   }

   const HalfEdgeTemplate* getTwin() const
   {
      return twin;
   }

   void setTwin(HalfEdgeTemplate* newTwin)
   {
      twin = newTwin;
      newTwin->twin = this;
   }

   HalfEdgeTemplate* getNext()
   {
      return next;
   }

   const HalfEdgeTemplate* getNext() const
   {
      return next;
   }

   void setNext(HalfEdgeTemplate* newNext)
   {
      next = newNext;
      newNext->prev = this;
   }

   HalfEdgeTemplate* getPrev()
   {
      return prev;
   }

   const HalfEdgeTemplate* getPrev() const
   {
      return prev;
   }

   void setPrev(HalfEdgeTemplate* newPrev)
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

class DoublyConnectedEdgeList {
public:

private:
   mutable Matrix<Int> matrix_representation;
   static constexpr Int null_id() { return std::numeric_limits<Int>::max(); }

   using flip_sequence = std::list<Int>;

private:
   Array<Vertex> vertices;
   Array<HalfEdge> halfedges;
   Array<Face> faces;
   bool with_faces;

public:
   DoublyConnectedEdgeList() = default;

   DoublyConnectedEdgeList(const DoublyConnectedEdgeList& list){
      copy_from(list);
   }

   DoublyConnectedEdgeList &operator=(const DoublyConnectedEdgeList& other){
      this->copy_from(other);
      return *this;
   }

   void copy_from(const DoublyConnectedEdgeList& other)
   {
      with_faces = other.with_faces;
      if(with_faces){
         resize(other.vertices.size(), other.halfedges.size(), other.faces.size());
      } else {
         resize(other.vertices.size(), other.halfedges.size());
      }
      for(Int i=0; i<vertices.size(); i++){
         vertices[i].incidentEdge = &halfedges[other.vertices[i].getIncidentEdge()->getID()];
      }
      for(Int i=0; i<halfedges.size(); i++){
         halfedges[i].twin = &halfedges[other.halfedges[i].getTwin()->getID()];
         halfedges[i].next = &halfedges[other.halfedges[i].getNext()->getID()];
         halfedges[i].prev = &halfedges[other.halfedges[i].getPrev()->getID()];
         halfedges[i].head = &vertices[other.halfedges[i].getHead()->getID()];
         halfedges[i].length = other.halfedges[i].getLength();
         if(with_faces)
            halfedges[i].face = &faces[other.halfedges[i].getFace()->getID()];
      }
      if(with_faces){
         for(Int i=0; i<faces.size(); i++){
            faces[i].half_edge = &halfedges[other.faces[i].getHalfEdge()->getID()];
            faces[i].det_coord = other.faces[i].getDetCoord();
         }
      }
   }

   friend DoublyConnectedEdgeList polymake::graph::conway_ambo_impl(const DoublyConnectedEdgeList& old);

   // DoublyConnectedEdgeList kis(const DoublyConnectedEdgeList& old);
   friend DoublyConnectedEdgeList polymake::graph::conway_kis_impl(const DoublyConnectedEdgeList& old);

   // DoublyConnectedEdgeList snub(const DoublyConnectedEdgeList& old);
   friend DoublyConnectedEdgeList polymake::graph::conway_snub_impl(const DoublyConnectedEdgeList& old);

   DoublyConnectedEdgeList dual() const;

   bool operator==(const DoublyConnectedEdgeList& other) const{
      return other.toMatrixInt() == toMatrixInt();
   }

   /*
   const Array<Vertex>& getVertices() const
   {
      return vertices;
   }

   const Array<HalfEdge>& getHalfEdges() const
   {
      return halfedges;
   }
   */

   const Array<Face>& getFaces() const
   {
      return faces;
   }

   // get the number of vertices corresponding to an DCEL input array
   static Int getNumVert(const Matrix<Int>& half_edge_list);

   // get the number of triangles corresponding to an DCEL input array
   static Int getNumTriangs(const Matrix<Int>& half_edge_list);

   // Construct a DCEL out of a given half edge list.
   // The ith element in dcel_data is [i.head, (i+1).head, i.next, (i+1).next, i.face, (i+1).face].
   // The latter two entries may be omitted if no faces are specified
   explicit DoublyConnectedEdgeList(const Matrix<Int>& half_edge_list);

   DoublyConnectedEdgeList(const Array<Array<Int>>& vif_cyclic_normals);

   DoublyConnectedEdgeList(const Matrix<Int>& half_edge_list, const Vector<Rational>& coords);

   const Matrix<Int>& toMatrixInt() const;

   Array<Array<Int>> faces_as_cycles() const;

private:
   void populate(const Matrix<Int>& half_edge_list);
   void populate();
   void resize();
   void resize(Int nvert, Int nhe);
   void resize(Int nvert, Int nhe, Int nfaces);

   void connect_halfedges(HalfEdge* prev, HalfEdge* next){
      prev->setNext(next);
      next->setPrev(prev);
   }

   friend struct pm::spec_object_traits< pm::Serialized< polymake::graph::dcel::DoublyConnectedEdgeList > >;
   // set the incidences of an edge (which are two half edges) according to the input
   void setEdgeIncidences(Int halfEdgeId, Int headId, Int twinHeadId, Int nextId, Int twinNextId);

   // set face incidences of an edge
   void setFaceIncidences(Int half_edge_id, Int face_id, Int twin_face_id);

   void verifyHalfedge(Int& counter, const std::pair<Int, Int>& key, Map<std::pair<Int, Int>, Int>& existing_edges);

   void insert_container();

public:
   // return the halfedges-vertices incidence matrix
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
      return this->halfedges.size();
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

   // return the index of the given halfedge
   Int getHalfEdgeId(const HalfEdge* halfEdge) const
   {
      if (halfEdge >= halfedges.begin() && halfEdge < halfedges.end())
         return halfEdge - halfedges.begin();
      return null_id();
   }

   // return a pointer to the half edge with the given id
   const HalfEdge* getHalfEdge(const Int id) const
   {
      return &halfedges[id];
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

   template<typename ptr>
   void print_id_if_not_null(std::ostringstream& bos, const ptr* p, bool comma) const {
      if(p != nullptr) wrap(bos) << p->getID();
      else wrap(bos) << "NULL";
      if(comma) wrap(bos) << " ";
   }

   void debug_print() const {
      std::ostringstream bos;
      wrap(bos) << "VERTICES (incidentEdge)" << endl;
      for(const auto& vertex : vertices){
         wrap(bos) << vertex.getID() << ": ";
         print_id_if_not_null(bos, vertex.getIncidentEdge(), true);
         bos << endl;
      }
      wrap(bos) << "HALFEDGES" << endl;
      wrap(bos) << to_string();
      wrap(bos) << "FACES (half_edge)" << endl;
      for(const auto& face : faces){
         wrap(bos) << face.getID() << ": ";
         print_id_if_not_null(bos, face.getHalfEdge(), true);
         bos << endl;
      }
      cout << bos.str() << endl;
   }


   std::string to_string() const {
      std::ostringstream bos;
      wrap(bos) << "N_vertices: " << getNumVertices() << endl;
      if(with_faces){
         wrap(bos) << "Halfedges list: (edgeIndex: twinIndex nextIndex prevIndex headIndex faceIndex)" << endl;
      } else {
         wrap(bos) << "Halfedges list: (edgeIndex: twinIndex nextIndex prevIndex headIndex)" << endl;
      }
      for(const auto& he : halfedges){
         wrap(bos) << he.getID() << ": ";
         print_id_if_not_null(bos, he.getTwin(), true);
         print_id_if_not_null(bos, he.getNext(), true);
         print_id_if_not_null(bos, he.getPrev(), true);
         print_id_if_not_null(bos, he.getHead(), true);
         if(with_faces){
            print_id_if_not_null(bos, he.getFace(), true);
         }
         wrap(bos) << "(";
         if(he.getPrev() != nullptr) print_id_if_not_null(bos, he.getPrev()->getHead(), false);
         else wrap(bos) << "NOPREV";
         wrap(bos) << " ---> ";
         print_id_if_not_null(bos, he.getHead(), false);
         wrap(bos) << ")";
         wrap(bos) << "" << endl;
      }
      return bos.str();
   }

   template <typename Output> friend
   Output& operator<< (GenericOutput<Output>& out, const DoublyConnectedEdgeList& me)
   {
      out.top() << me.to_string();
      return out.top();
   }

}; // end class DoublyConnectedEdgeList

} // namespace dcel
} // end graph namespace
} // end polymake namespace


namespace pm{
   template<>
      struct spec_object_traits< Serialized< polymake::graph::dcel::DoublyConnectedEdgeList > > :
      spec_object_traits<is_composite> {

         typedef polymake::graph::dcel::DoublyConnectedEdgeList masquerade_for;

         typedef Matrix<Int> elements;

         template <typename Me, typename Visitor>
            static void visit_elements(Me& me, Visitor& v) //for data_load
            {
               v << me.matrix_representation;
               me.resize();
               me.populate();
            }

         template <typename Visitor>
            static void visit_elements(const pm::Serialized<masquerade_for>& me, Visitor& v) //for data_save
            {
               me.toMatrixInt();
               v << me.matrix_representation;
            }
      };

}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
