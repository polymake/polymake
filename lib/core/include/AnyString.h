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

#ifndef POLYMAKE_ANY_STRING_H
#define POLYMAKE_ANY_STRING_H

#include <string>
#include <cstring>

namespace polymake {

//! Convenience wrapper for strings passed to various functions.
//! @note: it does not own any resources, it merely conveys a reference to data stored elsewhere.

struct AnyString {
   template <size_t n>
   AnyString(const char (&s)[n])
      : ptr(s+0)
      , len(n-1) {}

   AnyString(const std::string& s)
      : ptr(s.c_str())
      , len(s.size()) {}

   AnyString(std::nullptr_t)
      : ptr(nullptr)
      , len(0) {}

   AnyString()
      : AnyString(nullptr) {}

   AnyString(const char* p, size_t l)
      : ptr(p)
      , len(l) {}

   explicit operator bool() const { return ptr != nullptr; }

   friend
   std::string operator+ (const AnyString& s1, const AnyString& s2)
   {
      std::string s(s1.ptr, s1.len);
      s.append(s2.ptr, s2.len);
      return s;
   }

   friend
   std::string&& operator+ (std::string&& s1, const AnyString& s2)
   {
      s1.append(s2.ptr, s2.len);
      return std::move(s1);
   }

   template <size_t n>
   friend
   std::string operator+ (const char (&s1)[n], const AnyString& s2)
   {
      return AnyString(s1) + s2;
   }

   const char* ptr;
   size_t len;
};

// useful in conditional expressions:
// instead of  x ? "X" : "NOT_X"
// write       x ? Str("X") : Str("NOT_X")
// in order to avoid gratuitous strlen() calls

template <size_t n>
inline
AnyString Str(const char (&s)[n])
{
   return AnyString(s);
}

inline
AnyString CStr(const char* s)
{
   return AnyString(s, strlen(s));
}

}

// for a transitional period:
namespace pm {
using polymake::AnyString;
using polymake::Str;
using polymake::CStr;
}

#endif // POLYMAKE_ANY_STRING_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
