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

#include "polymake/Ring.h"
#include <sstream>

namespace pm {

const Ring_base::id_type* Ring_base::find_by_key(repo_by_key_type& repo_by_key, const ring_key_type& key)
{
   id_type& id = repo_by_key[key];
   if (id == 0) {
      id = ++global_id;
   }
   return &id;
}

const Ring_base::id_type* Ring_base::find_by_arity(repo_by_key_type& repo_by_key, repo_by_arity_type& repo_by_arity, const simplified_ring_key& s_key)
{
   const id_type* &id_ptr = repo_by_arity[s_key];
   if (id_ptr == NULL) {
      ring_key_type full_key;
      full_key.first.resize(s_key.arity);
      full_key.second = s_key.coeff_ring_id;

      for (int i=0; i<s_key.arity; ++i) {
         std::ostringstream tmp;
         tmp << s_key.varname << i;
         full_key.first[i] = tmp.str();
      }

      id_ptr = find_by_key(repo_by_key, full_key);
   }
   return id_ptr;
}

Ring_base::id_type Ring_base::global_id = 0;

}

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
