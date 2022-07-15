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

#ifndef LIBNORMALIZ_NMZ_HASHLIBRARY_HPP
#define LIBNORMALIZ_NMZ_HASHLIBRARY_HPP

// We are using "hash-library" by Stephan Brumme
// to compute SHA-256 hashes.
// See
// https://create.stephan-brumme.com/hash-library/
// https://github.com/stbrumme/hash-library

#include <string>
#include <vector>

namespace libnormaliz {

using namespace std;

// Compute SHA-256 hash of a string, excluding final zero
// Return hex-values as string over '0',...,'9','a',...,'f'

string sha256str(const string& text, bool verbose = false);

// Compute SHA-256 hash of a string, excluding final zero
// Return as vector<char> of size sha256.HashBytes (== 32)
vector<unsigned char> sha256hexvec(const string& text, bool verbose = false);

}  // namespace libnormaliz
#endif
