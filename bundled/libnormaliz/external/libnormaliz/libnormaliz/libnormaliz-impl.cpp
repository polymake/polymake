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

include "libnormaliz/libnormaliz.cpp"

namespace libnormaliz {

template class Cone<long>;
template class Cone<long long int>;
template class Cone<mpz_class>;

template class Matrix<long>;
template class Matrix<long long int>;
template class Matrix<mpz_class>;

template class Sublattice_Representation<long>;
template class Sublattice_Representation<long long int>;
template class Sublattice_Representation<mpz_class>;

// template class Lineare_Transformation<long>;
// template class Lineare_Transformation<long long int>;
// template class Lineare_Transformation<mpz_class>;

// template Lineare_Transformation<long> Transformation(const Matrix<long>& M);
// template Lineare_Transformation<long long int> Transformation(const Matrix<long long int>& M);
// template Lineare_Transformation<mpz_class> Transformation(const Matrix<mpz_class>& M);

template size_t decimal_length<long>(long);
template size_t decimal_length<long long int>(long long int);
template size_t decimal_length<mpz_class>(mpz_class);

template mpz_class gcd<mpz_class>(const mpz_class& a, const mpz_class& b);
template mpz_class lcm<mpz_class>(const mpz_class& a, const mpz_class& b);
template mpz_class permutations<mpz_class>(const size_t& a, const size_t& b);

template ostream& operator<< <long long>(ostream& out, const vector<long long>& v);
template ostream& operator<< <mpz_class>(ostream& out, const vector<mpz_class>& v);
template ostream& operator<< < vector<mpz_class> >(ostream& out, const vector< vector<mpz_class> >& v);

}
