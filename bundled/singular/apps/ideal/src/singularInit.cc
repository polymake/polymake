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

#include <dlfcn.h>

#include "polymake/ideal/singularInit.h"
#include "polymake/ideal/internal/singularUtils.h"

namespace polymake {
namespace ideal {
namespace singular {



   bool singular_initialized = 0;
   Map<std::string, idhdl> singular_function_map;
   Map<std::string, bool> loaded_libraries;

void load_library(std::string lib){
   init_singular();
   if (loaded_libraries.exists(lib)) 
      return;
   sleftv arg,r1,r2;
   // load the singular library lib:
   memset(&arg,0,sizeof(arg));
   memset(&r1,0,sizeof(r1));
   memset(&r2,0,sizeof(r2));
   arg.rtyp=STRING_CMD;
   arg.data=omStrDup(lib.c_str());
   r2.rtyp=LIB_CMD;
   int err=iiExprArith2(&r1,&r2,'(',&arg);
   if (err) {
      printf("interpreter returns %d\n",err);
      throw std::runtime_error("*** singular: loading "+lib+" failed ***");
   }
   loaded_libraries[lib]=1;
}

void singular_error_handler(const char* error)
{
   errorreported = 0;
   throw std::runtime_error(error);
}


// Initialize Singular:
void init_singular() 
{
   if(singular_initialized)
      return;
#if POLYMAKE_DEBUG
   cerr << "*** trying to determine path of libsingular ***" << endl;
#endif
   Dl_info dli;
   if (!dladdr((void*)&siInit,&dli)) {
      throw std::runtime_error("*** could not find symbol from libsingular ***");
   }
#if POLYMAKE_DEBUG
   cerr << "*** found libsingular at: " << dli.dli_fname << endl;
#endif
   
   char* cpath = omStrDup(dli.dli_fname);
   siInit(cpath);
   WerrorS_callback = &singular_error_handler;
   
#if POLYMAKE_DEBUG
   cerr << "*** singular siInit done. ***" << endl;
#else
   // make singular library loading quiet
   si_opt_2 &= ~Sy_bit(V_LOAD_LIB);
   si_opt_2 &= ~Sy_bit(V_REDEFINE);
#endif
   singular_initialized = 1;
}

// This function returns the idhdl of the function to be used.
// If the handle does not exist the function is looked up and the handle
// is created.
// All handles are stored globally.
idhdl get_singular_function(std::string s) {
   if(!singular_function_map.exists(s)) {
      // now, get the procedure to call
      idhdl function=ggetid(s.c_str());
      if (function==NULL)
         throw std::runtime_error("singular function not found: "+s);
      singular_function_map[s] = function;
   }
   return singular_function_map[s];
}

} // end namespace singular

UserFunction4perl("# @category Singular interface"
                  "# Loads a SINGULAR library"
                  "# @param String s",
                  &singular::load_library, "load_singular_library($)");


} // end namespace ideal
} // end namespace polymake
