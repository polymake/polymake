/* Copyright (c) 1997-2016
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

#ifndef POLYMAKE_PERL_CONSTANTS_H
#define POLYMAKE_PERL_CONSTANTS_H

#ifdef __cplusplus

#include "polymake/AnyString.h"

namespace pm { namespace perl {
#endif

enum value_flags {
   value_mutable=0, value_read_only=1, value_expect_lval=2, value_alloc_magic=4,
   value_allow_undef=8, value_allow_non_persistent=16, value_ignore_magic=32,
   value_trusted=0, value_not_trusted=64, value_allow_conversion=128,
   value_allow_store_ref=256, value_allow_store_temp_ref=512,
   value_allow_store_any_ref=value_allow_store_ref|value_allow_store_temp_ref
};

enum class_kind {
   class_is_scalar, class_is_container, class_is_composite, class_is_opaque, class_is_kind_mask=0xf,
   class_is_assoc_container=0x100, class_is_sparse_container=0x200, class_is_set=0x400,
   class_is_serializable=0x800, class_is_declared=0x1000
};

#ifdef __cplusplus

constexpr value_flags operator| (value_flags a, value_flags b)
{
   return static_cast<value_flags>(int(a) | int(b));
}

inline
value_flags& operator|= (value_flags& a, value_flags b)
{
   a = a | b;
   return a;
}

} }
#endif

#endif // POLYMAKE_PERL_CONSTANTS_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
