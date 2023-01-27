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
#include <deque>

namespace polymake { namespace polytope {

/** Encapsulating the beneath-beyond algorithm.
    One instance is used to solve exactly one convex-hull problem.

    @tmplparam E numerical type of the coordinates
*/
template <typename E>
class beneath_beyond_algo {
public:
   beneath_beyond_algo()
      : expect_redundant(false)
      , make_triangulation(true)
      , is_cone(false)
      , compute_vertices(false)
   {
      dual_graph.attach(facets);
      dual_graph.attach(ridges);
   }

   /// flag==true: input points may contain redundancies (INPUT_RAYS, INEQUALITIES)
   /// flag==false: inpit points are free from redundancies (RAYS, FACETS)
   beneath_beyond_algo& expecting_redundant(bool flag)
   {
      expect_redundant = flag;
      return *this;
   }

   /// flag==true: input points are cone rays/facets
   /// flag==false: input points are polytope vertices/facets
   beneath_beyond_algo& for_cone(bool flag)
   {
      is_cone = flag;
      return *this;
   }

   /// flag==true: compute the triangulation
   /// flag==false: don't compute the triangulation
   beneath_beyond_algo& making_triangulation(bool flag)
   {
      make_triangulation = flag;
      return *this;
   }

   /// flag==true: input vectors are hyperplanes, result is vertices
   /// flag==false: input vectors are rays, result is facets
   beneath_beyond_algo& computing_vertices(bool flag)
   {
      compute_vertices = flag;
      return *this;
   }

   void compute(const Matrix<E>& rays, const Matrix<E>& lins)
   {
#if POLYMAKE_DEBUG
      enable_debug_output();
#endif
      compute(rays, lins, entire(sequence(0, rays.rows())));
   }

   // TODO: bundle all results in a structure, move all numbers into it
   template <typename Iterator>
   void compute(const Matrix<E>& rays, const Matrix<E>& lins, Iterator perm);

   Matrix<E> getFacets() const;
   IncidenceMatrix<> getVertexFacetIncidence() const;
   Matrix<E> getAffineHull() const;
   Matrix<E> getVertices() const;

   Bitset getNonRedundantPoints() const
   {
      Bitset result(sequence(0, source_points->rows()));
      result -= interior_points;
      return result;
   }

   Set<Int> getNonRedundantLinealities() const
   {
      const Int n_points = source_points->rows();
      Set<Int> result = points_in_lineality_basis;
      for (const Int p : source_lineality_basis)
         result += p + n_points;
      return result;
   }

   Matrix<E> getLinealities() const
   {
      return linealities_so_far;
   }

   Graph<> getDualGraph() const
   {
      return dual_graph;
   }

   Array<Set<Int>> getTriangulation() const
   {
      return Array<Set<Int>>(triang_size, triangulation.rbegin());
   }


   bool getGenericPosition() const
   {
      return generic_position;
   }

protected:
   // connects a facet with a triangulation simplex
   struct incident_simplex {
      const Set<Int>* simplex;  // an element of triangulation
      Int opposite_vertex;      // the only vertex NOT belonging to the facet

      incident_simplex(const Set<Int>& simplex_arg, Int vertex_arg)
         : simplex(&simplex_arg), opposite_vertex(vertex_arg) { }
   };

   // description of a facet; stored as a node attribute of the dual graph
   struct facet_info {
      Vector<E> normal;         // normal vector, directed inside the polyhedron
      E sqr_normal;             // sqr(normal)
      Int orientation;          // sign(normal * current-point-to-be-added) during one algo step
                                // =0 : facet is incident, <0 : facet is violated (and will die)
      Set<Int> vertices;        // ... incident to the facet

      // simplices from the polytope triangulation contributing to the triangulation of this facet
      using simplex_list = std::list<incident_simplex>;
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
         to->orientation = from->orientation;
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

   const Matrix<E>* source_points;
   Matrix<E> transformed_points;
   const Matrix<E>* points;
   const Matrix<E>* source_linealities;
   Matrix<E> linealities_so_far;
   const Matrix<E>* linealities;
   Matrix<E> lineality_transform;
   bool expect_redundant;
   bool make_triangulation;
   bool is_cone;
   bool compute_vertices;

   enum class compute_state { zero, one, low_dim, full_dim };
   compute_state state;

