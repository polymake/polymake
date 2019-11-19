/* Copyright (c) 1997-2019
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

#include "polymake/client.h"
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/FacetList.h"
#include "polymake/hash_map"
#include <sstream>

namespace polymake { namespace topaz {

perl::Object h_induced_quotient(perl::Object p_in,const Set<int>& V_in,perl::OptionSet options)
{
   perl::Object p_out("SimplicialComplex");

   const Array< Set<int> > C = p_in.give("FACETS");
   const bool no_labels=options["no_labels"];
   const int n_vert = p_in.give("N_VERTICES");

   int apex=n_vert;

   if (!no_labels) {
      Array<std::string> L = p_in.give("VERTEX_LABELS");
      L.resize(n_vert+1);

      const std::string apex_label;
      if(!(options["apex"]>>L[n_vert])) {
         std::ostringstream label;
         label << "apex";
         std::string l=label.str(), ll=l;
         // creating vertex map
         hash_map<std::string,int> map(L.size());
         int count = 0;
         for (const auto& label_in : L)
            map[label_in] = count++;
         // test if ll is unique
         int i=0;
         while (map.find(ll) != map.end()) {
            ++i;
            label.str("");
            label << l << "_" << i;
            ll=label.str();
         }

         L[n_vert] = ll;
      }
      p_out.take("VERTEX_LABELS") << L;
   }

   // checking input
   if (V_in.front()<0 || V_in.back()>n_vert-1)
      throw std::runtime_error("h_induced_quotient: Specified vertices are not contained in VERTICES.");

   // computing the subcomplex
   FacetList Quotient(C.begin(), C.end());
   for (auto it=entire(C); !it.at_end(); ++it) {
      Set<int> S=V_in*(*it);
      if (!S.empty()) {
         S+=apex;
         Quotient.insertMax(S);
      }
   }


   p_out.set_description()<<"A homotopy equivalent complex to " << p_in.name() << " modulo the subcomplexed induced by the vertices " <<V_in << "."<<endl;

   p_out.take("FACETS") << Quotient;

   return p_out;
}

UserFunction4perl("# @category Producing a new simplicial complex from others"
                  "# Let //C// be the given simplicial and A the subcomplex induced by"
                  "# the given //vertices//. Then this function produces a simplicial complex"
                  "# homotopy equivalent to //C// mod A by adding the cone over A with"
                  "# apex a to //C//."
                  "# The label of the apex my be specified via the option //apex//."
                  "# @option Bool no_labels Do not create [[VERTEX_LABELS]]. default: 0"
                  "# @option String apex"
                  "# @param SimplicialComplex C"
                  "# @param Set<Int> vertices"
                  "# @return SimplicialComplex",
                  &h_induced_quotient,"h_induced_quotient(SimplicialComplex, $ { no_labels => 0, apex => undef})");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
