/* Copyright (c) 1997-2023
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

//Fundamental triangulation in CG construction for the case when k=2 and l=0.
DoublyConnectedEdgeList conway_CG_fundtri2(DoublyConnectedEdgeList& old){
   DoublyConnectedEdgeList result;
   Int n_vertices = old.getNumVertices() + old.getNumEdges();
   // New vertex order: First take the order of old vertices, then one vertex per each old edge
   Int n_halfedges = old.getNumHalfEdges()*2 + old.getNumFaces()*2*3;
   // New halfedge order: For each old edge create new 4 new edges. First two located on the old half edge, and one located on each face that contains the old edge which are parallel to central ones.
   Int n_faces = old.getNumFaces()*4;
   // New face order: We add 1 new face for each old Halfedge, and then add one face for each old face
   result.resize(n_vertices,n_halfedges,n_faces);
   Int halfedge_counter = 0;
   Int vertex_counter = old.getNumVertices();
   for(Int i=0; i<old.getNumHalfEdges(); i++){
      const HalfEdge* he = old.getHalfEdge(i);
      const Int edge_index = he->getID()/2; //Assuming that twin pairs of DCEL is given consecutively.
      const Int next_edge_index = he->getNext()->getID()/2; //Assuming that twin pairs of DCEL is given consecutively.
      const Int twin_next_edge_index = he->getTwin()->getNext()->getID()/2; //Assuming that twin pairs of DCEL is given consecutively.
      const Int head_index = he->getHead()->getID();
      const Int twin_head_index = he->getTwin()->getHead()->getID();
      const Int next_head_index = he->getNext()->getHead()->getID();
      const Int twin_next_head_index = he->getTwin()->getNext()->getHead()->getID();
      //We only consider old halfedges whose head index is larger than its tail index.
      //When writing new he and he_twin pair, he is always directed according to the chosen old he
      if( he->getHead()->getID() > he->getTwin()->getHead()->getID()){
         //First write new "central" halfedges located on the old edge.
         HalfEdge& first_central_he = result.halfedges[halfedge_counter++];
         HalfEdge& first_central_he_twin = result.halfedges[halfedge_counter++];
         HalfEdge& second_central_he = result.halfedges[halfedge_counter++];
         HalfEdge& second_central_he_twin = result.halfedges[halfedge_counter++];
         //We set heads of each new central edge and their twins.
         first_central_he.setHead(&result.vertices[vertex_counter+edge_index]);
         first_central_he_twin.setHead(&result.vertices[twin_head_index]);
         second_central_he.setHead(&result.vertices[head_index]);
         second_central_he_twin.setHead(&result.vertices[vertex_counter+edge_index]);
         //Set twins of central halfedges
         first_central_he.setTwin(&first_central_he_twin);
         second_central_he.setTwin(&second_central_he_twin);
         //Set faces for central half edges. Each old half edge enumerates the new faces adjacent to the new central he's
         first_central_he.setFace(&result.faces[he->getID()]);
         first_central_he_twin.setFace(&result.faces[he->getTwin()->getNext()->getID()]);
         second_central_he.setFace(&result.faces[he->getNext()->getID()]);
         second_central_he_twin.setFace(&result.faces[he->getTwin()->getID()]);
         // We set the nexts of central half edges for central edges.
         // The nexts of central edges depend on how the face index of he and he_twin are ordered.
         Int cn1_index,cn2_index,cnt1_index,cnt2_index;
         Int next_face_index = he->getNext()->getFace()->getID();
         Int next_twin_face_index = he->getNext()->getTwin()->getFace()->getID();
         if (next_head_index > head_index) {
            if (next_twin_face_index > next_face_index) {
               //If next he is already directed from low to high and next_he face index is smaller than next_he_twin face index
               cn1_index = 8 * next_edge_index + 4;
               cn2_index = 8 * next_edge_index ;
            }else{
               //If next he is already directed from low to high and next_he_twin face index is smaller than next_he face index
               cn1_index = 8 * next_edge_index + 4 + 2;
               cn2_index = 8 * next_edge_index ;
            }
         }else{
            //If next he is not directed from low to high and next_he face index is smaller than next_he_twin face index
            if (next_twin_face_index > next_face_index) {
               cn1_index = 8 * next_edge_index + 4 + 1;
               cn2_index = 8 * next_edge_index  + 2 + 1;
            }else{
               //If next he is not directed from low to high and next_he_twin face index is smaller than next_he face index
               cn1_index = 8 * next_edge_index + 4 + 2 + 1;
               cn2_index = 8 * next_edge_index + 2 + 1;
            }
         }
         //We compute the next indices of each central edge twin as above.
         Int twin_next_face_index = he->getTwin()->getNext()->getFace()->getID();
         Int twin_next_twin_face_index = he->getTwin()->getNext()->getTwin()->getFace()->getID();
         if (twin_next_head_index > twin_head_index) {
            if (twin_next_twin_face_index > twin_next_face_index) {
               cnt1_index = 8 * twin_next_edge_index ;
               cnt2_index = 8 * twin_next_edge_index + 4;
            }else{
               cnt1_index = 8 * twin_next_edge_index ;
               cnt2_index = 8 * twin_next_edge_index + 4 + 2;
            }
         }else{
            if (twin_next_twin_face_index > twin_next_face_index) {
               cnt1_index = 8 * twin_next_edge_index + 2 + 1;
               cnt2_index = 8 * twin_next_edge_index  + 4 + 1;
            }else{
               cnt1_index = 8 * twin_next_edge_index + 2 + 1;
               cnt2_index = 8 * twin_next_edge_index + 4 + 2 + 1;
            }
         }
         //Connect the central edge and central edge twins nexts.
         HalfEdge& cn1 = result.halfedges[cn1_index];
         result.connect_halfedges(&first_central_he,&cn1);
         HalfEdge& cn2 = result.halfedges[cn2_index];
         result.connect_halfedges(&second_central_he,&cn2);
         HalfEdge& cnt1 = result.halfedges[cnt1_index];
         result.connect_halfedges(&first_central_he_twin,&cnt1);
         HalfEdge& cnt2 = result.halfedges[cnt2_index];
         result.connect_halfedges(&second_central_he_twin,&cnt2);


         //Introduce new extra halfedges, for each old edge we define one new edge per each old face that contains the old edge.
         HalfEdge& face1edge = result.halfedges[halfedge_counter++];
         HalfEdge& face1edge_twin = result.halfedges[halfedge_counter++];
         HalfEdge& face2edge = result.halfedges[halfedge_counter++];
         HalfEdge& face2edge_twin = result.halfedges[halfedge_counter++];

         face1edge.setTwin(&face1edge_twin);
         face2edge.setTwin(&face2edge_twin);

         // We compute the heads and the nexts of the extra edges in the faces.
         Int f1n_index,f2n_index,f1tn_index,f2tn_index;
         //The next indices depend on whether the face index of the old he is smaller than the face index of the old he twin or not.
         //We first consider the case when old he face index is smaller than old he_twin.
         if (he->getFace()->getID() < he->getTwin()->getFace()->getID()){
            face1edge.setHead(&result.vertices[vertex_counter+ he->getNext()->getID()/2]);
            face1edge_twin.setHead(&result.vertices[vertex_counter+ he->getNext()->getNext()->getID()/2]);
            face2edge.setHead(&result.vertices[vertex_counter+ he->getTwin()->getNext()->getNext()->getID()/2]);
            face2edge_twin.setHead(&result.vertices[vertex_counter+ he->getTwin()->getNext()->getID()/2]);

            //While setting new faces, we number new face adjacent to central edges first, then add one more face corresponding to each old face by using the enumeration on old faces.
            face1edge.setFace(&result.faces[he->getNext()->getNext()->getID()]);
            face1edge_twin.setFace(&result.faces[3*old.getNumFaces()+he->getFace()->getID()]);
            face2edge.setFace(&result.faces[3*old.getNumFaces()+he->getTwin()->getFace()->getID()]);
            face2edge_twin.setFace(&result.faces[he->getTwin()->getNext()->getNext()->getID()]);

            // We set nexts for each new extra halfedges on the old faces. Each old edge defines 2 new edges on the old faces
            // First edge is located on the old face with smaller ID, and the last edge is located on the other face.
            // We check head indexes to determine he or he_twin is written first, he is written first if he->getHead() is larger than he_twin->getHead()
            // Each old edge creates 8 new half edges in total (4central + 4 on faces).
            if (next_head_index > head_index) {
               f1n_index = 8 * next_edge_index + 2;
               if (next_twin_face_index > next_face_index){
                  f1tn_index = 8 * next_edge_index + 4 + 1;
               }else{
                  f1tn_index = 8 * next_edge_index + 4 + 2 + 1;
               }
            }else{
               f1n_index = 8 * next_edge_index + 1;
               if (next_twin_face_index > next_face_index) {
                  f1tn_index = 8 * next_edge_index + 4 ;
               }else{
                  f1tn_index = 8 * next_edge_index + 4 + 2;
               }
            }
            if (twin_next_head_index > twin_head_index) {
               f2tn_index = 8 * twin_next_edge_index + 2;
               if (twin_next_twin_face_index > twin_next_face_index) {
                  f2n_index = 8 * twin_next_edge_index + 4 + 1;
               }else{
                  f2n_index = 8 * twin_next_edge_index + 4 + 2 + 1;
               }
            }else{
               f2tn_index = 8 * twin_next_edge_index + 1;
               if (twin_next_twin_face_index > twin_next_face_index) {
                  f2n_index = 8 * twin_next_edge_index + 4 ;
               }else{
                  f2n_index = 8 * twin_next_edge_index + 4 + 2;
               }
            }


         }else{
            // This is the case when old he_twin face index is smaller than old he face index.
            // In this case, among the faces that are adjacent to our he we first pick the face that belongs to he_twin.
            face2edge.setHead(&result.vertices[vertex_counter+ he->getNext()->getID()/2]);
            face2edge_twin.setHead(&result.vertices[vertex_counter+ he->getNext()->getNext()->getID()/2]);
            face1edge.setHead(&result.vertices[vertex_counter+ he->getTwin()->getNext()->getNext()->getID()/2]);
            face1edge_twin.setHead(&result.vertices[vertex_counter+ he->getTwin()->getNext()->getID()/2]);

            // See above
            face1edge.setFace(&result.faces[3*old.getNumFaces()+he->getTwin()->getFace()->getID()]);
            face1edge_twin.setFace(&result.faces[he->getTwin()->getNext()->getNext()->getID()]);
            face2edge.setFace(&result.faces[he->getNext()->getNext()->getID()]);
            face2edge_twin.setFace(&result.faces[3*old.getNumFaces()+he->getFace()->getID()]);

            //See above
            if (next_head_index > head_index) {
               f2n_index = 8 * next_edge_index + 2;
               if (next_twin_face_index > next_face_index){
                  f2tn_index = 8 * next_edge_index + 4 + 1;
               }else{
                  f2tn_index = 8 * next_edge_index + 4 + 2 + 1;
               }
            }else{
               f2n_index = 8 * next_edge_index + 1;
               if (next_twin_face_index > next_face_index) {
                  f2tn_index = 8 * next_edge_index + 4 ;
               }else{
                  f2tn_index = 8 * next_edge_index + 4 + 2;
               }
            }
            if (twin_next_head_index > twin_head_index) {
               f1tn_index = 8 * twin_next_edge_index + 2;
               if (twin_next_twin_face_index > twin_next_face_index) {
                  f1n_index = 8 * twin_next_edge_index + 4 + 1;
               }else{
                  f1n_index = 8 * twin_next_edge_index + 4 + 2 + 1;
               }
            }else{
               f1tn_index = 8 * twin_next_edge_index + 1;
               if (twin_next_twin_face_index > twin_next_face_index) {
                  f1n_index = 8 * twin_next_edge_index + 4 ;
               }else{
                  f1n_index = 8 * twin_next_edge_index + 4 + 2;
               }
            }
         }
         //Connect the nexts of extra edges and extra edge twins
         HalfEdge& f1n = result.halfedges[f1n_index];
         result.connect_halfedges(&face1edge,&f1n);
         HalfEdge& f2n = result.halfedges[f2n_index];
         result.connect_halfedges(&face2edge,&f2n);
         HalfEdge& f1tn = result.halfedges[f1tn_index];
         result.connect_halfedges(&face1edge_twin,&f1tn);
         HalfEdge& f2tn = result.halfedges[f2tn_index];
         result.connect_halfedges(&face2edge_twin,&f2tn);
      }
   }
   return result;
}

//Fundamental triangulation in CG construction for the case when k=3 and l=0.
//    ov1
//     /_\   first face       For each old edge we add 9 new edges( corresponds to the small 9 horizontal line segments),
//    /_ _\                   if ov2 > ov3 then the new he is directed from ov2 to ov3, otherwise new he twin is directed from ov2 to ov3.
//ov2/_ _ _\ov3               We add 3 new edges that lie on the old edge first -> central edges
//   \ _ _ /                  First face is the one with a smaller old face index, face1edge1 and face1edge2 are the edges in the middle layer of first face,
//    \ _ /   second face     face1edge3 is the single edge that lies on the top part of the first face.
//     \/                     face2edge1, face2edge2 and face2edge3 is defined similarly for the second face.
//     ov4
DoublyConnectedEdgeList conway_CG_fundtri3(DoublyConnectedEdgeList& old){
   DoublyConnectedEdgeList result;
   Int n_vertices = old.getNumVertices() + old.getNumEdges() * 2 + old.getNumFaces();
   Int n_halfedges = old.getNumHalfEdges()*3 + old.getNumFaces() * 18;
   Int n_faces = old.getNumFaces() * 9;
   result.resize(n_vertices,n_halfedges,n_faces);
   Int halfedge_counter = 0;
   Int vertex_counter = old.getNumVertices();
   for(Int i=0; i<old.getNumHalfEdges(); i++){
      const HalfEdge* he = old.getHalfEdge(i);
      const Int edge_index = he->getID()/2; //Assuming that twin pairs of DCEL is given consecutively.
      const Int next_edge_index = he->getNext()->getID()/2; //Assuming that twin pairs of DCEL is given consecutively.
      const Int twin_next_edge_index = he->getTwin()->getNext()->getID()/2; //Assuming that twin pairs of DCEL is given consecutively.
      const Int head_index = he->getHead()->getID();
      const Int twin_head_index = he->getTwin()->getHead()->getID();
      const Int next_head_index = he->getNext()->getHead()->getID();
      const Int twin_next_head_index = he->getTwin()->getNext()->getHead()->getID();
      const Int face_index = he->getFace()->getID();
      const Int twin_face_index = he->getTwin()->getFace()->getID();

      //We only consider old halfedges whose head index is larger than its tail index.
      //When writing new he and he_twin pair, new he is always directed according to the chosen old he
      if( he->getHead()->getID() > he->getTwin()->getHead()->getID()){
         // We first add new edges that occur in old edges
         HalfEdge& first_central_he = result.halfedges[halfedge_counter++];
         HalfEdge& first_central_he_twin = result.halfedges[halfedge_counter++];
         HalfEdge& second_central_he = result.halfedges[halfedge_counter++];
         HalfEdge& second_central_he_twin = result.halfedges[halfedge_counter++];
         HalfEdge& third_central_he = result.halfedges[halfedge_counter++];
         HalfEdge& third_central_he_twin = result.halfedges[halfedge_counter++];
         // Set heads of central edges
         first_central_he.setHead(&result.vertices[vertex_counter+edge_index*2]);
         first_central_he_twin.setHead(&result.vertices[twin_head_index]);
         second_central_he.setHead(&result.vertices[vertex_counter+edge_index*2+1]);
         second_central_he_twin.setHead(&result.vertices[vertex_counter+edge_index*2]);
         third_central_he.setHead(&result.vertices[head_index]);
         third_central_he_twin.setHead(&result.vertices[vertex_counter+edge_index*2+1]);
         //Set twins of central edges
         first_central_he.setTwin(&first_central_he_twin);
         second_central_he.setTwin(&second_central_he_twin);
         third_central_he.setTwin(&third_central_he_twin);
         //Set faces of central edges, these faces are numbered w.r.t old he incides. Each old he defines 2 such new face.
         first_central_he.setFace(&result.faces[2*he->getID()]);
         first_central_he_twin.setFace(&result.faces[2*he->getTwin()->getNext()->getID()]);
         second_central_he.setFace(&result.faces[2*he->getID()+1]);
         second_central_he_twin.setFace(&result.faces[2*he->getTwin()->getID()+1]);
         third_central_he.setFace(&result.faces[2*he->getNext()->getID()]);
         third_central_he_twin.setFace(&result.faces[2*he->getTwin()->getID()]);
         // We set the nexts of central half edges.
         // The nexts of central edges depend on how the face index of he and he_twin are ordered.
         Int cn1_index,cn2_index,cn3_index,cnt1_index,cnt2_index,cnt3_index;
         Int next_face_index = he->getNext()->getFace()->getID();
         Int next_twin_face_index = he->getNext()->getTwin()->getFace()->getID();
         if (next_head_index > head_index) {
            if (next_twin_face_index > next_face_index) {
               //If next he is already directed from low to high and next_he face index is smaller than next_he_twin face index
               cn1_index = 18 * next_edge_index + 6 + 4;
               cn2_index = 18 * next_edge_index + 6;
               cn3_index = 18 * next_edge_index ;
            }else{
               //If next he is already directed from low to high and next_he_twin face index is smaller than next_he face index
               cn1_index = 18 * next_edge_index + 6*2 + 4;
               cn2_index = 18 * next_edge_index + 6*2 ;
               cn3_index = 18 * next_edge_index ;
            }
         }else{
            if (next_twin_face_index > next_face_index) {
               //If next he is not directed from low to high and next_he face index is smaller than next_he_twin face index
               cn1_index = 18 * next_edge_index + 6 + 4 + 1;
               cn2_index = 18 * next_edge_index + 6 + 2 + 1 ;
               cn3_index = 18 * next_edge_index + 4 + 1;
            }else{
               //If next he is not directed from low to high and next_he_twin face index is smaller than next_he face index
               cn1_index = 18 * next_edge_index + 6*2 + 4 + 1;
               cn2_index = 18 * next_edge_index + 6*2 + 2 + 1 ;
               cn3_index = 18 * next_edge_index + 4 + 1;
            }
         }
         // We set the nexts of the twins of central half edges.
         // The nexts of central edges depend on how the face index of he and he_twin are ordered, see the case above.
         Int twin_next_face_index = he->getTwin()->getNext()->getFace()->getID();
         Int twin_next_twin_face_index = he->getTwin()->getNext()->getTwin()->getFace()->getID();
         if (twin_next_head_index > twin_head_index) {
            if (twin_next_twin_face_index > twin_next_face_index) {
               cnt1_index = 18 * twin_next_edge_index ;
               cnt2_index = 18 * twin_next_edge_index + 6;
               cnt3_index = 18 * twin_next_edge_index + 6 + 4;
            }else{
               cnt1_index = 18 * twin_next_edge_index ;
               cnt2_index = 18 * twin_next_edge_index + 6*2;
               cnt3_index = 18 * twin_next_edge_index + 6*2 + 4;
            }
         }else{
            if (twin_next_twin_face_index > twin_next_face_index) {
               cnt1_index = 18 * twin_next_edge_index + 4 + 1;
               cnt2_index = 18 * twin_next_edge_index + 6 + 2 + 1;
               cnt3_index = 18 * twin_next_edge_index + 6 + 4 + 1;
            }else{
               cnt1_index = 18 * twin_next_edge_index + 4 + 1;
               cnt2_index = 18 * twin_next_edge_index + 6*2 + 2 + 1;
               cnt3_index = 18 * twin_next_edge_index + 6*2 + 4 + 1;
            }
         }
         //Connect the nexts of central he and central he twins
         HalfEdge& cn1 = result.halfedges[cn1_index];
         result.connect_halfedges(&first_central_he,&cn1);
         HalfEdge& cn2 = result.halfedges[cn2_index];
         result.connect_halfedges(&second_central_he,&cn2);
         HalfEdge& cn3 = result.halfedges[cn3_index];
         result.connect_halfedges(&third_central_he,&cn3);
         HalfEdge& cnt1 = result.halfedges[cnt1_index];
         result.connect_halfedges(&first_central_he_twin,&cnt1);
         HalfEdge& cnt2 = result.halfedges[cnt2_index];
         result.connect_halfedges(&second_central_he_twin,&cnt2);
         HalfEdge& cnt3 = result.halfedges[cnt3_index];
         result.connect_halfedges(&third_central_he_twin,&cnt3);
         // Next we add 6 extra edges, 3 for each of the two faces that is adjacent to the old edge.
         // We add the edges that lie in a face with smaller index first
         HalfEdge& face1edge1 = result.halfedges[halfedge_counter++];
         HalfEdge& face1edge1_twin = result.halfedges[halfedge_counter++];
         HalfEdge& face1edge2 = result.halfedges[halfedge_counter++];
         HalfEdge& face1edge2_twin = result.halfedges[halfedge_counter++];
         HalfEdge& face1edge3 = result.halfedges[halfedge_counter++];
         HalfEdge& face1edge3_twin = result.halfedges[halfedge_counter++];
         HalfEdge& face2edge1 = result.halfedges[halfedge_counter++];
         HalfEdge& face2edge1_twin = result.halfedges[halfedge_counter++];
         HalfEdge& face2edge2 = result.halfedges[halfedge_counter++];
         HalfEdge& face2edge2_twin = result.halfedges[halfedge_counter++];
         HalfEdge& face2edge3 = result.halfedges[halfedge_counter++];
         HalfEdge& face2edge3_twin = result.halfedges[halfedge_counter++];
         //Set twins of the extra edges on the faces
         face1edge1.setTwin(&face1edge1_twin);
         face1edge2.setTwin(&face1edge2_twin);
         face1edge3.setTwin(&face1edge3_twin);
         face2edge1.setTwin(&face2edge1_twin);
         face2edge2.setTwin(&face2edge2_twin);
         face2edge3.setTwin(&face2edge3_twin);
         //TODO: figure out why this does not work without initializing the indexes as 0.
         Int f1e1n_index =  0;
         Int f1e2n_index =  0;
         Int f1e3n_index =  0;
         Int f2e1n_index =  0;
         Int f2e2n_index =  0;
         Int f2e3n_index =  0;
         Int f1e1tn_index =  0;
         Int f1e2tn_index =  0;
         Int f1e3tn_index =  0;
         Int f2e1tn_index =  0;
         Int f2e2tn_index =  0;
         Int f2e3tn_index =  0;

         //The HeadID of each new he depends on the face index that this new he is located in.
         //We first consider the case when he->getFace()->getID() < he->getTwin()->getFace()->getID()
         if (face_index < twin_face_index){
            face1edge1.setHead(&result.vertices[vertex_counter + old.getNumEdges()*2 + face_index]);
            face1edge2_twin.setHead(&result.vertices[vertex_counter + old.getNumEdges()*2 + face_index]);
            face2edge1.setHead(&result.vertices[vertex_counter + old.getNumEdges()*2 + twin_face_index]);
            face2edge2_twin.setHead(&result.vertices[vertex_counter + old.getNumEdges()*2 + twin_face_index]);
            if(he->getNext()->getNext()->getHead()->getID() > he->getNext()->getHead()->getID()){
               face1edge1_twin.setHead(&result.vertices[vertex_counter+ (he->getNext()->getNext()->getID()/2)*2 + 1 ]);
               face1edge3_twin.setHead(&result.vertices[vertex_counter+ (he->getNext()->getNext()->getID()/2)*2 ]);
            }else{
               face1edge1_twin.setHead(&result.vertices[vertex_counter+ (he->getNext()->getNext()->getID()/2)*2 ]);
               face1edge3_twin.setHead(&result.vertices[vertex_counter+ (he->getNext()->getNext()->getID()/2)*2 + 1 ]);
            }
            if(he->getNext()->getHead()->getID() > he->getHead()->getID()){
               face1edge2.setHead(&result.vertices[vertex_counter+ (he->getNext()->getID()/2)*2]);
               face1edge3.setHead(&result.vertices[vertex_counter+ (he->getNext()->getID()/2)*2+1]);
            }else{
               face1edge2.setHead(&result.vertices[vertex_counter+ (he->getNext()->getID()/2)*2+1]);
               face1edge3.setHead(&result.vertices[vertex_counter+ (he->getNext()->getID()/2)*2]);
            }
            if(he->getTwin()->getNext()->getHead()->getID() > he->getTwin()->getHead()->getID()){
               face2edge1_twin.setHead(&result.vertices[vertex_counter + (he->getTwin()->getNext()->getID()/2)*2 ]);
               face2edge3_twin.setHead(&result.vertices[vertex_counter + (he->getTwin()->getNext()->getID()/2)*2 + 1]);
            }else{
               face2edge1_twin.setHead(&result.vertices[vertex_counter + (he->getTwin()->getNext()->getID()/2)*2 + 1 ]);
               face2edge3_twin.setHead(&result.vertices[vertex_counter + (he->getTwin()->getNext()->getID()/2)*2 ]);
            }
            if(he->getTwin()->getNext()->getNext()->getHead()->getID() > he->getTwin()->getNext()->getHead()->getID()){
               face2edge2.setHead(&result.vertices[vertex_counter + (he->getTwin()->getNext()->getNext()->getID()/2)*2 + 1 ]);
               face2edge3.setHead(&result.vertices[vertex_counter+ (he->getTwin()->getNext()->getNext()->getID()/2)*2 ]);
            }else{
               face2edge2.setHead(&result.vertices[vertex_counter + (he->getTwin()->getNext()->getNext()->getID()/2)*2 ]);
               face2edge3.setHead(&result.vertices[vertex_counter+ (he->getTwin()->getNext()->getNext()->getID()/2)*2 + 1]);
            }
            face1edge1.setFace(&result.faces[2*he->getNext()->getNext()->getID() + 1]);
            face1edge1_twin.setFace(&result.faces[2*old.getNumHalfEdges()+he->getNext()->getID()]);
            face1edge2.setFace(&result.faces[2*he->getNext()->getID() + 1]);
            face1edge2_twin.setFace(&result.faces[2*old.getNumHalfEdges()+he->getNext()->getNext()->getID()]);
            face1edge3.setFace(&result.faces[2*he->getNext()->getNext()->getID()]);
            face1edge3_twin.setFace(&result.faces[2*old.getNumHalfEdges()+he->getID()]);
            face2edge1.setFace(&result.faces[2*he->getTwin()->getNext()->getID() + 1]);
            face2edge1_twin.setFace(&result.faces[2*old.getNumHalfEdges()+he->getTwin()->getNext()->getNext()->getID()]);
            face2edge2.setFace(&result.faces[2*he->getTwin()->getNext()->getNext()->getID() + 1]);
            face2edge2_twin.setFace(&result.faces[2*old.getNumHalfEdges()+he->getTwin()->getNext()->getID()]);
            face2edge3.setFace(&result.faces[2*he->getTwin()->getNext()->getNext()->getID()]);
            face2edge3_twin.setFace(&result.faces[2*old.getNumHalfEdges()+he->getTwin()->getID()]);

            // We set nexts for each new extra halfedges on the old faces. Each old edge defines 6 new edges on the old faces
            // First three edge is located on the old face with smaller ID, and the last edge is located on the other face.
            // We check head indexes to determine he or he_twin is written first, he is written first if he->getHead() is larger than he_twin->getHead()
            // Each old edge creates 18 new half edges in total (9 central + 9 on faces).
            if (next_head_index > head_index) {
               f1e2n_index = 18 * next_edge_index + 2;
               f1e3n_index = 18 * next_edge_index + 4;
               if (next_twin_face_index > next_face_index){
                  f1e1n_index = 18 * next_edge_index + 6 + 2;
                  f1e1tn_index = 18 * next_edge_index + 6 + 4 + 1;
                  f1e2tn_index = 18 * next_edge_index + 6 + 1;
                  f1e3tn_index = 18 * next_edge_index + 6 + 2 + 1;
               }else{
                  f1e1n_index = 18 * next_edge_index + 6*2 + 2;
                  f1e1tn_index = 18 * next_edge_index + 6*2 + 4 + 1;
                  f1e2tn_index = 18 * next_edge_index + 6*2 + 1;
                  f1e3tn_index = 18 * next_edge_index + 6*2 + 2 + 1;
               }
            }else{
               f1e2n_index = 18 * next_edge_index + 2 + 1;
               f1e3n_index = 18 * next_edge_index + 1;
               if (next_twin_face_index > next_face_index) {
                  f1e1n_index = 18 * next_edge_index + 6 + 1;
                  f1e1tn_index = 18 * next_edge_index + 6 + 4 ;
                  f1e2tn_index = 18 * next_edge_index + 6 + 2;
                  f1e3tn_index = 18 * next_edge_index + 6 ;
               }else{
                  f1e1n_index = 18 * next_edge_index + 6*2 + 1;
                  f1e1tn_index = 18 * next_edge_index + 6*2 + 4;
                  f1e2tn_index = 18 * next_edge_index + 6*2 + 2;
                  f1e3tn_index = 18 * next_edge_index + 6*2 ;
               }
            }
            if (twin_next_head_index > twin_head_index) {
               f2e1tn_index = 18 * twin_next_edge_index + 2;
               f2e3tn_index = 18 * twin_next_edge_index + 4;
               if (twin_next_twin_face_index > twin_next_face_index) {
                  f2e1n_index = 18 * twin_next_edge_index + 6 + 1;
                  f2e2n_index = 18 * twin_next_edge_index + 6 + 4 + 1;
                  f2e3n_index = 18 * twin_next_edge_index + 6 + 2 + 1;
                  f2e2tn_index = 18 * twin_next_edge_index + 6 + 2;
               }else{
                  f2e1n_index = 18 * twin_next_edge_index + 6*2 + 1;
                  f2e2n_index = 18 * twin_next_edge_index + 6*2 + 4 + 1;
                  f2e3n_index = 18 * twin_next_edge_index + 6*2 + 2 + 1;
                  f2e2tn_index = 18 * twin_next_edge_index + 6*2 + 2;
            }
            }else{
               f2e1tn_index = 18 * twin_next_edge_index + 2 + 1;
               f2e3tn_index = 18 * twin_next_edge_index + 1;
               if (twin_next_twin_face_index > twin_next_face_index) {
                  f2e1n_index = 18 * twin_next_edge_index + 6 + 2;
                  f2e2n_index = 18 * twin_next_edge_index + 6 + 4 ;
                  f2e3n_index = 18 * twin_next_edge_index + 6 ;
                  f2e2tn_index = 18 * twin_next_edge_index + 6 + 1;
               }else{
                  f2e1n_index = 18 * twin_next_edge_index + 6*2 + 2;
                  f2e2n_index = 18 * twin_next_edge_index + 6*2 + 4 ;
                  f2e3n_index = 18 * twin_next_edge_index + 6*2 ;
                  f2e2tn_index = 18 * twin_next_edge_index + 6*2 + 1;
               }
            }
         }else{   //Now we consider the case when he->getFace()->getID() > he->getTwin()->getFace()->getID()
            face1edge1.setHead(&result.vertices[vertex_counter + old.getNumEdges()*2 + twin_face_index]);
            face1edge2_twin.setHead(&result.vertices[vertex_counter + old.getNumEdges()*2 + twin_face_index]);
            face2edge1.setHead(&result.vertices[vertex_counter + old.getNumEdges()*2 + face_index]);
            face2edge2_twin.setHead(&result.vertices[vertex_counter + old.getNumEdges()*2 + face_index]);
            if(he->getNext()->getNext()->getHead()->getID() > he->getNext()->getHead()->getID()){
               face2edge1_twin.setHead(&result.vertices[vertex_counter+ (he->getNext()->getNext()->getID()/2)*2 + 1 ]);
               face2edge3_twin.setHead(&result.vertices[vertex_counter+ (he->getNext()->getNext()->getID()/2)*2 ]);
            }else{
               face2edge1_twin.setHead(&result.vertices[vertex_counter+ (he->getNext()->getNext()->getID()/2)*2 ]);
               face2edge3_twin.setHead(&result.vertices[vertex_counter+ (he->getNext()->getNext()->getID()/2)*2 + 1 ]);
            }
            if(he->getNext()->getHead()->getID() > he->getHead()->getID()){
               face2edge2.setHead(&result.vertices[vertex_counter+ (he->getNext()->getID()/2)*2]);
               face2edge3.setHead(&result.vertices[vertex_counter+ (he->getNext()->getID()/2)*2+1]);
            }else{
               face2edge2.setHead(&result.vertices[vertex_counter+ (he->getNext()->getID()/2)*2+1]);
               face2edge3.setHead(&result.vertices[vertex_counter+ (he->getNext()->getID()/2)*2]);
            }
            if(he->getTwin()->getNext()->getHead()->getID() > he->getTwin()->getHead()->getID()){
               face1edge1_twin.setHead(&result.vertices[vertex_counter + (he->getTwin()->getNext()->getID()/2)*2 ]);
               face1edge3_twin.setHead(&result.vertices[vertex_counter + (he->getTwin()->getNext()->getID()/2)*2 + 1]);
            }else{
               face1edge1_twin.setHead(&result.vertices[vertex_counter + (he->getTwin()->getNext()->getID()/2)*2 + 1 ]);
               face1edge3_twin.setHead(&result.vertices[vertex_counter + (he->getTwin()->getNext()->getID()/2)*2 ]);
            }
            if(he->getTwin()->getNext()->getNext()->getHead()->getID() > he->getTwin()->getNext()->getHead()->getID()){
               face1edge2.setHead(&result.vertices[vertex_counter + (he->getTwin()->getNext()->getNext()->getID()/2)*2 + 1 ]);
               face1edge3.setHead(&result.vertices[vertex_counter+ (he->getTwin()->getNext()->getNext()->getID()/2)*2 ]);
            }else{
               face1edge2.setHead(&result.vertices[vertex_counter + (he->getTwin()->getNext()->getNext()->getID()/2)*2 ]);
               face1edge3.setHead(&result.vertices[vertex_counter+ (he->getTwin()->getNext()->getNext()->getID()/2)*2 + 1]);
            }
            face2edge1.setFace(&result.faces[2*he->getNext()->getNext()->getID() + 1]);
            face2edge1_twin.setFace(&result.faces[2*old.getNumHalfEdges()+he->getNext()->getID()]);
            face2edge2.setFace(&result.faces[2*he->getNext()->getID() + 1]);
            face2edge2_twin.setFace(&result.faces[2*old.getNumHalfEdges()+he->getNext()->getNext()->getID()]);
            face2edge3.setFace(&result.faces[2*he->getNext()->getNext()->getID()]);
            face2edge3_twin.setFace(&result.faces[2*old.getNumHalfEdges()+he->getID()]);
            face1edge1.setFace(&result.faces[2*he->getTwin()->getNext()->getID() + 1]);
            face1edge1_twin.setFace(&result.faces[2*old.getNumHalfEdges()+he->getTwin()->getNext()->getNext()->getID()]);
            face1edge2.setFace(&result.faces[2*he->getTwin()->getNext()->getNext()->getID() + 1]);
            face1edge2_twin.setFace(&result.faces[2*old.getNumHalfEdges()+he->getTwin()->getNext()->getID()]);
            face1edge3.setFace(&result.faces[2*he->getTwin()->getNext()->getNext()->getID()]);
            face1edge3_twin.setFace(&result.faces[2*old.getNumHalfEdges()+he->getTwin()->getID()]);
            // We set the nexts for this case, see the explanation given for the first case above.
            if (next_head_index > head_index) {
               f2e2n_index = 18 * next_edge_index + 2;
               f2e3n_index = 18 * next_edge_index + 4;
               if (next_twin_face_index > next_face_index){
                  f2e1n_index = 18 * next_edge_index + 6 + 2;
                  f2e1tn_index = 18 * next_edge_index + 6 + 4 + 1;
                  f2e2tn_index = 18 * next_edge_index + 6 + 1;
                  f2e3tn_index = 18 * next_edge_index + 6 + 2 + 1;
               }else{
                  f2e1n_index = 18 * next_edge_index + 6*2 + 2;
                  f2e1tn_index = 18 * next_edge_index + 6*2 + 4 + 1;
                  f2e2tn_index = 18 * next_edge_index + 6*2 + 1;
                  f2e3tn_index = 18 * next_edge_index + 6*2 + 2 + 1;
               }
            }else{
               f2e2n_index = 18 * next_edge_index + 2 + 1;
               f2e3n_index = 18 * next_edge_index + 1;
               if (next_twin_face_index > next_face_index) {
                  f2e1n_index = 18 * next_edge_index + 6 + 1;
                  f2e1tn_index = 18 * next_edge_index + 6 + 4;
                  f2e2tn_index = 18 * next_edge_index + 6 + 2;
                  f2e3tn_index = 18 * next_edge_index + 6;
               }else{
                  f2e1n_index = 18 * next_edge_index + 6*2 + 1;
                  f2e1tn_index = 18 * next_edge_index + 6*2 + 4;
                  f2e2tn_index = 18 * next_edge_index + 6*2 + 2;
                  f2e3tn_index = 18 * next_edge_index + 6*2;
               }
            }
            if (twin_next_head_index > twin_head_index) {
               f1e1tn_index = 18 * twin_next_edge_index + 2;
               f1e3tn_index = 18 * twin_next_edge_index + 4;
               if (twin_next_twin_face_index > twin_next_face_index) {
                  f1e1n_index = 18 * twin_next_edge_index + 6 + 1;
                  f1e2n_index = 18 * twin_next_edge_index + 6 + 4 + 1;
                  f1e3n_index = 18 * twin_next_edge_index + 6 + 2 + 1;
                  f1e2tn_index = 18 * twin_next_edge_index + 6 + 2;
               }else{
                  f1e1n_index = 18 * twin_next_edge_index + 6*2 + 1;
                  f1e2n_index = 18 * twin_next_edge_index + 6*2 + 4 + 1;
                  f1e3n_index = 18 * twin_next_edge_index + 6*2 + 2 + 1;
                  f1e2tn_index = 18 * twin_next_edge_index + 6*2 + 2;
            }
            }else{
               f1e1tn_index = 18 * twin_next_edge_index + 2 + 1;
               f1e3tn_index = 18 * twin_next_edge_index + 1;
               if (twin_next_twin_face_index > twin_next_face_index) {
                  f1e1n_index = 18 * twin_next_edge_index + 6 + 2;
                  f1e2n_index = 18 * twin_next_edge_index + 6 + 4 ;
                  f1e3n_index = 18 * twin_next_edge_index + 6 ;
                  f1e2tn_index = 18 * twin_next_edge_index + 6 + 1;
               }else{
                  f1e1n_index = 18 * twin_next_edge_index + 6*2 + 2;
                  f1e2n_index = 18 * twin_next_edge_index + 6*2 + 4 ;
                  f1e3n_index = 18 * twin_next_edge_index + 6*2 ;
                  f1e2tn_index = 18 * twin_next_edge_index + 6*2 + 1;
               }
            }
         }
         //Connect the nexts of the extra half edges and their twins that show up on old faces.
         HalfEdge& f1e1n = result.halfedges[f1e1n_index];
         result.connect_halfedges(&face1edge1,&f1e1n);
         HalfEdge& f1e2n = result.halfedges[f1e2n_index];
         result.connect_halfedges(&face1edge2,&f1e2n);
         HalfEdge& f1e3n = result.halfedges[f1e3n_index];
         result.connect_halfedges(&face1edge3,&f1e3n);
         HalfEdge& f1e1tn = result.halfedges[f1e1tn_index];
         result.connect_halfedges(&face1edge1_twin,&f1e1tn);
         HalfEdge& f1e2tn = result.halfedges[f1e2tn_index];
         result.connect_halfedges(&face1edge2_twin,&f1e2tn);
         HalfEdge& f1e3tn = result.halfedges[f1e3tn_index];
         result.connect_halfedges(&face1edge3_twin,&f1e3tn);
         HalfEdge& f2e1n = result.halfedges[f2e1n_index];
         result.connect_halfedges(&face2edge1,&f2e1n);
         HalfEdge& f2e2n = result.halfedges[f2e2n_index];
         result.connect_halfedges(&face2edge2,&f2e2n);
         HalfEdge& f2e3n = result.halfedges[f2e3n_index];
         result.connect_halfedges(&face2edge3,&f2e3n);
         HalfEdge& f2e1tn = result.halfedges[f2e1tn_index];
         result.connect_halfedges(&face2edge1_twin,&f2e1tn);
         HalfEdge& f2e2tn = result.halfedges[f2e2tn_index];
         result.connect_halfedges(&face2edge2_twin,&f2e2tn);
         HalfEdge& f2e3tn = result.halfedges[f2e3tn_index];
         result.connect_halfedges(&face2edge3_twin,&f2e3tn);
      }
   }
   return result;
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
