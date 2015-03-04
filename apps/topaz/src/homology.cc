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
#include "polymake/topaz/SimplicialComplex_as_FaceMap.h"
#include "polymake/topaz/ChainComplex.h"
#include "polymake/Array.h"

namespace polymake { namespace topaz {

typedef Integer CoefficientType;
typedef ChainComplex< CoefficientType, SimplicialComplex_as_FaceMap<int> > chain_complex;
typedef Array<chain_complex::homology_type> homology_group_list;
typedef Array<chain_complex::cycle_type> cycle_group_list;

homology_group_list homology(const Array< Set<int> >& F, bool co, int dim_low, int dim_high)
{
   const SimplicialComplex_as_FaceMap<int> SC(F);
   const chain_complex CC(SC,dim_high,dim_low);
   homology_group_list H(CC.size());
   if (!co)
      copy(entire(homologies(CC)), H.rbegin());
   else
      copy(entire(cohomologies(CC)), H.begin());
   return H;
}

namespace {
template <typename Complex, typename HomologyOutputIterator, typename CycleOutputIterator>
void store_homologies_and_cycles(const Complex& CC, const SimplicialComplex_as_FaceMap<int>& SC,
                                 HomologyOutputIterator hom_it, CycleOutputIterator cycle_it)
{
   for (typename Complex::iterator chain_it=CC.begin();  !chain_it.at_end();  ++chain_it, ++hom_it, ++cycle_it) {
      *hom_it = *chain_it;

      const int n=chain_it.cycle_coeffs().cols() - empty_cols(chain_it.cycle_coeffs());
      cycle_it->coeffs.resize(chain_it.cycle_coeffs().rows(), n);
      Cols< chain_complex::cycle_type::coeff_matrix >::iterator coeff_it=cols(cycle_it->coeffs).begin();
      cycle_it->faces.resize(n);
      chain_complex::cycle_type::face_list::iterator cycle_face_it=cycle_it->faces.begin();

      for (typename Entire< typename SimplicialComplex_as_FaceMap<int>::Faces_of_Dim >::iterator
              face_it=entire(SC.faces_of_dim(chain_it.dim()));  !face_it.at_end();  ++face_it)
         if (! chain_it.cycle_coeffs().col(face_it.data()).empty()) {
            *cycle_face_it = *face_it;  ++cycle_face_it;
            *coeff_it = chain_it.cycle_coeffs().col(face_it.data());  ++coeff_it;
         }
   }
}
}

// list return value:
// (homology groups, cycle groups)
perl::ListReturn homology_and_cycles(const Array< Set<int> >& F, bool co, int dim_low, int dim_high)
{
   const SimplicialComplex_as_FaceMap<int> SC(F);
   const chain_complex CC(SC,dim_high,dim_low);
   homology_group_list H(CC.size());
   cycle_group_list CYC(CC.size());

   if (!co)
      store_homologies_and_cycles(homologies_and_cycles(CC), SC, H.rbegin(), CYC.rbegin());
   else
      store_homologies_and_cycles(cohomologies_and_cocycles(CC), SC, H.begin(), CYC.begin());

   perl::ListReturn results;
   results << H << CYC;
   return results;
}

UserFunction4perl("# @category Topology\n"
                  "# Calculate the __(co-)homology groups__ of a simplicial complex.\n"
                  "# @param Array<Set<int>> complex"
                  "# @param Bool co set to true for __co__homology"
                  "# @option Int dim_low narrows the dimension range of interest, with negative values being treated as co-dimensions"
                  "# @option Int dim_high see //dim_low//",
                  &homology, "homology($$; $=0, $=-1)");

UserFunction4perl("# @category Topology\n"
                  "# Calculate the __(co-)homology groups__ and __cycle representatives__ of a simplicial complex.\n"
                  "# @param Array<Set<int>> complex"
                  "# @param Bool co set to true for __co__homology"
                  "# @option Int dim_low narrows the dimension range of interest, with negative values being treated as co-dimensions"
                  "# @option Int dim_high see //dim_low//",
                  &homology_and_cycles, "homology_and_cycles(Array<Set> $; $=0, $=-1)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
