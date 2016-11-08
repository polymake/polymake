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

namespace polymake { namespace polytope {

template <typename E> template <typename Iterator>
void beneath_beyond_algo<E>::compute(Iterator perm)
{
   if (perm.at_end()) return;

   // the first point
   int d2=points.cols()-2, p1=*perm;  ++perm;
   null_space(entire(item2container(points[p1])), black_hole<int>(), black_hole<int>(), AH);

   // look for the second point different from the first one
   while (true) {
      if (perm.at_end()) {
         // the special case: a single point
         triang_size=1;
         triangulation.push_back(scalar2set(p1));
         // There is one empty facet in this case and the point is also a facet normal
         int f0=dual_graph.add_node();
         facets[f0].vertices = Set<int>();
         facets[f0].normal = points[p1];
         return;
      }
      int p2=*perm;  ++perm;
      null_space(entire(item2container(points[p2])), black_hole<int>(), black_hole<int>(), AH);
      if (AH.rows()==d2) {
         // two different points found: initialize the polytope
         start_with_points(p1,p2);
         break;
      }
      if (!already_VERTICES) interior_points += p2;
   }

   // as long as the affine hull is not empty...
   while (AH.rows() && !perm.at_end()) {
      add_point_low_dim(*perm);
#if POLYMAKE_DEBUG
      if (debug==do_check) {
         check_p(*perm);
         if (!std::is_same<Iterator, sequence::iterator>::value) points_so_far+=*perm;
      }
#endif
      ++perm;
   }

   // then take the shortcut
   while (!perm.at_end()) {
      add_point_full_dim(*perm);
#if POLYMAKE_DEBUG
      if (debug==do_check) {
         check_p(*perm);
         if (!std::is_same<Iterator, sequence::iterator>::value) points_so_far+=*perm;
      }
#endif
      ++perm;
   }

   if (!facet_normals_valid) {
      facet_normals_low_dim();
      facet_normals_valid=true;
   }

   // sweep out the unneeded facets from the recycling list
   dual_graph.squeeze();
#if POLYMAKE_DEBUG
   if (debug >= do_dump) {
      cout << "final ";
      dump();
   }
#endif
}

template <typename E>
void beneath_beyond_algo<E>::start_with_points(int p1, int p2)
{
   int f0=dual_graph.add_node();
   facets[f0].vertices=scalar2set(p1);
   int f1=dual_graph.add_node();
   facets[f1].vertices=scalar2set(p2);
   dual_graph.edge(f0,f1);
   vertices_so_far=scalar2set(p1)+scalar2set(p2);
   triangulation.push_back(vertices_so_far);
   triang_size=1;
   facets[f0].simplices.push_back(incident_simplex(triangulation.front(),p2));
   facets[f1].simplices.push_back(incident_simplex(triangulation.front(),p1));
   valid_facet=0;
#if POLYMAKE_DEBUG
   if (debug >= do_dump)
      cout << "starting points: " << p1 << ' ' << p2 << "\nAH:\n" << AH << endl;
#endif
   if ((facet_normals_valid=!AH.rows())) {      // dimension==1, will need the facet normals immediately
      facets[f0].coord_full_dim(*this);
      facets[f1].coord_full_dim(*this);
   }
}

/** @param p the new point
    @param f the facet index to start from
    @retval index of the violated/incident facet or -1 if nothing found
*/
template <typename E>
int beneath_beyond_algo<E>::descend_to_violated_facet(int f, int p)
{

   visited_facets+=f;
   E fxp= facets[f].normal * points[p];
   if ((facets[f].orientation=sign(fxp))<=0) return f;

   // starting facet stays valid in this step: let's look for another one violated by p.
   // The search is performed in the dual graph, following the steepest descend of the
   // (square of the) distance between p and the facets

   if (!already_VERTICES) vertices_this_step += facets[f].vertices;
   fxp=fxp*fxp/facets[f].sqr_normal;    // square of the distance from p to the facet

   int nextf;
   do {
#if POLYMAKE_DEBUG
      if (debug >= do_full_dump)
         cout << " *" << f << '(' << fxp << ')';
#endif
      nextf=-1;
      for (Entire<Graph<>::adjacent_node_list>::iterator neighbor=entire(dual_graph.adjacent_nodes(f)); !neighbor.at_end(); ++neighbor) {
         const int f2=*neighbor;
         if (visited_facets.contains(f2)) continue;

         visited_facets+=f2;
         E f2xp= facets[f2].normal * points[p];
         if ((facets[f2].orientation=sign(f2xp))<=0) return f2;

         if (!already_VERTICES) vertices_this_step += facets[f2].vertices;
         f2xp=f2xp*f2xp/facets[f2].sqr_normal;
#if POLYMAKE_DEBUG
         if (debug >= do_full_dump)
            cout << ' ' << f2 << '(' << f2xp << ')';
#endif
         if (f2xp<=fxp) {
            nextf=f2;
            fxp=f2xp;
         }
      }
   } while ((f=nextf)>=0);

   return f;    // -1 : local minimum of sqr(distance) reached
}

template <typename Set> static inline
int first_or_none(const Set& set)
{
   typename Entire<Set>::const_iterator s=entire(set);
   return s.at_end() ? -1 : *s;
}

template <typename E>
void beneath_beyond_algo<E>::add_point_full_dim(int p)
{
#if POLYMAKE_DEBUG
   if (debug >= do_dump)
      cout << "point " << p << "=[ " << points[p] << " ] : valid facets";
#endif
   // reset the working variables
   visited_facets.clear();
   if (!already_VERTICES) vertices_this_step.clear();

   // first try the facet added last in the previous step
   int try_facet=valid_facet;
   do {
      if ((try_facet=descend_to_violated_facet(try_facet,p))>=0) {
         update_facets(try_facet,p);
         return;
      }
      for (Entire< Nodes< Graph<> > >::iterator f=entire(nodes(dual_graph)); !f.at_end(); ++f)
         if (!visited_facets.contains(f.index())) {
            try_facet=f.index(); break;
         }
   } while (try_facet>=0);

   // no violated facet found: p must be a redundant point

   if (!already_VERTICES) {
      interior_points += p;
#if POLYMAKE_DEBUG
      if (debug >= do_dump)
         cout << "\ninterior points: " << interior_points
              << "\n=======================================" << endl;
#endif
   }
}

template <typename E>
void beneath_beyond_algo<E>::facet_normals_low_dim()
{
   // facets must be orthogonal to the affine hull
   const int d=points.cols();
   facet_nullspace=unit_matrix<E>(d);
   SparseMatrix<E> AHaff=AH;
   // make all hyperplanes going thru the origin, but leave the far hyperplane untouched
   for (typename Entire< Rows< SparseMatrix<E> > >::iterator r=entire(rows(AHaff));  !r.at_end();  ++r)
      if (*r != unit_vector<E>(d,0))
         r->erase(0);
   null_space(entire(rows(AHaff)), black_hole<int>(), black_hole<int>(), facet_nullspace);

   for (typename Entire<facets_t>::iterator f=entire(facets);  !f.at_end();  ++f) {
      f->coord_low_dim(*this);
#if POLYMAKE_DEBUG
      if (debug >= do_dump) cout << f.index() << ": =[" << f->normal << " ]\n";
#endif
   }
}

template <typename E>
void beneath_beyond_algo<E>::add_point_low_dim(int p)
{
   // update the affine hull
   int codim=AH.rows();
   null_space(entire(item2container(points[p])), black_hole<int>(), black_hole<int>(), AH);

   if (AH.rows()<codim) {
      // point set dimension increased
      if (facet_nullspace.rows()) {
         generic_position=false;        // the base facet is more than a simplex
         facet_nullspace.clear();
      }

      // build a pyramid with the former polytope as a base and the point as an apex
      int nf_index=dual_graph.add_node();
      facet_info& nf=facets[nf_index];
      nf.vertices=vertices_so_far;
      vertices_so_far+=p;

      // triangulation simplices are 'pyramidized' too
      for (typename Entire<Triangulation>::iterator simplex=entire(triangulation); !simplex.at_end(); ++simplex) {
         *simplex += p;
         nf.simplices.push_back(incident_simplex(*simplex,p));
      }

      for (Entire<ridges_t>::iterator r=entire(ridges); !r.at_end(); ++r)
         *r += p;
      facet_normals_valid=!AH.rows();
      for (Entire< Nodes< Graph<> > >::iterator f=entire(nodes(dual_graph));  !f.at_end();  ++f) {
         // for all facets, except the new one
         if (f.index() != nf_index) {
            ridges(f.index(),nf_index)=facets[*f].vertices;
            facets[*f].vertices+=p;
         }
         if (facet_normals_valid)
            // the polytope became full-dimensional: will need the facet coordinates the whole rest of the time
            facets[*f].coord_full_dim(*this);
      }
#if POLYMAKE_DEBUG
      if (debug >= do_dump) cout << "point " << p << ": dim increased\nAH:\n" << AH << endl;
#endif
   } else {
      // point set dimension not increased
      if (!facet_normals_valid) {
         // the polytope was a simplex, the facet coordinates are still not computed;
         // now we need them for the visibility region search.
         facet_normals_low_dim();
         facet_normals_valid=true;
      }
      add_point_full_dim(p);
   }
}

template <typename E> inline
void beneath_beyond_algo<E>::update_facets(int f, int p)
{
#if POLYMAKE_DEBUG
   if (debug >= do_dump) cout << "\nupdating:";
#endif
   // queue for BFS
   std::list<int> Q;
   Q.push_back(f);
   if (!already_VERTICES) interior_points_this_step.clear();
   std::list<int> incident_facets;
   if (facets[f].orientation==0) {
      facets[f].vertices += p;
      generic_position=false;
      incident_facets.push_back(f);
   }

   /* BFS in the visible hemisphere.
      We visit all facets violated by or incident with p.
      Incident facets are important since they can contain redundant points not discovered before this iteration.
   */
   while (!Q.empty()) {
      f=Q.front(); Q.pop_front();
      int f_orientation=facets[f].orientation;
      // remember the position where the new simplices will end
      typename Triangulation::iterator new_simplex_end=triangulation.begin();

      if (f_orientation<0) {
         // the facet is violated
#if POLYMAKE_DEBUG
         if (debug >= do_dump) cout << " -" << f;
#endif
         if (!already_VERTICES) interior_points_this_step += facets[f].vertices;

         // build new triangulation simplices using the triangulation of the facet
         for (typename Entire<typename facet_info::simplex_list>::iterator is=entire(facets[f].simplices);
              !is.at_end(); ++is) {
            triangulation.push_front(*is->simplex);
            ++triang_size;
            // just take the existing simplex and replace the vertex behind the facet by the new point
            (triangulation.front() -= is->opposite_vertex) += p;
         }
#if POLYMAKE_DEBUG
      } else {
         if (debug >= do_dump) cout << " " << f;
#endif
      }

      // check the neighbor facets
      for (Entire<Graph<>::out_edge_list>::iterator e=entire(dual_graph.out_edges(f)); !e.at_end(); ++e) {
         const int f2=e.to_node();
         facet_info& nbf=facets[f2];
         if (!visited_facets.contains(f2)) {
            visited_facets+=f2;
            nbf.orientation=sign(nbf.normal * points[p]);
            if (nbf.orientation==0) {
               // incident facet
               nbf.vertices += p;
               generic_position=false;
               incident_facets.push_back(f2);
            }
            if (nbf.orientation<=0)
               Q.push_back(f2);
            else if (!already_VERTICES)
               vertices_this_step += nbf.vertices;
         }

         if (f_orientation<0) {
            if (nbf.orientation>0) {
               // found a ridge on the visibility border: create a new facet
               int nf_index=dual_graph.add_node();
               facet_info& nf=facets[nf_index];
               nf.vertices=ridges[*e] + p;
               if (AH.rows())
                  nf.coord_low_dim(*this);
               else
                  nf.coord_full_dim(*this);
#if POLYMAKE_DEBUG
               if (debug==do_check) check_f(nf_index, p);
#endif
               ridges(nf_index,f2)=ridges[*e];
               incident_facets.push_back(nf_index);
               nf.add_incident_simplices(triangulation.begin(), new_simplex_end);
            } else if (nbf.orientation==0) {
               nbf.add_incident_simplices(triangulation.begin(), new_simplex_end);
            }
         } else if (nbf.orientation==0) {
            ridges[*e] += p;    // include the point into the edge, since it's incident to both facets
         }
      }

      if (f_orientation<0) dual_graph.delete_node(f);
   }

   if (!already_VERTICES) {
      if (interior_points_this_step.empty()) { // = no violated facets visited
         interior_points += p;
#if POLYMAKE_DEBUG
         if (debug >= do_full_dump)
            cout << "\ninterior points: " << interior_points
                 << "\n=======================================" << endl;
#endif
         return;
      } else {
         interior_points_this_step -= vertices_this_step;
         interior_points += interior_points_this_step;
      }
   }

   /// The final phase of the step: create new edges in the dual graph
   int min_ridge=points.cols()-AH.rows()-2;

   for (typename Entire< std::list<int> >::iterator f_it=entire(incident_facets); !f_it.at_end(); ++f_it) {
      f=*f_it;
      const bool vis=visited_facets.contains(f);

      typename Entire< std::list<int> >::iterator f2_it=f_it;
      for (++f2_it; !f2_it.at_end(); ++f2_it) {
         const int f2=*f2_it;
         // if both facets are incident to p, they could already have a connecting edge
         if (vis && visited_facets.contains(f2) && dual_graph.edge_exists(f,f2)) continue;

         const Set<int> ridge= facets[f].vertices * facets[f2].vertices;
         if (ridge.size()>=min_ridge) {
            bool add=true;
            Entire<Graph<>::out_edge_list>::iterator e=entire(dual_graph.out_edges(f));
            while (!e.at_end()) {
               int inc=incl(ridges[*e],ridge);
               if (inc==2) {
                  ++e;
               } else {
                  if (inc<=0) dual_graph.out_edges(f).erase(e++);
                  if (inc>=0) { add=false; break; }
               }
            }
            if (add) ridges(f,f2)=ridge;
         }
      }
   }

   if (AH.rows()) vertices_so_far+=p;
#if POLYMAKE_DEBUG
   if (debug>=do_dump) {
      dump();
      cout << "\ninterior points: " << interior_points
           << "\n=======================================" << endl;
   }
#endif
   valid_facet=f;
}

template <typename E> inline
void beneath_beyond_algo<E>::facet_info::coord_full_dim (const beneath_beyond_algo<E>& A)
{
   normal=rows(null_space(A.points.minor(vertices,All))).front();
   if (normal * A.points[(A.vertices_so_far - vertices).front()] < 0) normal.negate();
   sqr_normal=sqr(normal);
}

template <typename E>
void beneath_beyond_algo<E>::facet_info::coord_low_dim (const beneath_beyond_algo<E>& A)
{
   ListMatrix< SparseVector<E> > Fn=A.facet_nullspace;
   null_space(entire(rows(A.points.minor(vertices,All))), black_hole<int>(), black_hole<int>(), Fn);
   normal=rows(Fn).front();
   if (normal * A.points[(A.vertices_so_far - vertices).front()] < 0) normal.negate();
   sqr_normal=sqr(normal);
}

template <typename Top> inline
int single_or_nothing(const GenericSet<Top,int>& s)
{
   int x=-1;
   typename Entire<Top>::const_iterator e=entire(s.top());
   if (!e.at_end()) {
      x=*e; ++e;
      if (!e.at_end()) x=-1;
   }
   return x;
}

template <typename E> template <typename Iterator> inline
void beneath_beyond_algo<E>::facet_info::add_incident_simplices(Iterator s, Iterator s_end)
{
   for (; s != s_end; ++s) {
      int opv=single_or_nothing(*s-vertices);
      if (opv>=0) simplices.push_back(incident_simplex(*s,opv));
   }
}

#if POLYMAKE_DEBUG
template <typename E>
void beneath_beyond_algo<E>::dump() const
{
   cout << "dual_graph:\n";
   const bool show_normals= debug==do_full_dump && (!AH.rows() || facet_nullspace.rows());
   for (Entire< Nodes< Graph<> > >::const_iterator f=entire(nodes(dual_graph)); !f.at_end(); ++f) {
      cout << f.index() << ": " << facets[*f].vertices;
      if (show_normals) cout << "=[ " << facets[*f].normal << " ]";
      if (debug==do_full_dump) {
         for (Entire< Graph<>::out_edge_list >::const_iterator e=entire(f.out_edges()); !e.at_end(); ++e)
            cout << " (" << e.to_node() << ' ' << ridges[*e] << ')';
         cout << " <<";
         for (typename Entire<typename facet_info::simplex_list>::const_iterator s=entire(facets[*f].simplices); !s.at_end(); ++s)
         cout << ' ' << *s->simplex << '-' << s->opposite_vertex;
         cout << " >>" << endl;
      } else {
         cout << ' ' << f.adjacent_nodes() << endl;
      }
   }
}

template <typename E> inline
void beneath_beyond_algo<E>::check_fp(int f_index, const facet_info& f, int p, std::ostringstream& errors) const
{
   const E prod= points[p] * f.normal;
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
void beneath_beyond_algo<E>::check_p(int p) const
{
   if (!AH.rows() || facet_nullspace.rows()) {
      std::ostringstream errors;

      for (Entire< Nodes< Graph<> > >::const_iterator f=entire(nodes(dual_graph)); !f.at_end(); ++f)
         check_fp(f.index(), facets[*f], p, errors);

      if (!errors.str().empty())
         throw std::runtime_error("beneath_beyond_algo - consistency checks failed:\n" + errors.str());
   }
}

template <typename E>
void beneath_beyond_algo<E>::check_f(int f, int last_p) const
{
   std::ostringstream errors;
   const facet_info& fi=facets[f];

   if (points_so_far.empty()) {
      for (typename Entire<sequence>::const_iterator p=entire(range(0,last_p)); !p.at_end(); ++p)
         check_fp(f, fi, *p, errors);
   } else {
      for (typename Entire<Bitset>::const_iterator p=entire(points_so_far); !p.at_end(); ++p)
         check_fp(f, fi, *p, errors);
   }

   if (!errors.str().empty())
      throw std::runtime_error("beneath_beyond_algo - consistency checks failed:\n" + errors.str());
}

template <typename E>
void beneath_beyond_algo<E>::dump_p(int p) const
{
   if (!AH.rows() || facet_nullspace.rows()) {
      for (Entire< Nodes< Graph<> > >::const_iterator f=entire(nodes(dual_graph)); !f.at_end(); ++f)
         if (f.degree()) {
            E prod= points[p] * facets[*f].normal;
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
