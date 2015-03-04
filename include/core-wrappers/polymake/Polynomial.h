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

#ifndef POLYMAKE_CORE_WRAPPERS_Polynomial_H
#define POLYMAKE_CORE_WRAPPERS_Polynomial_H

#include_next "polymake/Polynomial.h"
#include "polymake/client.h"

namespace polymake { namespace perl_bindings {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T, typename T0, typename T1>
   RecognizeType4perl("Polymake::common::UniMonomial", (T0,T1), UniMonomial<T0,T1>)

   template <typename T, typename T0, typename T1>
   RecognizeType4perl("Polymake::common::Monomial", (T0,T1), Monomial<T0,T1>)

   template <typename T, typename T0, typename T1>
   RecognizeType4perl("Polymake::common::UniPolynomial", (T0,T1), UniPolynomial<T0,T1>)

   template <typename T, typename T0, typename T1>
   RecognizeType4perl("Polymake::common::UniTerm", (T0,T1), UniTerm<T0,T1>)

   template <typename T, typename T0, typename T1>
   RecognizeType4perl("Polymake::common::Term", (T0,T1), Term<T0,T1>)

   template <typename T, typename T0, typename T1>
   RecognizeType4perl("Polymake::common::Polynomial", (T0,T1), Polynomial<T0,T1>)

///==== Automatically generated contents end here.  Please do not delete this line. ====
} }

#endif // POLYMAKE_CORE_WRAPPERS_Polynomial_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
