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
#include "polymake/group/representations.h"
#include "polymake/group/orbit.h"
#include "polymake/group/sparse_isotypic_components.h"
#include "polymake/polytope/symmetrized_cocircuit_equations_0.h"
#include "polymake/polytope/symmetrized_cocircuit_equations.h"

namespace polymake { namespace polytope {

template<typename Scalar, typename SetType>
ListMatrix<SparseVector<int> > 
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
ListMatrix<SparseVector<int> >
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

FunctionTemplate4perl("symmetrized_cocircuit_equations_0<Scalar,SetType>($ Matrix<Scalar> IncidenceMatrix Array<Array<Int>> Array<SetType> Array<SetType> { filename=>'', reduce_rows=>0, log_frequency=>0 })");

FunctionTemplate4perl("symmetrized_foldable_cocircuit_equations_0<Scalar,SetType>($ Matrix<Scalar> IncidenceMatrix Array<Array<Int>> Array<SetType> Array<SetType> { filename=>'', reduce_rows=>0, log_frequency=>0 })");

template<typename Scalar, typename SparseSet>
perl::Object
projected_symmetrized_cocircuit_equations_impl(perl::Object c,
                                               const Array<SparseSet>& representative_interior_ridge_simplices,
                                               const Array<SparseSet>& representative_maximal_simplices,
                                               const Set<int>& isotypic_components,
                                               bool reduce_rows)
{
   const Matrix<Scalar> rays = c.give("RAYS");
   const Array<Array<int>> original_generators = c.give("GROUP.RAYS_ACTION.GENERATORS");
   const int order = c.give("GROUP.ORDER");
   const Matrix<Rational> character_table = c.give("GROUP.CHARACTER_TABLE");
   const group::ConjugacyClasses conjugacy_classes = c.give("GROUP.RAYS_ACTION.CONJUGACY_CLASSES");

   group::SparseIsotypicBasis<SparseSet> isotypic_basis(0);
   for (int i : isotypic_components) {
      const auto b_i = group::sparse_isotypic_basis_impl(order, original_generators, conjugacy_classes, character_table[i], representative_maximal_simplices);
      isotypic_basis.append(b_i.size(), entire(b_i));
   }

   return projected_symmetrized_cocircuit_equations_impl_impl(rays, representative_interior_ridge_simplices, isotypic_components, character_table, conjugacy_classes, isotypic_basis, reduce_rows);
}

FunctionTemplate4perl("projected_symmetrized_cocircuit_equations_impl<Scalar,SetType>(Cone<Scalar>, Array<SetType>, Array<SetType>; Set<Int>=scalar2set(0), $=1)");

template<typename Scalar, typename SparseSet>
auto
combinatorial_symmetrized_cocircuit_equations(perl::Object c,
                                              const Array<SparseSet>& representative_interior_ridge_simplices,
                                              const Array<SparseSet>& representative_maximal_simplices,
                                              const Set<int>& isotypic_components,
                                              perl::OptionSet options)
{
   const Matrix<Scalar> rays = c.give("RAYS");
   const Matrix<Rational> character_table = c.give("GROUP.CHARACTER_TABLE");
   const group::ConjugacyClasses conjugacy_classes = c.give("GROUP.RAYS_ACTION.CONJUGACY_CLASSES");
   const std::string filename = options["filename"];

   return combinatorial_symmetrized_cocircuit_equations_impl(rays, representative_interior_ridge_simplices, isotypic_components, character_table, conjugacy_classes, filename);
}

UserFunctionTemplate4perl("# @category Symmetry"
                          "# calculate the projection of the cocircuit equations to a direct sum of isotypic components"
                          "# and represent it combinatorially"
                          "# @param Cone P"
                          "# @param Array<SetType> rirs representatives of interior ridge simplices"
                          "# @param Array<SetType> rmis representatives of maximal interior simplices"
                          "# @param Set<Int> the list of indices of the isotypic components to project to; default [0], which"
                          "# amounts to summing all cocircuit equations corresponding to the orbit of each ridge."
                          "# @option String filename where large output should go to. 'filename=>\"-\"' writes to stdout."
                          "# @return Array<Pair<SetType, HashMap<SetType,Rational>>> indexed_cocircuit_equations a list of"
                          "# interior ridge simplices together with the corresponding sparsely represented cocircuit equation",
                          "combinatorial_symmetrized_cocircuit_equations<Scalar,SetType>(Cone<Scalar>, Array<SetType>, Array<SetType>; Set<Int>=scalar2set(0), { filename=> '' })");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:

 
