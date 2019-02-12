/* Copyright (c) 1997-2018
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

#include "polymake/client.h"
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/hash_set"
#include <sstream>

namespace polymake { namespace topaz {
  
perl::Object cone(perl::Object p_in, int k, perl::OptionSet options)
{
   Array< Set<int> > C = p_in.give("FACETS");
   const int n_v = p_in.give("N_VERTICES");
   Array<std::string> apex_labels;

   if (options["apex_labels"] >> apex_labels) {
      if (k==0) {
         k=apex_labels.size();
         if (!k) throw std::runtime_error("cone - empty apex label list");
      } else if (k != apex_labels.size())
         throw std::runtime_error("cone - wrong number of apex labels");
   } else {
      assign_max(k,1);
   }

   for (auto it=entire(C); !it.at_end(); ++it)
      *it += sequence(n_v,k);

   perl::Object p_out("SimplicialComplex");
   p_out.set_description() << k << "-cone of " << p_in.name() << endl;
   p_out.take("FACETS") << C;

   if (!options["no_labels"]) {
      Array<std::string> L = p_in.give("VERTEX_LABELS");
      hash_set<std::string> old_L(L.begin(), L.end());

      if (!apex_labels.empty()) {
         for (auto l=entire(apex_labels); !l.at_end(); ++l)
            if (! old_L.insert(*l).second)
               throw std::runtime_error("cone - apex labels not unique");

         L.append(k, apex_labels.begin());

      } else {

         int gen=0;
         std::string gen_suffix="";
         bool unique;
         L.resize(n_v+k);
         do {
            unique=true;
            for (int count=0; count<k; ++count) {
               std::ostringstream label;
               label << "apex" << gen_suffix;
               if (k>1) label << '_' << count;

               std::string l=label.str();
               if (old_L.find(l) != old_L.end()) {
                  ++gen;
                  label.str("");
                  label << '_' << gen;
                  gen_suffix=label.str();
                  unique=false;
                  break;
               }
               L[n_v+count] = l;
            }
         } while (!unique);
      }

      p_out.take("VERTEX_LABELS") << L;
   }

   return p_out;
}

UserFunction4perl("# @category Producing a new simplicial complex from others"
                  "# Produce the //k//-cone over a given simplicial complex."
                  "# @param SimplicialComplex complex"
                  "# @param Int k default is 1"
                  "# @option Array<String> apex_labels labels of the apex vertices."
                  "#  Default labels have the form ''apex_0, apex_1, ...''."
                  "#  In the case the input complex has already vertex labels of this kind,"
                  "#  the duplicates are avoided."
                  "# @option Bool no_labels Do not create [[VERTEX_LABELS]]. default: 0"
                  "# @return SimplicialComplex"
                  "# @example The following creates the cone with two apices over the triangle,"
                  "# with custom apex labels. The resulting complex is the 4-simplex."
                  "# > $c = cone(simplex(2),2,apex_labels=>['foo','bar']);"
                  "# > print $c->FACETS;"
                  "# | {0 1 2 3 4}"
                  "# > print $c->VERTEX_LABELS;"
                  "# | 0 1 2 foo bar",
                  &cone, "cone(SimplicialComplex; $=0, { apex_labels => undef, no_labels => 0 })");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
