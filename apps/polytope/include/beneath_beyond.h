/* Copyright (c) 1997-2015
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

#ifndef POLYMAKE_POLYTOPE_BENEATH_BEYOND_H
#define POLYMAKE_POLYTOPE_BENEATH_BEYOND_H

#include "polymake/client.h"
#include "polymake/linalg.h"
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/ListMatrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Graph.h"
#include "polymake/Bitset.h"
#include "polymake/Set.h"
#include "polymake/Array.h"
#include "polymake/list"

namespace polymake { namespace polytope {

/** Encapsulating the beneath-beyond algorithm.
    One instance is used to solve exactly one convex-hull problem.

    @tmplparam E numerical type of the coordinates
*/
template <typename E>
class beneath_beyond_algo {
public:

   /** The only constructor, initializing the whole stuff.
       @param points_arg  rows are points with homogeneous coordinates
       @param already_VERTICES_arg  true if it is already known that there are no redundant points in the input
   */
   beneath_beyond_algo(const Matrix<E>& points_arg, bool already_VERTICES_arg)
      : points(points_arg),
        already_VERTICES(already_VERTICES_arg), generic_position(already_VERTICES_arg),
        facet_normals_valid(false),
        AH(unit_matrix<E>(points.cols())),
        interior_points(!already_VERTICES ? points.rows() : 0),
        vertices_this_step(!already_VERTICES ? points.rows() : 0),
        interior_points_this_step(!already_VERTICES ? points.rows() : 0)
   {
#if POLYMAKE_DEBUG
      switch (perl::get_debug_level()) {
      case 0:
         debug=do_nothing;
         break;
      case 1:
         debug=do_check;
         break;
      case 2:
         debug=do_dump;
         break;
      default:
         debug=do_full_dump;
         break;
      }
#endif
      dual_graph.attach(facets);
      dual_graph.attach(ridges);
   }

   /** The main and only method which performs the computation.
       @param perm  an end-sensitive input iterator over a sequence of cardinal numbers 0..#points-1
                    or a permutation thereof.  It determines the order in which the points are successively
                    added, and hence the resulting placing triangulation.
   */
   template <typename Iterator>
   void compute(Iterator perm);

   // The rest public methods must be called after compute(); they merely retrieve the computation results.
   // The names are self-explanatory.

   Matrix<E> getFacets() const;
   IncidenceMatrix<> getVertexFacetIncidence() const;
   Matrix<E> getAffineHull() const;
   Matrix<E> getVertices() const;
   Graph<> getDualGraph() const;
   Array< Set<int> > getTriangulation() const;
   bool getGenericPosition() const;

protected:
   // connects a facet with a triangulation simplex
   struct incident_simplex {
      const Set<int>* simplex;  // an element of triangulation
      int opposite_vertex;      // the only vertex NOT belonging to the facet

      incident_simplex(const Set<int>& simplex_arg, int vertex_arg)
         : simplex(&simplex_arg), opposite_vertex(vertex_arg) { }
   };

   // description of a facet; stored as a node attribute of the dual graph
   struct facet_info {
      Vector<E> normal;         // normal vector, directed inside the polyhedron
      E sqr_normal;             // sqr(normal)
      int orientation;          // sign(normal * current-point-to-be-added) during one algo step
                                // =0 : facet is incident, <0 : facet is violated (and will die)
      Set<int> vertices;        // ... incident to the facet

      // simplices from the polytope triangulation contributing to the triangulation of this facet
      typedef std::list<incident_simplex> simplex_list;
      simplex_list simplices;

      // compute the normal vector etc. assuming the full-dimensional case (Affine Hull is empty)
      void coord_full_dim(const beneath_beyond_algo&);

      // compute the normal vector etc. in the low-dimensional case
      void coord_low_dim(const beneath_beyond_algo&);

      // check the intersection of triangulation simplices from [s, s_end) with the facet,
      // include those with a (d-1)-face ompletely belonging to the facet in its simplex_list
      template <typename Iterator>
      void add_incident_simplices(Iterator s, Iterator s_end);

