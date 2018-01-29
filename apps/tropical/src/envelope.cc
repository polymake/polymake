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

#include "polymake/client.h"
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/TropicalNumber.h"
#include "polymake/linalg.h"

namespace polymake { namespace tropical {

	template <typename Addition, typename Scalar>
		perl::Object envelope(const Matrix<TropicalNumber<Addition,Scalar> > &m)
		{
			const int n(m.rows()), d(m.cols());

			// The coordinates are in the (1+d+n)-dimensional space W = Z x Y:
			// 0: homogenizing coordinate; 1..d+1: Z; d+2..d+1+n: Y
			Matrix<Scalar> I(n*d,n+d+1); // initialized as zero matrix
			for (int i=0; i < n; ++i)
				for (int j=0; j < d; ++j) {
					if(!is_zero(m(i,j))) {
						I(i*d+j,0)=Scalar(m(i,j));
						I(i*d+j,1+j)=  -1;//Addition::orientation();
						I(i*d+j,1+d+i)=  1;//Addition::orientation(); 
					}
				}

			Vector<Scalar> normalizing_equation(unit_vector<Scalar>(n+d+1,1));

			perl::Object p_out(perl::ObjectType::construct<Scalar>("polytope::Polytope"));

			p_out.take("INEQUALITIES") << remove_zero_rows(I);
			p_out.take("EQUATIONS") << normalizing_equation;
			return p_out;
		}

	FunctionTemplate4perl("envelope<Addition,Scalar>(Matrix<TropicalNumber<Addition,Scalar> >)");

	/*
		UserFunctionTemplate4perl("# @category Producing another object"
		"# Given points in tropical projective space, compute an ordinary unbounded polyhedron such that"
		"# the tropical convex hull of the input is the bounded subcomplex of the latter."
		"# Cf."
		"# \t Develin & Sturmfels math.MG/0308254v2, Lemma 22."
		"# Warning: This client does not implement the reverse transformation to [[poly2trop]]."
		"# @param TropicalPolytope T"
		"# @tparam Coord"
		"# @return polytope::Polytope",
		"trop2poly<Coord>(TropicalPolytope<Coord>)");*/
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
