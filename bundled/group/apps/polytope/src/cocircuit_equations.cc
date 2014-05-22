/* Copyright (c) 1997-2014
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
                    const Array<Set<int> >& interior_simplices, 
                    perl::OptionSet options)
{
   return cocircuit_equations_impl(d, points, VIF, interior_simplices, options, false);
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



FunctionTemplate4perl("cocircuit_equations<Scalar>($ Matrix<Scalar> IncidenceMatrix Array<Set> { filename=>'', reduce_rows=>1, log_frequency=>0 })");
FunctionTemplate4perl("foldable_cocircuit_equations<Scalar>($ Matrix<Scalar> IncidenceMatrix Array<Set> Array<Set> { filename=>'', reduce_rows=>1, log_frequency=>0 })");


} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:

 
