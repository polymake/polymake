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

#ifndef POLYMAKE_POLYNOMIALVARNAMES_H
#define POLYMAKE_POLYNOMIALVARNAMES_H

#include "polymake/Array.h"
#include "polymake/vector"
#include <string>

namespace pm {

class PolynomialVarNames
{
public:
  /// the default naming scheme depending on the coefficient nesting level:
  /// x, y, z, u, v, w for nesting levels 0..6 and t for anything above
  static constexpr char default_varname(int nesting_level)
  {
    return static_cast<char>(nesting_level < 3 ? 'x' + nesting_level : nesting_level < 6 ? 'v' + nesting_level-3 : 't');
  }

  /// set the default naming scheme
  explicit PolynomialVarNames(int nesting_level);

  /// set the desired names
  /// the last name in the sequence works as a pattern for all excess variables:
  /// it is concatenated with "_NN" where NN is the variable index less the number of fixed names
  explicit PolynomialVarNames(const Array<std::string>& names)
    : explicit_names(names) {}

  /// get the current list of explicit names
  const Array<std::string>& get_names() const { return explicit_names; }

  /// get the name of the variable with the given index
  /// @param index variable index
  /// @param n_vars number of variables in the polynomial class
  /// if the index exceeds the number of the explicit names, the variable name will be generated
  /// according to the pattern "{LastExplicitName}_{IndexExcess}"
  /// if the index refers to the last explicit name and \a n_vars exceeds the number of explicit names,
  /// the variable name is "{LastExplicitName}_0"
  const std::string& operator() (Int index, Int n_vars) const;

  /// set the explicit variable names
  void set_names(const Array<std::string>& names);

  void swap(PolynomialVarNames& other);

private:
  /// explicit variable names set by the application
  Array<std::string> explicit_names;
  /// the cache for automatically generated names of variables with indexes beyond the explicit list
  mutable std::vector<std::string> generated_names;
};

}

namespace polymake {
   using pm::PolynomialVarNames;
}

#endif // POLYMAKE_POLYNOMIALVARNAMES_H
