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

#include "polymake/type_utils.h"
#include "polymake/AnyString.h"
#include <cxxabi.h>
#include <cstring>
#include <cstdlib>

namespace polymake {
namespace {

void remove_uninteresting_ns_prefix(std::string& name, const AnyString& prefix)
{
  size_t pos=0;
  while ((pos=name.find(prefix.ptr, pos, prefix.len)) != name.npos) {
    name.erase(pos, prefix.len);
  }
}

}

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
  std::free(dem_buffer);

#ifdef __clang__
  remove_uninteresting_ns_prefix(name, "__1::");
#endif
  remove_uninteresting_ns_prefix(name, "__cxx11::");

  return name;
}

}

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
