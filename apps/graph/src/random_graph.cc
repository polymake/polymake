/* Copyright (c) 1997-2014
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
#include "polymake/Graph.h"
#include "polymake/RandomGenerators.h"
#include "polymake/graph/connected.h"

namespace polymake { namespace graph {

namespace {

Graph<> erdos_renyi_impl(const int n, const Rational& p, UniformlyRandom<AccurateFloat>::iterator& random)
{
   Graph<> g(n);
   for (int i=0; i<n-1; ++i)
      for (int j=i+1; j<n; ++j)
         if (*random <= p)
            g.edge(i,j);
   return g;
}

} // end anonymous namespace


perl::Object random_graph(const int n, perl::OptionSet options)
{
   if (n < 2)
      throw std::runtime_error("need at least 2 nodes");

   const Rational p = options["p"];
   const RandomSeed seed(options["seed"]);
   UniformlyRandom<AccurateFloat> rg(seed);  // Generator of random AccurateFloat numbers from [0, 1)
   UniformlyRandom<AccurateFloat>::iterator random = rg.begin();
   
   Graph<> g = erdos_renyi_impl(n, p, random);

   const bool try_connected = options["try_connected"];
   if (try_connected) {
      const int max_attempts = options["max_attempts"];
      int ct(0);
      while (!is_connected(g) && ct++ < max_attempts) 
         g = erdos_renyi_impl(n, p, random);

      if (!is_connected(g))
         throw std::runtime_error("Failed to find a connected graph. Try again or increase p");
   }

   perl::Object G("Graph<>");
   G.take("N_NODES") << n;
   G.take("N_EDGES") << g.edges();
   if (try_connected) 
      G.take("CONNECTED") << true;
   G.take("ADJACENCY") << g;
   G.set_description() << (try_connected ? "Connected " : "")
                       << "Erdos-Renyi random graph instance for p=" << p << " on " << n << " nodes; seed = " << seed.get() << endl;
   return G;
}

UserFunction4perl("# @category Producing from scratch\n"
                  "# Constructs a random graph with //n// nodes according to the Erdos-Renyi model."
                  "# Each edge is chosen uniformly with probability //p//."
                  "# @param Int n"
                  "# @option Rational p the probability of an edge occurring; default 1/2"
                  "# @option Bool try_connected whether to try to generate a connected graph, default 1"
                  "# @option Int max_attempts If //connected// is set, specifies "
                  "#   how many times to try to make a connected random graph before giving up."
                  "# @option Int seed controls the outcome of the random number generator;"
                  "#   fixing a seed number guarantees the same outcome."
                  "# @return Graph",
                  &random_graph, "random_graph($ { p => 1/2, try_connected => 1, max_attempts => 1000, seed => undef } )");


} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
