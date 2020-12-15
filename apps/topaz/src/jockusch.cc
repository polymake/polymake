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
#include "polymake/Array.h"
#include "polymake/Map.h"
#include "polymake/topaz/jockusch.h"

namespace polymake { namespace topaz {

namespace {


Set<Int>
antipode(const Set<Int>& facet)
{
   Set<Int> a;
   for (const auto& i: facet)
      a += -i;
   return a;
}

void
add_with_antipode(const Set<Int>& a,
                  std::vector<Set<Int>>& container)
{
   container.push_back(a);
   container.push_back(antipode(a));
}
   
} // end anonymous namespace


std::vector<Set<Int>>      
jockusch_delta_3n(Int n)
{
   std::vector<Set<Int>> phase_1;

   for (Int i=1; i <= n-3; ++i) {
      add_with_antipode(Set<Int> { i, i+1, n-1, n }, phase_1);
      add_with_antipode(Set<Int> {-i,-i-1, n-1, n }, phase_1);
   }

   add_with_antipode(Set<Int> { 1l, -n+2,  n-1,  n }, phase_1);
   add_with_antipode(Set<Int> { 1l, -n+2, -n+1,  n }, phase_1);
   add_with_antipode(Set<Int> { 1l, -n+2, -n+1, -n }, phase_1);

   return phase_1;
}



std::vector<Set<Int>>
jockusch_phase_2(Int n)
{
   std::vector<Set<Int>> phase_2;

   for (Int l=3; l <= n-2; ++l) {
      for (Int i=1; i <= l-2; ++i) {
         add_with_antipode(Set<Int> { i, i+1, l, l+2 }, phase_2);
         add_with_antipode(Set<Int> {-i,-i-1, l, l+2 }, phase_2);
      }
      add_with_antipode(Set<Int> { 1l, -l+1, l, l+2 }, phase_2);
   }

   for (Int l=2; l <= n-3; ++l) {
      add_with_antipode(Set<Int> { l,  l+1, l+2, -l-3 }, phase_2);
      add_with_antipode(Set<Int> {-1l, l,   l+2, -l-3 }, phase_2);
   }

   return phase_2;
}


std::vector<Set<Int>>
jockusch_phase_3(Int n)
{
   std::vector<Set<Int>> phase_3;

   add_with_antipode(Set<Int> { 1,  2, -3,  4 }, phase_3);
   add_with_antipode(Set<Int> { 1,  2,  3, -4 }, phase_3);
   add_with_antipode(Set<Int> { 1, -2,  3, -4 }, phase_3);

   return phase_3;
}   

// ----- Novik & Zheng 4-spheres

std::vector<Set<Int>>
nz_4_delta_4n(Int n)
{
   std::vector<Set<Int>> delta_4n;

   for (Int i=1; i <= n-4; ++i) {
      add_with_antipode(Set<Int> { i, i+1,n-2,n-1,n}, delta_4n);
      add_with_antipode(Set<Int> {-i,-i-1,n-2,n-1,n}, delta_4n);
   }

   add_with_antipode(Set<Int> {1l, -n+3,  n-2, n-1, n}, delta_4n);
   add_with_antipode(Set<Int> {1l, -n+3, -n+2, n-1, n}, delta_4n);
   add_with_antipode(Set<Int> {1l, -n+3, -n+2,-n+1, n}, delta_4n);
   add_with_antipode(Set<Int> {1l, -n+3, -n+2,-n+1,-n}, delta_4n);
 
   return delta_4n;
}

std::vector<Set<Int>>
nz_4_phase_2(Int n)
{
   std::vector<Set<Int>> phase;

   for (Int l=6; l <= n; ++l) {
      for (Int i=1; i <= l-5; ++i) {
         add_with_antipode(Set<Int> { i, i+1,l-3,l-2,l}, phase);
         add_with_antipode(Set<Int> { i, i+1,l-3,l-1,l}, phase);
         add_with_antipode(Set<Int> {-i,-i-1,l-3,l-2,l}, phase);
         add_with_antipode(Set<Int> {-i,-i-1,l-3,l-1,l}, phase);
      }
      add_with_antipode(Set<Int> {1l, -l+4, l-3, l-2,l}, phase);
      add_with_antipode(Set<Int> {1l, -l+4, l-3, l-1,l}, phase);
      add_with_antipode(Set<Int> {1l, -l+4,-l+3, l-2,l}, phase);
      add_with_antipode(Set<Int> {1l, -l+4,-l+2, l-1,l}, phase);
      add_with_antipode(Set<Int> {1l, -l+4,-l+2,-l+1,l}, phase);
      add_with_antipode(Set<Int> {1l, -l+4,-l+3,-l+1,l}, phase);
      add_with_antipode(Set<Int> {-l+4, -l+3,-l+2, l-1,l}, phase);
      add_with_antipode(Set<Int> {-l+4, -l+3,-l+2,-l+1,l}, phase);
   }
   return phase;
}

std::vector<Set<Int>>
nz_4_phase_3(Int n)
{
   std::vector<Set<Int>> phase;
   add_with_antipode(Set<Int> {-1, 2,-3, 4,-5}, phase);
   add_with_antipode(Set<Int> { 1, 2,-3, 4,-5}, phase);
   add_with_antipode(Set<Int> { 1, 2, 3, 4,-5}, phase);
   add_with_antipode(Set<Int> { 1, 2, 3,-4,-5}, phase);
   add_with_antipode(Set<Int> { 1,-2,-3, 4,-5}, phase);
   add_with_antipode(Set<Int> { 1,-2, 3, 4,-5}, phase);
   add_with_antipode(Set<Int> { 1,-2, 3,-4,-5}, phase);
   add_with_antipode(Set<Int> {-1,-2,-3, 4,-5}, phase);
   add_with_antipode(Set<Int> {-1,-2, 3, 4,-5}, phase);
   add_with_antipode(Set<Int> {-1,-2, 3,-4,-5}, phase);

   return phase;
}
   
BigObject
jockusch_3_sphere(const Int n,
                  const OptionSet options)
{
   if (n<4) throw std::runtime_error("need n>=4");

   const std::vector<Set<Int>> phase_1 = jockusch_delta_3n(n);
   const std::vector<Set<Int>> phase_2 = jockusch_phase_2(n);
   const std::vector<Set<Int>> phase_3 = jockusch_phase_3(n);

   Map<Int,Int> index_of;
   Int max_index(0);
   Array<Int> symmetry_gen(2*n);
   for (Int i=1; i <= n; ++i) {
      index_of[ i] = max_index++;
      index_of[-i] = max_index++;
      symmetry_gen[2*i-2] = 2*i-1;
      symmetry_gen[2*i-1] = 2*i-2;
   }

   Array<Set<Int>> facets(phase_1.size() + phase_2.size() + phase_3.size());
   auto fit = entire(facets);
   for (const auto& p : { phase_1, phase_2, phase_3 }) {
      auto pit = p.begin();
      for (; pit != p.end(); ++pit, ++fit) {
         Set<Int> f;
         for (const auto& i: *pit)
            f += index_of[i];
         *fit = f;
      }
   }

   const Int label_style = options["label_style"];
   
   Array<std::string> labels(2*n);
   for (Int i=1; i <= n; ++i) {
      labels[index_of[ i]] = std::to_string(i);
      labels[index_of[-i]] = !label_style
         ? "\\bar{" + std::to_string(i) + "}"
         : std::to_string(i) + "'";
   }
   
   BigObject s("SimplicialComplex");
   s.set_description() << "Jockusch 3-sphere on 2*" << n << " vertices" << endl;
   s.take("FACETS") << facets;
   s.take("VERTEX_LABELS") << labels;

   BigObject G("group::Group");
   BigObject action("group::PermutationAction");
   Array<Array<Int>> gens(1);
   gens[0] = symmetry_gen;
   action.take("GENERATORS") << gens;
   G.take("PERMUTATION_ACTION") << action;
   s.take("GROUP") << G;
   
   return s;
}

BigObject
jockusch_3_ball(const Int n,
                const OptionSet options)
{
   if (n<4) throw std::runtime_error("need n>=4");

   const std::vector<Set<Int>> phase_1 = jockusch_delta_3n(n);

   Map<Int,Int> index_of;
   Int max_index(0);
   Array<Int> symmetry_gen(2*n);
   for (Int i=1; i <= n; ++i) {
      index_of[ i] = max_index++;
      index_of[-i] = max_index++;
      symmetry_gen[2*i-2] = 2*i-1;
      symmetry_gen[2*i-1] = 2*i-2;
   }

   Array<Set<Int>> facets(phase_1.size());
   auto fit = entire(facets);
   auto pit = entire(phase_1);
   for (; !pit.at_end(); ++pit, ++fit) {
      Set<Int> f;
      for (const auto& i: *pit)
         f += index_of[i];
      *fit = f;
   }

   const Int label_style = options["label_style"];
   
   Array<std::string> labels(2*n);
   for (Int i=1; i <= n; ++i) {
      labels[index_of[ i]] = std::to_string(i);
      labels[index_of[-i]] = !label_style
         ? "\\bar{" + std::to_string(i) + "}"
         : std::to_string(i) + "'";
   }
   
   BigObject s("SimplicialComplex");
   s.set_description() << "Jockusch 3-ball on 2*" << n << " vertices" << endl;
   s.take("FACETS") << facets;
   s.take("VERTEX_LABELS") << labels;

   BigObject G("group::Group");
   BigObject action("group::PermutationAction");
   Array<Array<Int>> gens(1);
   gens[0] = symmetry_gen;
   action.take("GENERATORS") << gens;
   G.take("PERMUTATION_ACTION") << action;
   s.take("GROUP") << G;
   
   return s;
}
      
BigObject
nz_4_sphere(const Int n,
            const OptionSet options)
{
   if (n<5) throw std::runtime_error("need n>=5");

   const std::vector<Set<Int>> phase_1 = nz_4_delta_4n(n);
   const std::vector<Set<Int>> phase_2 = nz_4_phase_2(n);
   const std::vector<Set<Int>> phase_3 = nz_4_phase_3(n);

   Map<Int,Int> index_of;
   Int max_index(0);
   Array<Int> symmetry_gen(2*n);
   for (Int i=1; i <= n; ++i) {
      index_of[ i] = max_index++;
      index_of[-i] = max_index++;
      symmetry_gen[2*i-2] = 2*i-1;
      symmetry_gen[2*i-1] = 2*i-2;
   }

   Array<Set<Int>> facets(phase_1.size() + phase_2.size() + phase_3.size());
   auto fit = entire(facets);
   for (const auto& p : { phase_1, phase_2, phase_3 }) {
      auto pit = p.begin();
      for (; pit != p.end(); ++pit, ++fit) {
         Set<Int> f;
         for (const auto& i: *pit)
            f += index_of[i];
         *fit = f;
      }
   }

   const Int label_style = options["label_style"];
   
   Array<std::string> labels(2*n);
   for (Int i=1; i <= n; ++i) {
      labels[index_of[ i]] = std::to_string(i);
      labels[index_of[-i]] = !label_style
         ? "\\bar{" + std::to_string(i) + "}"
         : std::to_string(i) + "'";
   }
   
   BigObject s("SimplicialComplex");
   s.set_description() << "Novik-Zheng 4-sphere on 2*" << n << " vertices" << endl;
   s.take("FACETS") << facets;
   s.take("VERTEX_LABELS") << labels;

   BigObject G("group::Group");
   BigObject action("group::PermutationAction");
   Array<Array<Int>> gens(1);
   gens[0] = symmetry_gen;
   action.take("GENERATORS") << gens;
   G.take("PERMUTATION_ACTION") << action;
   s.take("GROUP") << G;
   
   return s;
}

BigObject
nz_4_ball(const Int n,
          const OptionSet options)
{
   if (n<5) throw std::runtime_error("need n>=5");

   const std::vector<Set<Int>> phase_1 = nz_4_delta_4n(n);

   Map<Int,Int> index_of;
   Int max_index(0);
   Array<Int> symmetry_gen(2*n);
   for (Int i=1; i <= n; ++i) {
      index_of[ i] = max_index++;
      index_of[-i] = max_index++;
      symmetry_gen[2*i-2] = 2*i-1;
      symmetry_gen[2*i-1] = 2*i-2;
   }

   Array<Set<Int>> facets(phase_1.size());
   auto fit = entire(facets);
   auto pit = entire(phase_1);
   for (; !pit.at_end(); ++pit, ++fit) {
      Set<Int> f;
      for (const auto& i: *pit)
         f += index_of[i];
      *fit = f;
   }

   const Int label_style = options["label_style"];
   
   Array<std::string> labels(2*n);
   for (Int i=1; i <= n; ++i) {
      labels[index_of[ i]] = std::to_string(i);
      labels[index_of[-i]] = !label_style
         ? "\\bar{" + std::to_string(i) + "}"
         : std::to_string(i) + "'";
   }
   
   BigObject s("SimplicialComplex");
   s.set_description() << "Novik-Zheng 4-ball on 2*" << n << " vertices" << endl;
   s.take("FACETS") << facets;
   s.take("VERTEX_LABELS") << labels;

   BigObject G("group::Group");
   BigObject action("group::PermutationAction");
   Array<Array<Int>> gens(1);
   gens[0] = symmetry_gen;
   action.take("GENERATORS") << gens;
   G.take("PERMUTATION_ACTION") << action;
   s.take("GROUP") << G;
   
   return s;
}
      

UserFunction4perl("# @category Producing from scratch"
                  "# Create Jockusch's centrally symmetric 3-sphere Delta^3_n on 2n vertices"
                  "# see Lemma 3.1 in arxiv.org/abs/2005.01155"
                  "# @param Int n an integer >= 4"
                  "# @option Int label_style: 0(default) with dashes; 1 with bars"
                  "# @return SimplicialComplex",
                  &jockusch_3_sphere, "jockusch_3_sphere($; { label_style => 0 })");

UserFunction4perl("# @category Producing from scratch"
                  "# Create the ball B^{3,1}_n contained in Jockusch's centrally symmetric 3-sphere Delta^3_n on 2n vertices"
                  "# see Lemma 3.1 in arxiv.org/abs/2005.01155"
                  "# @param Int n an integer >= 4"
                  "# @option Int label_style: 0(default) with dashes; 1 with bars"
                  "# @return SimplicialComplex",
                  &jockusch_3_ball, "jockusch_3_ball($; { label_style => 0 })");
      
UserFunction4perl("# @category Producing from scratch"
                  "# Create Novik & Zheng's centrally symmetric 4-sphere Delta^4_n on 2n vertices"
                  "# see arxiv.org/abs/2005.01155"
                  "# @param Int n an integer >= 5"
                  "# @option Int label_style: 0(default) with dashes; 1 with bars"
                  "# @return SimplicialComplex",
                  &nz_4_sphere, "nz_4_sphere($; { label_style => 0 })");

UserFunction4perl("# @category Producing from scratch"
                  "# Create the ball B^{4,1}_n contained in Novik & Zheng's centrally symmetric 4-sphere Delta^4_n on 2n vertices"
                  "# see arxiv.org/abs/2005.01155"
                  "# @param Int n an integer >= 5"
                  "# @option Int label_style: 0(default) with dashes; 1 with bars"
                  "# @return SimplicialComplex",
                  &nz_4_ball, "nz_4_ball($; { label_style => 0 })");


} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
    
    
