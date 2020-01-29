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

#include "polymake/color.h"
#include "polymake/internal/comparators.h"
#include <cmath>

namespace pm {

void RGB::verify() const
{
   if (red<0   || red>1)   throw color_error("RGB: Red value out of range");
   if (green<0 || green>1) throw color_error("RGB: Green value out of range");
   if (blue<0  || blue>1)  throw color_error("RGB: Blue value out of range");
}

void RGB::scale_and_verify()
{
   if ((red>1 || green>1 || blue>1) && floor(red)==red && floor(green)==green && floor(blue)==blue) {
      red/=255; green/=255; blue/=255;
   }
   verify();
}

RGB::RGB(const HSV& hsv)
{
   const double H=hsv.hue/60.,
      S=hsv.saturation,
      V=hsv.value;

   const double primary=floor(H),
      secondary=H-primary,
      a=(1.-S)*V,
      b=(1.-S*secondary)*V,
      c=V+a-b;

   switch (int(primary)) {
   case 0: red=V; green=c; blue=a; break;
   case 1: red=b; green=V; blue=a; break;
   case 2: red=a; green=V; blue=c; break;
   case 3: red=a; green=b; blue=V; break;
   case 4: red=c; green=a; blue=V; break;
   case 5: red=V; green=a; blue=b; break;
   }
}

void HSV::verify() const
{
   if (hue<0        || hue>360)      throw color_error("HSV: Hue value out of range");
   if (saturation<0 || saturation>1) throw color_error("HSV: Saturation value out of range");
   if (value<0      || value>1)      throw color_error("HSV: Value value out of range");
}

HSV::HSV(const RGB& rgb)
{
   const double R = rgb.red,
                G = rgb.green,
                B = rgb.blue;

   double max = R, min = R;
   assign_min_max(min,max,G);
   assign_min_max(min,max,B);

   if (max == 0)
      saturation = 0;
   else
      saturation = (max-min)/max;
   
   value = max;

   if (saturation != 0) {
      if (R == max) {
         if (G == min)
            hue = B == min ? 0. : 5.+(max-B)/(max-min);
         else
            hue = 1.-(max-G)/(max-min);
      } else if (G == max) {
         if (B == min)
            hue = 1.+(max-R)/(max-min);
         else
            hue = 3.-(max-B)/(max-min);
      } else {
         if (R == min)
            hue = 3.+(max-G)/(max-min);
         else
            hue = 5.-(max-R)/(max-min);
      }
      hue *= 60.;
   } else {
      hue = 0.;
   }
}

} // end namespace pm

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
