/* Copyright (c) 1997-2022
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

#include <iostream>
#include <limits>

namespace pm {

class streambuf_ext : public std::streambuf {
protected:
   char* input_limit;
   streambuf_ext() : input_limit(nullptr) { }

public:
   using size_type = std::streamsize;

   streambuf_ext(const std::string& src)
      : input_limit(nullptr)
   {
      char* text = const_cast<char*>(src.c_str());
      setg(text, text, text + src.size());
   }

   bool set_input_width(size_type w);
   void reset_input_width(bool slurfed);
   void rewind(size_type w);

   virtual size_type lines() { return 0; } // to be overloaded

   void gbump(size_type n)
   {
      if (__builtin_expect(n <= std::numeric_limits<int>::max(), 1))
         std::streambuf::gbump(static_cast<int>(n));
      else
         setg(eback(), gptr() + n, egptr());
   }
};

}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
