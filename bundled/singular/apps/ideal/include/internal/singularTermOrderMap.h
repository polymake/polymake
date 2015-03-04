/* Copyright (c) 1997-2015
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

#ifndef POLYMAKE_IDEAL_SINGULAR_TERM_ORDER_MAP_H
#define POLYMAKE_IDEAL_SINGULAR_TERM_ORDER_MAP_H

#include "polymake/Map.h"
#include "polymake/ideal/internal/singularTermOrderData.h"

#include <Singular/libsingular.h>

namespace polymake { 
namespace ideal {
namespace singular {

class SingularTermOrderMap{
   
   Map<std::pair<Ring<>::id_type, SingularTermOrderData<Matrix<int> > >, idhdl> matrixOrderings;
   Map<std::pair<Ring<>::id_type, SingularTermOrderData<Vector<int> > >, idhdl> vectorOrderings;
   Map<std::pair<Ring<>::id_type, SingularTermOrderData<std::string > >, idhdl> singularOrderings;

public:
    // Assignment operators:
    idhdl& operator [] (const std::pair<Ring<>::id_type, SingularTermOrderData<Matrix<int> > > &pair){
        return matrixOrderings[pair];
    }
    idhdl& operator [] (const std::pair<Ring<>::id_type, SingularTermOrderData<Vector<int> > > &pair){
        return vectorOrderings[pair];
    }
    idhdl& operator [] (const std::pair<Ring<>::id_type, SingularTermOrderData<std::string > > &pair){
        return singularOrderings[pair];
    }
    // Existence operators:
    bool exists (const std::pair<Ring<>::id_type, SingularTermOrderData<Matrix<int> > > &pair){
        return matrixOrderings.exists(pair);
    }
    bool exists (const std::pair<Ring<>::id_type, SingularTermOrderData<Vector<int> > > &pair){
        return vectorOrderings.exists(pair);
    }
    bool exists (const std::pair<Ring<>::id_type, SingularTermOrderData<std::string > > &pair){
        return singularOrderings.exists(pair);
    }
};

} // end namespace singular
} // end namespace ideal
} // end namespace polymake

#endif

