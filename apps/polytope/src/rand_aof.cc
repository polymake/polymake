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

#include "polymake/client.h"
#include "polymake/vector"
#include "polymake/list"
#include "polymake/RandomGenerators.h"
#include "polymake/Vector.h"
#include "polymake/Rational.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/FacetList.h"

namespace polymake { namespace polytope {

Vector<Rational> rand_aof(perl::Object p, int start, perl::OptionSet options)
{
   const bool simple=p.give("SIMPLE");
   if (!simple)
      throw std::runtime_error("polytope is not simple");

   const IncidenceMatrix<> Boundary=p.give("VERTICES_IN_FACETS");
   const int n=Boundary.cols(); // number of facets of the complex = number of vertices of the polytope
   const int d=Boundary.col(0).size()-1; // dimension of the boundary complex

   const RandomSeed seed(options["seed"]);
   UniformlyRandomRanged<long> random(n, seed);

   if (start>=0) {
      if (start>=n)
         throw std::runtime_error("start vertex out of range");
   } else {
      // take a random start vertex unless specified
      start=random.get();
   }

   // abstract objective function with minimum at start
   typedef std::list<int> vertex_list;
   vertex_list AOF;
   AOF.push_back(start);

   // facets of the complex/vertices of the polytope not considered yet
   Set<int> available(sequence(0,start)+sequence(start+1,n-start-1));

   // union of all the facets shelled so far; i.e. a ball
   Set<int> ball(Boundary.col(start));

   std::vector<int> candidates;
   candidates.reserve(n);
   while (available.size()>1) {
      // determine which of the facets could continue the shelling
      for (Entire< Set<int> >::const_iterator it=entire(available); !it.at_end(); ++it) {
         const Set<int> intersection_with_previous(Boundary.col(*it) * ball);
         switch (intersection_with_previous.size()-d) {
         case 0: // must be contained in a unique facet
            for (Entire<vertex_list>::const_iterator v=entire(AOF); !v.at_end(); ++v)
               if (incl(intersection_with_previous, Boundary.col(*v))<=0) { // contained in this facet
                  candidates.push_back(*it);
                  break;
               }
            break;
         case 1: {
            FacetList intersection_as_a_complex(n);
            for (Entire<vertex_list>::const_iterator v=entire(AOF); !v.at_end(); ++v)
               intersection_as_a_complex.replaceMax(Boundary.col(*it) * Boundary.col(*v));
            if (intersection_as_a_complex.size()>d)
               break;
            bool pure=true;
            for (Entire<FacetList>::const_iterator s=entire(intersection_as_a_complex); !s.at_end(); ++s)
               if (s->size()!=d) {
                  pure=false;
                  break;
               }
            if (pure)
               candidates.push_back(*it);
            break;
         }
         default:
            break;
         }
      }
      if (candidates.empty()) // shelling cannot be extended
         break;

      // choose one at random
      random.upper_limit()=candidates.size();
      const int this_one=candidates[random.get()];
      candidates.clear();
      AOF.push_back(this_one);
      ball += Boundary.col(this_one);
      available -= this_one;
   }

   if (available.size()!=1) {
      std::ostringstream err;
      wrap(err) << "NON_EXTENDIBLE_PARTIAL_SHELLING: " << AOF << '\n';
      throw std::runtime_error(err.str());
   }

   AOF.push_back(available.front());
   Vector<Rational> AOF_Vec(n);
   int cnt=0;
   for (Entire<vertex_list>::iterator x=entire(AOF); !x.at_end(); ++x)
      AOF_Vec[*x]=cnt++;
   return AOF_Vec;
}

UserFunction4perl("# @category Optimization"
                  "# Produce a random abstract objective function on a given __simple__ polytope //P//."
                  "# It is assumed that the boundary complex of the dual polytope is extendibly shellable."
                  "# If, during the computation, it turns out that a certain partial shelling cannot be extended,"
                  "# then this is given instead of an abstract objective function."
                  "# It is possible (but not required) to specify the index of the starting vertex //start//."
                  "# @param Polytope P a __simple__ polytope"
                  "# @param Int start the index of the starting vertex; default value: random"
                  "# @option Int seed controls the outcome of the random number generator;"
                  "#   fixing a seed number guarantees the same outcome. "
                  "# @return Vector<Rational>",
                  &rand_aof,"rand_aof(Polytope; $=-1, { seed => undef } )");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
