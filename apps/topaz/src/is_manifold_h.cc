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
#include "polymake/topaz/is_sphere_h.h"
#include <sstream>

namespace polymake { namespace topaz {

int is_manifold_client(perl::Object p, perl::OptionSet options)
{
   const HasseDiagram HD = p.give("HASSE_DIAGRAM");
   const bool is_closed = p.give("CLOSED_PSEUDO_MANIFOLD");
   
   const int strategy=options["strategy"];
   int n_stable_rounds=0; // meaningless initialization to avoid a compiler warning
   if (!(options["stable_rounds"] >> n_stable_rounds))
      n_stable_rounds=(HD.dim()-1) * 1000;

   const bool verbose=options["verbose"];
   const RandomSeed seed(options["seed"]);
   UniformlyRandom<Integer> random_source(seed);

   bool res_undef = false;
   for (Entire<sequence>::iterator n=entire(HD.node_range_of_dim(0)); !n.at_end(); ++n) {
      int local_strategy = strategy;
      const std::list< Set<int> > link=as_iterator_range(link_in_HD(HD,*n));

      bool is_bos= is_closed ? is_sphere_h(link, random_source, local_strategy, n_stable_rounds)
                             : is_ball_or_sphere_h(link, random_source, local_strategy, n_stable_rounds);

      while (!is_bos && ++local_strategy <= 1) {
         if (verbose)
            cout << "is_manifold_h: after " << n_stable_rounds
                 << " iterations without improvement:\nUnable to determine, whether link("
                 << HD.face(*n) << ") is a ball or a sphere.\n"
                 << "Trying strategy " << local_strategy << "." << endl;
         
         is_bos= is_closed ? is_sphere_h(link, random_source, local_strategy, n_stable_rounds)
                           : is_ball_or_sphere_h(link, random_source, local_strategy, n_stable_rounds);
      }

      if (!is_bos) {
         res_undef = true;

         if (verbose)
            cout << "is_manifold_h: after " << n_stable_rounds
                 << " iterations without improvement:\nUnable to determine, whether link("
                 << HD.face(*n) << ") is a ball or a sphere." << endl;

         if (!options["all"])  break;
      }
   }

   if (res_undef)
      return -1;
   else
      return 1;
}

Function4perl(&is_manifold_client, "is_manifold_h(SimplicialComplex { strategy=>0, stable_rounds=>undef, verbose=>0, all=>0, seed=>undef })");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