      // enables efficient memory management in the dual graph
      friend void relocate(facet_info* from, facet_info* to)
      {
         relocate(&from->normal, &to->normal);
         relocate(&from->sqr_normal, &to->sqr_normal);
         to->orientation=from->orientation;
         relocate(&from->vertices, &to->vertices);
         pm::relocate(&from->simplices, &to->simplices);
      }
#if POLYMAKE_DEBUG
      template <typename Output> friend
      Output& operator<< (GenericOutput<Output>& os, const facet_info& me)
      {
         return os.top() << me.normal << ' ' << me.vertices;
      }
#endif
   };

   const Matrix<E>& points;
   bool already_VERTICES, generic_position, facet_normals_valid;

   Graph<> dual_graph;
   typedef NodeMap<Undirected,facet_info> facets_t;
   typedef EdgeMap<Undirected, Set<int> > ridges_t;
   facets_t facets;
   ridges_t ridges;

   ListMatrix< SparseVector<E> > AH,    // affine hull
                                 facet_nullspace;  // its affine nullspace - is not computed as long as
                                                   //  the consumed input vertices comprise a simplex
   Bitset interior_points;              // indices of points which are not vertices

   typedef std::list< Set<int> > Triangulation;

   Triangulation triangulation;
   int triang_size;     // = triangulation.size();

   int valid_facet;     // a facet where to start the visibility border search

   // These are working variables valid within one algo step.
   // We define them as instance variables nevertheless to avoid the repeating allocation and deallocation.
   Bitset vertices_this_step,           // points proved to be non-redundant
          interior_points_this_step,    // points that could be redundant
          visited_facets;               // facets seen

   // accumulates the non-redundant points; is filled until the polytope turns out to be full-dimensional
   Set<int> vertices_so_far;

   /// create the start configuration: a line segment connecting two points
   /// @param p1,p2 row indices in the points matrix
   void start_with_points(int p1, int p2);

   // add the next point, given by the row index.
   // Recalculates the affine hull. If its dimension did not decrease, delegates the rest work to add_point_full_dim()
   void add_point_low_dim(int p);

   // The first phase of the step: looking for a facet violated by point p. If found, calls update_facets()
   void add_point_full_dim(int p);

   // helper function for add_point_full_dim
   int descend_to_violated_facet(int f, int p);

   // helper function
   void facet_normals_low_dim();

   /// The main phase of the step: detect all facets to be deleted, create new facets and simplices
   /// @param f the index of the first facet violated by or incident with point p
   void update_facets(int f, int p);

#if POLYMAKE_DEBUG
   void dump() const;
   void dump_p(int p) const;
   void check_f(int f, int last_p) const;
   void check_p(int p) const;
   void check_fp(int f_index, const facet_info& f, int p, std::ostringstream& errors) const;
   enum debug_kind { do_nothing, do_check, do_dump, do_full_dump };
   debug_kind debug;
   Bitset points_so_far;
#endif
};

template <typename E> inline
Matrix<E> beneath_beyond_algo<E>::getFacets() const
{
   // TODO: move instead of copy
   return Matrix<E>(dual_graph.nodes(), points.cols(),
                    entire(attach_member_accessor(facets, ptr2type<facet_info, Vector<E>, &facet_info::normal>())));
}

template <typename E> inline
Matrix<E> beneath_beyond_algo<E>::getAffineHull() const
{
   return AH;
}

template <typename E> inline
Matrix<E> beneath_beyond_algo<E>::getVertices() const
{
   return points.minor(~interior_points,All);
}

template <typename E> inline
IncidenceMatrix<> beneath_beyond_algo<E>::getVertexFacetIncidence() const
{
   IncidenceMatrix<> VIF(dual_graph.nodes(), points.rows(),
                         attach_member_accessor(facets, ptr2type<facet_info, Set<int>, &facet_info::vertices>()).begin());
   if (already_VERTICES) return VIF;
   return VIF.minor(All,~interior_points);
}

template <typename E> inline
Graph<> beneath_beyond_algo<E>::getDualGraph() const
{
   return dual_graph;
}

template <typename E> inline
Array< Set<int> > beneath_beyond_algo<E>::getTriangulation() const
{
   return Array< Set<int> >(triang_size, triangulation.rbegin());
}

template <typename E> inline
bool beneath_beyond_algo<E>::getGenericPosition() const
{
   return generic_position;
}

} } // end namespace polymake

#include "polymake/polytope/beneath_beyond.tcc"

#endif // POLYMAKE_POLYTOPE_BENEATH_BEYOND_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
