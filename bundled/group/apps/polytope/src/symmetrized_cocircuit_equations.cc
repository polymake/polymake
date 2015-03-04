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
#include "polymake/polytope/symmetrized_cocircuit_equations.h"

namespace polymake { namespace polytope {

template<typename Scalar>
ListMatrix<SparseVector<int> > 
symmetrized_cocircuit_equations(int d,
                                const Matrix<Scalar>& V,
                                const IncidenceMatrix<>& VIF,
                                const Array<Array<int> >& generators,
                                const Array<SetType>& interior_ridge_reps,
                                const Array<SetType>& facet_reps,
                                perl::OptionSet options) 
{
   return symmetrized_cocircuit_equations_impl(d, V, VIF, generators, interior_ridge_reps, facet_reps, options, false);
}

template<typename Scalar>
ListMatrix<SparseVector<int> >
symmetrized_foldable_cocircuit_equations(int d,
                                         const Matrix<Scalar>& V,
                                         const IncidenceMatrix<>& VIF,
                                         const Array<Array<int> >& generators,
                                         const Array<SetType>& interior_ridge_reps,
                                         const Array<SetType>& facet_reps,
                                         perl::OptionSet options) 
{
   return symmetrized_foldable_cocircuit_equations_impl(d, V, VIF, generators, interior_ridge_reps, facet_reps, options, false);
}

FunctionTemplate4perl("symmetrized_cocircuit_equations<Scalar>($ Matrix<Scalar> IncidenceMatrix Array<Array<Int>> Array<common::boost_dynamic_bitset> Array<common::boost_dynamic_bitset> { filename=>'', reduce_rows=>0, log_frequency=>0 })");

FunctionTemplate4perl("symmetrized_foldable_cocircuit_equations<Scalar>($ Matrix<Scalar> IncidenceMatrix Array<Array<Int>> Array<common::boost_dynamic_bitset> Array<common::boost_dynamic_bitset> { filename=>'', reduce_rows=>0, log_frequency=>0 })");


} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:

 
