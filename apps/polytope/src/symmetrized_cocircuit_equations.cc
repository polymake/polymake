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
#include "polymake/group/representations.h"
#include "polymake/group/orbit.h"
#include "polymake/group/sparse_isotypic_components.h"
#include "polymake/polytope/symmetrized_cocircuit_equations_0.h"
#include "polymake/polytope/symmetrized_cocircuit_equations.h"

namespace polymake { namespace polytope {

template<typename Scalar, typename SetType>
ListMatrix<SparseVector<int>> 
symmetrized_cocircuit_equations_0(int d,
                                  const Matrix<Scalar>& V,
                                  const IncidenceMatrix<>& VIF,
                                  const Array<Array<int> >& generators,
                                  const Array<SetType>& interior_ridge_reps,
                                  const Array<SetType>& facet_reps,
                                  perl::OptionSet options) 
{
   return symmetrized_cocircuit_equations_0_impl(d, V, VIF, generators, interior_ridge_reps, facet_reps, options, false);
}

template<typename Scalar, typename SetType>
ListMatrix<SparseVector<int>>
symmetrized_foldable_cocircuit_equations_0(int d,
                                           const Matrix<Scalar>& V,
                                           const IncidenceMatrix<>& VIF,
                                           const Array<Array<int> >& generators,
                                           const Array<SetType>& interior_ridge_reps,
                                           const Array<SetType>& facet_reps,
                                           perl::OptionSet options) 
{
   return symmetrized_foldable_cocircuit_equations_0_impl(d, V, VIF, generators, interior_ridge_reps, facet_reps, options, false);
}

template<typename Scalar, typename SparseSet>
perl::Object
projected_symmetrized_cocircuit_equations_impl(perl::Object c,
                                               const Array<SparseSet>& representative_interior_ridge_simplices,
                                               const Array<SparseSet>& representative_maximal_simplices,
                                               const Set<int>& isotypic_components,
                                               bool reduce_rows)
{
   const bool is_config = c.isa("PointConfiguration");

   const Matrix<Scalar> rays = is_config
      ? c.give("POINTS")
      : c.give("RAYS");

   const Array<Array<int>> original_generators = is_config
      ? c.give("GROUP.POINTS_ACTION.GENERATORS")
      : c.give("GROUP.RAYS_ACTION.GENERATORS");

   const int order = c.give("GROUP.ORDER");
   const Matrix<Rational> character_table = c.give("GROUP.CHARACTER_TABLE");

   const group::ConjugacyClasses<> conjugacy_classes = is_config
      ? c.give("GROUP.POINTS_ACTION.CONJUGACY_CLASSES")
      : c.give("GROUP.RAYS_ACTION.CONJUGACY_CLASSES");

   group::SparseIsotypicBasis<SparseSet> isotypic_basis(0);
   for (int i: isotypic_components) {
      const auto b_i = group::sparse_isotypic_basis_impl(order, original_generators, conjugacy_classes, character_table[i], representative_maximal_simplices);
      isotypic_basis.append(b_i.size(), entire(b_i));
   }

   return projected_symmetrized_cocircuit_equations_impl_impl(rays, representative_interior_ridge_simplices, isotypic_components, character_table, conjugacy_classes, isotypic_basis, reduce_rows);
}

template<typename Scalar, typename SparseSet>
auto
combinatorial_symmetrized_cocircuit_equations(perl::Object c,
                                              const Array<SparseSet>& representative_interior_ridge_simplices,
                                              const Array<SparseSet>& representative_maximal_simplices,
                                              const Set<int>& isotypic_components,
                                              perl::OptionSet options)
{
   const bool is_config = c.isa("PointConfiguration");

   const Matrix<Scalar> rays = is_config
      ? c.give("POINTS")
      : c.give("RAYS");

   const Matrix<Rational> character_table = c.give("GROUP.CHARACTER_TABLE");

   const group::ConjugacyClasses<> conjugacy_classes = is_config
      ? c.give("GROUP.POINTS_ACTION.CONJUGACY_CLASSES")
      : c.give("GROUP.RAYS_ACTION.CONJUGACY_CLASSES");

   const std::string filename = options["filename"];

   return combinatorial_symmetrized_cocircuit_equations_impl(rays, representative_interior_ridge_simplices, isotypic_components, character_table, conjugacy_classes, filename);
}

template<typename Scalar, typename SetType>
Array<Set<int>>
cocircuit_equations_support_reps(const Matrix<Scalar>& points,
                                 const Array<Array<int>>& gens,
                                 const Array<SetType>& representative_interior_ridge_simplices,
                                 const Array<SetType>& representative_maximal_simplices,
                                 perl::OptionSet options)
{
   int n_maximal = 0;
   IndexOfType<SetType> index_of_maximal;
   for (const auto& s: representative_maximal_simplices)
      index_of_maximal[s] = n_maximal++;

   const group::PermlibGroup Gamma(gens);
   Array<Set<int>> supports(representative_interior_ridge_simplices.size());

   const std::string filename = options["filename"];
   std::ofstream outfile;
   if (filename != "" && filename != "-")
      outfile = std::ofstream(filename.c_str(), std::ios_base::trunc);
   std::ostream& os = (filename == "-" ? perl::cout : outfile);

   for (int i=0; i<representative_interior_ridge_simplices.size(); ++i) {
      const group::SparseSimplexVector<SetType> cocircuit_equation = cocircuit_equation_of_ridge_impl(points, representative_interior_ridge_simplices[i]);
      Set<int> support;
      for (const auto m: cocircuit_equation)
         if (!is_zero(m.second))
            support += index_of_maximal.at(Gamma.lex_min_representative(m.first));
      if (filename.size())
         wrap(os) << support << endl;
      else
         supports[i] = support;
   }
   return supports;
}
                                                   

FunctionTemplate4perl("symmetrized_cocircuit_equations_0<Scalar,SetType>($ Matrix<Scalar> IncidenceMatrix Array<Array<Int>> Array<SetType> Array<SetType> { filename=>'', reduce_rows=>0, log_frequency=>0 })");

FunctionTemplate4perl("symmetrized_foldable_cocircuit_equations_0<Scalar,SetType>($ Matrix<Scalar> IncidenceMatrix Array<Array<Int>> Array<SetType> Array<SetType> { filename=>'', reduce_rows=>0, log_frequency=>0 })");

FunctionTemplate4perl("projected_symmetrized_cocircuit_equations_impl<Scalar=Rational,SetType>($, Array<SetType>, Array<SetType>; Set<Int>=scalar2set(0), $=1)");

      
UserFunctionTemplate4perl("# @category Symmetry"
                          "# calculate the projection of the cocircuit equations to a direct sum of isotypic components"
                          "# and represent it combinatorially"
                          "# @param Cone P"
                          "# @param Array<SetType> rirs representatives of interior ridge simplices"
                          "# @param Array<SetType> rmis representatives of maximal interior simplices"
                          "# @param Set<Int> comps the list of indices of the isotypic components to project to; default [0], which"
                          "# amounts to summing all cocircuit equations corresponding to the orbit of each ridge."
                          "# @option String filename where large output should go to. 'filename=>\"-\"' writes to stdout."
                          "# @return Array<Pair<SetType, HashMap<SetType,Rational>>> indexed_cocircuit_equations a list of"
                          "# interior ridge simplices together with the corresponding sparsely represented cocircuit equation",
                          "combinatorial_symmetrized_cocircuit_equations<Scalar=Rational,SetType>($, Array<SetType>, Array<SetType>; Set<Int>=scalar2set(0), { filename=> '' })");

UserFunctionTemplate4perl("# @category Symmetry"
                          "# write the indices of the representatives of the support of the cocircuit equations to a file"
                          "# @param Matrix<Scalar> points"
                          "# @param Array<Array<Int>> gens the generators of the action of the symmetry group"
                          "# @param Array<SetType> rirs representatives of interior ridge simplices"
                          "# @param Array<SetType> rmis representatives of maximal interior simplices"
                          "# @option String filename where large output should go to. 'filename=>\"-\"' writes to stdout."
                          "# @return Int 1",
                          "cocircuit_equations_support_reps<Scalar,SetType>(Matrix<Scalar>, Array<Array<Int>>, Array<SetType>, Array<SetType>, { filename => '' })");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:

 
