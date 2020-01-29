/* Copyright (c) 1997-2020
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
#include "polymake/Matrix.h"
#include "polymake/Set.h"
#include "polymake/PowerSet.h"
#include "polymake/Map.h"
#include "polymake/Rational.h"
#include "polymake/Vector.h"
#include "polymake/linalg.h"
#include "polymake/RandomGenerators.h"
#include "polymake/polytope/gale_tools.h"

namespace polymake { namespace polytope {

namespace {

Matrix<Rational> rand_cyclic_gale(Int d, Int n, const RandomSeed& seed)
{
   /*
     This function produces a random instance of a Gale diagram of a
     cyclic polytope.  

     More precisely, it generates an ordered set G of n random vectors
     in R^{n-d-1} whose cocircuits are alternating.  

     This means that for every linear hyperplane through a set H of
     n-d-2 vectors, the signs of G setminus H w.r.t. to a normal
     vector n_H of H alternate when enumerated in the order implicit
     in G.

     To achieve this, we consistently choose the direction of n_H so
     that the first element of G setminus H has positive inner product
     with n_H. To find the feasible cone for the i-th new vector, we
     use (-1)^i times the stored, fixed normal vectors.
    */

   UniformlyRandom<Rational> random(seed);
   const Int gdim = n-d-1;

   // first, trivial cases
   assert(gdim>0);
   if (gdim==1) {
      Matrix<Rational> G(n,gdim);
      for (Int i = 0; i < n; ++i)
         G(i,0) = (i%2) ? -random.get() : random.get();
      return G;
   }
   // now gdim >= 2

   Matrix<Rational> 
      normal_vectors(Int(Integer::binom(n, gdim-1)), gdim),
      G(n, gdim); // the rows will be the Gale vectors
   auto
      nrit = entire(rows(normal_vectors)),
      grit = entire(rows(G));
   Int i = 0; // how many vertices of G have been found already
   Int rws = 0; // number of non-zero rows of normal_vectors matrix

   // initialize the first entries of the Gale diagram with unit vectors; 
   // this is ok because we may transform the Gale diagram by a linear transform
   // moreover, the initial normal vectors point inward in the cone
   for (; i<gdim; ++i, ++rws, ++nrit, ++grit)
      *nrit = *grit = unit_vector<Rational>(gdim, i); 

   bool parity_flag (true);

   while (i<n) { // add a new vector to the Gale diagram
      // find the generators of the cone of allowed positions for the new vector
      BigObject c("Cone<Rational>");
      c.take("INEQUALITIES") << (parity_flag ? Matrix<Rational>(-normal_vectors.minor(sequence(0,rws),All))
                                                               : normal_vectors.minor(sequence(0,rws),All));
      const Matrix<Rational> gens = c.give("RAYS");
    /*  cout<<"without zeros: "<<gens<<endl;

      BigObject c2("Cone<Rational>");
      c2.take("INEQUALITIES") << (parity_flag ? Matrix<Rational>(-normal_vectors)
                                                               : normal_vectors);
      const Matrix<Rational> gens2 = c2.give("RAYS");
      cout<<"with zeros: "<<gens2<<endl;*/


      // pick a random vector inside this allowed cone
      {
         Vector<Rational> v(gdim);
         for (auto rit = entire(rows(gens)); !rit.at_end(); ++rit)
            v += random.get() * (*rit);
         *grit = v;
      }
      
      // add the normal vectors of the new hyperplanes generated using this vector
      if (i < n-1) { // but don't waste time on the last point
         for (auto r = entire(all_subsets_of_k(sequence(0,i), gdim-2)); !r.at_end(); ++r) {
            Set<Int> ridge(*r);
            ridge += i;
            const Vector<Rational> v = null_space(G.minor(ridge, All))[0];
            const Int j = (sequence(0,i) - ridge).front(); 
            // the inner product with first vector not in ridge is chosen positive
            *nrit = (v*G.row(j) < 0) ? Vector<Rational>(-v) : v;  
            ++nrit;
            ++rws;
         }
      }
      ++i;
      ++grit;
      parity_flag = !parity_flag;
   }
   return G;
}

} // end anonymous namespace

BigObject rand_cyclic(Int d, Int n, OptionSet options)
{
   if (d<2 || n<d+2) throw std::runtime_error("rand_cyclic: need d >= 2 and n >= d+2");

   const RandomSeed seed(options["seed"]);

   // Calculate a random Gale transform, 
   // and balance it, so that (1,...,1) is in its kernel
   const Matrix<Rational> G = balance(rand_cyclic_gale(d, n, seed));

   // Transform it to primal space
   Matrix<Rational> V = T(null_space(T(G)));
   assert(V.cols() == d+1);
   assert(V.rows() == n);

   // usually V will have some vertices at infinity (ie, zeros in the
   // first column), so we put ones into the first column.  This does
   // not change the combinatorics by the balancedness of G,
   V.col(0).fill(1);

   // done.
   BigObject p("Polytope<Rational>");
   p.set_description() << "Random instance of the cyclic polytope C(" 
                       << d << "," 
                       << n << "). Produced by rand_cyclic for seed=" << seed.get() << endl;
   p.take("CONE_AMBIENT_DIM") << d+1;
   p.take("VERTICES") << V;
   p.take("GALE_TRANSFORM") << G;
   return p;
}

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Computes a random instance of a cyclic polytope of dimension //d// on //n// vertices"
                  "# by randomly generating a Gale diagram whose cocircuits have alternating signs."
                  "# @param Int d the dimension"
                  "# @param Int n the number of vertices"
                  "# @option Int seed controls the outcome of the random number generator;"
                  "#   fixing a seed number guarantees the same outcome."
                  "# @return Polytope",
                  &rand_cyclic,"rand_cyclic($$ { seed => undef })");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
