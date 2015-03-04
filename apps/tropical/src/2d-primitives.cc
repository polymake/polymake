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

#include "polymake/tropical/2d-primitives.h"

namespace polymake { namespace tropical {

int segment_below(const Vector<Rational>& a, const Vector<Rational>& b, const Vector<Rational>& c)
{
   int ba_ray_above=ray_above(b,a), ca_ray_above=ray_above(c,a);
   if (ba_ray_above>=0 && ca_ray_above>=0) {
      int bc_right=right(b,c);
      if (bc_right==0)
         return y_above(b,c);
      else
         return bc_right;
   }
   if (ba_ray_above<0 && ca_ray_above<0) {
      int bc_y_above=y_above(b,c);
      if (bc_y_above==0)
         return right(b,c);
      else
         return bc_y_above;
   }
   if (ba_ray_above>=0)
      return -1;
   else
      return 1;
}

int segment_above(const Vector<Rational>& a, const Vector<Rational>& b, const Vector<Rational>& c)
{
   int ba_y_above=y_above(b,a), ca_y_above=y_above(c,a);
   if (ba_y_above>=0 && ca_y_above>=0) {
      int bc_right=right(b,c);
      if (bc_right==0)
         return y_above(b,c);
      else
         return bc_right;
   }
   if (ba_y_above<0 && ca_y_above<0) {
      int bc_ray_above=ray_above(b,c);
      if (bc_ray_above==0)
         return -right(b,c);
      else
         return bc_ray_above;
   }
   if (ba_y_above>=0)
      return 1;
   else
      return -1;
}

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
