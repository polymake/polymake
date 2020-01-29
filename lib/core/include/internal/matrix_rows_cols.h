/* Copyright (c) 1997-2020
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

#ifndef POLYMAKE_INTERNAL_MATRIX_ROWS_COLS_H
#define POLYMAKE_INTERNAL_MATRIX_ROWS_COLS_H

#include "polymake/internal/Wary.h"

namespace pm {

template <typename TMatrix> class Rows;
template <typename TMatrix> class Cols;

template <typename TMatrix>
struct spec_object_traits< Rows<TMatrix> >
   : spec_object_traits<is_container> {
   typedef TMatrix masquerade_for;
   static constexpr bool is_lazy         = object_traits<TMatrix>::is_lazy,
                         is_always_const = object_traits<TMatrix>::is_always_const;
   static constexpr int is_resizeable= object_traits<TMatrix>::is_resizeable ? 1 : 0;
   static constexpr IO_separator_kind IO_separator = IO_sep_enforce;
};
template <typename TMatrix>
struct spec_object_traits< Cols<TMatrix> >
   : spec_object_traits< Rows<TMatrix> > {};

template <typename TMatrix>
auto rows(TMatrix&& m)
   // gcc 5 needs this crutch
   -> typename inherit_ref_norv<Rows<unwary_t<pure_type_t<TMatrix>>>, TMatrix&>::type
{
   return reinterpret_cast<typename inherit_ref_norv<Rows<unwary_t<pure_type_t<TMatrix>>>, TMatrix&>::type>(unwary(m));
}

template <typename TMatrix>
auto cols(TMatrix&& m)
   // gcc 5 needs this crutch
   -> typename inherit_ref_norv<Cols<unwary_t<pure_type_t<TMatrix>>>, TMatrix&>::type
{
   return reinterpret_cast<typename inherit_ref_norv<Cols<unwary_t<pure_type_t<TMatrix>>>, TMatrix&>::type>(unwary(m));
}

template <typename TMatrix>
class Rows<Wary<TMatrix>> : public Rows<TMatrix> { };

template <typename TMatrix>
class Cols<Wary<TMatrix>> : public Cols<TMatrix> { };

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
