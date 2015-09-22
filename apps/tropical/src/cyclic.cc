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
#include "polymake/TropicalNumber.h"

namespace polymake { namespace tropical {

	template <typename Addition>
		perl::Object cyclic(const int d, const int n)
		{
			if (d<2 || n<=d) {
				throw std::runtime_error("n > d >= 2 required");
			}
			Matrix<TropicalNumber<Addition> > V(n,d+1);

			for (int i=0; i<n; ++i)
				for (int j=0; j<=d; ++j)
					V(i,j)= TropicalNumber<Addition>(Addition::orientation() * i*j);
			perl::Object p(perl::ObjectType::construct<Addition>("Cone"));
			p.set_description()<<"Tropical cyclic "<<d<<"-polytope with "<<n<< " vertices"<<endl;
			p.take("POINTS") << V;
			return p;
		}

	UserFunctionTemplate4perl("# @category Producing a tropical polytope"
			"# Produces a tropical cyclic //d//-polytope with //n// vertices."
			"# Cf."
			"# \t Josephine Yu & Florian Block, arXiv: math.MG/0503279."
			"# @param int d the dimension"
			"# @param int n the number of generators"
			"# @tparam Addition Min or Max."
			"# @return Cone<Addition>",
			"cyclic<Addition>($,$)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
