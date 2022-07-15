/*
 * Normaliz
 * Copyright (C) 2007-2022  W. Bruns, B. Ichim, Ch. Soeger, U. v. d. Ohe
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * As an exception, when this program is distributed through (i) the App Store
 * by Apple Inc.; (ii) the Mac App Store by Apple Inc.; or (iii) Google Play
 * by Google Inc., then that store may impose any digital rights management,
 * device limits and/or redistribution restrictions that are required by its
 * terms of service.
 */

//---------------------------------------------------------------------------
#ifndef LIBNORMALIZ_LIST_OPERATIONS_H
#define LIBNORMALIZ_LIST_OPERATIONS_H

//---------------------------------------------------------------------------

#include <vector>
#include <list>
#include <set>
#include <ostream>
#include <map>
#include <iostream>

#include "libnormaliz/general.h"
#include "libnormaliz/matrix.h"
#include "libnormaliz/dynamic_bitset.h"

namespace libnormaliz {
using namespace std;

//---------------------------------------------------------------------------
//                          Data access
//---------------------------------------------------------------------------

template <typename T>
std::ostream& operator<<(std::ostream& out, const list<T>& l) {
    for (const auto& i : l) {
        out << i << " ";
    }
    out << std::endl;
    return out;
}

//---------------------------------------------------------------------------
//                         List operations
//---------------------------------------------------------------------------
/*
template <typename Integer>
vector<Integer> l_multiplication(const list<vector<Integer> >& l, const vector<Integer>& v) {
    int s = l.size();
    vector<Integer> p(s);
    s = 0;
    for (const auto& i : l) {
        p[s++] = v_scalar_product(*i, v);  // maybe we loose time here?
    }
    return p;
}

//---------------------------------------------------------------------------

template <typename Integer>
list<vector<Integer> > l_list_x_matrix(const list<vector<Integer> >& l, const Matrix<Integer>& M) {
    list<vector<Integer> > result;
    vector<Integer> p;
    for (const auto& i : l) {
        p = M.VxM(i);
        result.push_back(p);
    }
    return result;
}
//---------------------------------------------------------------------------

template <typename Integer>
void l_cut(list<vector<Integer> >& l, int size) {
    for (auto& i : l) {
        i.resize(size);
    }
}
*/

/*
template <typename Integer>
void l_cut_front(list<vector<Integer> >& l, int size);
// cuts all the vectors in l to a given size, maintaining the back
*/

//---------------------------------------------------------------------------

template <typename T>
void random_order(list<T>& LL) {
    vector<typename list<T>::iterator> list_order;
    size_t nrLL = LL.size();
    list_order.reserve(nrLL);
    auto p = LL.begin();
    for (size_t k = 0; k < nrLL; ++k, ++p) {
        list_order.push_back(p);
    }
    for (size_t k = 0; k < 10 * nrLL; ++k) {
        swap(list_order[rand() % nrLL], list_order[rand() % nrLL]);
    }
    list<T> new_order;
    for (size_t k = 0; k < nrLL; ++k) {
        new_order.push_back(*list_order[k]);
    }
    LL.clear();
    LL.splice(LL.begin(), new_order);
}

//---------------------------------------------------------------------------

template <typename T>
void random_order(list<T>& LL, typename list<T>::iterator from, typename list<T>::iterator to) {
    list<T> MM;
    MM.splice(MM.begin(), LL, from, to);
    random_order(MM);
    LL.splice(LL.begin(), MM);
}

// formerly map_operations.h

template <typename key, typename T>
std::ostream& operator<<(std::ostream& out, const map<key, T>& M) {
    for (const auto& it : M) {
        out << it.first << ":" << it.second << "  ";
    }
    out << std::endl;
    return out;
}

//---------------------------------------------------------------------------

template <typename key>
bool contains(const set<key>& m, const key& k) {
    return (m.find(k) != m.end());
}

//---------------------------------------------------------------------------

template <typename key, typename T>
bool contains(const map<key, T>& m, const key& k) {
    return (m.find(k) != m.end());
}

//---------------------------------------------------------------------------

template <typename key, typename T>
map<key, T> count_in_map(const vector<key>& v) {
    map<key, T> m;
    T size = v.size();
    for (T i = 0; i < size; ++i) {
        m[v[i]]++;
    }
    return m;
}

template <typename key, typename T>
vector<key> to_vector(const map<key, T>& M) {
    vector<key> v;
    for (const auto& it : M) {
        for (T i = 0; i < it.second; i++) {
            v.push_back(it.first);
        }
    }
    return v;
}

// A vector can be considered as a map index --> value.
// This function inverts the assignment, provided the entries of the vector
// are pairwise different
// If entries are equal, the highets index is chosen.
// The "injectivity" can be checked outside by comparing sizes.
template <typename T>
map<T, key_t> map_vector_to_indices(const vector<T>& v) {
    map<T, key_t> index_map;
    for (size_t i = 0; i < v.size(); ++i) {
        index_map[v[i]] = i;
    }
    return index_map;
}

//--------------------------------------------------------------------------
// remove all entries that appear exactly twice (or with even multiplicity)
// it must be possible to sorten the list

template <typename T>
void remove_twins(list<T>& L) {
    L.sort();
    auto S = L.begin();  // remove all subfacets that appear twice
    for (; S != L.end();) {
        auto del = S;
        ++del;
        if (del != L.end() && *S == *del) {
            S = L.erase(S);
            S = L.erase(S);
        }
        else
            S++;
    }
}

//--------------------------------------------------------------------------
// remove all entries whose "first" appears twice (or with even multiplicity)
// it must be possible to sorten the list
// L must be a list of pairs

template <typename T>
void remove_twins_in_first(list<T>& L, bool is_sorted = false) {
    if (L.size() <= 1)
        return;

    if (!is_sorted)
        L.sort();

    auto S = L.begin();  // remove all items that appear twice in first component
    for (; S != L.end();) {
        auto del = S;
        del++;
        ;
        if (S->first == del->first) {
            L.erase(S);
            S = L.erase(del);
        }
        else
            S++;
    }
}

// The following is correct, but it is significantly slower
// than the combination of merge and remove remove_twins_in_first.
template <typename T>
void merge_first_unique(list<T>& L, list<T>& R) {
    list<T> result;

    auto L_it = L.begin();
    auto R_it = R.begin();
    while (true) {
        if (L_it == L.end()) {
            result.splice(result.end(), R, R_it, R.end());
            break;
        }
        if (R_it == R.end()) {
            result.splice(result.end(), L, L_it, L.end());
            break;
        }
        if (L_it->first == R_it->first) {
            L_it++;
            R_it++;
            continue;
        }
        if (L_it->first < R_it->first) {
            auto L_res = L_it;
            L_it++;
            result.splice(result.end(), L, L_res);
            continue;
        }
        auto R_res = R_it;
        R_it++;
        result.splice(result.end(), R, R_res);
    }

    swap(L, result);
    R.clear();
}

template <typename T>
void test_print(list<T>& L) {
    cout << "=====================" << endl;

    for (auto& E : L)
        cout << L->first << "    " << L->second << endl;

    cout << "=====================" << endl;
}

}  // namespace libnormaliz

//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
