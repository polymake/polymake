/* Copyright (c) 1997-2019
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
#include "polymake/Graph.h"
#include "polymake/Integer.h"
#include "polymake/RandomGenerators.h"
#include "polymake/graph/connected.h"

namespace polymake { namespace graph {

namespace {

// Graph<> erdos_renyi_Mmodel(const int n, int M, UniformlyRandom<AccurateFloat>::iterator& random)
// {
//    Graph<> g(n);
//    for (int i=0; i<n-1; ++i)
//       for (int j=i+1; j<n; ++j)
//          if (*random <= M)
//             g.edge(i,j);
//    return g;
// }


class Generator_Mmodel {
   private:
      const int n;
      const int M;
      UniformlyRandomRanged<long> rg;
      UniformlyRandomRanged<long>::iterator random;

   public:
   Generator_Mmodel(const RandomSeed& seed, const int nin, const int& Min) : n(nin), M(Min), rg(n, seed) {
      if(M > Integer::binom(n, 2)){
         throw std::runtime_error(std::string("Cannot produce graph with more than ") + std::to_string(convert_to<int>(Integer::binom(n, 2))) + std::string(" (< ") + std::to_string(M) + std::string(") edges on ") + std::to_string(n) + std::string(" nodes."));
      }
      random = rg.begin();
   }

   Graph<> next(){
      Graph<> g(n);
      int c = 0;
      while(c < M){
         int source = *random;
         int target = source;
         while(target == source){
            target = *random;
         }
         g.edge(source, target);
         c = g.edges();
      }
      return g;
   }
};

class Generator_pmodel {
   private:
      int n;
      UniformlyRandom<AccurateFloat> rg;
      UniformlyRandom<AccurateFloat>::iterator random;
      const Rational p;

   public:
   Generator_pmodel(const RandomSeed& seed, int nin, const Rational& pin) : n(nin), rg(seed), p(pin) {
      random = rg.begin();
   }

   Graph<> next()
   {
      Graph<> g(n);
      for (int i=0; i<n-1; ++i)
         for (int j=i+1; j<n; ++j)
            if (*random <= p)
               g.edge(i,j);
      return g;
   }
};

template<typename Generator>
perl::Object erdos_renyi_model(Generator& gen, const bool try_connected, const int max_attempts) {
   Graph<> g = gen.next();
   if(try_connected){
      int ct(0);
      while (!is_connected(g) && ct++ < max_attempts) 
         g = gen.next();
      if (!is_connected(g))
         throw std::runtime_error("Failed to find a connected graph. Try again or increase p");
   }
   perl::Object result("Graph<>");
   result.take("N_NODES") << g.nodes();
   result.take("N_EDGES") << g.edges();
   if (try_connected) 
      result.take("CONNECTED") << true;
   result.take("ADJACENCY") << g;
   return result;
}

} // end anonymous namespace


perl::Object random_graph(const int n, perl::OptionSet options)
{
   if (n < 2)
      throw std::runtime_error("need at least 2 nodes");

   bool hasp = options.exists("p");
   if(options.exists("p") && options.exists("M")){
      throw std::runtime_error("Please only select one out of p and M.");
   }
   if(!options.exists("p") && !options.exists("M")){
      // Default is p-model with p=1/2;
      hasp = true;
   }

   const bool try_connected = options["try_connected"];
   const int max_attempts = options["max_attempts"];
   const RandomSeed seed(options["seed"]);
   Graph<> g;
   Rational p(1,2);
   if(hasp){
      if(options.exists("p")){
         options["p"] >> p;
      }
      Generator_pmodel gen(seed, n, p);
      perl::Object result = erdos_renyi_model(gen, try_connected, max_attempts);
      result.set_description() << (try_connected ? "Connected " : "")
                          << "Erdos-Renyi random graph instance for p=" << p << " on " << n << " nodes; seed = " << seed.get() << endl;
      return result;
   } else {
      int M = int(options["M"]);
      Generator_Mmodel gen(seed, n, M);
      perl::Object result = erdos_renyi_model(gen, try_connected, max_attempts);
      result.set_description() << (try_connected ? "Connected " : "")
                          << "Erdos-Renyi random graph instance for M=" << M << " edges on " << n << " nodes; seed = " << seed.get() << endl;
      return result;
   }

}

UserFunction4perl("# @category Producing a graph\n"
                  "# Constructs a random graph with //n// nodes according to the Erdos-Renyi model."
                  "# The default is the G(n, p) model: Each edge is chosen uniformly with probability //p//."
                  "# Optionally one can switch to the G(n, M) model to get a random graph on //n// nodes with exactly //M// edges."
                  "# See P. Erdős and A. Rényi. On random graphs. Publ. Math. 6, 290--297 (1959; Zbl 0092.15705)"
                  "# @param Int n"
                  "# @option Rational p the probability of an edge occurring; default 1/2"
                  "# @option Int M the number of edges in the graph"
                  "# @option Bool try_connected whether to try to generate a connected graph, default 1"
                  "# @option Int max_attempts If //connected// is set, specifies "
                  "#   how many times to try to make a connected random graph before giving up."
                  "# @option Int seed controls the outcome of the random number generator;"
                  "#   fixing a seed number guarantees the same outcome."
                  "# @return Graph"
                  "# @example [nocompare] The following produces a connected graph on 10 nodes using a specific seed for a random graph model, where an edge between two nodes occurs with probabilty 0.1."
                  "# > $g = random_graph(10,p=>0.1,try_connected=>1,max_attempts=>50,seed=>100000);"
                  "# > print $g->N_EDGES;"
                  "# | 9",
                  &random_graph, "random_graph($ { p => undef, M => undef, try_connected => 1, max_attempts => 1000, seed => undef } )");


} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
