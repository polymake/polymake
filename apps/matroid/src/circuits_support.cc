/* Copyright (c) 1997-2019
   Ewgenij Gawrilow, Michael Joswig, and the polymake team
   Technische Universität Berlin, Germany
   https://polymake.org

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
#include "polymake/list"
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/TropicalNumber.h"



namespace polymake { namespace matroid {

  /**
   * @brief Computes [[CIRCUITS]] and [[N_ELEMENTS]] from [[VALUATION_ON_CIRCUITS]].
   */
  template
  <typename Addition, typename Scalar>
  void circuits_supports(perl::Object vm) {
      //Extract valuated circuits
      Matrix<TropicalNumber<Addition,Scalar> > valuation = vm.give("VALUATION_ON_CIRCUITS");

      Array<Set<int> > circuits(valuation.rows());
      for(int r = 0; r < valuation.rows(); r++) {
	   Set<int> c;
	   for(int i = 0; i < valuation.cols(); i++) {
	      if( TropicalNumber<Addition,Scalar>::zero() != valuation(r,i)) c+= i;
	   }
	   circuits[r] = c;
      }

      vm.take("CIRCUITS") << circuits;
      vm.take("N_ELEMENTS") << valuation.cols();

  }//END circuits_supports

  FunctionTemplate4perl("circuits_supports<Addition,Scalar>(ValuatedMatroid<Addition,Scalar>)");

} }
