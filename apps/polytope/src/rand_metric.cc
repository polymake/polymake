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
#include "polymake/Matrix.h"
#include "polymake/RandomGenerators.h"

namespace polymake { namespace polytope {

template <typename Scalar>
Matrix<Scalar> rand_metric(int n, perl::OptionSet options)
{
   RandomSeed seed(options["seed"]);
   UniformlyRandom<Scalar> random_source(seed);
   Matrix<Scalar> metric(n,n);
   for (int i=0; i<n;++i)
      for (int j=i+1;j<n;++j)
         metric(i,j)=metric(j,i)=1+random_source.get();

   return metric;
}

Matrix<Integer> rand_metric_int(int n,int digits, perl::OptionSet options)
{

   const RandomSeed seed(options["seed"]);
   UniformlyRandom<Integer> random(seed);

   const Integer d=Integer::pow(10,digits);
   Matrix<Integer> metric(n,n);
   for (int i=0; i<n;++i)
      for (int j=i+1;j<n;++j)
         metric(i,j)=metric(j,i)=d+random.get()%d;

   return metric;
}


UserFunctionTemplate4perl("# @category Producing a polytope from scratch"
                          "# Produce an //n//-point metric with random distances. The values are uniformily"
                          "# distributed in [1,2]."
                          "# @tparam Scalar element type of the result matrix"
                          "# @param Int n"
                          "# @option Int seed controls the outcome of the random number generator;"
                          "#   fixing a seed number guarantees the same outcome. "
                          "# @return Matrix",
                          "rand_metric<Scalar=Rational>($ { seed => undef })");

UserFunctionTemplate4perl("# @category Producing a polytope from scratch"
                          "# Produce an //n//-point metric with random distances. The values are uniformily"
                          "# distributed in [1,2]."
                          "# @tparam Scalar element type of the result matrix"
                          "# @param Int n"
                          "# @option Int seed controls the outcome of the random number generator;"
                          "#   fixing a seed number guarantees the same outcome. "
                          "# @return Matrix",
                          "rand_metric_int($$ { seed => undef })");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
