#include "polymake/client.h"
#include "polymake/Set.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/graph/DoublyConnectedEdgeList.h"
#include "polymake/topaz/FlipVisitor.h"


namespace polymake { namespace fan {

   using namespace topaz;

typedef std::list<int> flip_sequence;

// provided a hyperbolic surface given by a doubly connected edge list, this computes all
// flip words and the coresponding secondary fan, and stores them in the surface object.
void secondary_fan_and_flipwords(perl::Object surface){

   Array<Array<int>> dcel_data = surface.give("DCEL_DATA");
   const Vector<Rational> penner_coord = surface.give("PENNER_COORDINATES");
   DoublyConnectedEdgeList dcel{dcel_data};
   dcel.setMetric( penner_coord );
   Vector<Rational> angleVec = dcel.angleVector();

   // construct the iterator that will compute the flip_words and secondary fan
   Graph<Directed> delaunay_graph(1);
   FlipVisitor fvis( delaunay_graph, dcel );
   BFSiterator< Graph<Directed>, VisitorTag<topaz::FlipVisitor> > bfs_it
         ( delaunay_graph , std::move(fvis) , nodes(delaunay_graph).front() );

   while ( !bfs_it.at_end() )
   {
      bfs_it++;
   }

   // get rays and cells of the secondary fan
   Matrix<Rational> fan_rays_matrix( bfs_it.node_visitor().getfan_num_vert() , bfs_it.node_visitor().getdim() );
   Vector<Rational> all_minus_one( -1 * ones_vector<Rational>( fan_rays_matrix.cols() ) );
   for( const auto it: bfs_it.node_visitor().getfan_vertices() )
   {
      Vector<Rational> vec = it.first;
      for( int j = 1 ; j < vec.size() ; j++ ) vec[j] = angleVec[j-1]*vec[j];
      vec = dcel.normalize( vec );
      fan_rays_matrix[ it.second ] = vec;
   }
   Matrix<Rational> secondary_fan_rays = fan_rays_matrix.minor(range_from(1), range_from(1));
   Array<Set<int>> secondary_fan_cells(bfs_it.node_visitor().getfan_cells());

   //TODO later we want a fan subobject instead
   perl::Object fan("PolyhedralFan<Rational>");
   fan.take("RAYS") << secondary_fan_rays;
   fan.take("MAXIMAL_CONES") << secondary_fan_cells;
   surface.take("SECONDARY_FAN") << fan;

   //old
   //surface.take("SECONDARY_FAN_RAYS") << secondary_fan_rays;
   //surface.take("SECONDARY_FAN_CELLS") << secondary_fan_cells;

   // get flip words
   Array< flip_sequence > flips{ bfs_it.node_visitor().getflipIds_to_node().size() };
   for ( const auto it:  bfs_it.node_visitor().getflipIds_to_node() )
   {
      flip_sequence seq( it.second );
      // remove double flips at the beginning
      if( seq.size() > 1 ){
         int front = seq.front();
         seq.pop_front();
         if( front != seq.front() ) seq.push_front(front);
         else seq.pop_front();
      }
      flips[ it.first ] = seq;
   }
   surface.take("FLIP_WORDS") << flips;
}

Function4perl(&secondary_fan_and_flipwords, "secondary_fan_and_flipwords($)");




}} //end namespaces
