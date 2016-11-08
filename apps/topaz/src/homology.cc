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
#include "polymake/topaz/HomologyComplex.h"
#include "polymake/topaz/ChainComplex.h"
#include "polymake/Array.h"

namespace polymake { namespace topaz {

typedef SimplicialComplex_as_FaceMap<int> FaceMap;

namespace{
template<typename Coeff, typename MatrixType, typename ComplexType>
Array<HomologyGroup<Coeff>> compute_homology(const HomologyComplex<Coeff,MatrixType,ComplexType> & HC, bool co, int dim_low, int dim_high)
{
   Array<HomologyGroup<Coeff>> H(HC.size());
   if (!co)
      copy_range(entire(homologies(HC)), H.rbegin());
   else
      copy_range(entire(cohomologies(HC)), H.begin());
   return H;
}}

Array<HomologyGroup<Integer>> homology_sc(const Array< Set<int> >& F, bool co, int dim_low, int dim_high){
   const FaceMap SC(F);
   const HomologyComplex< Integer, SparseMatrix<Integer>, FaceMap > HC(SC,dim_high,dim_low);
   return compute_homology<Integer,SparseMatrix<Integer>,FaceMap>(HC,co,dim_low,dim_high);
}

template<typename Coeff, typename MatrixType>
Array<HomologyGroup<Coeff>> homology(const ChainComplex<MatrixType> & CC, bool co, int dim_low, int dim_high){
   HomologyComplex< Coeff, MatrixType, ChainComplex<MatrixType> > HC(CC, dim_high, dim_low);
   return compute_homology<Coeff,MatrixType,ChainComplex<MatrixType>>(HC,co,dim_low,dim_high);
}


namespace {
template <typename Complex, typename HomologyOutputIterator, typename CycleOutputIterator>
void store_homologies_and_cycles_sc(const Complex& CC, const FaceMap& SC,
                                 HomologyOutputIterator hom_it, CycleOutputIterator cycle_it)
{
   for (typename Complex::iterator chain_it=CC.begin();  !chain_it.at_end();  ++chain_it, ++hom_it, ++cycle_it) {
      *hom_it = *chain_it;

      const int n=chain_it.cycle_coeffs().cols() - empty_cols(chain_it.cycle_coeffs());
      cycle_it->coeffs.resize(chain_it.cycle_coeffs().rows(), n);
      //Cols< chain_complex::cycle_type::coeff_matrix >::iterator coeff_it=cols(cycle_it->coeffs).begin();
      auto coeff_it=cols(cycle_it->coeffs).begin();
      cycle_it->faces.resize(n);
      //chain_complex::cycle_type::face_list::iterator cycle_face_it=cycle_it->faces.begin();
      auto cycle_face_it=cycle_it->faces.begin();

      for (typename Entire< typename FaceMap::Faces_of_Dim >::iterator
              face_it=entire(SC.faces_of_dim(chain_it.dim()));  !face_it.at_end();  ++face_it)
         if (! chain_it.cycle_coeffs().col(face_it.data()).empty()) {
            *cycle_face_it = *face_it;  ++cycle_face_it;
            *coeff_it = chain_it.cycle_coeffs().col(face_it.data());  ++coeff_it;
         }
   }
}

template <typename Complex, typename HomologyOutputIterator, typename CycleOutputIterator>
void store_homologies_and_cycles(const Complex& CC, HomologyOutputIterator hom_it, CycleOutputIterator cycle_it)
{
   for (typename Complex::iterator chain_it=CC.begin();  !chain_it.at_end();  ++chain_it, ++hom_it, ++cycle_it) {
      *hom_it = *chain_it;
      *cycle_it=chain_it.cycle_coeffs();
   }

}
}

// list return value:
// (homology groups, cycle groups)
perl::ListReturn homology_and_cycles_sc(const Array< Set<int> >& F, bool co, int dim_low, int dim_high)
{
   const FaceMap SC(F);
   const HomologyComplex< Integer, SparseMatrix<Integer>, FaceMap > HC(SC,dim_high,dim_low);
   Array<HomologyGroup<Integer>> H(HC.size());
   Array<CycleGroup<Integer>> CYC(HC.size());

   if (!co)
      store_homologies_and_cycles_sc(homologies_and_cycles(HC), SC, H.rbegin(), CYC.rbegin());
   else
      store_homologies_and_cycles_sc(cohomologies_and_cocycles(HC), SC, H.begin(), CYC.begin());

   perl::ListReturn results;
   results << H << CYC;
   return results;
}

// list return value:
// (homology groups, coefficient matrices)
// general chain complex version
template<typename Coeff, typename MatrixType>
perl::ListReturn homology_and_cycles(const ChainComplex<MatrixType> & CC, bool co, int dim_low, int dim_high)
{
   const HomologyComplex< Coeff, SparseMatrix<Coeff>, ChainComplex<MatrixType> > HC(CC,dim_high,dim_low);
   Array<HomologyGroup<Coeff>> H(HC.size());
   Array<MatrixType> CYC(HC.size());

   if (!co)
      store_homologies_and_cycles(homologies_and_cycles(HC), H.rbegin(), CYC.rbegin());
   else
      store_homologies_and_cycles(cohomologies_and_cocycles(HC), H.begin(), CYC.begin());

   perl::ListReturn results;
   results << H << CYC;
   return results;
}

template<typename R>
SparseMatrix<R> bd_matrix(Array<Set<int>> F, int d){
   const FaceMap SCFM(F);
   return SCFM.boundary_matrix<R>(d);
}

UserFunction4perl("# @category Topology\n"
                  "# Calculate the __(co-)homology groups__ of a simplicial complex.\n"
                  "# @param Array<Set<int>> complex"
                  "# @param Bool co set to true for __co__homology"
                  "# @option Int dim_low narrows the dimension range of interest, with negative values being treated as co-dimensions"
                  "# @option Int dim_high see //dim_low//",
                  &homology_sc, "homology(Array<Set> $; $=0, $=-1)");

UserFunction4perl("# @category Topology\n"
                  "# Calculate the __(co-)homology groups__ and __cycle representatives__ of a simplicial complex.\n"
                  "# @param Array<Set<int>> complex"
                  "# @param Bool co set to true for __co__homology"
                  "# @option Int dim_low narrows the dimension range of interest, with negative values being treated as co-dimensions"
                  "# @option Int dim_high see //dim_low//",
                  &homology_and_cycles_sc, "homology_and_cycles(Array<Set> $; $=0, $=-1)");

UserFunctionTemplate4perl("# @category Topology\n"
                  "# Returns the __d-boundary matrix__ of a simplicial complex.\n"
                  "# @param Array<Set<Int>> complex"
                  "# @param Int d dimension of the matrix"
				  "# @tparam R desired coefficient type",
                  "bd_matrix<R>(Array<Set<Int>> Int)");

UserFunctionTemplate4perl("# @category Topology"
"# Calculate the __(co-)homology groups__ of a chain complex.\n"
"# @param ChainComplex CC The chain complex for which to compute homology."
"# @param Bool co set to true for __co__homology"
"# @option Int dim_low narrows the dimension range of interest, with negative values being treated as co-dimensions"
"# @option Int dim_high see //dim_low//"
"# @tparam Coeff The coefficient type for homology computation."
"# @tparam MatrixType The Matrix type for homology computation."
"# @return Array<HomologyGroup<E>>",
                  "homology<Coeff,MatrixType>(ChainComplex<MatrixType> $; $=0, $=-1)");

UserFunctionTemplate4perl("# @category Topology"
                  "# Calculate the __(co-)homology groups__ and __cycle coefficient matrices_ of a chain complex.\n"
"# @param ChainComplex CC The chain complex for which to compute homology."
"# @param Bool co set to true for __co__homology"
"# @option Int dim_low narrows the dimension range of interest, with negative values being treated as co-dimensions"
"# @option Int dim_high see //dim_low//"
"# @tparam Coeff The coefficient type for homology computation."
"# @tparam MatrixType The Matrix type for homology computation."
,
                  "homology_and_cycles<Coeff,MatrixType>(ChainComplex<MatrixType> $; $=0, $=-1)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
