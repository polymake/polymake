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
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/Matrix.h"
#include "polymake/TropicalNumber.h"
#include "polymake/internal/type_union.h"

namespace polymake { namespace tropical {

	template <typename Addition, typename Scalar>
		perl::Object matroid_polytope(perl::Object m, const Scalar& value)
		{
			const Array< Set<int> > bases=m.give("BASES");
			const int n_bases=bases.size();
			const int n_elements=m.give("N_ELEMENTS");
			const TropicalNumber<Addition,Scalar> tvalue(value);


			typedef typename pm::concat_list<Addition, Scalar>::type  cone_type;
			perl::Object t(perl::ObjectType::construct<cone_type>("Cone"));

			
			Matrix<TropicalNumber<Addition,Scalar> > V(n_bases,n_elements);
				V.fill( TropicalNumber<Addition,Scalar>::one());

			for (int b=0; b<n_bases; ++b) {
				for (Entire< Set<int> >::const_iterator i=entire(bases[b]); !i.at_end(); ++i)
					V(b,(*i))=tvalue;
			}

			t.take("POINTS") << V;

			return t;
		}

	InsertEmbeddedRule("REQUIRE_APPLICATION matroid\n\n");

	UserFunctionTemplate4perl("# @category Producing a tropical polytope"
			"# Produce the tropical matroid polytope from a matroid //m//."
			"# Each vertex corresponds to a basis of the matroid,"
			"# the non-bases coordinates get value 0, the bases coordinates"
			"# get value //v//, default is -orientation."
			"# @param matroid::Matroid m"
			"# @param Scalar v value for the bases"
			"# @tparam Addition Min or Max"
			"# @return Cone<Addition,Scalar>",
			"matroid_polytope<Addition,Scalar=Rational>(matroid::Matroid; $ = -Addition->orientation())");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
