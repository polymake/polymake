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
#ifndef VECTOR_OPERATIONS_H
#define VECTOR_OPERATIONS_H
//---------------------------------------------------------------------------

#include <vector>
#include <ostream>
#include <list>

#include <libnormaliz/libnormaliz.h>
#include <libnormaliz/integer.h>
#include <libnormaliz/convert.h>

namespace libnormaliz {
using std::vector;

//---------------------------------------------------------------------------
//							Data access
//---------------------------------------------------------------------------

template <typename T>
std::ostream& operator<< (std::ostream& out, const vector<T>& vec) {
    for (size_t i=0; i<vec.size(); ++i) {
        out << vec[i] << " ";
    }
    out << std::endl;
    return out;
}

//---------------------------------------------------------------------------
//					    	Vector operations
//---------------------------------------------------------------------------
template<typename Integer>
Integer v_scalar_product(const vector<Integer>& a,const vector<Integer>& b);

//returns the scalar product of the vector a with the end of the vector b
template<typename Integer>
Integer v_scalar_product_unequal_vectors_end(const vector<Integer>& a,const vector<Integer>& b);

//returns the addition a + b, vectors must be of equal size
template<typename Integer>
vector<Integer> v_add(const vector<Integer>& a,const vector<Integer>& b);
template<typename Integer>
vector<Integer> v_add_overflow_check(const vector<Integer>& a,const vector<Integer>& b);
template<typename Integer>
void v_add_result(vector<Integer>& result, const size_t length, const vector<Integer>& a,const vector<Integer>& b);

//adds b to a reduces the result modulo m, a and b must be reduced modulo m!
template<typename Integer>
vector<Integer>& v_add_to_mod(vector<Integer>& a, const vector<Integer>& b, const Integer& m);

//---------------------------------------------------------------------------
//							abs, gcd and lcm
//---------------------------------------------------------------------------

// takes the absolute value of the elements and returns a reference to the changed vector
template<typename Integer>
vector<Integer>& v_abs(vector<Integer>& v);

// returns the vector of absolute values, does not change the argument
template<typename Integer>
vector<Integer> v_abs_value(vector<Integer>& v);

//returns gcd of the elements of v
template<typename Integer>
Integer v_gcd(const vector<Integer>& v);

//returns lcm of the elements of v
template<typename Integer>
Integer v_lcm(const vector<Integer>& v);

//returns lcm of the elements of v from index k up to index j
template<typename Integer>
Integer v_lcm_to(const vector<Integer>& v,const size_t k, const size_t j);

//divides the elements by their gcd and returns the gcd
template<typename Integer>
Integer v_make_prime(vector<Integer>& v);

//---------------------------------------------------------------------------
//							Scalar operations
//---------------------------------------------------------------------------

//v = v * scalar
template<typename Integer>
void v_scalar_multiplication(vector<Integer>& v, const Integer& scalar){
    size_t i,size=v.size();
    for (i = 0; i <size; i++) {
        v[i] *= scalar;
    }
}

//returns v * scalar mod modulus
template<typename Integer>
vector<Integer> v_scalar_mult_mod(const vector<Integer>& v, const Integer& scalar, const Integer& modulus, bool& success);

template<typename Integer>
void v_scalar_division(vector<Integer>& v, const Integer& scalar);
//v = v / scalar, all the elements of v must be divisible with the scalar

template<typename Integer>
void v_reduction_modulo(vector<Integer>& v, const Integer& modulo);
//v = v mod modulo

//---------------------------------------------------------------------------
//								Test
//---------------------------------------------------------------------------

template<typename Integer>
bool v_test_scalar_product(const vector<Integer>& a,const vector<Integer>& b, const Integer& result, const long& m);
// test the main computation for arithmetic overflow
// uses multiplication mod m

//---------------------------------------------------------------------------
//							   General vector operations
//---------------------------------------------------------------------------

//returns a new vector with the content of a extended by b
template<typename T>
vector<T> v_merge(const vector<T>& a, const T& b);

//returns a new vector with the content of a and b
template<typename T>
vector<T> v_merge(const vector<T>& a, const vector<T>& b);

//returns a new vector with the last size entries of v
template<typename T>
vector<T> v_cut_front(const vector<T>& v, size_t size);

//the input vectors must be ordered of equal size
//if u is different from v by just one element, it returns that element
//else returns 0 (the elements of u and v are >0)
//int v_difference_ordered_fast(const vector<size_t>& u,const vector<size_t>& v);


template<typename Integer>
bool compare_last (const vector<Integer>& a, const vector<Integer>& b)
{
    return a.back() < b.back();
}

//returns a key vector containing the positions of non-zero entrys of v
template<typename Integer>
vector<key_t> v_non_zero_pos(const vector<Integer>& v);

// counts the number of positive entries
template<typename Integer>
size_t v_nr_positive(const vector<Integer>& v);

// check whether the vector only contains 0
template<typename Integer>
bool v_is_zero(const vector<Integer>& v);

template<typename Integer>
bool v_is_symmetric(const vector<Integer>& v);

template<typename Integer>
bool v_is_nonnegative(const vector<Integer>& v);

template<typename Integer>
Integer v_max_abs(const vector<Integer>& v){
	Integer tmp = 0;
	for (size_t i=0; i<v.size(); i++){
		if (Iabs(v[i])>tmp) tmp=Iabs(v[i]);
	}
	return tmp;
}

//---------------------------------------------------------------------------
//							   bool vector operations
//---------------------------------------------------------------------------

vector<bool> v_bool_andnot(const vector<bool>& a, const vector<bool>& b);

// swaps entry i and j of the vector<bool> v
void v_bool_entry_swap(vector<bool>& v, size_t i, size_t j);

//---------------------------------------------------------------------------
//							  Special
//---------------------------------------------------------------------------

// computes integral simplex containing a rational vector
template<typename Integer>
void approx_simplex(const vector<Integer>& q, std::list<vector<Integer> >& approx,const long k);

vector<key_t> identity_key(size_t n);

// compute the degree vector of a hsop
template<typename Integer>
vector<Integer> degrees_hsop(const vector<Integer> gen_degrees,const vector<size_t> heights);

//---------------------------------------------------------------------------
//                            Sorting
//---------------------------------------------------------------------------

template <typename T>
void order_by_perm(vector<T>& v, const vector<key_t>& permfix);

} // namespace

//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
