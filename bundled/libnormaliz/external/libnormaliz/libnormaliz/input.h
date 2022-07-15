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

#include <iostream>
#include <cctype>  // std::isdigit
#include <limits>  // numeric_limits
#include <fstream>
#include <map>
#include <string>

#include "libnormaliz/options.h"
#include "libnormaliz/input_type.h"
// #include "libnormaliz/list_and_map_operations.h"
#include "libnormaliz/cone_property.h"
#include "libnormaliz/matrix.h"

#ifndef NORMALIZ_INPUT_H
#define NORMALIZ_INPUT_H

namespace libnormaliz {

template <typename Number>
InputMap<Number> readNormalizInput(istream& in,
                                   OptionsHandler& options,
                                   map<NumParam::Param, long>& num_param_input,
                                   string& polynomial,
                                   renf_class_shared& number_field);

// here defined for use in interfaces
void read_number_field_strings(istream& in, string& mp_string, string& indet, string& emb_string);

template <typename Integer>
Matrix<Integer> readMatrix(const string project);

inline mpq_class mpq_read(istream& in) {
    const string numeric = "+-0123456789/.e";
    in >> std::ws;
    string s;
    char c;
    bool is_float = false;
    while (in.good()) {
        c = in.peek();
        size_t pos = numeric.find(c);
        if (pos == string::npos)
            break;
        if (pos > 12)
            is_float = true;
        in >> c;
        s += c;
    }

    if (s == "") {
        string t;
        t += c;
        throw BadInputException("Empty number string preceding character " + t +
                                ". Most likely mismatch of amb_space and matrix format or forgotten keyword.");
    }

    // cout << "t " << s << " f " << is_float << endl;

    if (s[0] == '+')
        s = s.substr(1);  // must suppress + sign for mpq_class

    try {
        if (!is_float) {
            return mpq_class(s);
        }
        else
            return dec_fraction_to_mpq(s);
    } catch (const std::exception& e) {
        cerr << e.what() << endl;
        throw BadInputException("Illegal number string " + s + " in input, Exiting.");
    }
}

// To be used from other sources
inline void string2coeff(mpq_class& coeff, const string& s) {
    // cout << "SSSSSS " << s << endl;

    const string numeric = "+-0123456789/.e ";  // must allow blank
    for (auto& c : s) {
        size_t pos = numeric.find(c);
        if (pos == string::npos)
            throw BadInputException("Illegal character in numerical string");
    }

    stringstream sin(s);
    coeff = mpq_read(sin);
    // coeff=mpq_class(s);
}
}  // namespace libnormaliz

#endif
