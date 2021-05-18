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
#include "polymake/topaz/BistellarComplex.h"
#include "polymake/graph/compare.h"
#include "polymake/RandomGenerators.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/list"
#include <sstream>
#include <cmath>
#include <cstdlib>

namespace polymake { namespace topaz {
namespace {

bool improved(const Array<Int>& min, const Array<Int>& cand, const Int obj)
{
   switch (obj) {
   case 0: // lex order
      for (Int i = 0; i < min.size(); ++i) {
         if (cand[i] < min[i]) return true;
         if (cand[i] > min[i]) return false;
      }
      return false;

      /* The following seems to give undesirable results.
         Left here for future experiments.
   case 1:  // rev lex order
      for (Int i = min.size()-1; i >= 0; ++i) {
         if (cand[i] < min[i]) return true;
         if (cand[i] > min[i]) return false;
      }
      return false;
      */

   default:
      return accumulate(cand, operations::add()) < accumulate(min, operations::add());
   }
}

} // end empty namespace

bool bistellar(BigObject p1, BigObject p_in, OptionSet options, const bool compare=true)
{
   Vector<Int> distribution_src;
   bool dist = options["distribution"]>>distribution_src;

   const Int verbose = options["verbose"];

   if (verbose)
      cout << "initialising ...\n";

   const Lattice<BasicDecoration>& HD = p_in.give("HASSE_DIAGRAM");
   const Int dim = HD.rank()-2;
   const Int size = HD.nodes_of_rank(dim+1).size();

   const bool is_closed = p_in.give("CLOSED_PSEUDO_MANIFOLD");
   const bool is_pmf = p_in.give("PSEUDO_MANIFOLD");
   if (!is_pmf)
      throw std::runtime_error("bistellar: Complex is not a PSEUDO_MANIFOLD.");

   // read complex to compare to
   Int n_facets_comp = 0;
   IncidenceMatrix<> facets_comp;
   if (compare) {
      n_facets_comp = p1.give("N_FACETS");
      IncidenceMatrix<> tmp_facets = p1.give("FACETS");
      facets_comp = tmp_facets;
   }

   // determin min_n_facets
   Int min_n_facets;
   if (!(options["min_n_facets"] >> min_n_facets))
      min_n_facets = is_closed ? dim+2 : 1;

   const RandomSeed seed(options["seed"]);

   Int n_rounds;
   if (!(options["rounds"] >> n_rounds))
      n_rounds = 100 * size;
   const bool abs = options["abs"];

   Int obj = 1;
   options["obj"] >> obj;
   if (obj < 0 || obj > 2)
      throw std::runtime_error("bistellar: no objective function {0|1|2} specified.");

   Int max_relax;
   if (!(options["max_relax"]>>max_relax))
      max_relax = std::max(dim, size/10);
   const Int init_max_relax = max_relax;
   const bool my_constant = options["constant"];
   Int heat;
   if (!( options["heat"]>>heat))
      heat = std::max(dim, size/10);
   const Int init_heat = heat;
   Int min_heat_dim = dist ? dim - distribution_src.size()+1 : (dim+1)/2;

   // compute distribution
   if (!dist) {  // no distribution specified
      distribution_src = Vector<Int>(dim-min_heat_dim+1, 10);
      distribution_src[dim-min_heat_dim] = 1;
   }
   else {
      if (distribution_src.size()>dim+1)
         throw std::runtime_error("bistellar: distribution has too many values.");
      if (distribution_src.empty())
         throw std::runtime_error("bistellar: distribution is empty.");
   }

   UniformlyRandom<Integer> random_source(seed);
   DiscreteRandom distribution(distribution_src, random_source);

   BistellarComplex BC(HD, random_source, verbose==1, is_closed, options["allow_rev_move"]);

   Array<Int> min_f_vector, min_flip_vector;
   FacetList F = BC.facets();
   min_flip_vector = BC.flip_vector();

   if (verbose) {
      cout << "\nseed:           " << seed.get()
           << "\ndim:            " << dim
           << "\nrounds:         " << n_rounds
           << "\nabs:            " << (abs?"true":"false")
           << "\nobjective:      " << obj
           << "\nrelax:          " << max_relax
           << "\nheat:           " << heat
           << "\nconstant:       " << (my_constant?"true":"false")
           << "\nallow_rev_move: " << (options["allow_rev_move"]?"true":"false")
           << "\nmin_n_facets:   " << min_n_facets
           << "\nverbose:        " << verbose
           << "\ndistribution:   " << distribution_src;
      if (compare) cout << "\nTESTING FOR PL-HOMEOMORPHY";
      cout << "\n\n... done\n";
   }

   // check for ball or sphere
   bool bos = false;
   bool mnf = false;
   bool is_pl = false;

   const bool quiet =options["quiet"];
   if (is_closed && BC.n_facets() == dim+2) {
      if (!quiet)
         cout << "\n\nThe complex is a " << dim << "-sphere.\n\n";
      bos = true;
   }
   if (BC.n_facets() == 1) {
      if (!quiet)
         cout  << "\n\nThe complex is a " << dim << "-ball.\n\n";
      bos = true;
   }
   if (BC.n_facets() <= min_n_facets) {
      if (!quiet)
         cout  << "\n\nThe complex has at most " << min_n_facets << " facets."
               << "\nFurther simplification might be possible.\n\n";
      mnf = true;
   }
   if (compare) {
      if (BC.n_facets()==n_facets_comp && graph::isomorphic(BC.as_incidence_matrix(),facets_comp)) {
         if (!quiet) cout << "\n\nsimplicial complexes are pl-homeomorphic.\n\n";
         is_pl = true;
      }
      if (BC.n_facets()<n_facets_comp) {
         if (!quiet) cout  << "\n\nThe complex has less facets than test complex."
                           << "\nFurther simplification might be possible.\n\n";
         mnf = true;
      }
   }

   Int rounds = 0;
   if (!bos && !mnf && !is_pl) {
      Int stable_rounds = 0, relax = 0, heating = 0;
      for ( ; (!abs && stable_rounds<n_rounds) || (abs && rounds<n_rounds); ++stable_rounds, ++rounds) {
         if (verbose && rounds%verbose==0) {
            cout << "\n" << rounds << ": current best " << "flip_vector: " << min_flip_vector
                 << "     n_facets: " << F.size() << endl;
            if (verbose==1)
               cout << "flip_vector: " << BC.flip_vector() << "     n_facets: " << BC.n_facets() << endl;
         }

         if (relax >= max_relax) {  // heating up
            relax = 0;
            heating = heat;
         }

         if (verbose==1 && heating>0)
            cout << "HEATING UP for another " << heating << " moves\n";
         if (verbose>1 && heating>0 && verbose<=5*init_heat && heating==heat)
            cout << rounds << ": current flip_vector: " << BC.flip_vector()
                 << "\nHEATING UP for " << heating << " moves\n";

         if (heating>0) {
            --heating;
            const Int rnd_d = min_heat_dim + distribution.get();
            const Int move_dim = BC.find_move(rnd_d);
            if (rnd_d != move_dim)
               BC.min_rev_move(min_heat_dim);
            else
               BC.execute_move();

            continue;
         }

         // make smallest reversed move
         const Int move_dim = BC.min_rev_move();

         if (move_dim >= (dim+1)/2)   // up or eaven move
            ++relax;

         else // down move
            if ( improved(min_flip_vector,BC.flip_vector(),obj)) {  // new triangulation found
               stable_rounds = 0;
               relax = 0;

               min_flip_vector = BC.flip_vector();
               F=BC.facets();
               if (!my_constant) {
                  max_relax = std::min( init_max_relax, std::max(dim,init_max_relax*BC.n_facets()/size) );
                  heat = std::min( init_heat, std::max(dim,init_heat*BC.n_facets()/size) );
               }

               if (verbose == 1)
                  cout << "new smallest triangulation found\n";

               // check for sphere
               if ( is_closed && BC.n_facets()==dim+2) {
                  if (!quiet)
                     cout << "\n" << rounds
                          << ": flip_vector: " << min_flip_vector
                          << "     n_facets: " << F.size()
                          << "\n\nThe complex is a " << dim << "-sphere.\n\n";
                  break;
               }

               // check for ball
               if ( BC.n_facets()==1 ) {
                  if (!quiet)
                     cout << "\n" << rounds
                              << ": flip_vector: " << min_flip_vector
                              << "     n_facets: " << F.size()
                              << "\n\nThe complex is a " << dim << "-ball.\n\n";
                  break;
               }

               // check min_n_facets
               if ( BC.n_facets()<=min_n_facets ) {
                 if (!quiet)
                    cout << "\n" << rounds
                         << ": flip_vector: " << min_flip_vector
                         << "     n_facets: " << F.size()
                         << "\n\nThe complex has at most " << min_n_facets << " facets."
                         << "\nFurther simplification might be possible.\n\n";
                 break;
               }
            }

         // check for comb. isomorphism
         if (compare) {
            if (BC.n_facets()==n_facets_comp && graph::isomorphic(BC.as_incidence_matrix(),facets_comp)) {
               if (!quiet)
                  cout << "\n" << rounds
                       << ": flip_vector: " << min_flip_vector
                       << "     n_facets: " << F.size() << "\n\nsimplicial complexes are pl-homeomorphic.\n\n";
               is_pl=true;
               break;
            }
            if (BC.n_facets()<n_facets_comp) {
               if (!quiet)
                  cout << "\n" << rounds
                       << ": flip_vector: " << min_flip_vector
                       << "     n_facets: " << F.size() <<"\n\nThe complex has less facets than test complex."
                       << "\nFurther simplification might be possible.\n\n";
               break;
            }
         }
      }  // end searching

      if (!quiet && (stable_rounds==n_rounds || rounds==n_rounds) ) {
         cout << "\nAll " << n_rounds
              << " moves were executed";
         if (!abs)
            cout << " without improvement" << ".\nflip_vector: " << min_flip_vector
                 << "     n_facets: " << F.size() << "\nFurther simplification might be possible.\n\n";
         if (compare)
            cout << "no pl-homeomorphism found\n\n";
      }

   }  // end !bos

   if (!compare) {  // output small traingulation
      F.squeeze();
      if (verbose)
         p1.set_description() << "Simplicial complex obtained from " << p_in.name()
                              << " by a series of " << "bistellar flips."
                              << "\nparameters for the bistellar function:"
                              << "\nseed:           " << seed.get()
                              << "\nrounds:         " << n_rounds
                              << "\nabs:            " << (abs?"true":"false")
                              << "\ntotal rounds:   " << rounds
                              << "\nobjective:      " << obj
                              << "\nrelax:          " << init_max_relax
                              << "\nheat:           " << init_heat
                              << "\nconstant:       " << (my_constant?"true":"false")
                              << "\nallow_rev_move: " << (options["allow_rev_move"]?"true":"false")
                              << "\nmin_n_facets:   " << min_n_facets
                              << "\ndistribution:   " << distribution_src<<endl;
      else
         p1.set_description() << "Simplicial complex obtained from " << p_in.name()
                              << " by a series of " << "bistellar flips."<<endl;

      p1.take("FACETS") << F;
   }
   return is_pl;
}


BigObject bistellar_simplification(BigObject p_in, OptionSet options)
{
   BigObject p_out("SimplicialComplex");
   bistellar(p_out, p_in, options, 0);
   return p_out;
}

bool pl_homeomorphic(BigObject p_in1, BigObject p_in2, OptionSet options)
{
   return bistellar(p_in1,p_in2,options,1);
}

UserFunction4perl("# @category Comparing"
                  "# Tries to determine whether two complexes are pl-homeomorphic by using"
                  "#  bistellar flips and a simulated annealing strategy."
                  "# "
                  "# You may specify the maximal number of //rounds//, how often the system"
                  "# may //relax// before heating up and how much //heat// should be applied."
                  "# The function stops computing, once the size of the triangulation has not decreased"
                  "# for //rounds// iterations. If the //abs// flag is set, the function stops"
                  "# after //rounds// iterations regardless of when the last improvement took place."
                  "# Additionally, you may set the threshold //min_n_facets// for the number of facets when"
                  "# the simplification ought to stop. Default is d+2 in the [[CLOSED_PSEUDO_MANIFOLD]] case"
                  "# and 1 otherwise."
                  "# "
                  "# If you want to influence the distribution of the dimension of the moves when warming up"
                  "# you may do so by specifying a //distribution//. The number of values in //distribution//"
                  "# determines the dimensions used for heating up. The heating and relaxing parameters decrease dynamically"
                  "# unless the //constant// flag is set. The function prohibits to execute the reversed move of a move"
                  "# directly after the move itself unless the //allow_rev_move// flag is set. Setting the"
                  "# //allow_rev_move// flag might help solve a particular resilient problem."
                  "# "
                  "# If you are interested in how the process is coming along, try the //verbose// option."
                  "# It specifies after how many rounds the current best result is displayed."
                  "# "
                  "# The //obj// determines the objective function used for the optimization. If //obj// is set to 0,"
                  "# the function searches for the triangulation with the lexicographically smallest f-vector,"
                  "# if //obj// is set to 1, the function searches for the triangulation with the reversed-lexicographically"
                  "# smallest f-vector and if //obj// is set to 2 the sum of the f-vector entries is used."
                  "# The default is 1."
                  "# @param SimplicialComplex complex1"
                  "# @param SimplicialComplex complex2"
                  "# @option Int rounds"
                  "# @option Bool abs"
                  "# @option Int obj"
                  "# @option Int relax"
                  "# @option Int heat"
                  "# @option Bool constant"
                  "# @option Bool allow_rev_move"
                  "# @option Int min_n_facets"
                  "# @option Int verbose"
                  "# @option Int seed"
                  "# @option Bool quiet"
                  "# @option Array<Int> distribution"
                  "# @return Bool",
                  &pl_homeomorphic,"pl_homeomorphic(SimplicialComplex SimplicialComplex { rounds => undef, abs => 0,  obj => undef,  relax => undef, heat => undef, constant => 0, allow_rev_move=> 0, min_n_facets => undef, verbose => 0, seed => undef, quiet => 0, distribution => undef })");

UserFunction4perl("CREDIT none\n\n"
                  "# @category Producing a new simplicial complex from others"
                  "#  Heuristic for simplifying the triangulation of the given manifold"
                  "#  without changing its PL-type. The function uses"
                  "#  bistellar flips and a simulated annealing strategy."
                  "# "
                  "# You may specify the maximal number of //rounds//, how often the system"
                  "# may //relax// before heating up and how much //heat// should be applied."
                  "# The function stops computing, once the size of the triangulation has not decreased"
                  "# for //rounds// iterations. If the //abs// flag is set, the function stops"
                  "# after //rounds// iterations regardless of when the last improvement took place."
                  "# Additionally, you may set the threshold //min_n_facets// for the number of facets when"
                  "# the simplification ought to stop. Default is d+2 in the [[CLOSED_PSEUDO_MANIFOLD]] case"
                  "# and 1 otherwise."
                  "# "
                  "# If you want to influence the distribution of the dimension of the moves when warming up"
                  "# you may do so by specifying a //distribution//. The number of values in //distribution//"
                  "# determines the dimensions used for heating up. The heating and relaxing parameters decrease dynamically"
                  "# unless the //constant// flag is set. The function prohibits to execute the reversed move of a move"
                  "# directly after the move itself unless the //allow_rev_move// flag is set. Setting the"
                  "# //allow_rev_move// flag might help solve a particular resilient problem."
                  "# "
                  "# If you are interested in how the process is coming along, try the //verbose// option."
                  "# It specifies after how many rounds the current best result is displayed."
                  "# "
                  "# The //obj// determines the objective function used for the optimization. If //obj// is set to 0,"
                  "# the function searches for the triangulation with the lexicographically smallest f-vector,"
                  "# if //obj// is set to any other value the sum of the f-vector entries is used."
                  "# The default is 1."
                  "# @param SimplicialComplex complex"
                  "# @option Int rounds"
                  "# @option Bool abs"
                  "# @option Int obj"
                  "# @option Int relax"
                  "# @option Int heat"
                  "# @option Bool constant"
                  "# @option Bool allow_rev_move"
                  "# @option Int min_n_facets"
                  "# @option Int verbose"
                  "# @option Int seed"
                  "# @option Bool quiet"
                  "# @option Array<Int> distribution"
                  "# @return SimplicialComplex"
                  "# @example The following example applies bistellar simplification to the second barycentric subdivision of the boundary of the 4-simplex to recover the boundary of the 4-simplex itself."
                  "# > $s = iterated_barycentric_subdivision(simplex(4) -> BOUNDARY, 2);"
                  "# > print bistellar_simplification($s) -> F_VECTOR;"
                  "# | 5 10 10 5",
                  &bistellar_simplification,"bistellar_simplification(SimplicialComplex { rounds => undef, abs => 0,  obj => undef,  relax => undef, heat => undef, constant => 0, allow_rev_move=> 0, min_n_facets => undef, verbose => undef, seed => undef, quiet => 0, distribution => undef })");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
