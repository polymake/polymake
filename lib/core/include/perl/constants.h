/* Copyright (c) 1997-2023
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

#pragma once

#include "polymake/AnyString.h"

namespace pm { namespace perl {

enum class ValueFlags {
   is_mutable = 0, read_only = 1, alloc_magic = 2, expect_lval = 4,
   allow_undef = 8, allow_non_persistent = 0x10, ignore_magic = 0x20,
   is_trusted = 0, not_trusted = 0x40, allow_conversion = 0x80,
   allow_store_ref = 0x100, allow_store_temp_ref = 0x200,
   allow_store_any_ref = allow_store_ref | allow_store_temp_ref
};

enum class ClassFlags {
   none = 0,
   is_scalar = 0, is_container, is_composite, is_opaque, kind_mask=0xf,
   is_assoc_container = 0x100, is_sparse_container = 0x200, is_set = 0x400,
   is_serializable = 0x800, is_sparse_serialized = 0x1000, is_declared = 0x2000,
   is_ordered = 0x4000
};

enum class Returns {
   normal,
   lvalue,
   list,
   empty  // should be void but it's a reserved keyword
};

// function argument reference classification
enum {
   arg_is_const_ref,
   arg_is_lval_ref,
   arg_is_univ_ref,
   arg_is_const_or_rval_ref
};

constexpr ValueFlags operator| (ValueFlags a, ValueFlags b)
{
   return static_cast<ValueFlags>(int(a) | int(b));
}

constexpr ValueFlags operator& (ValueFlags a, ValueFlags b)
{
   return static_cast<ValueFlags>(int(a) & int(b));
}

constexpr bool operator* (ValueFlags a, ValueFlags b)
{
   return (int(a) & int(b)) != 0;
}

constexpr ClassFlags operator| (ClassFlags a, ClassFlags b)
{
   return static_cast<ClassFlags>(int(a) | int(b));
}

constexpr ClassFlags operator& (ClassFlags a, ClassFlags b)
{
   return static_cast<ClassFlags>(int(a) & int(b));
}

constexpr bool operator* (ClassFlags a, ClassFlags b)
{
   return (int(a) & int(b)) != 0;
}

inline
ValueFlags& operator|= (ValueFlags& a, ValueFlags b)
{
   a = a | b;
   return a;
}

inline
ClassFlags& operator|= (ClassFlags& a, ClassFlags b)
{
   a = a | b;
   return a;
}

template <typename L>
void set_bit_flags(L& l, unsigned int r)
{
   l = static_cast<L>(static_cast<unsigned int>(l) | r);
}

template <typename L, typename R>
std::enable_if_t<std::is_enum<R>::value>
set_bit_flags(L& l, R r)
{
   set_bit_flags(l, static_cast<unsigned int>(r));
}

template <typename L>
void clear_bit_flags(L& l, unsigned int r)
{
   l = static_cast<L>(static_cast<unsigned int>(l) & ~r);
}

template <typename L, typename R>
std::enable_if_t<std::is_enum<R>::value>
clear_bit_flags(L& l, R r)
{
   clear_bit_flags(l, static_cast<unsigned int>(r));
}

} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
