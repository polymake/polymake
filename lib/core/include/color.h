/* Copyright (c) 1997-2019
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

/** @file color.h
    @brief Implementation of pm::RGB and pm::HSV classes
 */

#ifndef POLYMAKE_COLOR_H
#define POLYMAKE_COLOR_H

#include "polymake/GenericStruct.h"
#include "polymake/GenericIO.h"
#include <stdexcept>

namespace pm {

/// An exception of this type is thrown by an attempt to assign a wrong value to some color component.
class color_error : public std::domain_error {
public:
   color_error(const std::string& what_arg) : std::domain_error(what_arg) { }
};

class HSV;

/** @class RGB
    @brief Color description in %RGB space: Red-Green-Blue additive color model.
*/
class RGB : public GenericStruct<RGB> {
public:
   DeclSTRUCT( DeclFIELD(red,double)
               DeclFIELD(green,double)
               DeclFIELD(blue,double) );
protected:
   void verify() const;
   void scale_and_verify();

public:
   /// defaults to black
   RGB() : red(0), green(0), blue(0) {}

   /// constructor from individual color channels
   RGB(double r, double g, double b) : red(r), green(g), blue(b) { verify(); }

   /// ... variant for int parameters
   RGB(int r, int g, int b) : red(r), green(g), blue(b) { scale_and_verify(); }

   /// converts from HSV representation
   RGB(const HSV&);

   friend struct redirect_object_traits<RGB>;
};

template <>
struct redirect_object_traits<RGB>
   : choose_generic_object_traits<RGB> {

   template <typename T, typename CursorRef>
   static void visit_elements(RGB& me, composite_reader<T,CursorRef>& reader)
   {
      me.visit_fields(reader);
      me.scale_and_verify();
   }

   using choose_generic_object_traits<RGB>::visit_elements;
};

/** @class HSV
    @brief Color description in %HSV space

    Hue-Saturation-Value color model.  This color model is more natural with
    respect to human physiology than the RGB model.
*/
class HSV : public GenericStruct<HSV> {
public:
   DeclSTRUCT( DeclFIELD(hue,double)
               DeclFIELD(saturation,double)
               DeclFIELD(value,double) );
protected:
   void verify() const;
public:
   /// defaults to black
   HSV() : hue(0), saturation(0), value(0) {}

   /// create from channels
   HSV(double h, double s, double v) : hue(h), saturation(s), value(v) { verify(); }

   /// converts from RGB representation
   HSV(const RGB&);
};

} // end namespace pm

namespace polymake {
   using pm::RGB;
   using pm::HSV;
}

#endif // POLYMAKE_COLOR_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
