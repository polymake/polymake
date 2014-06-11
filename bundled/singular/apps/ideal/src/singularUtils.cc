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

#include <dlfcn.h>

#include "polymake/ideal/singularFull.h"

namespace polymake {
namespace ideal {
namespace singular {

perl::ListReturn singular_get_var(const std::string varname){
   init_singular();
   int nest = myynest;
   myynest = 1;
   idhdl var = ggetid(omStrDup(varname.c_str()));
   myynest = nest;
   if (var == NULL)
      throw std::runtime_error("singular_get_int: could not find variable '"+varname+"'");
   perl::ListReturn res;
   
   switch(var->typ){
      case INT_CMD:
         {
            Integer i((long) IDDATA(var));
            res << i;
            break;
         }
      case INTVEC_CMD:
         {
            const intvec *iv = (intvec*) IDDATA(var);
            Vector<Integer> pmvec(iv->length());
            int i = 0;
            for(Entire<Vector<Integer> >::iterator it = entire(pmvec); !it.at_end(); ++it, ++i){
               *it = (*iv)[i];
            }
            res << pmvec;
            break;
         }
      case INTMAT_CMD:
         {
            const intvec *iv = (intvec*) IDDATA(var);
            Matrix<Integer> pmmat(iv->rows(), iv->cols());
            
            int i = 0;
            for(Integer *elem = concat_rows(pmmat).begin(); elem != concat_rows(pmmat).end(); ++elem, ++i){
               *elem = (*iv)[i];
            }
            res << pmmat;
            break;
         }
      /* case POLY_CMD:
         {
            break;
         }
      case STRING_CMD:
         break;*/
      default:
         throw std::runtime_error("singular_get_var does not work for this variable type.");
   }
   return res;
}


void singular_eval(const std::string cmd){
   init_singular();
   int nest = myynest;
   currentVoice=feInitStdin(NULL);
   myynest = 1;
   // the return is needed to stop the interpreter
   int err=iiAllStart(NULL,omStrDup((cmd+";return();").c_str()),BT_proc,0);
   myynest = nest;
   if (err) {
      errorreported = 0; // reset error handling
      std::ostringstream os;
      os << "singular interpreter returns " << err;
      throw std::runtime_error(os.str());
   }
}


long singular_get_int(const std::string varname){
   init_singular();
   int nest = myynest;
   myynest = 1;
   idhdl var = ggetid(omStrDup(varname.c_str()));
   myynest = nest;
   if (var == NULL)
      throw std::runtime_error("singular_get_int: could not find variable '"+varname+"'");
   if (var->typ != INT_CMD)
      throw std::runtime_error("singular_get_int: variable '"+varname+"' not an int");
   return (long) IDDATA(var);
}

} // end namespace singular

UserFunction4perl("CREDIT Singular\n\n"
                  "# @category Algebra"
                  "# Executes given string with Singular"
                  "# @param String s",
                  &singular::singular_eval, "singular_eval($)");

UserFunction4perl("CREDIT Singular\n\n"
                  "# @category Algebra"
                  "# Retrieves an int variable from 'Singular'"
                  "# @param String s",
                  &singular::singular_get_int, "singular_get_int($)");

UserFunction4perl("CREDIT Singular\n\n"
                  "# @category Algebra"
                  "# Retrieves a variable from 'Singular'"
                  "# @param String s variable name"
                  "# @return perl::ListReturn",
                  &singular::singular_get_var, "singular_get_var($)");

} // end namespace ideal
} // end namespace polymake