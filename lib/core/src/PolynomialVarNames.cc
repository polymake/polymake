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

#include "polymake/PolynomialVarNames.h"

namespace pm {

PolynomialVarNames::PolynomialVarNames(int nesting_level)
  : explicit_names(1, std::string(1, default_varname(nesting_level))) {}

void PolynomialVarNames::set_names(const Array<std::string>& names)
{
  if (names.empty()) throw std::runtime_error("PolynomialVarNames - empty name list");
  generated_names.clear();
  explicit_names = names;
}

const std::string& PolynomialVarNames::operator() (Int index, Int n_vars) const
{
  if (index < 0)
    throw std::runtime_error("PolynomialVarNames - invalid variable index");
  // if the list of explicit names exactly matches the number of variables, the last name is taken verbatim
  if (index + (index+1 < n_vars) < explicit_names.size())
    return explicit_names[index];
  index -= explicit_names.size()-1;
  if (static_cast<size_t>(index) >= generated_names.size()) {
    generated_names.reserve(index+1);
    for (Int i = generated_names.size(); i <= index; ++i)
      generated_names.emplace_back(explicit_names.back()+"_"+std::to_string(i));
  }
  return generated_names[index];
}

void PolynomialVarNames::swap(PolynomialVarNames& other)
{
  explicit_names.swap(other.explicit_names);
  generated_names.swap(other.generated_names);
}

}
