/* Copyright (c) 1997-2018
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

#include "polymake/topaz/BistellarComplex.h"
#include "polymake/topaz/hasse_diagram.h"

namespace polymake { namespace topaz {

void BistellarComplex::init(const Lattice<BasicDecoration>& HD)
{
   // test if the complex is closed.
   if (!closed) {
      const auto B = boundary_of_pseudo_manifold(HD);
      closed = B.empty();

      if (!closed) {
         // compute C + cone(bound(C))
         std::list< Set<int> > S;

         for (const auto f : HD.nodes_of_rank(HD.rank()-1)) {
            S.push_back(HD.face(f));
            const int w = HD.face(f).back();
            if (w >= verts)
               verts = w+1;
         }
         apex = verts;
         ++verts;

         for (auto b=entire(B); !b.at_end(); ++b)
            S.push_back(b->face+apex);
         const Lattice<BasicDecoration> new_HD = hasse_diagram_from_facets(Array<Set<int> >(S));

         // compute raw options
         for (int d=0; d<=dim; ++d) {
            for (const auto n : new_HD.nodes_of_rank(d+1)) {
               const Set<int>& face = new_HD.face(n);

               if (d==0 && face.front() == apex)  // the apex is not an option
                  continue;

               if (d==dim) {  // each facet is an option
                  the_facets.insert(face);
                  raw_options[d].insert(face, Set<int>());

               } else {
                  Set<int> V;
                  accumulate_in(link_in_HD(new_HD, n), operations::add(), V);

                  if (V.size()+face.size()==dim+2)  // face is raw option
                     raw_options[d].insert(face,V);
               }
            }
         }
      }
   }

   if (closed) {
      for (int d=0; d<=dim; ++d) {
         for (const auto n : HD.nodes_of_rank(d+1)) {
            const Set<int> face=HD.face(n);

            if (d==0) {    // face is a vertex
               const int v = face.front();
               if (v >= verts)
                  verts = v+1;
            }

            if (d==dim) {  // each facet is an option
               the_facets.insert(face);
               raw_options[d].insert(face, Set<int>());

            } else {
               Set<int> V;
               accumulate_in(link_in_HD(HD, n), operations::add(), V);

               if (V.size()+face.size()==dim+2)   // face is raw option
                  raw_options[d].insert(face,V);
            }
         }
      }
   }
}

int BistellarComplex::find_move(const int dim_min, const int dim_max)
{
   for (int d=dim_min; d<=dim_max; ++d) {
      const RandomPermutation< Array<option> > P(raw_options[d].options(), random_source);

      for (RandomPermutation< Array<option> >::const_iterator opt=P.begin(); !opt.at_end(); ++opt)
         if ( (allow_rev_move || incl(opt->first,rev_move)!=0) &&
              (d==dim || the_facets.findSupersets(opt->second).at_end()) ) {
            next_move = *opt;
            return opt->first.size() - 1;
         }
   }

   throw std::runtime_error("BistellarComplex: No move found.");
}

bool BistellarComplex::is_option(const Set<int>& f, Set<int>& V) const
{
   if (!closed && f.size()==1 && f.front()==apex)  // apex is not an option
      return false;

   for (FacetList::superset_iterator star = the_facets.findSupersets(f);
        !star.at_end(); ++star)
      V += *star;
   V -= f;

   return V.size()+f.size()==dim+2;
}

void BistellarComplex::execute_move()
{
   const Set<int> face = next_move.first;
   const int face_dim = face.size()-1;
   if (face_dim==dim)  // allocate an index for the new vertex
      next_move.second = scalar2set(verts++);
   const Set<int> co_face= next_move.second;
   if (!allow_rev_move)  rev_move = co_face;

   if (verbose)
      cout << "BistellarComplex: executing move of dim "
           << face.size()-1 << ": (" << face << "," << co_face << ")\n";

   // update the_flip_vector
   if (dim%2==1 || face_dim!=dim/2) {
      if (face_dim < (dim+1)/2)
         --the_flip_vector[face_dim];
      else
         ++the_flip_vector[dim-face_dim];
   }

   // remove star(face) from raw_options and the_facets
   std::list< Set<int> > star;
   the_facets.eraseSupersets(face, std::back_inserter(star));

   Lattice<BasicDecoration> star_HD = hasse_diagram_from_facets(Array<Set<int> >(star));
   for (int d=0; d<=dim; ++d)
      for (const auto n : star_HD.nodes_of_rank(d+1))
         raw_options[d].remove(star_HD.face(n));

   // add co_face * boundary(face)
   std::list< Set<int> > new_facets;
   for (auto w=entire(face); !w.at_end(); ++w) {
      Set<int> f=face;
      f-=*w;
      f+=co_face;

      the_facets.insert(f);
      new_facets.push_back(f);
   }

   // find new raw_options
   Lattice<BasicDecoration> local_HD = hasse_diagram_from_facets(Array<Set<int> >(new_facets));
   for (int d=0; d<=dim; ++d)
      for (const auto n : local_HD.nodes_of_rank(d+1)) {
         const Set<int>& f = local_HD.face(n);

         if (d==dim) {
            raw_options[d].insert(f, Set<int>());
         } else {
            Set<int> V;
            if (is_option(f,V))
               raw_options[d].insert(f,V);
         }
      }
}

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
