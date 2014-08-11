/* Copyright (c) 1997-2014
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


#ifndef POLYMAKE_IDEAL_SINGULAR_INIT_H
#define POLYMAKE_IDEAL_SINGULAR_INIT_H

// polymake includes
#include "polymake/client.h"
// #include "polymake/Array.h"
#include "polymake/Ring.h"
#include "polymake/Map.h"
#include "polymake/ListMatrix.h"
// #include "polymake/Polynomial.h"

namespace polymake { 
namespace ideal {
namespace singular {
   
   extern bool singular_initialized;

   // Store loaded libraries.
   extern Map<std::string, bool> loaded_libraries;

   // Init functions:
   void init_singular();
   void load_library(std::string lib);
   void singular_error_handler(const char* error);



} // end namespace singular
} // end namespace ideal
} // end namespace polymake


#endif
