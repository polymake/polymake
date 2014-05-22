/* Copyright (c) 2012
   by authors as mentioned on:
   https://github.com/lkastner/polymake_algebra/wiki/Authors

   Project home:
   https://github.com/lkastner/polymake_algebra

   For licensing we cite the original Polymake code:

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version: http://www.gnu.org/licenses/gpl.txt.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*/

#ifndef POLYMAKE_IDEAL_SINGULAR_INIT_H
#define POLYMAKE_IDEAL_SINGULAR_INIT_H

// Singular includes
#include <Singular/libsingular.h>
#include <kernel/GBEngine/stairc.h>
#include <coeffs/mpr_complex.h>

#include <Singular/fevoices.h>

// polymake includes
#include "polymake/client.h"
// #include "polymake/Array.h"
#include "polymake/Ring.h"
#include "polymake/Map.h"
#include "polymake/ListMatrix.h"
// #include "polymake/Polynomial.h"
#include "polymake/internal/shared_object.h"

namespace polymake { 
namespace ideal {
namespace singular {
   
   extern bool singular_initialized;

   // Storing the handles for the Singular functions globally.
   extern Map<std::string, idhdl> singular_function_map;
   // Store loaded libraries.
   extern Map<std::string, bool> loaded_libraries;


   // Init functions:
   void init_singular();
   idhdl get_singular_function(std::string s);
   void load_library(std::string lib);
   void singular_error_handler(const char* error);



} // end namespace singular
} // end namespace ideal
} // end namespace polymake


#endif
