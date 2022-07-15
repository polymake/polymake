
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
#ifdef NMZ_HASHLIBRARY
#include <hash-library/sha256.h>
#endif

#include <iostream>
#include "general.h"
#include "libnormaliz/nmz_hash.h"
// hash-library by Stephan Brumme
// https://create.stephan-brumme.com/hash-library/
// https://github.com/stbrumme/hash-library

namespace libnormaliz {

using namespace std;

// Compute SHA-256 hash of a string, excluding final zero
// Return hex-values as string over '0',...,'9','a',...,'f'
// return string of 32 zeros as default if hash-library is not present
string sha256str(const string& text, bool verbose) {
#ifdef NMZ_HASHLIBRARY
    SHA256 sha256;
    return sha256(text);
#else
    if (verbose)
        verboseOutput() << "sha256str called but hash-library not present; "
                           "returning default value."
                        << endl;
    // string s = "00000000000000000000000000000000"; // 32 zeros
    string s = "0";
    return s;
#endif
}

// Compute SHA-256 hash of a string, excluding final zero
// Return as vector<unsigned char> of size sha256.HashBytes (== 32)
vector<unsigned char> sha256hexvec(const string& text, bool verbose) {
#ifdef NMZ_HASHLIBRARY
    SHA256 sha256;
    sha256.add(text.c_str(), text.size());
    unsigned char rawHash[sha256.HashBytes];
    sha256.getHash(rawHash);
    int s = sizeof(rawHash) / sizeof(rawHash[0]);
    // assuming sizeof(unsigned char) == 1, then s == sha256.HashBytes == 32
    vector<unsigned char> v(rawHash, rawHash + s);
    return v;
#else
    if (verbose)
        verboseOutput() << "sha256hexvec called but hash-library not present; "
                           "returning default value."
                        << endl;
    // vector<unsigned char> v(32, '0');
    vector<unsigned char> v(1, '0');
    // vector<unsigned char> v;
    // cout << v.size() << endl;
    return v;
#endif
}
}  // namespace libnormaliz
