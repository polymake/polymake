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

#include "polymake/Integer.h"
#include "polymake/internal/converters_basic_defs.h"
#include <iostream>
#include <polymake/GenericIO.h>
#include <limits>

namespace pm {

   class GF2 {

      protected:

         bool boolrep;

      public:

         GF2() : boolrep(false) {}

         explicit GF2(const Int& n) : boolrep(n % 2) {}
         
         explicit GF2(const Integer& n) : boolrep(n % 2) {}

         GF2(const GF2& gf2) : boolrep(gf2.to_bool()) {}

         inline bool to_bool() const {
            return boolrep;
         }

         // arithmetic operations:

         GF2& negate()
         {
            return *this;
         }
         
         friend GF2 operator+ (const GF2& a, const GF2& b) { return GF2(a.to_bool() != b.to_bool()); }
         friend GF2 operator- (const GF2& a, const GF2& b) { return a+b; }
         friend GF2 operator* (const GF2& a, const GF2& b) { return GF2(a.to_bool() && b.to_bool()); }
         friend GF2 operator/ (const GF2& a, const GF2& b) {
            if (!b.to_bool())
               throw std::domain_error("Divide by zero exception");
            return a;
         }

         inline GF2& operator+= (const GF2& a) { return (*this = *this + a); }
         inline GF2& operator-= (const GF2& a) { return (*this += a); }
         inline GF2& operator*= (const GF2& a) { return (*this = *this * a); }
         inline GF2& operator/= (const GF2& a) { return (*this = *this / a); }

         inline GF2& operator++ () { return (*this += GF2(1)); }
         inline GF2& operator-- () { return (*this -= GF2(1)); }

         friend GF2 operator- (const GF2& a) { return a; }

         friend bool operator== (const GF2& a, const GF2& b) { return a.boolrep == b.boolrep; }
         friend bool operator!= (const GF2& a, const GF2& b) { return !(a == b); }

         // I/O:

      protected:

         // TODO why is this here?
         template <typename Input>
            Input& read(Input& in)
            {
               return in >> boolrep;
            }


      public:

         template <typename Input> friend
            Input& operator>> (GenericInput<Input>& in, GF2& me)
            {
               return in.top() >> me.boolrep;
            }

         template <typename Output> friend
            Output& operator<< (GenericOutput<Output>& out, const GF2& me)
            {
               return out.top() << me.to_bool();
            }

   };

   template <typename T, bool _is_integer=std::numeric_limits<T>::is_integer>
      class conv_to_GF2;

   template <typename T>
      class conv_to_GF2<T, true> {
         public:
            typedef T argument_type;
            typedef GF2 result_type;
            result_type operator() (typename pm::function_argument<T>::type x) const { return result_type(x%2); }
      };

   template <>
      struct choose_generic_object_traits<pm::GF2, false, false> :
      spec_object_traits< is_scalar > {
         typedef void generic_type;
         typedef is_scalar generic_tag;
         typedef pm::GF2 persistent_type;

         static bool is_zero(const pm::GF2& a) {
            return !a.to_bool();
         }

         static bool is_one(const pm::GF2& a) {
            return a.to_bool();
         }

         static const persistent_type& zero() {
            static pm::GF2 zero = pm::GF2(0);
            return zero;
         }

         static const persistent_type& one() {
            static pm::GF2 one = pm::GF2(1);
            return one;
         }
      };

   template <>
      struct algebraic_traits<pm::GF2> {
         typedef pm::GF2 field_type;
      };

   template <>
   struct hash_func<GF2, is_scalar> {
      size_t operator() (const GF2& a) const
      {
         return a.to_bool();
      }
   };
}


namespace polymake {
   using pm::GF2;
}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
