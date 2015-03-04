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

#include "polymake/client.h"
#include "polymake/topaz/complex_tools.h"
#include "polymake/FacetList.h"
#include "polymake/RandomSubset.h"
#include "polymake/list"

namespace polymake { namespace topaz {
namespace {

bool is_contractible(const std::list< Set<int> >& link_v,
                     const std::list< Set<int> >& link_w,
                     const std::list< Set<int> >& link_e)
{     
   const int d=link_v.front().size();
   for (int k=0; k<d-1; ++k) {
      const PowerSet<int> k_skel_vw = k_skeleton(link_v,k) * k_skeleton(link_w,k);
      const PowerSet<int> k_skel_e = k_skeleton(link_e,k);

      if (incl(k_skel_vw,k_skel_e)!=0)
         return false;
   }

   return (k_skeleton(link_v,d-1)*k_skeleton(link_w,d-1)).empty();
}

}

perl::Object edge_contraction(perl::Object p_in, perl::OptionSet options)
{
   const int n_vert = p_in.give("N_VERTICES");
   FacetList F = p_in.give("FACETS");

   // add cone over boundary if F is not closed
   const bool is_closed = p_in.give("CLOSED_PSEUDO_MANIFOLD");
   if (!is_closed) {
      const Array< Set<int> > B = p_in.give("BOUNDARY.FACETS");
      for (Entire< Array< Set<int> > >::const_iterator b=entire(B); !b.at_end(); ++b)
         F.insert(*b+n_vert);
   }

   const Set<int> vert = range(0,n_vert-1);
   Set<int> contracted_vert;
   PowerSet<int> tested_edges;
   const RandomSeed seed(options["seed"]);
   RandomPermutation< Set<int> > P(vert,seed);
   for (RandomPermutation< Set<int> >::const_iterator v=P.begin(); !v.at_end(); ++v) {
      if (contracted_vert.contains(*v))
         continue;

      Set<int> V;
      for (FacetList::iteratorMax star = F.findMax(scalar2set(*v)); !star.at_end(); ++star)
         V += *star-*v;

      for (Entire< Set<int> >::const_iterator w=entire(V); !w.at_end(); ++w) {
         const Set<int> e = scalar2set(*v)+scalar2set(*w);
         if (tested_edges.contains(e))
            continue;
         tested_edges += e;
         
         std::list< Set<int> > link_v, link_w, link_e;
         for (FacetList::iteratorMax star = F.findMax(scalar2set(*v)); !star.at_end(); ++star)
            link_v.push_back(*star-*v);
         for (FacetList::iteratorMax star = F.findMax(scalar2set(*w)); !star.at_end(); ++star)
            link_w.push_back(*star-*w);
         for (FacetList::iteratorMax star = F.findMax(e); !star.at_end(); ++star)
            link_e.push_back(*star-e);

         if (is_contractible(link_v,link_w,link_e)) { // e is contarctible
            F.eraseMin(scalar2set(*w)); // remove star(w)

            // update star(w)-star(e)
            for (Entire< std::list< Set<int> > >::const_iterator l=entire(link_w); !l.at_end(); ++l)
               if (((*l)*(*v)).empty())
                  F.insert(*l + *v);

            contracted_vert += *w;
         }
      }
   }

   if (!is_closed)  // remove apex
      F.eraseMin(scalar2set(n_vert));
   F.squeeze();

   perl::Object p_out("SimplicialComplex");
   p_out.set_description() << "Simplicial complex obtained from " << p_in.name() << " by a series of edge contractions;\n"
                              "seed=" << seed.get() << endl;
   p_out.take("FACETS") << F;
   return p_out;
}

UserFunction4perl("# @category Producing a new simplicial complex from others\n"
                  "# Heuristic for simplifying the triangulation of the given manifold\n"
                  "# without changing its PL-type. Choosing a random order of the vertices,\n"
                  "# the function tries to contract all incident edges.\n"
                  "# @param SimplicialComplex complex"
                  "# @option Int seed"
                  "# @return SimplicialComplex",
                  &edge_contraction, "edge_contraction(SimplicialComplex { seed=>undef })");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
