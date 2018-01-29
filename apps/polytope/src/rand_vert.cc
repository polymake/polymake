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
#include "polymake/Matrix.h"
#include "polymake/Set.h"
#include "polymake/RandomSubset.h"

namespace polymake { namespace polytope {

template <typename Coord>
Matrix<Coord> rand_vert(const Matrix<Coord> &V, int n_vertices_out, perl::OptionSet options)
{
   const int n_vertices_in=V.rows();
   if (n_vertices_out > n_vertices_in)
      throw std::runtime_error("number of output vertices too high\n");

   Set<int> random_vertices=select_random_subset(sequence(0,n_vertices_in), n_vertices_out, RandomSeed(options["seed"]));
   return V.minor(random_vertices, All);
}

UserFunctionTemplate4perl("# @category Producing a polytope from polytopes"
                          "# Selects //n// random vertices from the set of vertices //V//."
                          "# This can be used to produce random polytopes which are neither simple nor simplicial as follows:"
                          "# First produce a simple polytope (for instance at random, by using rand_sphere, rand, or unirand)."
                          "# Then use this client to choose among the vertices at random."
                          "# Generalizes random 0/1-polytopes."
                          "# @param Matrix V the vertices of a polytope"
                          "# @param Int n the number of random points"
                          "# @option Int seed controls the outcome of the random number generator;"
                          "#   fixing a seed number guarantees the same outcome."
                          "# @return Matrix",
                          "rand_vert(Matrix, $, { seed=>undef })");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
