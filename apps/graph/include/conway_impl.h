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

#include "polymake/Matrix.h"
#include "polymake/graph/DoublyConnectedEdgeList.h"

namespace polymake {
namespace graph {

using DoublyConnectedEdgeList = graph::dcel::DoublyConnectedEdgeList;
using HalfEdge = graph::dcel::HalfEdge;
using Face = graph::dcel::Face;
using Vertex = graph::dcel::Vertex;

// 3-cube as DCEL
DoublyConnectedEdgeList conway_seed_impl() {
   Matrix<Int> DCEL_data = {
      { 0, 2, 2, 21, 0, 4 },
      { 4, 0, 4, 18, 0, 2 },
      { 6, 4, 6, 17, 0, 5 },
      { 2, 6, 0, 22, 0, 3 },
      { 7, 3, 10, 20, 1, 3 },
      { 5, 7, 12, 23, 1, 5 },
      { 1, 5, 14, 16, 1, 2 },
      { 3, 1, 8, 19, 1, 4 },
      { 4, 5, 3, 11, 2, 5 },
      { 1, 0, 13, 1, 2, 4 },
      { 2, 3, 7, 15, 3, 4 },
      { 7, 6, 9, 5, 3, 5 }};
   return DoublyConnectedEdgeList(DCEL_data);
}


DoublyConnectedEdgeList conway_ambo_impl(const DoublyConnectedEdgeList& old){
   DoublyConnectedEdgeList result;
   Int n_halfedges = 2*old.getNumHalfEdges();
   Int n_vertices = old.getNumHalfEdges()/2;
   Int n_faces = old.getNumVertices() + old.getNumFaces();
   result.resize(n_vertices, n_halfedges, n_faces);

   Int n = old.getNumHalfEdges();
   Int halfedge_counter = 0;

   for(Int i=0; i<n; i++){
      const HalfEdge* he = old.getHalfEdge(i);
      Int vertex_id, next_vertex_id;
      if (i%2 == 0){vertex_id = i/2;
      }else{vertex_id = (i-1)/2;}
      if (he->getNext()->getID()%2 == 0){next_vertex_id = he->getNext()->getID()/2;
      }else{next_vertex_id = (he->getNext()->getID()-1)/2;}

      // We add two new halfedges.
      // newhe: Points to next of old halfedge
      // newhe_twin: Points from the next of old halfedge
      HalfEdge& newhe = result.halfedges[halfedge_counter++];
      HalfEdge& newhe_twin = result.halfedges[halfedge_counter++];

      newhe.setHead(&result.vertices[next_vertex_id]);
      newhe_twin.setHead(&result.vertices[vertex_id]);
      newhe.setTwin(&newhe_twin);

      newhe.setFace(&result.faces[he->getFace()->getID()]);
      newhe_twin.setFace(&result.faces[old.getNumFaces()+he->getHead()->getID()]);

      result.connect_halfedges(&newhe,&result.halfedges[2*he->getNext()->getID()]);
      result.connect_halfedges(&result.halfedges[2*he->getPrev()->getID()], &newhe);


      result.connect_halfedges(&newhe_twin,&result.halfedges[2*he->getTwin()->getPrev()->getID()+1]);
      result.connect_halfedges(&result.halfedges[2*he->getNext()->getTwin()->getID()+1],&newhe_twin);


      //result.faces[old.getNumFaces()+he->getTwin()->getHead()->getID()].setHalfEdge(&newhe_twin);
      //result.faces[he->getFace()->getID()].setHalfEdge(&newhe);

    }

   return result;
}


DoublyConnectedEdgeList conway_kis_impl(const DoublyConnectedEdgeList& old){
   DoublyConnectedEdgeList result;
   Int n_halfedges = 3*old.getNumHalfEdges();
   Int n_vertices = old.getNumVertices() + old.getNumFaces();
   Int n_faces = old.getNumHalfEdges();
   result.resize(n_vertices, n_halfedges, n_faces);
   result.populate(old.toMatrixInt());

   Int n = old.getNumFaces();
   // Remember the starting edges of the old faces so we can overwrite the
   // existing faces.
   Array<HalfEdge*> starting_edges(n);
   for(Int i=0; i<n; i++){
      starting_edges[i] = result.faces[i].getHalfEdge();
   }
   Int halfedge_counter = old.getNumHalfEdges();
   Int vertex_counter = old.getNumVertices();
   Int face_counter = 0;
   for(Int i=0; i<n; i++){
      Vertex& center = result.vertices[vertex_counter++];
      HalfEdge *start = starting_edges[i], *he = start;
      Int start_face = face_counter, next_face, j = 0, first_pointing_in_ID = -1;
      do {
         // We will change the 'next' edge, so we need to remember the old
         // next.
         HalfEdge* nexthe = he->getNext();

         // We add two new halfedges.
         // newhe: Points outwards
         // newhe_twin: Points towards center
         HalfEdge& newhe = result.halfedges[halfedge_counter++];
         HalfEdge& newhe_twin = result.halfedges[halfedge_counter++];
         newhe.setTwin(&newhe_twin);

         newhe_twin.setHead(&center);
         newhe.setHead(he->getHead());
         if(nexthe == start){
            next_face = start_face;
            // Close the last cycle
            result.connect_halfedges(&result.halfedges[first_pointing_in_ID], &newhe);
         } else {
            next_face = face_counter+1;
         }
         he->setFace(&result.faces[face_counter]);
         newhe.setFace(&result.faces[next_face]);
         newhe_twin.setFace(&result.faces[face_counter]);

         result.faces[next_face].setHalfEdge(nexthe);
         result.faces[face_counter].setHalfEdge(he);
         if(j > 0){
            result.connect_halfedges(&newhe_twin, &result.halfedges[halfedge_counter-4]);
         } else {
            // At this point we don't know yet how long this cycle will be,
            // so we have to remember the first edge pointing inwards, so we
            // can link it up later.
            first_pointing_in_ID = newhe_twin.getID();
         }

         result.connect_halfedges(&newhe, nexthe);
         result.connect_halfedges(he, &newhe_twin);

         he = nexthe;
         face_counter++;
         j++;
      } while(he != start);
   }
   return result;
}


DoublyConnectedEdgeList conway_snub_impl(const DoublyConnectedEdgeList& old){
   DoublyConnectedEdgeList result;
   Int n_halfedges = 5*old.getNumHalfEdges();
   //Numbered in bijection to old Half-edges
   Int n_vertices = old.getNumHalfEdges();
   //We have faces corresponding to vertices, faces
   //corresponding to faces and faces corresponding
   //to halfedges
   Int n_faces = old.getNumHalfEdges()+old.getNumFaces()+old.getNumVertices();
   result.resize(n_vertices, n_halfedges, n_faces);
   //We populate part of DCEL result with DCEL old
   result.populate(old.toMatrixInt());

   Int n_old_faces = old.getNumFaces();
   // Remember the starting edges of the old faces
   Array<HalfEdge*> starting_edges(n_old_faces);
   for(Int i=0; i<n_old_faces; i++){
      starting_edges[i] = old.faces[i].getHalfEdge();
   }

   Array<HalfEdge> oldhalfedges=old.halfedges;

   Int halfedge_counter=old.getNumHalfEdges();
   Int face_counter=n_old_faces;
   for(Int i=0; i<n_old_faces; i++){
      HalfEdge *start = starting_edges[i], *he = start;
      do {
         HalfEdge* nexthe = he->getNext();

         // We add five new halfedges.
         // oldhe: Half-edge resembling the correponding old one
         // longhe: Half-edge whose head is the same as oldhe and
         //         whose tail is the head of the previous half-edge
         //         of oldhe when seen as an old half-edge
         // longhe_twin: twin of longhe
         // shorthe: Half-edge whose head is the tail of oldhe and whose
         //          tail is the same as the tail of shorthe
         // shorthe_twin: twin of shorthe

         //In this first part we populate the halfedges in the faces
         //corresponding to the old polytope, and the faces
         //corresponding to the halfedges
         HalfEdge& longhe = result.halfedges[halfedge_counter++];
         HalfEdge& longhe_twin = result.halfedges[halfedge_counter++];
         longhe.setTwin(&longhe_twin);
         HalfEdge& shorthe = result.halfedges[halfedge_counter++];
         HalfEdge& shorthe_twin = result.halfedges[halfedge_counter++];
         shorthe.setTwin(&shorthe_twin);

         longhe.setHead(&result.vertices[he->getID()]);
         longhe_twin.setHead(&result.vertices[he->getPrev()->getID()]);
         shorthe.setHead(&result.vertices[he->getTwin()->getID()]);
         shorthe_twin.setHead(&result.vertices[he->getPrev()->getID()]);
         result.halfedges[he->getID()].setHead(&result.vertices[he->getID()]);

         result.connect_halfedges(&result.halfedges[he->getID()],&longhe_twin);
         result.connect_halfedges(&longhe_twin,&shorthe);
         result.connect_halfedges(&shorthe,&result.halfedges[he->getID()]);

         if(he!=start){
            // Close the last cycle
            Int ID_prev=he->getPrev()->getID();
            HalfEdge* longhe_prev=result.halfedges[ID_prev].getNext()->getTwin();
            result.connect_halfedges(longhe_prev,&longhe);
            if (nexthe == start){
		    HalfEdge* longhe_next=result.halfedges[start->getID()].getNext()->getTwin();
		    result.connect_halfedges(&longhe,longhe_next);
	    }
         }

         longhe.setFace(&result.faces[i]);
         result.halfedges[he->getID()].setFace(&result.faces[face_counter]);
         longhe_twin.setFace(&result.faces[face_counter]);
         shorthe.setFace(&result.faces[face_counter]);

         result.faces[face_counter].setHalfEdge(&result.halfedges[he->getID()]);
         result.faces[i].setHalfEdge(&longhe);

	 face_counter++;
         he = nexthe;
      } while(he != start);
   }


   for(Int i=0; i<n_old_faces; i++){
      HalfEdge *start = starting_edges[i], *he = start;
      do {
      	 //In this second part we populate the halfedges in the faces
         //corresponding to the old vertices
         HalfEdge* nexthe = he->getNext();
         HalfEdge* shorthe_curr=result.halfedges[he->getID()].getPrev()->getTwin();
         HalfEdge* shorthe_next=result.halfedges[he->getTwin()->getNext()->getID()].getPrev()->getTwin();

         result.connect_halfedges(shorthe_next,shorthe_curr);
         shorthe_curr->setFace(&result.faces[face_counter+he->getPrev()->getHead()->getID()]);
         result.faces[face_counter+he->getPrev()->getHead()->getID()].setHalfEdge(shorthe_curr);

         he = nexthe;
      } while(he != start);

   }

   return result;
}



} // namespace graph
} // namespace polytope


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
