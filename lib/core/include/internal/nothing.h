/* Copyright (c) 1997-2021
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

#include "polymake/internal/type_manip.h"

namespace pm {

/// Structure denoting the absence of data
struct nothing {
   void operator= (const nothing&) const { }
   bool operator== (const nothing&) const { return true; }
   bool operator!= (const nothing&) const { return false; }
   bool operator< (const nothing&) const { return false; }
   bool operator> (const nothing&) const { return false; }
   bool operator<= (const nothing&) const { return true; }
   bool operator>= (const nothing&) const { return true; }
};

inline void relocate(const nothing*, const nothing*) { }

template <>
struct spec_object_traits<nothing> : spec_object_traits<is_opaque> {
   typedef nothing model;       // prevent mixing this with any other type
};

} // end namespace pm

namespace polymake {
   using pm::nothing;
}

namespace std {
   /// Do nothing
   inline void swap(const pm::nothing&, const pm::nothing&) { }
}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
