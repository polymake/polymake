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

#ifndef _POLYMAKE_SIMPLE_ROOTS_H
#define _POLYMAKE_SIMPLE_ROOTS_H

#include "polymake/SparseMatrix.h"
#include "polymake/Rational.h"
#include "polymake/QuadraticExtension.h"

namespace polymake { namespace polytope {

SparseMatrix<Rational> simple_roots_type_A (const int n);
SparseMatrix<Rational> simple_roots_type_B (const int n);
SparseMatrix<Rational> simple_roots_type_C (const int n);
SparseMatrix<Rational> simple_roots_type_D (const int n);
SparseMatrix< QuadraticExtension<Rational> > simple_roots_type_E6 ();
SparseMatrix< QuadraticExtension<Rational> > simple_roots_type_E7 ();
SparseMatrix<Rational> simple_roots_type_E8 ();
SparseMatrix<Rational> simple_roots_type_F4 ();
SparseMatrix<Rational> simple_roots_type_G2 ();
SparseMatrix<QuadraticExtension<Rational> > simple_roots_type_H3 ();
SparseMatrix<QuadraticExtension<Rational> > simple_roots_type_H4 ();

} }

#endif // _POLYMAKE_SIMPLE_ROOTS_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
