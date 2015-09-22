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
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/PowerSet.h"
#include "polymake/TropicalNumber.h"

namespace polymake { namespace tropical {

	template<typename Addition>
		perl::Object hypersimplex(int d, int k)
		{
			if (d < 1)
				throw std::runtime_error("hypersimplex: dimension >= 2 required");
			if (k < 1 || k > d)
				throw std::runtime_error("hypersimplex: 1 <= k <= d required");

			perl::Object p(perl::ObjectType::construct<Addition>("Cone"));
			p.set_description() << "tropical (" << k << "," << d << ")-hypersimplex" << endl;

			// number of vertices
			const int n=Integer::binom(d+1,k).to_int();

			Matrix<TropicalNumber<Addition> > Vertices(n,d+1);
				Vertices.fill( TropicalNumber<Addition>::one());
			typename Rows< Matrix<TropicalNumber<Addition> > >::iterator v=rows(Vertices).begin();
			Subsets_of_k<sequence> enumerator(range(0,d), k);
			for (Subsets_of_k<sequence>::iterator s=entire(enumerator); !s.at_end(); ++s, ++v) {
				v->slice(*s).fill(TropicalNumber<Addition>(-Addition::orientation()));
			}
			p.take("POINTS") << Vertices;

			return p;
		}

	UserFunctionTemplate4perl("# @category Producing a tropical polytope"
			"# Produce the tropical hypersimplex &Delta;(//k//,//d//)."
			"# Cf." 
			"# \t M. Joswig math/0312068v3, Ex. 2.10."
			"# The value of //k// defaults to 1, yielding a tropical standard simplex."
			"# @param int d the dimension"
			"# @param int k the number of +/-1 entries"
			"# @tparam Addition Max or Min"
			"# @return Cone<Addition>",
			"hypersimplex<Addition>($;$=1)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
