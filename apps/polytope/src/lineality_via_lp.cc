/* Copyright (c) 1997-2018
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
#include <iostream>
#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/GenericMatrix.h"
#include "polymake/linalg.h"

namespace polymake { namespace polytope {

template <typename Scalar, typename TMatrix1, typename TMatrix2> inline
Matrix<Scalar> lineality_via_lp(const GenericMatrix<TMatrix1, Scalar>& inrays, const GenericMatrix<TMatrix2, Scalar>& inls)
{
	Matrix<Scalar> ls = Matrix<Scalar>(inls);
   perl::Object p(perl::ObjectType::construct<Scalar>("Polytope"));
   
   const int n_inrays = inrays.rows();
	const int d = inrays.cols();

	Matrix<Scalar> ineq = (zero_vector<Scalar>()|unit_matrix<Scalar>(n_inrays))/(ones_vector<Scalar>()|(-1)*unit_matrix<Scalar>(n_inrays));
   //need to remove zero rows to get a valid EQUATIONS matrix
	Matrix<Scalar> eq = zero_vector<Scalar>()|remove_zero_rows(T(inrays));
   p.take("INEQUALITIES") << ineq;
	p.take("EQUATIONS") << eq;
	Vector<Scalar> maxvert;
	Scalar maxval;
	Vector<Scalar> obj = ones_vector<Scalar>(n_inrays+1);

   do {
		perl::Object lp = p.add("LP");		
		lp.take("LINEAR_OBJECTIVE") << obj;
		lp.give("MAXIMAL_VERTEX") >> maxvert;
		lp.give("MAXIMAL_VALUE") >> maxval;
		for (int i=1; i<=n_inrays; i++) {
		   if (maxvert[i]!=0 && obj[i]>0) {
				obj[i] = 0;
				ls /= inrays.row(i-1);
			}
		}
	} while (maxval>1 && rank(ls)<d);	
   return ls.minor(basis_rows(ls),All);
}
FunctionTemplate4perl("lineality_via_lp<Scalar>(Matrix<type_upgrade<Scalar>,_>,Matrix<type_upgrade<Scalar>,_>)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
