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

//---------------------------------------------------------------------------

#ifndef MAP_OPERATIONS_H
#define MAP_OPERATIONS_H

//---------------------------------------------------------------------------

#include <map>
#include <ostream>

namespace libnormaliz {
using std::ostream;

template<typename key, typename T>
ostream& operator<< (ostream& out, const map<key, T> M) {
    typename map<key, T>::const_iterator it;
    for (it = M.begin(); it != M.end(); ++it) {
        out << it->first << ": " << it-> second << "  ";
    }
    out << std::endl;
    return out;
}

//---------------------------------------------------------------------------

template<typename key, typename T>
bool exists_element(const map<key, T>& m, const key& k){
    return (m.find(k) != m.end());
}

//---------------------------------------------------------------------------

template<typename key, typename T>
map<key, T> count_in_map (const vector<key> v) {
    map<key, T> m;
    T size = v.size();
    for (T i = 0; i < size; ++i) {
        m[v[i]]++;
    }
    return m;
}

template<typename key, typename T>
vector<key> to_vector (const map<key, T> M) {
    vector<key> v;
    typename map<key, T>::const_iterator it;
    for (it = M.begin(); it != M.end(); ++it) {
        for (T i = 0; i < it->second; i++) {
            v.push_back(it->first);
        }
    }
    return v;
}

}  //end namespace

//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
