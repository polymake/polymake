/* Copyright (c) 1997-2021
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

#include "polymake/client.h"
#include "polymake/topaz/complex_tools.h"
#include "polymake/topaz/is_sphere_h.h"

namespace polymake { namespace topaz {

// return values: 1=true, 0=false, -1=undef
Int is_ball_or_sphere_client(BigObject p, bool check_for_sphere, OptionSet options)
{
   const Array<Set<Int>> C = p.give("FACETS");
   const Int d = p.give("DIM");
   const Int n_vertices = p.give("N_VERTICES");

   // deterministic checks for low dimensions
   switch (d) {
   case 0:
      if (C.size()<3)
         return 1;
      else
         return 0;
   case 1:
      return is_ball_or_sphere(C, n_vertices, int_constant<1>());
   case 2:
      return is_ball_or_sphere(C, n_vertices, int_constant<2>());
   default:
      break;
   }

   // heuristics begin
   const Lattice<BasicDecoration>& HD = p.give("HASSE_DIAGRAM");

   Int strategy = options["strategy"];
   Int n_stable_rounds = 0; // meaningless initialization to avoid a compiler warning
   if (!(options["stable_rounds"] >> n_stable_rounds))
      n_stable_rounds=(HD.rank()-2)*1000; // default

   const bool verbose = options["verbose"];
   const RandomSeed seed(options["seed"]);
   UniformlyRandom<Integer> random_source(seed);

   Int is_ball_or_sphere = check_for_sphere ? is_sphere_h(HD, random_source, strategy, n_stable_rounds)
                                            : is_ball_or_sphere_h(HD, random_source, strategy, n_stable_rounds);
   while (is_ball_or_sphere < 0 && ++strategy <= 1) {
      if (verbose)
         cout << "is_ball_or_sphere_h: after " << n_stable_rounds
              << " iterations without improvement:\nUnable to determine, whether the complex is a ball or a sphere.\n"
              << "Trying strategy " << strategy << "." << endl;
      is_ball_or_sphere = check_for_sphere ? is_sphere_h(HD, random_source, strategy, n_stable_rounds)
                                           : is_ball_or_sphere_h(HD,random_source, strategy, n_stable_rounds);
   }

   if (verbose && is_ball_or_sphere < 0) {
      cout << "is_ball_or_sphere_h: after " << n_stable_rounds
           << " iterations without improvement:\nUnable to determine, whether the complex is a ball or a sphere.\n";
   }

   return is_ball_or_sphere;
}

Function4perl(&is_ball_or_sphere_client, "is_ball_or_sphere(SimplicialComplex $ { strategy=>0, stable_rounds=>undef, verbose=>0, seed=>undef })");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
