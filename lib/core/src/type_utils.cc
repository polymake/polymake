/* Copyright (c) 1997-2018
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

#include "polymake/type_utils.h"
#include <cxxabi.h>
#include <cstring>
#include <cstdlib>

namespace polymake {

std::string legible_typename(const std::type_info& ti)
{
   return legible_typename(ti.name());
}

std::string legible_typename(const char* typeid_name)
{
  int status;
  char* dem_buffer=abi::__cxa_demangle(typeid_name, nullptr, nullptr, &status);
  const char* dem=dem_buffer;
  if (status) return typeid_name;
  std::string name;
  while (const char* e=strstr(dem, "polymake::")) {
    name.append(dem, e);
    dem=e+10;
    if (strncmp(dem, "test::", 6)==0) dem+=6;
  }
  name.append(dem);
#ifdef __clang__
  size_t stdver=0;
  while ((stdver=name.find("__1::", stdver)) != name.npos) {
    name.erase(stdver, 5);
  }
#endif
  std::free(dem_buffer);
  return name;
}

}

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
