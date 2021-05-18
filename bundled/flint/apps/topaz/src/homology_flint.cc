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
#include "polymake/topaz/SimplicialComplex_as_FaceMap.h"
#include "polymake/topaz/HomologyComplex.h"
#include "polymake/topaz/HomologyComplexFlint.h"
#include "polymake/topaz/ChainComplex.h"
#include "polymake/Array.h"

namespace polymake { namespace topaz {

typedef SimplicialComplex_as_FaceMap<Int> FaceMap;

//////homology computation

namespace{
template<typename Coeff, typename MatrixType, typename ComplexType>
Array<HomologyGroup<Coeff>> compute_homology_flint(const HomologyComplexFlint<Coeff,MatrixType,ComplexType> & HC, bool co, Int dim_low, Int dim_high)
{
   Array<HomologyGroup<Coeff>> H(HC.size());
   if (!co)
      copy_range(entire(homologies(HC)), H.rbegin());
   else
      copy_range(entire(cohomologies(HC)), H.begin());
   return H;
}}

template<typename Complex>
Array<HomologyGroup<Integer>> homology_flint(const Complex & CC, bool co, Int dim_low, Int dim_high)
{
   HomologyComplexFlint< Integer, SparseMatrix<Integer>, Complex > HC(CC, dim_high, dim_low);
   return compute_homology_flint<Integer,SparseMatrix<Integer>,Complex>(HC,co,dim_low,dim_high);
}

Array<HomologyGroup<Integer>> homology_sc_flint(const Array<Set<Int>>& F, bool co, Int dim_low, Int dim_high)
{
   const FaceMap SC(F);
   return homology_flint<FaceMap>(SC,co,dim_low,dim_high);
}

//////homology with cycle groups


UserFunction4perl("# @category Topology\n"
                  "# Calculate the reduced __(co-)homology groups__ of a simplicial complex.\n"
                  "# @param Array<Set<Int>> complex"
                  "# @param Bool co set to true for __co__homology"
                  "# @option Int dim_low narrows the dimension range of interest, with negative values being treated as co-dimensions"
                  "# @option Int dim_high see //dim_low//",
                  &homology_sc_flint, "homology_flint(Array<Set> $; $=0, $=-1)");


UserFunction4perl("# @category Topology"
                    "# Calculate the __(co-)homology groups__ of a chain complex.\n"
                    "# @param ChainComplex CC The chain complex for which to compute homology."
                    "# @param Bool co set to true for __co__homology"
                    "# @option Int dim_low narrows the dimension range of interest, with negative values being treated as co-dimensions"
                    "# @option Int dim_high see //dim_low//"
                    "# @return Array<HomologyGroup<Integer>>"
                    "# @example To construct a small chain complex with only one non-zero differential:"
                    "# > $cc = new ChainComplex(new Array<SparseMatrix<Integer>>([[[2,0]]]));"
                    "# This prints its homology groups."
                    "#  > print homology_flint($cc,0);"
                    "# | ({(2 1)} 1)"
                    "# | ({} 0)"
                    "# The output means that the zeroth homology group has 2-torsion with multiplicity one, and betti number one."
                    "# The first homology group is empty.",
                  &homology_flint<ChainComplex<SparseMatrix<Integer>>>, "homology_flint(ChainComplex<SparseMatrix<Integer>> $; $=0, $=-1)");
} }




// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
