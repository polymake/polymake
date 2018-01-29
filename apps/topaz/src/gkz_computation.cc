#include "polymake/client.h"
#include "polymake/Set.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/graph/DoublyConnectedEdgeList.h"
#include "polymake/topaz/DomeVolumeVisitor.h"
#include "polymake/topaz/CoveringTriangulationVisitor.h"
#include "polymake/IncidenceMatrix.h"

namespace polymake { namespace topaz {

typedef std::list<int> flip_sequence;

// compute horocycle at w, given the three lengths of a triangle (u,v,w) and the horocycles at u and v
// the determinant along (u,v) needs to be positive!
Vector<Rational> thirdHorocycle( Vector<Rational> horo_u , Vector<Rational> horo_v , Rational lambda_uv , Rational lambda_vw , Rational lambda_wu )
{
   if( (horo_u[0]*horo_v[1]) - (horo_u[1]*horo_v[0]) <= 0 ) cout << "thirdHorocycle: determinant not positive" << endl;
   Vector<Rational> horo_w(2);   
   horo_w[0] = ( -1*( horo_u[0]*lambda_vw + horo_v[0]*lambda_wu ) )/lambda_uv;
   horo_w[1] = ( -1*( horo_u[1]*lambda_vw + horo_v[1]*lambda_wu ) )/lambda_uv;
   return horo_w;
}

// compute horo matrix of (unflipped) zero edge given horo coordinates
Matrix<Rational> compute_horo(DoublyConnectedEdgeList& dcel, Rational p_inf, Rational zero_head){

   HalfEdge zero = *(dcel.getHalfEdge(0));

   Rational q = zero.getLength()/p_inf;
   Rational p = zero_head*q;

   Matrix<Rational> horo(2,2);
   horo[0][0] = p_inf; horo[0][1] = 0;
   horo[1][0] = p;     horo[1][1] = q;
   return horo;
}

// Calculate the horo matrix of the (flipped) zeroth edge in dependence of the surrounding quadrangle,
// which is currently triangulated by (0,a,b) and (1,c,d).
Matrix<Rational> compute_horo_flipped(DoublyConnectedEdgeList& dcel, Matrix<Rational> horo_old){

   HalfEdge zero = *(dcel.getHalfEdge(0));

   Vector<Rational> horo_tail = horo_old[0];
   Vector<Rational> horo_head = horo_old[1];

   // horocycle third_zero=[p_1,q_1] at corner (a,b)
   HalfEdge a = *(zero.getNext());
   HalfEdge b = *(a.getNext());
   Vector<Rational> third_zero = thirdHorocycle( horo_tail, horo_head, zero.getLength(), a.getLength(), b.getLength() );

   // horocycle third_one=[p_2,q_2] at corner (c,d)
   HalfEdge one = *(zero.getTwin());
   HalfEdge c = *(one.getNext());
   HalfEdge d = *(c.getNext());
   Vector<Rational> third_one = thirdHorocycle( horo_head, -1*horo_tail, one.getLength(), c.getLength(), d.getLength() );

   // horocycles at the tail and head of the flipped edge 0 are -[p_2,q_2] and [p_1,q_1] respectively.
   Matrix<Rational> horo(2,2);
   horo[0] = -1*third_one;
   horo[1] = third_zero;
   return horo;
}


//Compute VERTICES_IN_FACETS of a a secondary polytope
IncidenceMatrix<> secPolyVif( Matrix<Rational> rays , IncidenceMatrix<> cells ){
   IncidenceMatrix<> M( rays.rows() , cells.rows() + rays.cols() );
   for ( int i = 0 ; i < rays.rows() ; ++i ) {
      for ( int j = 0 ; j < cells.rows(); ++j ) {
         if( cells[j].contains(i) ) {
            M(i,j) = 1;
         }
      }
      for ( int k = 0 ; k < rays.cols() ; ++k ) {
         if( rays[i][k] == 0 ) {
            M(i,cells.rows()+k) = 1;
         }
      }
   }
   return M;
}


// the main function. returns the secondary polyhedron of the triangulation given by the surface,
// computed up to the given dual tree depth, and laying out the zeroth half edge as specified by the
// coordinated p_inf and zero_head.
Matrix<Rational> gkz_vectors(perl::Object surface, int depth){
   
   if(depth<0)
   {
      throw std::runtime_error("gkz_vectors: invalid depth");
   }

   const Array< Array<int>> dcel_data = surface.give("DCEL_DATA");
   const Vector<Rational> penner_coord = surface.give("PENNER_COORDINATES");
   const Array< flip_sequence> flip_words = surface.give("FLIP_WORDS");
   std::pair<Rational, Rational> first_horo = surface.give("SPECIAL_POINT");
   Rational p_inf = first_horo.first;
   Rational zero_head = first_horo.second;


   // Set up the surface from dcel_data:
   DoublyConnectedEdgeList dcel = DoublyConnectedEdgeList(dcel_data);
   dcel.setMetric(penner_coord);

   // Initialize vertex matrix of secondary polyhedron.
   int num_vertices = dcel.getNumVertices();
   Matrix<Rational> vert( flip_words.size() , num_vertices );

   // Loop that visits every maximal cone of the secondary fan.
   // We flip to the corresponding Delaunay triangulation, calculate the GKZ vector via the DomeVolumeVisitor,
   // flip back, and start over.
   for( int i = 0 ; i < flip_words.size() ; i++ )
   {
      // We compute the position of the zeroth half edge (index 0) in the upper half plane.
      Matrix<Rational> zero_horo = compute_horo(dcel, p_inf, zero_head);

      for( std::list<int>::const_iterator it = flip_words[i].begin() ; it!=flip_words[i].end() ; ++it )
      {
         if ( *it == 0 ){
            // For every flip of the zeroth (half-)edge, we calculate the new coords in dependence
            // of the surrounding quadrangle.
            zero_horo = compute_horo_flipped(dcel, zero_horo);
         }

         dcel.flipEdge( *it );
      }

      // After we applied the i-th flip sequence, we calculate the GKZvector of the corresponding
      // triangulation via the DomeVolumeVisitor.

      DomeBuilder dome(dcel, zero_horo);
      vert[i] = dome.computeGKZVector(depth);

      // Flip back to reference triangulation from triangulation "i".
      // The "true" parameter leads to unflipping of the edges in the reverse ordering,
      // yielding the same orientations as in the beginning.
      dcel.flipEdges( flip_words[i] , true );
   }

   // Add the negative Orthant and 1's in the first column for homogenous coordinates.
   //vert /= -unit_matrix<Rational>( num_vertices );
   //vert = ( ones_vector<Rational>( flip_words.size() ) | zero_vector<Rational>( num_vertices ) ) | vert;
   vert = ones_vector<Rational>( flip_words.size() ) | vert;

   return vert;

}



// return the covering triangulation of the k-th Delaunay triangulation
perl::Object covering_triangulation(perl::Object surface, int flip_word_id, int depth){

   if(depth<0)
   {
      throw std::runtime_error("gkz_dome: invalid depth");
   }

   //const bool dome = options["dome"];
   const Array< Array<int>> dcel_data = surface.give("DCEL_DATA");
   const Vector<Rational> penner_coord = surface.give("PENNER_COORDINATES");
   const Array< flip_sequence> flip_words = surface.give("FLIP_WORDS");
   if(flip_word_id<0 || flip_word_id>=flip_words.size())
   {
      throw std::runtime_error("gkz_dome: invalid index of Delaunay triangulation");
   }
   flip_sequence flip_word = flip_words[flip_word_id];
   std::pair<Rational, Rational> first_horo = surface.give("SPECIAL_POINT");
   Rational p_inf = first_horo.first;
   Rational zero_head = first_horo.second;

   // Set up the surface from dcel_data:
   DoublyConnectedEdgeList dcel = DoublyConnectedEdgeList(dcel_data);
   dcel.setMetric(penner_coord);

   // We compute the position of the zeroth half edge (index 0) in the upper half plane.
   Matrix<Rational> zero_horo = compute_horo(dcel, p_inf, zero_head);

   for( std::list<int>::const_iterator it = flip_word.begin() ; it!=flip_word.end() ; ++it )
   {
      if ( *it == 0 ){
         // For every flip of the zeroth (half-)edge, we calculate the new coords in dependence
         // of the surrounding quadrangle.
         zero_horo = compute_horo_flipped(dcel, zero_horo);
      }
      dcel.flipEdge( *it );
   }

   // After we applied the i-th flip sequence, we calculate the covering triangulation via the CoveringTriangulationVisitor.
   /*Graph<Directed> dual_graph;
   dual_graph.add_node();
   CoveringTriangulationVisitor cvis( dual_graph, surface, zero_horo, depth );
   BFSiterator< Graph<Directed>, VisitorTag<topaz::CoveringTriangulationVisitor> > bfs_it( dual_graph , std::move(cvis) , nodes(dual_graph).front() );
   while ( !bfs_it.at_end() )
   {
      bfs_it++;
   }*/

   CoveringBuilder cov(dcel, zero_horo, depth);
   perl::Object triang = cov.computeCoveringTriangulation();

   // Flip back to reference triangulation from triangulation "i".
   // The "true" parameter leads to unflipping of the edges in the reverse ordering,
   // yielding the same orientations as in the beginning.
   dcel.flipEdges( flip_word , true );

   return triang;

}



perl::Object secondary_polyhedron(perl::Object surface, int d){
   
   if(d<0)
   {
      throw std::runtime_error("secondary_polyhedron: invalid depth");
   }

   Matrix<Rational> vert = gkz_vectors(surface, d);
   Matrix<Rational> neg = -unit_matrix<Rational>( vert.cols()-1 );
   neg =  zero_vector<Rational>( vert.cols()-1 ) | neg;
   vert /= neg;

   perl::Object sp("polytope::Polytope<Float>");
   sp.take("VERTICES") << vert;

   perl::Object secfan = surface.give("SECONDARY_FAN");
   const Matrix<Rational> rays = secfan.give("RAYS");
   const IncidenceMatrix<> cells = secfan.give("MAXIMAL_CONES");
   sp.take("VERTICES_IN_FACETS") << secPolyVif(rays,cells);

   return sp;
}



Function4perl(&gkz_vectors, "gkz_vectors($$)");
Function4perl(&covering_triangulation, "covering_triangulation($$$)");

UserFunction4perl("# @category Producing other objects\n"
                  "# Computes the secondary polyhedron of a hyperbolic surface up to a given depth\n"
                  "# of the spanning tree of the covering triangluation of the hypoerbolic plane."
                  "# @param HyperbolicSurface s"
                  "# @param Int depth"
                  "# @return polytope::Polytope<Float>",
                  &secondary_polyhedron, "secondary_polyhedron(HyperbolicSurface Int)");

/*
UserFunction4perl("# @category Producing other objects\n"
                  "# Computes the the triangulation of the Klein disc that covers the k-th Delaunay triangulation\n"
                  "# of the hyperbolic surface."
                  "# Set dome to 1 to lift the vertices in the light-cylinder according to the horocyclic decoration. "
                  "# @param HyperbolicSurface s"
                  "# @param Int k"
                  "# @param Int depth"
                  "# @option Bool dome"
                  ,
                  &covering_triangulation, "covering_triangulation($,$,$)");
*/


}}
