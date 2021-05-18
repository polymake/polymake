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
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/Set.h"
#include "polymake/Map.h"
#include "polymake/PowerSet.h"


namespace polymake { namespace polytope {

BigObject generalized_permutahedron(Int d, Map<Set<Int>,Rational> height)
{
   BigObject p("Polytope<Rational>");
   p.set_description() << "generalized permutahedron of dimension " << d << endl;

   // generate inequalities of the generalized permutahedron
   Matrix<Rational> ineqs(height.size(), d+1);
   Rows<Matrix<Rational>>::iterator r=rows(ineqs).begin();
   for (const auto& h : height) { 
      (*r)[0] = -h.second;
      for (auto i = entire(h.first); !i.at_end(); ++i){
         (*r)[*i] = 1;
      }
      r++;
   }
   
   // generate equality of the generalized permutahedron (if existent)
   if (height.exists(range(1,d))){
      Matrix<Rational> eq(1, d+1);
      eq[0][0] = -height[range(1,d)];
      for (int i = 1; i < d+1; ++i){
         eq[0][i] = 1;
      }
      p.take("EQUATIONS") << eq;
   }

   p.take("CONE_AMBIENT_DIM") << d+1;
   p.take("INEQUALITIES") << ineqs;
   return p;
}

BigObject SIM_body(Vector<Rational> alpha_list)
{
   const Int n = alpha_list.size();
   if (n < 2) {
      throw std::runtime_error("SIM-body: dimension must be at least 2");
   }
   Rational tmp = 0;
   for(Rational alpha : alpha_list) {
      if (alpha < tmp) {
         throw std::runtime_error("SIM-body: input is not ascending");
      }
      tmp = alpha;
   }
   Map<Set<Int>,Rational> height;
   auto curr_set = entire(all_subsets(range(1,n+1)));
   curr_set++;  // first entry is ignored, it is the empty set
   while (!curr_set.at_end()) {
      Rational sum = 0;
      // check whether n+1 is in the current set
      for (auto el = entire(*curr_set); !el.at_end(); el++){
         if (*el == n+1) {
            for(Int i = 0; i < (*curr_set).size()-1; i++) {
               sum += alpha_list[i];
	    }
         }
      }
      height[*curr_set] = sum;
      curr_set++;
   }

   return generalized_permutahedron(n+1, height);
}



UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Produce a generalized permutahedron via z<sub>I</sub> height function as in Postnikov: Permutohedra, associahedra, and beyond, IMRN (2009)."
                  
                  "# @param Int d The dimension"
                  "# @param Map<Set<Int>,Rational> height Values of the height functions for the different 0/1-directions, i.e. for h = height({1,2,4}) we have the inequality x1 + x2 + x4 >= h. The height value for the set containing all coordinates from 1 to d is interpreted as equality. If any value is missing, it will be skipped. Also it is not checked, if the values are consistent for a height function."
                  "# @return Polytope"
                  "# @example To create a generalized permutahedron in 3-space use"
                  "# > $m = new Map<Set,Rational>;"
                  "# > $m->{new Set(1)} = 0;"
                  "# > $m->{new Set(2)} = 0;"
                  "# > $m->{new Set(3)} = 0;"
                  "# > $m->{new Set(1,2)} = 1/4;"
                  "# > $m->{new Set(1,3)} = 1/4;"
                  "# > $m->{new Set(2,3)} = 1/4;"
                  "# > $m->{new Set(1,2,3)} = 1;"
                  "# > $p = generalized_permutahedron(3,$m);",
                  &generalized_permutahedron, "generalized_permutahedron($,$)");

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Produce a //n//-dimensional SIM-body as generalized permutahedron in //(n+1)//-space. SIM-bodies are defined in the article \"Duality and Optimality of Auctions for Uniform Distributions\" by Yiannis Giannakopoulos and Elias Koutsoupias."

                  "# @param Vector<Rational> alpha_list Vector with the parameters (a<sub>1</sub>,...,a<sub>n</sub>) s.t. a<sub>1</sub> <= ... <= a<sub>n</sub>."
                  "# @return Polytope"
		  "# @example To produce the //n//-dimensional SIM-body, use for example"
		  "# > $p = SIM_body(sequence(1,3));"
		  "# > $s = new Polytope(POINTS=>$p->VERTICES->minor(All,~[$p->CONE_DIM]));",
                  &SIM_body, "SIM_body($)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:

