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


// polymake includes
#include "polymake/client.h"
#include "polymake/Map.h"
#include "polymake/ListMatrix.h"

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


