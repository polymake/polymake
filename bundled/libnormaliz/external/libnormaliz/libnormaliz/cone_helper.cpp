/*
 * Normaliz
 * Copyright (C) 2007-2014  Winfried Bruns, Bogdan Ichim, Christof Soeger
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As an exception, when this program is distributed through (i) the App Store
 * by Apple Inc.; (ii) the Mac App Store by Apple Inc.; or (iii) Google Play
 * by Google Inc., then that store may impose any digital rights management,
 * device limits and/or redistribution restrictions that are required by its
 * terms of service.
 */

#ifdef NMZ_MIC_OFFLOAD
#pragma offload_attribute (push, target(mic))
#endif

#include "libnormaliz/cone_helper.h"
#include <vector>

namespace libnormaliz {
using std::vector;

//---------------------------------------------------------------------------

// determines the maximal subsets in a vector of subsets given by their indicator vectors
// result returned in is_max_subset -- must be initialized outside
// only set to false in this routine
// if a set occurs more than once, only the last instance is recognized as maximal
void maximal_subsets(const vector<vector<bool> >& ind, vector<bool>& is_max_subset) {

    if(ind.size()==0)
        return;

    size_t nr_sets=ind.size();
    size_t card=ind[0].size();
    vector<key_t> elem(card);

    for (size_t i = 0; i <nr_sets; i++) {
        if(!is_max_subset[i])  // already known to be non-maximal
            continue;

        size_t k=0; // counts the number of elements in set with index i
        for (size_t j = 0; j <card; j++) {
            if (ind[i][j]) {
                elem[k]=j;
                k++;
            }
        }

        for (size_t j = 0; j <nr_sets; j++) {
            if (i==j || !is_max_subset[j] ) // don't compare with itself or something known not to be maximal
                continue;
            size_t t;
            for (t = 0; t<k; t++) {
                if (!ind[j][elem[t]])
                    break; // not a superset
            }
            if (t==k) { // found a superset
                is_max_subset[i]=false;
                break; // the loop over j
            }
        }
    }
}

//---------------------------------------------------------------------------

#ifdef NMZ_MIC_OFFLOAD
#pragma offload_attribute (pop)
#endif

} //end namespace libnormaliz
