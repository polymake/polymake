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

//////homology computation

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

//FIXME #975 other coefficient and matrix types would be nice
/*template<typename Coeff, typename MatrixType>
Array<HomologyGroup<Coeff>> homology(const ChainComplex<MatrixType> & CC, bool co, int dim_low, int dim_high){
   HomologyComplex< Coeff, MatrixType, ChainComplex<MatrixType> > HC(CC, dim_high, dim_low);
   return compute_homology<Coeff,MatrixType,ChainComplex<MatrixType>>(HC,co,dim_low,dim_high);
}*/

template<typename Complex>
Array<HomologyGroup<Integer>> homology(const Complex & CC, bool co, int dim_low, int dim_high){
   HomologyComplex< Integer, SparseMatrix<Integer>, Complex > HC(CC, dim_high, dim_low);
   return compute_homology<Integer,SparseMatrix<Integer>,Complex>(HC,co,dim_low,dim_high);
}

Array<HomologyGroup<Integer>> homology_sc(const Array< Set<int> >& F, bool co, int dim_low, int dim_high){
   const FaceMap SC(F);
   return homology<FaceMap>(SC,co,dim_low,dim_high);
}

//////homology with cycle groups

namespace {
//simplicial complex version
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

//general chain complex version
template <typename Complex, typename HomologyOutputIterator>
void store_homologies_and_cycles(const Complex& CC, HomologyOutputIterator hom_it)
{
   for (typename Complex::iterator chain_it=CC.begin();  !chain_it.at_end();  ++chain_it, ++hom_it){
      *hom_it = std::make_pair( *chain_it, chain_it.cycle_coeffs() );
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

// general chain complex version
Array<std::pair<HomologyGroup<Integer>, SparseMatrix<Integer>>>
homology_and_cycles(const ChainComplex<SparseMatrix<Integer>> & CC, bool co, int dim_low, int dim_high)
{
   const HomologyComplex< Integer, SparseMatrix<Integer>, ChainComplex<SparseMatrix<Integer>> > HC(CC,dim_high,dim_low);
   Array<std::pair<HomologyGroup<Integer>,SparseMatrix<Integer>>> HCYC(HC.size());

   if (!co)
      store_homologies_and_cycles(homologies_and_cycles(HC), HCYC.rbegin());
   else
      store_homologies_and_cycles(cohomologies_and_cocycles(HC), HCYC.begin());

   return HCYC;
}


//////

template<typename Coeff, typename Complex>
typename std::enable_if<pm::is_field<Coeff>::value && !std::is_same<Complex, perl::Value>::value, Array<int>>::type
betti_numbers(const Complex& C){
   int dim = C.dim();
   Array<int> betti(dim+1);
   int r_next,r = 0;
   for(int d = dim; d>=0; --d){
      auto delta = C.template boundary_matrix<Coeff>(d);
      r_next = rank(delta);
      betti[d] = delta.rows() - r_next - r;
      r = r_next;
   }
   return betti;
}

template<typename Coeff>
typename std::enable_if<pm::is_field<Coeff>::value, Array<int>>::type
betti_numbers(perl::Object SC){
   Array<Set<int>> F = SC.give("FACETS");
   const FaceMap FM(F);
   return betti_numbers<Coeff, FaceMap>(FM);
}



UserFunction4perl("# @category Topology\n"
                  "# Calculate the reduced __(co-)homology groups__ of a simplicial complex.\n"
                  "# @param Array<Set<int>> complex"
                  "# @param Bool co set to true for __co__homology"
                  "# @option Int dim_low narrows the dimension range of interest, with negative values being treated as co-dimensions"
                  "# @option Int dim_high see //dim_low//",
                  &homology_sc, "homology(Array<Set> $; $=0, $=-1)");

UserFunction4perl("# @category Topology\n"
                  "# Calculate the reduced __(co-)homology groups__ and __cycle representatives__ of a simplicial complex.\n"
                  "# @param Array<Set<int>> complex"
                  "# @param Bool co set to true for __co__homology"
                  "# @option Int dim_low narrows the dimension range of interest, with negative values being treated as co-dimensions"
                  "# @option Int dim_high see //dim_low//",
                  &homology_and_cycles_sc, "homology_and_cycles(Array<Set> $; $=0, $=-1)");

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
"#  > print homology($cc,0);"
"# | ({(2 1)} 1)"
"# | ({} 0)"
"# The output means that the zeroth homology group has 2-torsion with multiplicity one, and betti number one."
"# The first homology group is empty.",
                  &homology<ChainComplex<SparseMatrix<Integer>>>, "homology(ChainComplex<SparseMatrix<Integer>> $; $=0, $=-1)");

UserFunction4perl("# @category Topology"
                  "# Calculate the __(co-)homology groups__ and __cycle coefficient matrices_ of a chain complex.\n"
"# @param ChainComplex<SparseMatrix<Integer>> CC The chain complex for which to compute homology."
"# @param Bool co set to true for __co__homology"
"# @option Int dim_low narrows the dimension range of interest, with negative values being treated as co-dimensions"
"# @option Int dim_high see //dim_low//"
"# @return Array<Pair<HomologyGroup, SparseMatrix>> For each dimension, contains the homology group and corresponding"
"#  cycle group coefficient matrix where each row of the matrix represents a generator, column indices referring to indices"
"#  of the chain group elements involved."
"# @example To construct a small chain complex with only one non-zero differential:"
"# > $cc = new ChainComplex(new Array<SparseMatrix<Integer>>([[[2,0]]]));"
"# This prints its homology groups and corresponding generators."
"# > print homology_and_cycles($cc,0);"
"# | (({(2 1)} 1)"
"# | <1 0"
"# | 0 1"
"# | >"
"# | )"
"# | (({} 0)"
"# | <>"
"# | )"
"# The output means that the zeroth homology group has 2-torsion with multiplicity one generated by the first elemen"
"# of the chain group, and free part of rank one generated by the second element. The first homology group is empty."
,&homology_and_cycles, "homology_and_cycles(ChainComplex<SparseMatrix<Integer>> $; $=0, $=-1)");

UserFunctionTemplate4perl("# @category Topology\n"
                  "# Calculate the betti numbers of a general chain complex over a field.\n"
                  "# @param ChainComplex C"
                  "# @tparam Coeff The coefficient field type for homology computation. Defaults to Rational"
                  "# @return Array<Int> containing the i-th  betti number at entry i"
                  "# @example The following constructs a simple chain complex with only one non-empty differential:"
                  "# > $cc = new ChainComplex(new Array<SparseMatrix<Integer>>([[[2,0]]]));"
                  "# You can print its betti numbers like this:"
                  "# > print betti_numbers($cc);"
                  "# | 1 0",
                  "betti_numbers<Coeff = Rational>(ChainComplex)");

UserFunctionTemplate4perl("# @category Topology\n"
                  "# Calculate the reduced betti numbers of a simplicial complex over a field.\n"
                  "# @param SimplicialComplex S"
                  "# @tparam Coeff The coefficient field type for homology computation. Defaults to Rational"
                  "# @return Array<Int> containing the i-th  betti number at entry i"
                  "# @example To print the betti numbers for the torus, do this:"
                  "# > $t = torus();"
                  "# > print betti_numbers($t);"
                  "# | 0 2 1",
                  "betti_numbers<Coeff = Rational>(SimplicialComplex)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
