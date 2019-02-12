/* Copyright (c) 1998-2016
   Ewgenij Gawrilow, Michael Joswig (Technische Universitaet Berlin, Germany)
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


#ifndef POLYMAKE_DOUBLY_CONNECTED_EDGE_LIST_H
#define POLYMAKE_DOUBLY_CONNECTED_EDGE_LIST_H

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

   class Vertex {

      private:
         // incoming half edge
         HalfEdge* incidentEdge;

      public:
         Vertex() : incidentEdge(NULL){
         };

         HalfEdge* getIncidentEdge() const{
            return this->incidentEdge;
         };

         void setIncidentEdge(HalfEdge* edge){
            this->incidentEdge = edge;
         };

   }; // end class Vertex



   class HalfEdge {

      protected:
         HalfEdge* twin;
         HalfEdge* next;
         HalfEdge* prev;
         Vertex* head;
         Rational length;

      public:
         HalfEdge():
            twin(NULL),
            next(NULL),
            prev(NULL),
            head(NULL),
            length(1)
      {
      };

         Rational getLength() const{
            return length;
         }

         void setLength ( Rational newLength ) {
            length = newLength;
         }

         HalfEdge* getTwin(){
            return this->twin;
         };
         const HalfEdge* getTwin() const {
            return this->twin;
         };

         void setTwin(HalfEdge* newTwin){
            this->twin = newTwin;
            newTwin->twin = this;
         };

         HalfEdge* getNext(){
            return this->next;
         };

         const HalfEdge* getNext() const {
            return this->next;
         };

         void setNext(HalfEdge* newNext){
            this->next = newNext;
            newNext->prev = this;
         };

         HalfEdge* getPrev(){
            return this->prev;
         };

         const HalfEdge* getPrev() const {
            return this->prev;
         };

         void setPrev(HalfEdge* newPrev){
            this->prev = newPrev;
            newPrev->next = this;
         };

         Vertex* getHead(){
            return this->head;
         };

         const Vertex* getHead() const {
            return this->head;
         };

         void setHead(Vertex* newHead){
            this->head = newHead;
            newHead->setIncidentEdge( this );
         };

   }; // end class HalfEdge


#define NULL_ID ( std::numeric_limits<int>::max() )

   class DoublyConnectedEdgeList {

      typedef std::list<int> flip_sequence;

      protected:
      Array<Vertex> vertices;
      Array<HalfEdge> edges;

      public:
      DoublyConnectedEdgeList(){}

      DoublyConnectedEdgeList( DoublyConnectedEdgeList const &list )
         : vertices( list.vertices )
           , edges( list.edges )
      {}

      Array<Vertex> getVertices() const
      {
         return this->vertices;
      }


      Array<HalfEdge> getEdges() const
      {
         return this->edges;
      }


      /*
         Construct a DCEL out of a given half edge list.
         Each list element reads: id of head-vertex, id of tail-vertex, id of next half edge, id of twin-next half edge
         */
      DoublyConnectedEdgeList( const Array< Array<int> >& half_edge_list )
      {
         int num_edges = half_edge_list.size();
         int num_vertices = 0;
         for( int i = 0 ; i < num_edges ; i++ )
         {
            if( half_edge_list[i][0] > num_vertices ) num_vertices = half_edge_list[i][0];
            if( half_edge_list[i][1] > num_vertices ) num_vertices = half_edge_list[i][1];
         }
         num_vertices++;

         vertices = Array<Vertex>(num_vertices);
         edges = Array<HalfEdge>(num_edges*2);
         for ( int i = 0; i < half_edge_list.size(); ++i )
         {
            setEdgeIncidences( i, half_edge_list[i][0], half_edge_list[i][1], half_edge_list[i][2], half_edge_list[i][3] );
         }

         //validate();
      }

      DoublyConnectedEdgeList( const Array< Array<int> >& half_edge_list , Vector<Rational> penner_coords) :
         DoublyConnectedEdgeList(half_edge_list) {
            setMetric(penner_coords);
         }


      /*
         //Sanity check for a DCEL.
      void validate(){
         int n_edges = edges.size();
         //validating edges
         for ( int i = 0; i < n_edges; ++i ){
            HalfEdge* e = &(edges[i]);
            if(e->getTwin()->getTwin() != e)
               cout<<"INCONSISTENCY WARNING: "<<getHalfEdgeId(e)<<".twin.twin is "
                  <<getHalfEdgeId(e->getTwin()->getTwin())<<endl;

            if(e->getHead() != e->getNext()->getTwin()->getHead())
               cout<<"INCONSISTENCY WARNING: head of "<<getHalfEdgeId(e)<<" is "
                  <<e->getHead()<<" instead of "<<getVertexId(e->getTwin()->getHead())<<endl;

            if(e->getNext()->getNext()->getNext() != e)
               cout<<"INCONSISTENCY WARNING: "<<getHalfEdgeId(e)<<".next.next.next is "
                  <<getHalfEdgeId(e->getNext()->getNext()->getNext())<<endl;

            int count = 0;
            HalfEdge* c = e->getTwin();
            while(count <= n_edges)
            {
               if(count == n_edges)
                  cout<<"INCONSISTENCY WARNING: started at "<<getHalfEdgeId(e)<<" and never reached it again"<<endl;
               c = c->getTwin()->getNext()->getNext();
               if(getHalfEdgeId(c) == getHalfEdgeId(e->getTwin()))
                  break;
               count++;
            }
         }

         //validating verts
         for ( int i = 0; i < vertices.size(); ++i ){
            Vertex* v = &(vertices[i]);
            if(v->getIncidentEdge()->getHead() != v)
               cout<<"INCONSISTENCY WARNING: head of "<<getHalfEdgeId(v->getIncidentEdge())<<" is "
                  <<v->getIncidentEdge()->getHead()<<" instead of "<<v<<endl;
            if(v->getIncidentEdge() == NULL)
               cout<<"INCONSISTENCY WARNING: incident egde of "<<v<<" is not set";
         }
      }
      */



      // set the incidences of an edge (which are two half edges) according to the input
      void setEdgeIncidences (int halfEdgeId, int headId, int twinHeadId, int nextId, int twinNextId)
      {
         HalfEdge* halfEdge = &( this->edges[2*halfEdgeId] );
         halfEdge->setHead( getVertex( headId ) );
         halfEdge->setNext( getHalfEdge( nextId ) );

         HalfEdge* twinHalfEdge = &( this->edges[2*halfEdgeId+1] );
         twinHalfEdge->setHead( getVertex( twinHeadId ) );
         twinHalfEdge->setNext( getHalfEdge( twinNextId ) );

         halfEdge->setTwin( twinHalfEdge );
      }


      // return the edges-vertices incidence matrix
      SparseMatrix<int> EdgeVertexIncidenceMatrix () const
      {
         SparseMatrix<int> Matrix( getNumEdges(), vertices.size() );

         for ( int i = 0 ; i < getNumEdges() ; ++i)
         {
            const HalfEdge* halfEdge = &( this->edges[2*i] );
            Matrix[i][ getVertexId( halfEdge->getHead() ) ] = 1;
            Matrix[i][ getVertexId( halfEdge->getTwin()->getHead() ) ] = 1;
         }

         return Matrix;
      }



      // return true if the edge of index 'edgeId' is flippable, the two half edges have id 'edgeId' and 'edgeId'+1
      bool isFlippable( int edgeId ) const
      {
         const HalfEdge* halfEdge = &( edges[ 2*edgeId ] );
         return ( halfEdge != halfEdge->getNext()
               && halfEdge != halfEdge->getNext()->getNext()
               && halfEdge != halfEdge->getNext()->getTwin()
               && halfEdge != halfEdge->getNext()->getNext()->getTwin() );
      }


      // flip half edge and its twin ccw
      void flipHalfEdge( HalfEdge* halfEdge )
      {
         HalfEdge* twin = halfEdge->getTwin();
         HalfEdge* a = halfEdge->getNext();
         HalfEdge* b = a->getNext();
         HalfEdge* c = twin->getNext();
         HalfEdge* d = c->getNext();

         if(halfEdge->getHead()->getIncidentEdge() == halfEdge)
            halfEdge->getHead()->setIncidentEdge(d);
         if(twin->getHead()->getIncidentEdge() == twin)
            twin->getHead()->setIncidentEdge(b);

         Rational newLength = ( ( a->getLength() * c->getLength() ) + ( b->getLength() * d->getLength() ) ) / halfEdge->getLength();
         halfEdge->setLength( newLength );
         twin->setLength( newLength );

         halfEdge->setHead( a->getHead() );
         halfEdge->setNext( b );
         b->setNext( c );
         c->setNext( halfEdge );

         twin->setHead( c->getHead() );
         twin->setNext( d );
         d->setNext( a );
         a->setNext( twin );

         //validate();
      }


      // flip edge of index 'edgeId'
      void flipEdge( int edgeId )
      {
         HalfEdge* halfEdge = &( this->edges[2*edgeId] );
         if ( halfEdge != halfEdge->getNext()
               && halfEdge != halfEdge->getNext()->getNext()
               && halfEdge != halfEdge->getNext()->getTwin()
               && halfEdge != halfEdge->getNext()->getNext()->getTwin() )
         {
            flipHalfEdge( halfEdge );
         }
      }



      // unflip half edge and its twin ccw
      void unflipHalfEdge( HalfEdge* halfEdge )
      {
         HalfEdge* twin = halfEdge->getTwin();
         HalfEdge* a = halfEdge->getNext();
         HalfEdge* b = a->getNext();
         HalfEdge* c = twin->getNext();
         HalfEdge* d = c->getNext();

         if(halfEdge->getHead()->getIncidentEdge() == halfEdge)
            halfEdge->getHead()->setIncidentEdge(d);
         if(twin->getHead()->getIncidentEdge() == twin)
            twin->getHead()->setIncidentEdge(b);

         Rational newLength = ( ( a->getLength() * c->getLength() ) + ( b->getLength() * d->getLength() ) ) / halfEdge->getLength();
         halfEdge->setLength( newLength );
         twin->setLength( newLength );

         halfEdge->setHead( c->getHead() );
         halfEdge->setNext( d );
         d->setNext( a );
         a->setNext( halfEdge );

         twin->setHead( a->getHead() );
         twin->setNext( b );
         b->setNext( c );
         c->setNext( twin );

         //validate();
      }


      // unflip edge of index 'edgeId'
      void unflipEdge( int edgeId )
      {
         HalfEdge* halfEdge = &( this->edges[2*edgeId] );
         if ( halfEdge != halfEdge->getNext()
               && halfEdge != halfEdge->getNext()->getNext()
               && halfEdge != halfEdge->getNext()->getTwin()
               && halfEdge != halfEdge->getNext()->getNext()->getTwin() )
         {
            unflipHalfEdge( halfEdge );
         }
      }


      // return the total number of vertices
      int getNumVertices() const
      {
         return this->vertices.size();
      }


      // returns the number of half edges
      int getNumHalfEdges() const
      {
         return this->edges.size();
      }


      // returns the number of edges
      int getNumEdges() const
      {
         int a = getNumHalfEdges() / 2;
         return a;
      }


      // return the index of the given vertex
      int getVertexId(const Vertex* vertex) const
      {
         for( int i = 0 ; i < vertices.size() ; ++i )
         {
            if( &(this->vertices[i] ) == vertex )
               return i;
         }
         return NULL_ID;
      }


      // return a pointer to the vertex with the given id
      Vertex* getVertex( int id )
      {
         Vertex* v = &( this->vertices[id] );
         return v;
      }


      // return the index of the given halfedge
      int getHalfEdgeId( const HalfEdge* halfEdge ) const
      {
         for( int i = 0 ; i < edges.size() ; ++i )
         {
            if( &(this->edges[i] ) == halfEdge )
               return i;
         }
         return NULL_ID;
      }


      // return a pointer to the half edge with the given id
      HalfEdge* getHalfEdge( int id )
      {
         HalfEdge* e = &( this->edges[id] );
         return e;
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
      Vector<int> getQuadId( int id ) const
      {
         Vector<int> quadVector = Vector<int>(8);
         const HalfEdge* halfEdge = &( this->edges[id] );

         int kl = getHalfEdgeId( halfEdge->getNext() );
         int il = getHalfEdgeId( halfEdge->getNext()->getNext() );
         int ij = getHalfEdgeId( halfEdge->getTwin()->getNext() );
         int jk = getHalfEdgeId( halfEdge->getTwin()->getNext()->getNext() );

         int i = getVertexId( halfEdge->getTwin()->getHead() );
         int j = getVertexId( halfEdge->getTwin()->getNext()->getHead() );
         int k = getVertexId( halfEdge->getHead() );
         int l = getVertexId( halfEdge->getNext()->getHead() );
         quadVector[0] = i;
         quadVector[1] = ij;
         quadVector[2] = j;
         quadVector[3] = jk;
         quadVector[4] = k;
         quadVector[5] = kl;
         quadVector[6] = l;
         quadVector[7] = il;

         return quadVector;
      }


      // set the lengths of the edges according to the input vector
      void setMetric( Vector< Rational > metric )
      {
         for ( int i = 0 ; i < getNumEdges() ; i++ )
         {
            edges[ 2*i ].setLength( metric[i] );
            edges[ 2*i+1 ].setLength( metric[i] );
         }
      }


      // return the lengths of the edges
      Vector< Rational > edgeLengths() const
      {
         Vector< Rational > metric( getNumEdges() );
         for ( int i = 0 ; i < getNumEdges() ; i++ ) metric[i] = edges[ 2*i ].getLength();
         return metric;
      }


      // calcutate the inequalities that define the secondary cone
      Matrix<Rational> DelaunayInequalities() const
      {
         Matrix<Rational> M( getNumEdges() + getNumVertices() , getNumVertices() + 1 );
         for ( int a = 0; a < getNumEdges(); a++ )
         {
            Vector<int> quadId = getQuadId(2*a);

            Rational ik = edges[ 2*a ].getLength();
            Rational kl = edges[ quadId[5] ].getLength();
            Rational il = edges[ quadId[7] ].getLength();
            Rational ij = edges[ quadId[1] ].getLength();
            Rational jk = edges[ quadId[3] ].getLength();

            int i = quadId[0];
            int j = quadId[2];
            int k = quadId[4];
            int l = quadId[6];

            M[a][i+1] += kl/(il*ik) + jk/(ij*ik);
            M[a][k+1] += il/(ik*kl) + ij/(ik*jk);
            M[a][j+1] += -ik/(ij*jk);
            M[a][l+1] += -ik/(il*kl);

         }

         for ( int j = 0 ; j < getNumVertices() ; j++ )
         {
            M[ getNumEdges() + j ][ j + 1 ] = 1 ;
         }
         return remove_zero_rows(M);
      }


      // for each valid facet of the secondary cone we collect the indices of those edges whose Delaunay inequalities define that facet
      Array< flip_sequence > flippableEdges( flip_sequence list_arg = std::list<int>() ) const
      {
         perl::Object p("polytope::Polytope<Rational>");
         Matrix<Rational> M = DelaunayInequalities();
         p.take("INEQUALITIES") << M;
         //IncidenceMatrix<> rays_in_facets = p.give("VERTICES_IN_FACETS");
         //Matrix<Rational> rays = p.give("VERTICES");
         Matrix<Rational> facets = p.give("FACETS");

         int numFacets = facets.rows() - 1;  // -1 for the far face [1:0: ... :0]

         Array< flip_sequence > flipList( numFacets );

         for ( int i = 0 ; i < numFacets ; i++ )
         {                                   // let's hope the far facet is always the last one...
            if ( validFacet( facets.row(i) ) )
            {
               flip_sequence active_edges_at_facet_i = list_arg;

               for ( int j = 0 ; j < M.rows() ; j++ )
               {
                  if ( is_equiv( M.row(j) , facets.row(i) ) )
                     active_edges_at_facet_i.push_back(j);
               }

               flipList[i] = active_edges_at_facet_i;
               }
            }

            return flipList;
         }


         Matrix<Rational> coneFacets() const
         {
            perl::Object p("polytope::Polytope<Rational>");
            p.take("INEQUALITIES") << DelaunayInequalities();
            return p.give("FACETS");
         }



         // return a set that contains the rays of the secondary cone, we normalize the rays
         Set< Vector<Rational> > coneRays() const
         {
            Set< Vector<Rational> > ray_set;
            perl::Object p("polytope::Polytope<Rational>");
            Matrix<Rational> M = DelaunayInequalities();
            p.take("INEQUALITIES") << M;
            Matrix<Rational> rays = p.give("VERTICES");

            for ( int i = 0 ; i < rays.rows() ; i++ )
               ray_set += normalize( rays.row(i) );

            return ray_set;
         }


         // normalize the Vector in the positive orthant by deviding by its 1-norm
         Vector<Rational> normalize( Vector<Rational> vector_arg ) const
         {
            Vector<Rational> normalized_vector( vector_arg );
            Rational norm = 0;
            for ( int i = 0 ; i < vector_arg.size() ; i++ )
               norm += vector_arg[i];

            normalized_vector.div_exact( norm );
            return normalized_vector;
         }


         // check if the facet is a potential candidate to flip the corresponding edges in the triangulation of the surface
         // we exclude the far facet ( 1 : 0 : ... : 0 ) and the coordinate hyperplanes ( 0 : ... : 1 : 0 : ... : 0 ) as well as ( 0 : ... : 0 )
         bool validFacet( Vector<Rational> facet_normal ) const
         {
            return ( nonZeros( facet_normal ) > 1 );
         }

         int nonZeros( Vector<Rational> facet_normal ) const
         {
            int non_zeros = 0;
            for ( int i = 0 ; i < facet_normal.size() ; i++ )
               if ( facet_normal[i] != 0 ) non_zeros++;

            return non_zeros;
         }



         // flip the edges of the given indices and in the given order: false = [left->right], true = [right->left]
         // return the flip_sequence, where possible former flips are included if given as optional input
         flip_sequence flipEdges_and_give_flips( flip_sequence edgeIds, flip_sequence former_flips = flip_sequence() , bool reverse = false )
         {
            if ( !reverse )
            {
               for( std::list<int>::iterator it = edgeIds.begin() ; it!=edgeIds.end() ; ++it )
               {
                  flipEdge( *it );
                  former_flips.push_back( *it );
               }
            }

            if ( reverse )
            {
               for ( std::list<int>::reverse_iterator rit = edgeIds.rbegin() ; rit!=edgeIds.rend(); ++rit )
               {
                  unflipEdge( *rit );
                  former_flips.push_back( *rit );
               }
            }
            return former_flips;
         }


         //  flip the edges of the given indices and in the given order: false = [left->right], true = [right->left]
         void flipEdges( flip_sequence edgeIds , bool reverse = false )
         {
            if ( !reverse )
            {
               for( std::list<int>::iterator it = edgeIds.begin() ; it!=edgeIds.end() ; ++it )
               {
                  flipEdge( *it );
               }
            }

            if ( reverse ) {
               for ( std::list<int>::reverse_iterator rit = edgeIds.rbegin() ; rit!=edgeIds.rend(); ++rit )
               {
                  unflipEdge( *rit );
               }
            }
         }


         //  we flip those edges whose inequalities are equivalent to the facet normal until the there is no such edge, we only flip through valid facets
         flip_sequence flipThroughFace( Vector<Rational> facet_normal , flip_sequence former_flips = flip_sequence() )
         {
            if( validFacet(facet_normal) )
            {
               // we perform at most this many flips to get through this facet
               int upper_flip_bound = 10 * facet_normal.size();

               int row_id = first_equiv_row( facet_normal );
               int counter = 0;

               while( row_id != -1 )
               {
                  // this does not change the actual former_flips, we work on a copy instead
                  former_flips.push_back( row_id );
                  flipEdge( row_id );
                  counter++;
                  row_id = first_equiv_row( facet_normal );
                  if ( counter > upper_flip_bound )
                  {
                     row_id = -1;
                     cout << "DoublyConnectedEdgeList->FlipThroughFace:" <<
                        "suggested number of flips exceeded 'upper flip bound'" << endl;
                  }
               }
               Vector<Rational> neighbor_facet{ -1 * facet_normal };
               if( first_equiv_row( neighbor_facet ) == -1 )
                  cout << "DoublyConnectedEdgeList->FlipThroughFace: new cone is not facet-neighbor" << endl;
               return former_flips;
            }
            flip_sequence no_flips = flip_sequence();
            return no_flips;
         }


         // return the index of the first Delaunay inequality matrix that is equivalent to the given inequality "ineq"; return -1 if there is no such row
         int first_equiv_row(const Vector<Rational>& ineq) const
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
           if (rank(vector2row(ineq_a)/ineq_b) == 1) {
             for (int i = 0 ; i < ineq_a.size(); ++i) {
               if (ineq_a[i] != 0) return ineq_b[i]/ineq_a[i] > 0;
             }
           }
           return false;
         }




         // check if the the edge with index id is Delaunay after scaling the horocycles by the weights
         bool is_Delaunay( int id , Vector<Rational> weights )
         {
            Vector<int> quadId = getQuadId(2*id);

            Rational ik = edges[ 2*id ].getLength();
            Rational kl = edges[ quadId[5] ].getLength();
            Rational il = edges[ quadId[7] ].getLength();
            Rational ij = edges[ quadId[1] ].getLength();
            Rational jk = edges[ quadId[3] ].getLength();
            int i = quadId[0];
            int j = quadId[2];
            int k = quadId[4];
            int l = quadId[6];

            // the +1 is because wstill have this 0 at the 0th position of every ray...
            return ( weights[i+1]*( kl/(il*ik) + jk/(ij*ik) ) + weights[k+1]*( il/(ik*kl) + ij/(ik*jk) ) >=
                  weights[j+1]*( ik/(ij*jk )) + weights[l+1]*( ik/(il*kl) ) );

         }



         // check if the triangulation is Delaunay w.r.t. the given weights, return id of the first edge that is not Delaunay or -1 if the triangulation is Delaunay
         int is_Delaunay( Vector<Rational> weights )
         {
            for( int i = 0 ; i < getNumEdges() ; i++ )
            {
               if( !is_Delaunay(i , weights ) ) return i;
            }
            return -1;
         }



         Vector<int> DelaunayConditions( Vector<Rational> weights )
         {
            Vector<int> condition_vector( getNumEdges() );
            for( int id = 0 ; id < getNumEdges() ; id++ )
            {
               Vector<int> quadId = getQuadId(2*id);

               Rational ik = edges[ 2*id ].getLength();
               Rational kl = edges[ quadId[5] ].getLength();
               Rational il = edges[ quadId[7] ].getLength();
               Rational ij = edges[ quadId[1] ].getLength();
               Rational jk = edges[ quadId[3] ].getLength();
               int i = quadId[0];
               int j = quadId[2];
               int k = quadId[4];
               int l = quadId[6];
               Rational left{ weights[i+1]*( kl/(il*ik) + jk/(ij*ik) ) + weights[k+1]*( il/(ik*kl) + ij/(ik*jk) ) };
               Rational right{ weights[j+1]*( ik/(ij*jk )) + weights[l+1]*( ik/(il*kl) ) };
               if (left > right )
                  condition_vector[id] = 1;
               else if ( left == right )
                  condition_vector[id] = 0;
               else
                  condition_vector[id] = -1;
            }
            return condition_vector;
         }

         // the flip algorithm, we flip edges that are non-Delaunay w.r.t. the weights as long as there are some
         flip_sequence flipToDelaunayAlt( Vector<Rational> weights )
         {
            flip_sequence flip_ids{};
            int non_delaunay = is_Delaunay( weights );
            while( non_delaunay != -1 )
            {
               flipEdge( non_delaunay );
               flip_ids.push_back( non_delaunay );
               non_delaunay = is_Delaunay( weights );
            }
            return flip_ids;
         }

         // return the angle sum of the vertex of index id
         Rational angleSum( int id )
         {
            Rational sum;
            Vertex* v = getVertex(id);
            HalfEdge* e = v->getIncidentEdge();
            HalfEdge* a = e->getTwin();
            HalfEdge* b = a->getNext();
            HalfEdge* c = b->getNext();
            Rational angle;
            angle = b->getLength() / ( a->getLength() * c->getLength() );
            sum = angle;
            while( getHalfEdgeId(c) != getHalfEdgeId(e) )
            {
               a = c->getTwin();
               b = a->getNext();
               c = b->getNext();
               angle = b->getLength() / ( a->getLength() * c->getLength() );
               sum = sum + angle;
            }
            return sum;
         }

         // return the angle sum vector
         Vector<Rational> angleVector()
         {
            Vector<Rational> angleVec( vertices.size() );
            for( int i = 0 ; i < vertices.size() ; i++ )
            {
               angleVec[i] = angleSum(i);
            }
            return angleVec;
         }



      }; // end class DoublyConnectedEdgeList

   } // end graph namespace
} // end polymake namespace
#endif // POLYMAKE_DOUBLY_CONNECTED_EDGE_LIST_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil

