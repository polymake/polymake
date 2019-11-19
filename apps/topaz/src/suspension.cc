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
#include "polymake/hash_set"
#include <sstream>

namespace polymake { namespace topaz {

perl::Object suspension(perl::Object p_in, int k, perl::OptionSet options)
{
        Array< Set<int> > C = p_in.give("FACETS");
        const int n_v = p_in.give("N_VERTICES");
        Array<std::string> apex_labels;

        if (options["apex_labels"] >> apex_labels) {
                if (k==0) {
                        k=apex_labels.size();
                        if (k%2) throw std::runtime_error("suspension - must specify an even number of apex labels");
                        if (!k) throw std::runtime_error("suspension - empty apex label list");
                        k/=2;
                } else if (k*2 != apex_labels.size())
                        throw std::runtime_error("suspension - wrong number of apex labels");
        } else {
                assign_max(k,1);
        }

        const sequence new_V(n_v,2*k);
        const int orig_size=C.size();
        C.resize(orig_size * (1<<k));

        sequence::const_iterator s_it=new_V.begin();
        for (int i=0; i<k; ++i) {
                Array< Set<int> > SUS(C.size());
                
                const int v1=*s_it;     ++s_it;
                const int v2=*s_it;     ++s_it;
                for (int j=0, j_limit=orig_size*(1<<i); j < j_limit; ++j) {
                        SUS[2*j] = C[j]+v1;
                        SUS[2*j+1] = C[j]+v2;
                }
                C=SUS;
        }
        perl::Object p_out("SimplicialComplex");
        p_out.set_description() << k << "-suspension of " << p_in.name() << endl;
        p_out.take("FACETS") << C;

        if (!options["no_labels"]) {
                Array<std::string> L = p_in.give("VERTEX_LABELS");
                hash_set<std::string> old_L(L.begin(), L.end());

                if (!apex_labels.empty()) {
                        for (auto l=entire(apex_labels); !l.at_end(); ++l)
                                if (! old_L.insert(*l).second)
                                        throw std::runtime_error("suspension - apex labels not unique");

                        L.append(k*2, apex_labels.begin());

                } else {

                        int gen=0;
                        std::string gen_suffix="";
                        bool unique;
                        L.resize(n_v+2*k);
                        do {
                                unique=true;
                                for (int count=0; count<2*k; ++count) {
                                        std::ostringstream label;
                                        label << "apex" << gen_suffix;
                                        if (k>1) label << '_' << count/2;
                                        label << (count%2 ? '-' : '+');
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
                  "# Produce the __//k//-suspension__ over a given simplicial complex."
                  "# @param SimplicialComplex complex"
                  "# @param Int k default value is 1"
                  "# @option Array<String> labels for the apices."
                  "#  By default apices are labeled with ''apex_0+'', ''apex_0-'', ''apex_1+'', etc."
                  "#  If one of the specified labels already exists, a unique one is made"
                  "#  by appending ''_i'' where //i// is some small number."
                  "# @option Bool no_labels Do not create [[VERTEX_LABELS]]. default: 0"
                  "# @return SimplicialComplex",
                  &suspension, "suspension(SimplicialComplex; $=0, { apex_labels => undef, no_labels => 0 })");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
