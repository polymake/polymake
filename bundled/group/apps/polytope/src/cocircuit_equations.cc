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
#include "polymake/polytope/cocircuit_equations.h"

namespace polymake { namespace polytope {

template<typename Scalar>
ListMatrix<SparseVector<int> > 
cocircuit_equations(int d, 
                    const Matrix<Scalar>& points, 
                    const IncidenceMatrix<>& VIF, 
                    const Array<Set<int> >& interior_ridge_simplices, 
                    const Array<Set<int> >& interior_simplices, 
                    perl::OptionSet options)
{
   return cocircuit_equations_impl(d, points, VIF, interior_ridge_simplices, interior_simplices, options);
}

template<typename Scalar>
SparseMatrix<int> 
cocircuit_equations(perl::Object P,
                    const Array<Set<int> >& interior_ridge_simplices, 
                    const Array<Set<int> >& interior_simplices, 
                    perl::OptionSet options)
{
   const int d = P.give("COMBINATORIAL_DIM");
   const Matrix<Scalar> points = P.give("RAYS");
   const IncidenceMatrix<> VIF = P.give("RAYS_IN_FACETS");
   return cocircuit_equations_impl(d, points, VIF, interior_ridge_simplices, interior_simplices, options);
}

template<typename Scalar>
SparseVector<int> 
cocircuit_equation(perl::Object P,
                   const Set<int>& interior_ridge, 
                   const Map<Set<int>, int>& index_of)
{
   const Matrix<Scalar> points = P.give("RAYS");
   return cocircuit_equation_of_impl(points, interior_ridge, index_of);
}

template<typename Scalar>
ListMatrix<SparseVector<int> >
foldable_cocircuit_equations(int d,
                             const Matrix<Scalar>& points,
                             const IncidenceMatrix<>& VIF,
                             const Array<SetType>& interior_ridges, // FIXME: Map
                             const Array<SetType>& facets,
                             perl::OptionSet options)
{
   return foldable_cocircuit_equations_impl(d, points, VIF, interior_ridges, facets, options, false);
}


template<typename Scalar>
ListMatrix<SparseVector<Rational> >
projected_cocircuit_equations(const perl::Object P,
                              const Set<int>& ridge_rep,
                              const Set<int>& isotypic_components)
{
   const Matrix<Scalar> points = P.give("VERTICES");
   const Matrix<Rational> character_table = P.give("GROUP.CHARACTER_TABLE");
   const ConjugacyClasses ccs = P.give("GROUP.CONJUGACY_CLASSES");
   const Array<BBitset> facet_reps = P.give("GROUP.REPRESENTATIVE_MAX_INTERIOR_SIMPLICES");
   hash_map<BBitset, int> index_of_facet;
   return projected_cocircuit_equations_impl(points, ridge_rep, isotypic_components, index_of_facet, character_table, ccs);
}

FunctionTemplate4perl("cocircuit_equations<Scalar>($ Matrix<Scalar> IncidenceMatrix Array<Set> Array<Set> { filename=>'', reduce_rows=>1, log_frequency=>0 })");

UserFunctionTemplate4perl("# @category Combinatorics"
                          "# A matrix whose rows contain the cocircuit equations of a cone C"
                          "# with respect to a list of interior simplices"
                          "# symmetries of the cone are NOT taken into account"
                          "# @param Cone C"
                          "# @param Array<Set> interior_ridge_simplices"
                          "# @param Array<Set> interior_simplices"
                          "# @option String filename where to write the output (default empty)"
                          "# @option Bool reduce_rows whether to perform row reduction (default 1)"
                          "# @option Int log_frequency how often to print log messages"
                          "# @return SparseMatrix<Int>",
                          "cocircuit_equations<Scalar>(Polytope<Scalar> Array<Set> Array<Set> { filename=>'', reduce_rows=>1, log_frequency=>0 })");

UserFunctionTemplate4perl("# @category Combinatorics"
                          "# The cocircuit equations of a cone C corresponding to some interior ridge rho"
                          "# with respect to a list of interior simplices"
                          "# symmetries of the cone are NOT taken into account"
                          "# @param Cone C"
                          "# @param Set<Int> rho the interior ridge"
                          "# @param Map<Set<Int>, Int> index_of the interior_simplices"
                          "# @return SparseVector<Int>",
                          "cocircuit_equation<Scalar>(Polytope<Scalar> Set Map<Set<Int>,Int>)");

FunctionTemplate4perl("foldable_cocircuit_equations<Scalar>($ Matrix<Scalar> IncidenceMatrix Array<Set> Array<Set> { filename=>'', reduce_rows=>1, log_frequency=>0 })");

UserFunctionTemplate4perl("# @category Combinatorics"
                          "# A SparseMatrix whose rows contain projections of the cocircuit equations of a cone C"
                          "# corresponding to the orbit of a specified ridge onto a direct sum of specified isotypic components"
                          "# @param Cone C"
                          "# @param Set<Int> ridge_rep interior ridge"
                          "# @param Set<Int> isotypic_components the isotypic components to project to"
                          "# @return SparseMatrix<Rational>",
                          "projected_cocircuit_equations<Scalar>(Polytope<Scalar> Set<Int> Set<Int>)");


} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:

 
