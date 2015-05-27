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

#ifndef POLYMAKE_TROPICAL_DUAL_ADDITION_VERSION_H
#define POLYMAKE_TROPICAL_DUAL_ADDITION_VERSION_H

#include "polymake/Rational.h"
#include "polymake/TropicalNumber.h"
#include "polymake/Vector.h"
#include "polymake/Polynomial.h"

namespace polymake { namespace tropical {

	template <typename Addition>
		TropicalNumber<typename Addition::dual > dual_addition_version(const TropicalNumber<Addition> &t, bool strong = true) {
			return TropicalNumber<typename Addition::dual>(
					(strong? -1 : 1) * Rational(t));
		}

	template <typename Addition>
		Vector<TropicalNumber<typename Addition::dual> > dual_addition_version(const Vector<TropicalNumber<Addition> > &v, bool strong = true) {
			Vector<TropicalNumber<typename Addition::dual> > r(v.dim());
			for(int i = 0; i < v.dim(); i++) {
				r[i] = dual_addition_version(v[i],strong);
			}
			return r;
		}

	
	template <typename Addition>
		Matrix<TropicalNumber<typename Addition::dual> > dual_addition_version(const Matrix<TropicalNumber<Addition> > &v, bool strong = true) {
			Matrix<TropicalNumber<typename Addition::dual> > r(v.rows(), v.cols());
			for(int i = 0; i < v.rows(); i++) {
				r.row(i) = dual_addition_version(Vector<TropicalNumber<Addition> >(v.row(i)),strong);
			}
			return r;
		}

	template <typename Addition>
		Ring<TropicalNumber<typename Addition::dual> > dual_addition_version(const Ring<TropicalNumber<Addition> > &r) {
			return Ring<TropicalNumber<typename Addition::dual> >(r.names());	
		}

	template <typename Addition>
		Polynomial<TropicalNumber<typename Addition::dual> > dual_addition_version(const Polynomial<TropicalNumber<Addition> > &p, bool strong = true) {
			Ring<TropicalNumber<typename Addition::dual> > dualr = dual_addition_version(p.get_ring());
			Polynomial<TropicalNumber<typename Addition::dual> > dualp(
					p.monomials_as_matrix(),
					dual_addition_version(p.coefficients_as_vector(),strong),dualr);
			return dualp;
		}



}}

#endif