   Graph<> dual_graph;
   using facets_t = NodeMap<Undirected,facet_info>;
   using ridges_t = EdgeMap<Undirected, Set<Int>>;
   facets_t facets;
   ridges_t ridges;

   ListMatrix<SparseVector<E>> AH;  // affine hull
   ListMatrix<SparseVector<E>> facet_nullspace;  // its affine nullspace - is not computed as long as
                                                 //  the consumed input vertices comprise a simplex
   Bitset interior_points;  // indices of points which are not vertices

   Set<Int> source_lineality_basis, points_in_lineality_basis;

   using Triangulation = std::list<Set<Int>>;
   Triangulation triangulation;

   // These are working variables valid within one algo step.
   // We define them as instance variables nevertheless to avoid the repeating allocation and deallocation.
   Bitset vertices_this_step,           // points proved to be non-redundant
          interior_points_this_step,    // points that could be redundant
          visited_facets;               // facets seen

   std::deque<Int> facet_queue;   // BFS queue for update_facets()

   // accumulates the non-redundant points; is filled until the polytope turns out to be full-dimensional
   Set<Int> vertices_so_far;
   Int triang_size;     // = triangulation.size();

   Int valid_facet;     // a facet where to start the visibility border search

   bool generic_position;
   bool facet_normals_valid;

   void process_point(Int p);

   void add_second_point(Int p);

   // add the next point, given by the row index.
   // Recalculates the affine hull. If its dimension did not decrease, delegates the rest work to add_point_full_dim()
   void add_point_low_dim(Int p);

   // The first phase of the step: looking for a facet violated by point p. If found, calls update_facets()
   void add_point_full_dim(Int p);

   // helper function for add_point_full_dim
   Int descend_to_violated_facet(Int f, Int p);

   // helper functions
   void facet_normals_low_dim();
   bool reduce_nullspace(ListMatrix<SparseVector<E>>& M, Int p) const;

   // The main phase of the step: detect all facets to be deleted, create new facets and simplices
   // @param f the index of the first facet violated by or incident with point p
   void update_facets(Int f, Int p);

   void process_new_lineality(Int p, const std::list<Int>& incident_facets);

   void transform_points();

   template <typename ISet>
   void add_linealities(const ISet& point_set);

   class stop_calculation {};

   void complain_redundant(Int p);

#if POLYMAKE_DEBUG
   void dump() const;
   void dump_p(Int p) const;
   void check_f(Int f, Int last_p) const;
   void check_p(Int p) const;
   void check_fp(Int f_index, const facet_info& f, Int p, std::ostringstream& errors) const;
   enum debug_kind { do_nothing, do_check, do_dump, do_full_dump };
   debug_kind debug;

