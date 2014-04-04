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

#ifndef POLYMAKE_INTERNAL_MATRIX_ROWS_COLS_H
#define POLYMAKE_INTERNAL_MATRIX_ROWS_COLS_H

#include "polymake/internal/Wary.h"

namespace pm {

template <typename Matrix> class Rows;
template <typename Matrix> class Cols;

template <typename Matrix>
struct spec_object_traits< Rows<Matrix> >
   : spec_object_traits<is_container> {
   typedef Matrix masquerade_for;
   static const bool is_lazy         = object_traits<Matrix>::is_lazy,
                     is_always_const = object_traits<Matrix>::is_always_const;
   static const int is_resizeable= object_traits<Matrix>::is_resizeable ? 1 : 0;
   static const IO_separator_kind IO_separator=IO_sep_enforce;
};
template <typename Matrix>
struct spec_object_traits< Cols<Matrix> >
   : spec_object_traits< Rows<Matrix> > {};

template <typename Matrix>
class Rows< Wary<Matrix> > : public Rows<Matrix> {};

template <typename Matrix>
class Cols< Wary<Matrix> > : public Cols<Matrix> {};

template <typename Matrix> inline
Rows<typename Concrete<Matrix>::type>& rows(Matrix& m)
{
   return reinterpret_cast<Rows<typename Concrete<Matrix>::type>&>(concrete(m));
}

template <typename Matrix> inline
const Rows<typename Concrete<Matrix>::type>& rows(const Matrix& m)
{
   return reinterpret_cast<const Rows<typename Concrete<Matrix>::type>&>(concrete(m));
}

template <typename Matrix> inline
Cols<typename Concrete<Matrix>::type>& cols(Matrix& m)
{
   return reinterpret_cast<Cols<typename Concrete<Matrix>::type>&>(concrete(m));
}

template <typename Matrix> inline
const Cols<typename Concrete<Matrix>::type>& cols(const Matrix& m)
{
   return reinterpret_cast<const Cols<typename Concrete<Matrix>::type>&>(concrete(m));
}

} // end namespace pm

namespace polymake {
   using pm::Rows;
   using pm::Cols;
}

#endif // POLYMAKE_INTERNAL_MATRIX_ROWS_COLS_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
