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

#ifndef POLYMAKE_CORE_WRAPPERS_GenericStruct_H
#define POLYMAKE_CORE_WRAPPERS_GenericStruct_H

#include_next "polymake/GenericStruct.h"
#include "polymake/client.h"

namespace pm { namespace perl {

template <typename Struct, int i>
struct StructUtils_helper {
   typedef typename Struct::field_types type_list;
   static const int next= i+1 < list_length<type_list>::value ? i+1 : i;
   typedef StructUtils_helper<Struct,next> recurse_down;

   static void gather_fields(ArrayHolder& arr)
   {
      size_t nl;
      const char* n=Struct::get_field_name(nl, int_constant<i>());
      arr.push(Scalar::const_string(n,nl));
      if (next>i) recurse_down::gather_fields(arr);
   }
};

template <typename Struct>
struct StructUtils {
   typedef typename Struct::field_types type_list;

   static SV* field_names()
   {
      ArrayHolder ret(list_length<type_list>::value);
      StructUtils_helper<Struct,0>::gather_fields(ret);
      return ret.get();
   }
};

} }

namespace polymake { namespace perl_bindings {

template <typename Struct>
SV* member_names(bait, GenericStruct<Struct>*)
{
   return pm::perl::StructUtils<Struct>::field_names();
}

///==== Automatically generated contents follow.    Please do not delete this line. ====
///==== Automatically generated contents end here.  Please do not delete this line. ====
} }

#endif // POLYMAKE_CORE_WRAPPERS_GenericStruct_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