   void enable_debug_output()
   {
      switch (get_debug_level()) {
      case 0:
         debug = do_nothing;
         break;
      case 1:
         debug = do_check;
         break;
      case 2:
         debug = do_dump;
         break;
      default:
         debug = do_full_dump;
         break;
      }
   }
#endif
};

template <typename E>
Matrix<E> beneath_beyond_algo<E>::getFacets() const
{
   const auto extract_normals = [this](){
      return Matrix<E>(dual_graph.nodes(), points->cols(),
                       entire(attach_member_accessor(facets, ptr2type<facet_info, Vector<E>, &facet_info::normal>())));
   };
   if (dual_graph.nodes() != 0 && linealities->rows() != 0) {
      return (extract_normals() | zero_matrix<E>(0, linealities->rows())) * T(lineality_transform);
   } else if (dual_graph.nodes() != 0) {
      return extract_normals();
   } else {
      // no facets but fix ambient dimension
      return Matrix<E>(0,source_points->cols());
   }
}

template <typename E>
Matrix<E> beneath_beyond_algo<E>::getAffineHull() const
{
   if (AH.rows() != 0 && linealities->rows() != 0) {
      return (AH | zero_matrix<E>(0, linealities->rows())) * T(lineality_transform);
   } else if (AH.rows() != 0) {
      return AH;
   } else {
      // no affine hull but fix ambient dimension
      return Matrix<E>(0,source_points->cols());
   }
}

template <typename E>
Matrix<E> beneath_beyond_algo<E>::getVertices() const
{
   Matrix<E> V = source_points->minor(~interior_points, All);
   if (linealities->rows() != 0 && !compute_vertices) {
      // canonicalize vertices: project them on the orthogonal complement of linealities
      Matrix<E> Lnorm = *linealities;
      for (auto l = entire(rows(Lnorm)); !l.at_end(); ++l)
         *l /= sqr(*l);
      V -= V * (T(Lnorm) * (*linealities));
   }
   return V;
}

template <typename E>
IncidenceMatrix<> beneath_beyond_algo<E>::getVertexFacetIncidence() const
{
   IncidenceMatrix<> VIF(dual_graph.nodes(), points->rows(),
                         attach_member_accessor(facets, ptr2type<facet_info, Set<Int>, &facet_info::vertices>()).begin());
   if (!expect_redundant) return VIF;
   return VIF.minor(All, ~interior_points);
}

template <typename E>
void beneath_beyond_algo<E>::transform_points()
{
   const auto lin_compl = null_space(*linealities);
   if (expect_redundant && lin_compl.rows() == 0)
      // solution space is empty
      throw stop_calculation();
   lineality_transform = inv(lin_compl / *linealities);
   transformed_points = (*source_points * lineality_transform).minor(All, sequence(0, source_points->cols() - linealities->rows()));
   points = &transformed_points;
}

template <typename E>
bool beneath_beyond_algo<E>::reduce_nullspace(ListMatrix<SparseVector<E>>& M, Int p) const
{
   return basis_of_rowspan_intersect_orthogonal_complement(M, points->row(p), black_hole<Int>(), black_hole<Int>());
}

template <typename E>
template <typename Iterator>
void beneath_beyond_algo<E>::compute(const Matrix<E>& rays, const Matrix<E>& lins, Iterator perm)
{
   source_points = &rays;
   source_linealities = &lins;

   linealities_so_far.resize(0,rays.cols());

   try {
      if (lins.rows() != 0) {
         if (expect_redundant) {
            source_lineality_basis = basis_rows(lins);
            linealities_so_far = lins.minor(source_lineality_basis, All);
            linealities = &linealities_so_far;
         } else {
            linealities = source_linealities;
         }
         transform_points();
      } else {
         points = source_points;
         linealities = expect_redundant ? &linealities_so_far : source_linealities;
      }

      generic_position = !expect_redundant;
      triang_size = 0;
      AH = unit_matrix<E>(points->cols());
      if (expect_redundant) {
         interior_points.resize(points->rows());
         vertices_this_step.resize(points->rows());
         interior_points_this_step.resize(points->rows());
      }

      for (state = compute_state::zero; !perm.at_end(); ++perm)
         process_point(*perm);
      if (state == compute_state::low_dim && !facet_normals_valid)
         facet_normals_low_dim();
   }
   catch (const stop_calculation&) {
#if POLYMAKE_DEBUG
      if (debug >= do_dump) cout << "stop: degenerated to full linear space" << endl;
#endif
      state = compute_state::zero;
      dual_graph.clear();
      vertices_so_far.clear();
      points = source_points;
      interior_points = sequence(0, source_points->rows());
      if (make_triangulation) {
         triangulation.clear();
         triang_size = 0;
      }
   }

   switch (state) {
   case compute_state::zero:
      if (!is_cone) {
         // empty polyhedron
         AH.resize(0, source_points->cols());
         linealities_so_far.resize(0, source_points->cols());
      }
      break;
   case compute_state::one:
      // There is one empty facet in this case and the point is also a facet normal
      facets[dual_graph.add_node()].normal = points->row(vertices_so_far.front());
      if (make_triangulation) {
         triang_size=1;
         triangulation.push_back(vertices_so_far);
      }
      break;
   case compute_state::low_dim:
   case compute_state::full_dim:
      dual_graph.squeeze();
      break;
   }

#if POLYMAKE_DEBUG
   if (debug >= do_dump) {
      cout << "final ";
      dump();
   }
#endif
}

template <typename E>
void beneath_beyond_algo<E>::process_point(const Int p)
{
   if (expect_redundant && is_zero(points->row(p))) {
      interior_points += p;
      return;
   }

   switch (state) {
   case compute_state::zero: {
      // the first point
      reduce_nullspace(AH, p);
      vertices_so_far = scalar2set(p);
      state = compute_state::one;
      break;
   }
   case compute_state::one: {
      add_second_point(p);
      break;
   }
   case compute_state::low_dim:
      add_point_low_dim(p);
#if POLYMAKE_DEBUG
      if (debug == do_check) check_p(p);
#endif
      break;
   case compute_state::full_dim:
      add_point_full_dim(p);
#if POLYMAKE_DEBUG
      if (debug == do_check) check_p(p);
#endif
      break;
   }
}

template <typename E>
void beneath_beyond_algo<E>::add_second_point(Int p)
{
   const Int p0 = vertices_so_far.front();
   if (reduce_nullspace(AH, p)) {
      // two different points found: initialize the polytope
      const Int f0 = dual_graph.add_node();
      facets[f0].vertices = vertices_so_far;
      const Int f1 = dual_graph.add_node();
      facets[f1].vertices = scalar2set(p);
      dual_graph.edge(f0, f1);
      vertices_so_far += p;
      if (make_triangulation) {
         triangulation.push_back(vertices_so_far);
         triang_size = 1;
         facets[f0].simplices.push_back(incident_simplex(triangulation.front(), p));
         facets[f1].simplices.push_back(incident_simplex(triangulation.front(), p0));
      }
      valid_facet = 0;
#if POLYMAKE_DEBUG
      if (debug >= do_dump)
         cout << "starting points: " << vertices_so_far << "\n" << points->minor(vertices_so_far, All) << "\nAH:\n" << AH << endl;
#endif
      if ((facet_normals_valid = (AH.rows() == 0))) {
         // dimension==1, will need the facet normals immediately
         facets[f0].coord_full_dim(*this);
         facets[f1].coord_full_dim(*this);
         state = compute_state::full_dim;
      } else {
         state = compute_state::low_dim;
      }
   } else if (expect_redundant) {
      // p and p0 must be collinear; if the signs are different, they build a linearity
      const auto sign_of = [](const auto& v) {
         Int s = 0;
         for (const auto& x : v) {
            if ((s = sign(x)) != 0) break;
         }
         return s;
      };
      if (sign_of(points->row(p0)) != sign_of(points->row(p))) {
         interior_points += p0;
         vertices_so_far.clear();
         add_linealities(scalar2set(p0));
#if POLYMAKE_DEBUG
         if (debug >= do_dump)
            cout << "linealities:\n" << linealities_so_far << endl;
#endif
         state = compute_state::zero;
      }
      interior_points += p;
   } else {
      complain_redundant(p);
   }
}

template <typename E>
template <typename ISet>
void beneath_beyond_algo<E>::add_linealities(const ISet& point_set)
{
   const Int lin_rows = linealities_so_far.rows();
   linealities_so_far /= source_points->minor(point_set, All);
   const Set<Int> lin_basis = basis_rows(linealities_so_far);
   linealities_so_far = linealities_so_far.minor(lin_basis, All);
   // if new rays have been added to the lineality matrix, their indexes must be greater than those of already known linealities
   if (lin_basis.size() > lin_rows) {
      // TODO: introduce shifted_set or something similar
      const Set<Int> new_points_in_basis(attach_operation(lin_basis - sequence(0, lin_rows), operations::fix2<Int, operations::sub>(lin_rows)));
      points_in_lineality_basis += select(point_set, new_points_in_basis);
   }
   transform_points();
   AH = unit_matrix<E>(points->cols());
}

template <typename E>
void beneath_beyond_algo<E>::complain_redundant(Int p)
{
   throw std::runtime_error("beneath_beyond_algo: found redundant point " + std::to_string(p) + " while none was expected");
}

/** @param p the new point
    @param f the facet index to start from
    @retval index of the violated/incident facet or -1 if nothing found
*/
template <typename E>
Int beneath_beyond_algo<E>::descend_to_violated_facet(Int f, Int p)
{
   visited_facets += f;
   E fxp = facets[f].normal * points->row(p);
   if ((facets[f].orientation = sign(fxp)) <= 0) return f;

   // starting facet stays valid in this step: let's look for another one violated by p.
   // The search is performed in the dual graph, following the steepest descend of the
   // (square of the) distance between p and the facets

   if (expect_redundant) vertices_this_step += facets[f].vertices;
   fxp = fxp * fxp / facets[f].sqr_normal;    // square of the distance from p to the facet

   Int nextf;
   do {
#if POLYMAKE_DEBUG
      if (debug >= do_full_dump)
         cout << " *" << f << '(' << fxp << ')';
#endif
      nextf = -1;
      for (auto neighbor = entire(dual_graph.adjacent_nodes(f)); !neighbor.at_end(); ++neighbor) {
         const Int f2 = *neighbor;
         if (visited_facets.contains(f2)) continue;

         visited_facets += f2;
         E f2xp = facets[f2].normal * points->row(p);
         if ((facets[f2].orientation = sign(f2xp)) <= 0) return f2;

         if (expect_redundant) vertices_this_step += facets[f2].vertices;
         f2xp = f2xp * f2xp / facets[f2].sqr_normal;
#if POLYMAKE_DEBUG
         if (debug >= do_full_dump)
            cout << ' ' << f2 << '(' << f2xp << ')';
#endif
         if (f2xp <= fxp) {
            nextf = f2;
            fxp = f2xp;
         }
      }
   } while ((f = nextf) >= 0);

   return f;    // -1 : local minimum of sqr(distance) reached
}

namespace {

template <typename TSet>
Int first_or_none(const TSet& set)
{
   auto s = entire(set);
   return s.at_end() ? -1 : *s;
}

}

template <typename E>
void beneath_beyond_algo<E>::add_point_full_dim(Int p)
{
#if POLYMAKE_DEBUG
   if (debug >= do_dump)
      cout << "point " << p << "=[ " << points->row(p) << " ] : valid facets";
#endif
   // reset the working variables
   visited_facets.clear();
   if (expect_redundant) vertices_this_step.clear();

   // first try the facet added last in the previous step
   Int try_facet = valid_facet;
   do {
      if ((try_facet = descend_to_violated_facet(try_facet, p)) >= 0) {
         update_facets(try_facet, p);
         return;
      }
      for (auto f=entire(nodes(dual_graph)); !f.at_end(); ++f) {
         if (!visited_facets.contains(f.index())) {
            try_facet = f.index();
            break;
         }
      }
   } while (try_facet >= 0);

   // no violated facet found: p must be a redundant point

   if (expect_redundant) {
      interior_points += p;
#if POLYMAKE_DEBUG
      if (debug >= do_dump)
         cout << "\ninterior points: " << interior_points
              << "\n=======================================" << endl;
#endif
   } else {
      complain_redundant(p);
   }
}

template <typename E>
void beneath_beyond_algo<E>::facet_normals_low_dim()
{
   // facets must be orthogonal to the affine hull
   const Int d = points->cols();
   facet_nullspace = unit_matrix<E>(d);
   if (is_cone) {
      null_space(entire(rows(AH)), black_hole<Int>(), black_hole<Int>(), facet_nullspace);
   } else {
      SparseMatrix<E> AHaff=AH;
      // make all hyperplanes going thru the origin, but leave the far hyperplane untouched
      const auto far_hyperplane = unit_vector<E>(d, 0);
      for (auto r=entire(rows(AHaff));  !r.at_end();  ++r)
         if (*r != far_hyperplane)
            r->erase(0);
      null_space(entire(rows(AHaff)), black_hole<Int>(), black_hole<Int>(), facet_nullspace);
   }

   for (auto f=entire(facets);  !f.at_end();  ++f) {
      f->coord_low_dim(*this);
#if POLYMAKE_DEBUG
      if (debug >= do_dump) cout << f.index() << ": " << f->vertices << "=[ " << f->normal << " ]\n";
#endif
   }
}

template <typename E>
void beneath_beyond_algo<E>::add_point_low_dim(Int p)
{
   // update the affine hull
   if (reduce_nullspace(AH, p)) {
      // point set dimension increased
      if (facet_nullspace.rows() != 0) {
         generic_position = false;        // the base facet is more than a simplex
         facet_nullspace.clear();
      }

      // build a pyramid with the former polytope as a base and the point as an apex
      const Int nf_index = dual_graph.add_node();
      facet_info& nf = facets[nf_index];
      nf.vertices = vertices_so_far;
      if (expect_redundant) nf.vertices -= interior_points;

      // triangulation simplices are 'pyramidized' too
      if (make_triangulation) {
         for (auto simplex = entire(triangulation); !simplex.at_end(); ++simplex) {
            *simplex += p;
            nf.simplices.push_back(incident_simplex(*simplex, p));
         }
      }

      vertices_so_far += p;
      if ((facet_normals_valid = AH.rows() == 0))
         state = compute_state::full_dim;

      for (auto r=entire(ridges); !r.at_end(); ++r)
         *r += p;
      for (auto f=entire(nodes(dual_graph));  !f.at_end();  ++f) {
         // for all facets, except the new one
         if (f.index() != nf_index) {
            ridges(f.index(), nf_index) = facets[*f].vertices;
            facets[*f].vertices += p;
         }
         if (facet_normals_valid) {
            // the polytope became full-dimensional: will need the facet coordinates the whole rest of the time
            facets[*f].coord_full_dim(*this);
#if POLYMAKE_DEBUG
            if (debug >= do_dump) cout << f.index() << ": " << facets[*f].vertices << "=[ " << facets[*f].normal << " ]\n";
#endif
         }
      }
#if POLYMAKE_DEBUG
      if (debug >= do_dump) cout << "point " << p << "=[" << (*points)[p] << "] : dim increased\nAH:\n" << AH << endl;
#endif

   } else {
      // point set dimension not increased
      if (!facet_normals_valid) {
         // the polytope was a simplex, the facet coordinates are still not computed;
         // now we need them for the visibility region search.
         facet_normals_low_dim();
         facet_normals_valid = true;
      }
      add_point_full_dim(p);
   }
}

template <typename E>
void beneath_beyond_algo<E>::update_facets(Int f, const Int p)
{
#if POLYMAKE_DEBUG
   if (debug >= do_dump) cout << "\nupdating:";
#endif
   facet_queue.clear();
   facet_queue.push_back(f);

   if (expect_redundant) interior_points_this_step.clear();

   std::list<Int> incident_facets;
   if (facets[f].orientation == 0) {
      facets[f].vertices += p;
      generic_position = false;
      incident_facets.push_back(f);
   }

   /* BFS in the visible hemisphere.
      We visit all facets violated by or incident with p.
      Incident facets are important since they can contain redundant points not discovered before this iteration.
   */
   while (!facet_queue.empty()) {
      f = facet_queue.front();  facet_queue.pop_front();
      const Int f_orientation = facets[f].orientation;
      // remember the position where the new simplices will end
      auto new_simplex_end = triangulation.begin();

      if (f_orientation < 0) {
         // the facet is violated
#if POLYMAKE_DEBUG
         if (debug >= do_dump) cout << " -" << f;
#endif
         if (expect_redundant) interior_points_this_step += facets[f].vertices;

         // build new triangulation simplices using the triangulation of the facet
         if (make_triangulation) {
            for (auto is = entire(facets[f].simplices); !is.at_end(); ++is) {
               triangulation.push_front(*is->simplex);
               ++triang_size;
               // just take the existing simplex and replace the vertex behind the facet by the new point
               (triangulation.front() -= is->opposite_vertex) += p;
            }
         }
#if POLYMAKE_DEBUG
      } else {
         if (debug >= do_dump) cout << " " << f;
#endif
      }

      // check the neighbor facets
      for (auto e=entire(dual_graph.out_edges(f)); !e.at_end(); ++e) {
         const Int f2 = e.to_node();
         facet_info& nbf = facets[f2];
         if (!visited_facets.contains(f2)) {
            visited_facets += f2;
            nbf.orientation = sign(nbf.normal * points->row(p));
            if (nbf.orientation == 0) {
               // incident facet
               nbf.vertices += p;
               generic_position = false;
               incident_facets.push_back(f2);
            }
            if (nbf.orientation <= 0)
               facet_queue.push_back(f2);
            else if (expect_redundant)
               vertices_this_step += nbf.vertices;
         }

         if (f_orientation < 0) {
            if (nbf.orientation > 0) {
               // found a ridge on the visibility border: create a new facet
               const Int nf_index = dual_graph.add_node();
               facet_info& nf = facets[nf_index];
               nf.vertices = ridges[*e] + p;
               if (AH.rows())
                  nf.coord_low_dim(*this);
               else
                  nf.coord_full_dim(*this);
#if POLYMAKE_DEBUG
               if (debug == do_check) check_f(nf_index, p);
#endif
               ridges(nf_index, f2) = ridges[*e];
               incident_facets.push_back(nf_index);
               if (make_triangulation) {
                  nf.add_incident_simplices(triangulation.begin(), new_simplex_end);
               }
            } else if (nbf.orientation == 0) {
               if (make_triangulation) {
                  nbf.add_incident_simplices(triangulation.begin(), new_simplex_end);
               }
            }
         } else if (nbf.orientation == 0) {
            ridges[*e] += p;    // include the point into the edge, since it's incident to both facets
         }
      }

      if (f_orientation < 0) dual_graph.delete_node(f);
   }

   if (expect_redundant) {
      if (interior_points_this_step.empty()) { // = no violated facets visited
         interior_points += p;
#if POLYMAKE_DEBUG
         if (debug >= do_full_dump)
            cout << "\ninterior points: " << interior_points
                 << "\n=======================================";
         if (debug >= do_dump) cout << endl;
#endif
         return;
      }
      if (vertices_this_step.empty()) {
         process_new_lineality(p, incident_facets);
         return;
      }
      interior_points_this_step -= vertices_this_step;
      interior_points += interior_points_this_step;
   }

   /// The final phase of the step: create new edges in the dual graph
   Int min_ridge = points->cols() - AH.rows()-2;

   for (auto f_it = entire(incident_facets); !f_it.at_end(); ++f_it) {
      f = *f_it;
      const bool vis = visited_facets.contains(f);

      auto f2_it = f_it;
      for (++f2_it; !f2_it.at_end(); ++f2_it) {
         const Int f2 = *f2_it;
         // if both facets are incident to p, they could already have a connecting edge
         if (vis && visited_facets.contains(f2) && dual_graph.edge_exists(f, f2)) continue;

         const Set<Int> ridge = facets[f].vertices * facets[f2].vertices;
         if (ridge.size() >= min_ridge) {
            bool add = true;
            auto e = entire(dual_graph.out_edges(f));
            while (!e.at_end()) {
               const Int inc = incl(ridges[*e], ridge);
               if (inc == 2) {
                  ++e;
               } else {
                  if (inc <= 0)
                     dual_graph.out_edges(f).erase(e++);
                  if (inc >= 0) {
                     add = false;
                     break;
                  }
               }
            }
            if (add) ridges(f,f2) = ridge;
         }
      }
   }

   if (AH.rows() != 0) {
      vertices_so_far += p;
      if (expect_redundant) vertices_so_far -= interior_points_this_step;
   }
#if POLYMAKE_DEBUG
   if (debug >= do_dump) {
      cout << "\n";
      dump();
      if (expect_redundant) cout << "\ninterior points: " << interior_points << "\n";
      cout << "=======================================" << endl;
   }
#endif
   valid_facet = f;
}

template <typename E>
void beneath_beyond_algo<E>::process_new_lineality(const Int p, const std::list<Int>& incident_facets)
{
   Set<Int> rays_in_lineality, candidate_points;

   if (incident_facets.empty()) {
      // all rays visited so far belong to the new lineality
      if (AH.rows() == 0)
         // lineality fills the entire affine hull - no reason to continue calculations
         throw stop_calculation();
      rays_in_lineality = vertices_so_far - interior_points;

   } else if (dual_graph.nodes() > 1) {
      // the intersection of all incident_facets is the new lineality,
      // other vertices are candidates for restart
      auto fac_it = entire(incident_facets);
      candidate_points = rays_in_lineality = facets[*fac_it].vertices;
      while (!(++fac_it).at_end()) {
         rays_in_lineality *= facets[*fac_it].vertices;
         candidate_points += facets[*fac_it].vertices;
      }
      candidate_points -= rays_in_lineality;
      rays_in_lineality -= p;

   } else {
      // in the special case of two points and two facets the only candidate point belongs to the "violated" facet
      candidate_points = interior_points_this_step;
      rays_in_lineality = facets[incident_facets.front()].vertices;
   }

   add_linealities(rays_in_lineality);
   interior_points_this_step -= candidate_points;
   interior_points += interior_points_this_step;
   interior_points += p;
   interior_points += rays_in_lineality;
   vertices_so_far.clear();
   dual_graph.clear();
   if (make_triangulation) {
      triangulation.clear();
      triang_size = 0;
   }
   state = compute_state::zero;
#if POLYMAKE_DEBUG
   if (debug >= do_dump)
      cout << "restart\nlinealities:\n" << linealities_so_far << endl;
#endif
   for (const Int cp : candidate_points) {
      process_point(cp);
   }
#if POLYMAKE_DEBUG
   if (debug >= do_dump)
      cout << "resume with remaining points" << endl;
#endif
}

template <typename E>
void beneath_beyond_algo<E>::facet_info::coord_full_dim(const beneath_beyond_algo<E>& A)
{
   normal = rows(null_space(A.points->minor(vertices, All))).front();
   if (normal * A.points->row((A.vertices_so_far - vertices).front()) < 0)
      normal.negate();
   sqr_normal = sqr(normal);
}

template <typename E>
void beneath_beyond_algo<E>::facet_info::coord_low_dim(const beneath_beyond_algo<E>& A)
{
   ListMatrix<SparseVector<E>> Fn = A.facet_nullspace;
   for (const Int v : vertices)
      A.reduce_nullspace(Fn, v);
   normal = rows(Fn).front();
   if (normal * A.points->row((A.vertices_so_far - vertices).front()) < 0)
      normal.negate();
   sqr_normal = sqr(normal);
}

template <typename TSet>
Int single_or_nothing(const GenericSet<TSet, Int>& s)
{
   Int x = -1;
   auto e = entire(s.top());
   if (!e.at_end()) {
      x = *e; ++e;
      if (!e.at_end()) x = -1;
   }
   return x;
}

template <typename E> template <typename Iterator>
void beneath_beyond_algo<E>::facet_info::add_incident_simplices(Iterator s, Iterator s_end)
{
   for (; s != s_end; ++s) {
      Int opv = single_or_nothing(*s - vertices);
      if (opv >= 0)
         simplices.push_back(incident_simplex(*s, opv));
   }
}

#if POLYMAKE_DEBUG
template <typename E>
void beneath_beyond_algo<E>::dump() const
{
   cout << "dual_graph:\n";
   const bool show_normals = debug == do_full_dump && (AH.rows() == 0 || facet_nullspace.rows() != 0);
   for (auto f=entire(nodes(dual_graph)); !f.at_end(); ++f) {
      cout << f.index() << ": " << facets[*f].vertices;
      if (show_normals) cout << "=[ " << facets[*f].normal << " ]";
      if (debug == do_full_dump) {
         for (auto e = entire(f.out_edges()); !e.at_end(); ++e)
            cout << " (" << e.to_node() << ' ' << ridges[*e] << ')';
         if (make_triangulation) {
            cout << " <<";
            for (auto s = entire(facets[*f].simplices); !s.at_end(); ++s)
               cout << ' ' << *s->simplex << '-' << s->opposite_vertex;
            cout << " >>";
         }
         cout << endl;
      } else {
         cout << ' ' << f.adjacent_nodes() << endl;
      }
   }
}

template <typename E>
void beneath_beyond_algo<E>::check_fp(Int f_index, const facet_info& f, Int p, std::ostringstream& errors) const
{
   const E prod = points->row(p) * f.normal;
   if (f.vertices.contains(p)) {
      if (prod!=0)
         wrap(errors) << "facet(" << f_index << ") * incident vertex(" << p << ")=" << prod << endl;
   } else {
      if (prod<=0)
         wrap(errors) << "facet(" << f_index << ") * non-incident vertex(" << p << ")=" << prod << endl;
   }
}

// various consistency checks
template <typename E>
void beneath_beyond_algo<E>::check_p(Int p) const
{
   if (AH.rows() == 0 || facet_nullspace.rows() != 0) {
      std::ostringstream errors;

      for (auto f=entire(nodes(dual_graph)); !f.at_end(); ++f)
         check_fp(f.index(), facets[*f], p, errors);

      if (!errors.str().empty())
         throw std::runtime_error("beneath_beyond_algo - consistency checks failed:\n" + errors.str());
   }
}

template <typename E>
void beneath_beyond_algo<E>::check_f(Int f, Int last_p) const
{
   std::ostringstream errors;
   const facet_info& fi=facets[f];

   for (auto p = entire(range(0, last_p)); !p.at_end(); ++p)
      check_fp(f, fi, *p, errors);

   if (!errors.str().empty())
      throw std::runtime_error("beneath_beyond_algo - consistency checks failed:\n" + errors.str());
}

template <typename E>
void beneath_beyond_algo<E>::dump_p(Int p) const
{
   if (!AH.rows() || facet_nullspace.rows()) {
      for (auto f=entire(nodes(dual_graph)); !f.at_end(); ++f)
         if (f.degree()) {
            const E prod = points->row(p) * facets[*f].normal;
            cout << "facet(" << f.index() << "): prod=" << prod << ", sqr_dist=" << double(prod*prod/facets[*f].sqr_normal) << '\n';
         }
   }
}
#endif // POLYMAKE_DEBUG

} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
