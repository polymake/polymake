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

#include "polymake/Map.h"
#include "polymake/ideal/internal/singularTermOrderData.h"

#include "polymake/ideal/internal/singularInclude.h"

namespace polymake { 
namespace ideal {
namespace singular {

class SingularTermOrderMap {

  Map<std::pair<int, SingularTermOrderData<Matrix<Int>>>, idhdl> matrixOrderings;
  Map<std::pair<int, SingularTermOrderData<Vector<Int>>>, idhdl> vectorOrderings;
  Map<std::pair<int, SingularTermOrderData<std::string>>, idhdl> singularOrderings;

public:
    // Assignment operators:
    idhdl& operator[] (const std::pair<int, SingularTermOrderData<Matrix<Int>>>& pair)
    {
        return matrixOrderings[pair];
    }
    idhdl& operator[] (const std::pair<int, SingularTermOrderData<Vector<Int>>>& pair)
    {
        return vectorOrderings[pair];
    }
    idhdl& operator[] (const std::pair<int, SingularTermOrderData<std::string>>& pair)
    {
        return singularOrderings[pair];
    }
    // Existence operators:
    bool exists(const std::pair<int, SingularTermOrderData<Matrix<Int>>>& pair)
    {
        return matrixOrderings.exists(pair);
    }
    bool exists(const std::pair<int, SingularTermOrderData<Vector<Int>>>& pair)
    {
        return vectorOrderings.exists(pair);
    }
    bool exists(const std::pair<int, SingularTermOrderData<std::string>>& pair)
    {
        return singularOrderings.exists(pair);
    }
};

} // end namespace singular
} // end namespace ideal
} // end namespace polymake


