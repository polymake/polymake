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

#include "polymake/client.h"  
#include "polymake/Graph.h"
#include "polymake/Set.h"
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/graph/graph_iterators.h"
#include "polymake/graph/DoublyConnectedEdgeList.h"
#include "polymake/topaz/FlipVisitor.h"

namespace polymake { namespace topaz {
/*
using namespace graph;

typedef Set< Vector<Rational> > Cone; // a 0-pointed cone is specified by its defining rays
typedef Map< Cone , int > Indexed_Cones; // each cone gets a label, namely the same label as its corresponding node in the Delaunay graph
typedef std::list<int> flip_sequence;
typedef Set< Matrix<Rational> > Cone_Inequalities;
typedef Map< Vector<Rational> , int > Fan_Vertices;
typedef std::list< Set<int> > Fan_Max_Cells;


class FlipVisitor : public NodeVisitor<> {

friend class DoublyConnectedEdgeList;
friend class SecondaryFan;

private:
   Graph< Directed >& delaunay_graph; // the graph we want to iterate through, gets build during iterating
   Indexed_Cones cones; // collect all cones, IDs as its corresponding node in the delaunay_graph
   //Map< int , DoublyConnectedEdgeList > dcel_map; // each nodeID (of the delaunay_graph) gets an DCEL
   Map< int , flip_sequence > flipIds_to_node; // for each node of the graph we save the list of indices to flip to the corresponding dcel
   DoublyConnectedEdgeList& dcel;
   Cone_Inequalities ineqs;
   Fan_Vertices fan_vertices;
   Fan_Max_Cells fan_cells;
   int fan_num_vert;
   int dim;

public:

   static const bool visit_all_edges=true; // this is needed for the BFS++ to not just stop in depth one


   FlipVisitor( Graph<Directed>& G, DoublyConnectedEdgeList& dcel ) 
      : delaunay_graph(G), dcel(dcel)
   {
      flipIds_to_node[0] = flip_sequence();
      cones[ dcel.coneRays() ] = 0; // add the first cone, from the starting dcel
      ineqs += dcel.DelaunayInequalities();
      dim = dcel.DelaunayInequalities().cols();
      Vector<Rational> origin = Vector<Rational>(dim);
      origin[0] = 1;
      fan_vertices[ origin ] = 0;
      fan_num_vert = 1;
      add_cone( dcel.coneRays() );
      
   }


   bool operator()( int n ) {
      return operator()( n , n );
   }

   

   // Preconditions: the node n_to is part of the Delaunay graph, as well is the flip sequence that transforms the base DCEL 
   // into the one corresponding to n_to, and also its cone in Indexed_Cones.
   // After this operation all the neighbors of n_to are in the graph (with the corr. edges) with their flip sequences and 
   // cones respectively
   
   bool operator()( int n_from , int n_to ) {
      
      if ( visited.contains( n_to ) ) 
         return false;
      
      // we flip the start-triangulation T(0) to the triangulation T(n_to) corresponding to node n_to
 
                                                               //cout << "FlipVisitor: wir flippen zu Kegel " << n_to << endl;
      dcel.flipEdges( flipIds_to_node[ n_to ] );
      
      // each entry of new_flipList contains a flip_sequence to a triangulation neighboring T(n_to)
      Array< flip_sequence > new_flipList = dcel.flippableEdges();
      
      // we add all the neighbors of the node n_to to the delaunay_graph, provided they haven't been added already
      // for the latter we characterize a triangulation by its secondary cone
      // we also update the Indexed_Cones and the flipIds_to_node map
      // each i in the following corresponds to a (valid) facet F(i) of T(n_to)
      for ( int i = 0 ; i < new_flipList.size() ; i++ ) {
         
         // for ( auto it = entire( flipList[i] ) ; !it.at_end() ; it++) {
           // list.flipEdge(*it);
         //}
         // 
      
      // we flip all edges of T(n_to) that are active at facet F(i)
      // for now all edges get flipped in order of their labeling
      // we compute the secondary cone of the obtained triangulation and add a new node if necessary
      // TODO: we may fix this by distiguishing commuting/non-commuting flips...
                                           
                                             //cout << "FlipVisitor: wir flippen zu neuem Nachbarkegel " << endl;
         flip_sequence new_flipIds = dcel.flipEdges_and_give_flips( new_flipList[i] , flipIds_to_node[ n_to ]);
        
                                                //cout << "Der Kegel hier ist ein neuer Kandidat: " << dcel.coneRays() << endl; 
                                       //if ( dcel.coneRays().size() < 4 ) cout << "er ist aber nicht volldimensional" << endl;

         if ( !cones.exists( dcel.coneRays() ) && dcel.coneRays().size() > 3 )  {
         
                                             //cout << "FlipVisitor: Cones hat folgende " << cones.size() << " Kegel: " << endl;
                                       //for ( auto it = entire(cones) ; !it.at_end() ; it++ ) {cout << "den hier: " << *it << endl;}


            int new_id = delaunay_graph.add_node();
                                       
                                       //cout << "FlipVisitor: neuer Nachbarkegel von " << n_to << " mit neuer id " << new_id << endl;
            delaunay_graph.add_edge( n_to , new_id );
            
            flipIds_to_node[ new_id ] = new_flipIds;
                                       
                                       //cout << "FlipVisitor: Kegel " << new_id << "  kann mit folgenden Flips erreicht werden: " << new_flipIds << endl;

            cones[ dcel.coneRays() ] = new_id;
            ineqs += dcel.DelaunayInequalities();
            add_cone( dcel.coneRays() );
         }
         
         // flip back to T(n_to)
                                             
                                           //cout << "FlipVisitor: zurück zu Kegel " << n_to << endl;
         
         dcel.flipEdges( new_flipList[i] , true );
      }
      
      // flip back to T(0)
      
                                             //cout << "FlipVisitor: zurück zu Kegel 0" << endl;

      dcel.flipEdges( flipIds_to_node[n_to] , true );
      visited += n_to;
      
      return true;
   }
   
   
   // when adding a cone we update the input data for the fan, namely the vertices and the maximal cells

   void add_cone ( Cone new_cone ){
      Set<int> fan_cell;

      for ( const auto it:  new_cone ) {

         if( !fan_vertices.exists(it) ){
            fan_vertices[ it ] = fan_num_vert;
            fan_cell += fan_num_vert-1;
            fan_num_vert++;
         }     
         else{
            if( fan_vertices[it] != 0 ) fan_cell += fan_vertices[it]-1;
         }
      
      }
      fan_cells.push_back( fan_cell );
   }


   Array< flip_sequence > getFlips() const{
      
      Array< flip_sequence > flips{ flipIds_to_node.size() };
         for ( const auto it: flipIds_to_node ){
               flips[ it.first ] = it.second;
         } 
      return flips;
   }
      
//   friend Array< flip_sequence > cone_flips();
   friend std::pair< Matrix<Rational> , Array<Set<int> > > DCEL_secondary_fan_input( DoublyConnectedEdgeList& dcel );
//   friend Indexed_Cones DCEL_secondary_fan( DoublyConnectedEdgeList& dcel );
//   friend Cone_Inequalities DCEL_secondary_fan_ineqs( DoublyConnectedEdgeList& dcel );
//   friend Matrix<Rational> DCEL_secondary_fan_input_vertices( DoublyConnectedEdgeList& dcel );
//   friend Array< Set<int> > DCEL_secondary_fan_input_cells( DoublyConnectedEdgeList& dcel );
   

}; // end class flip visitor

*/

/*class SecondaryFan{
   
friend class FlipVisitor;
private:
   DoublyConnectedEdgeList& surface;
   Matrix<Rational> fan_rays;
   Array< Set<int> > fan_cells;
   Array< flip_sequence > flip_words;
public:
   SecondaryFan( DoublyConnectedEdgeList& dcel ) 
      : surface( dcel ) 
   {
      Graph<Directed> delaunay_graph;
      delaunay_graph.add_node();
      FlipVisitor fvis( delaunay_graph, surface );
      BFSiterator< Graph<Directed>, VisitorTag<FlipVisitor> > bfs_it( delaunay_graph , std::move(fvis) , nodes(delaunay_graph).front() );
      
      while ( !bfs_it.at_end() ) {
         bfs_it++;
      }

      Matrix<Rational> fan_vert_matrix{ bfs_it.node_visitor().fan_num_vert , bfs_it.node_visitor().dim };
      for( const auto it: bfs_it.node_visitor().fan_vertices ){
           fan_vert_matrix[ it.second ] = it.first;
           fan_vert_matrix[ it.second ][0] = 1;
      }
      fan_rays = fan_vert_matrix.minor(range_from(1), All);
      
      int size = bfs_it.node_visitor().fan_cells.size();
      Array< Set<int> > fan_cells_array{ size };
      int curr = 0;
      for( auto  it = bfs_it.node_visitor().fan_cells.begin() ; it!=bfs_it.node_visitor().fan_cells.end() ; ++it ){
         fan_cells_array[ curr ] = *it;
         curr++;
      }
      fan_cells = fan_cells_array;
      
      Array< flip_sequence > flips{ bfs_it.node_visitor().flipIds_to_node.size() };
      for ( const auto it:  bfs_it.node_visitor().flipIds_to_node ){
         flips[ it.first ] = it.second;
      }
      flip_words = flips;
   }

   
   Array< Set<int> > getFanCells(){
      return fan_cells;
   }
   UserFunction4perl("# @category Producing other objects"
                     "# some strange function",
                     &getFanCells,"getFanCells()");
   
   Matrix<Rational> getFanRays(){
      return fan_rays;
   }
   UserFunction4perl("# @category Producing other objects"
                     "# some strange function",
                     &getFanRays,"getFanRays()");

   Array< flip_sequence > getFlipWords(){
      return flip_words;
   }
   UserFunction4perl("# @category Producing other objects"
                     "# some strange function",
                   &getFlipWords,"getFlipWords()");
}; //end class SecondaryFan
*/


std::pair< Matrix<Rational> , Array<Set<int> > > DCEL_secondary_fan_input( DoublyConnectedEdgeList& dcel ){

   Graph<Directed> delaunay_graph;
   delaunay_graph.add_node();
   FlipVisitor fvis(delaunay_graph,dcel);

   BFSiterator< Graph<Directed>, VisitorTag<FlipVisitor> > bfs_it( delaunay_graph , std::move(fvis) , nodes(delaunay_graph).front() );

   while ( !bfs_it.at_end() ) {
   bfs_it++;
   }
//cout << "hoppla" << endl;
   Matrix<Rational> fan_vert_matrix{ bfs_it.node_visitor().fan_num_vert , bfs_it.node_visitor().dim };
      for( const auto it: bfs_it.node_visitor().fan_vertices ){
         fan_vert_matrix[ it.second ] = it.first;
         fan_vert_matrix[ it.second ][0] = 1;
      }

   int size = bfs_it.node_visitor().fan_cells.size();
   Array< Set<int> > fan_cells_array{ size };
   int curr = 0;
   for( auto  it = bfs_it.node_visitor().fan_cells.begin() ; it!=bfs_it.node_visitor().fan_cells.end() ; ++it ){
      fan_cells_array[ curr ] = *it;
      curr++;
   //cout << "zu fan_cells kommt " << *it << " dazu" << endl;
   }
   std::pair< Matrix<Rational> , Array<Set<int> > > input_pair;
   input_pair.first = fan_vert_matrix.minor(range_from(1), All);
   input_pair.second = fan_cells_array;
   
   return input_pair; 
}
#if 0
UserFunction4perl("# @category Producing other objects"
                  "# some strange function",
                  &DCEL_secondary_fan_input, "DCEL_secondary_fan_input(DoublyConnectedEdgeList&)");
#endif


/*Indexed_Cones DCEL_secondary_fan( DoublyConnectedEdgeList& dcel ) {

   Graph<Directed> delaunay_graph;
   delaunay_graph.add_node();
   FlipVisitor fvis(delaunay_graph,dcel);

   BFSiterator< Graph<Directed>, VisitorTag<FlipVisitor> > bfs_it( delaunay_graph , std::move(fvis) , nodes(delaunay_graph).front() );

   while ( !bfs_it.at_end() ) {
      bfs_it++; 
   }
   
   return bfs_it.node_visitor().cones;
}

UserFunction4perl("# @category Producing other objects"
                  "# some strange function",
                  &DCEL_secondary_fan, "DCEL_secondary_fan(DoublyConnectedEdgeList&)");




Cone_Inequalities DCEL_secondary_fan_ineqs( DoublyConnectedEdgeList& dcel ) {

   Graph<Directed> delaunay_graph;
   delaunay_graph.add_node();
   FlipVisitor fvis(delaunay_graph,dcel);

   BFSiterator< Graph<Directed>, VisitorTag<FlipVisitor> > bfs_it( delaunay_graph , std::move(fvis) , nodes(delaunay_graph).front() );

   while ( !bfs_it.at_end() ) {
      bfs_it++;   
      }

   return bfs_it.node_visitor().ineqs;
}

UserFunction4perl("# @category Producing other objects"
                  "# some strange function2",                                             
                  &DCEL_secondary_fan_ineqs, "DCEL_secondary_fan_ineqs(DoublyConnectedEdgeList&)");



Matrix<Rational> DCEL_secondary_fan_input_vertices( DoublyConnectedEdgeList& dcel ) {

   Graph<Directed> delaunay_graph;
   delaunay_graph.add_node();
   FlipVisitor fvis(delaunay_graph,dcel);

   BFSiterator< Graph<Directed>, VisitorTag<FlipVisitor> > bfs_it( delaunay_graph , std::move(fvis) , nodes(delaunay_graph).front() );

   while ( !bfs_it.at_end() ) {
      bfs_it++;
      }

   Matrix<Rational> fan_vert_matrix{ bfs_it.node_visitor().fan_num_vert , bfs_it.node_visitor().dim };
   for( const auto it: bfs_it.node_visitor().fan_vertices ){
      fan_vert_matrix[ it.second ] = it.first;
      fan_vert_matrix[ it.second ][0] = 1;      
   }
   return fan_vert_matrix;
   }

#if 0
   UserFunction4perl("# @category Producing other objects"  
                     "# some strange function2",
                     &DCEL_secondary_fan_input_vertices, "DCEL_secondary_fan_input_vertices(DoublyConnectedEdgeList&)");
#endif



Array< Set<int> > DCEL_secondary_fan_input_cells( DoublyConnectedEdgeList& dcel ) {

   Graph<Directed> delaunay_graph;
   delaunay_graph.add_node();
   FlipVisitor fvis(delaunay_graph,dcel);

   BFSiterator< Graph<Directed>, VisitorTag<FlipVisitor> > bfs_it( delaunay_graph , std::move(fvis) , nodes(delaunay_graph).front() );

   while ( !bfs_it.at_end() ) {
      bfs_it++;
      }
   
   int size = bfs_it.node_visitor().fan_cells.size();
   Array< Set<int> > fan_cells_array{ size };
   int curr = 0;
   for( auto  it = bfs_it.node_visitor().fan_cells.begin() ; it!=bfs_it.node_visitor().fan_cells.end() ; ++it ){
      fan_cells_array[ curr ] = *it;
      curr++;
      }
   
   return fan_cells_array;
   }
#if 0
   UserFunction4perl("# @category Producing other objects"  
                     "# some strange function2",
                     &DCEL_secondary_fan_input_cells, "DCEL_secondary_fan_input_cells(DoublyConnectedEdgeList&)");
#endif
*/

} //end topaz namespace
} //end polymake namespace

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
