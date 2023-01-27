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

#include <stdexcept>

namespace pm {

class linalg_error : public std::runtime_error {
public:
   linalg_error(const std::string& what) : std::runtime_error(what) {}
};

class degenerate_matrix : public linalg_error {
public:
   degenerate_matrix() : linalg_error("matrix not invertible") {}
   degenerate_matrix(const std::string& what) : linalg_error(what) {}
};

class infeasible : public linalg_error {
public:
   infeasible() : linalg_error("infeasible system of linear equations or inequalities") {}
   infeasible(const std::string& what) : linalg_error(what) {}
};

class underdetermined : public linalg_error {
public:
   underdetermined() : linalg_error("underdetermined system of linear equations") {}
   underdetermined(const std::string& what) : linalg_error(what) {}
};


} // end namespace pm

namespace polymake {
   using pm::linalg_error;
   using pm::degenerate_matrix;
   using pm::infeasible;
   using pm::underdetermined;
}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
