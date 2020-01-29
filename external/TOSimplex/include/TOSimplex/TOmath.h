/* Copyright (c) 2018
   Thomas Opfer

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

#ifndef TOMATH_H
#define TOMATH_H

#if defined(__APPLE__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif

#include <gmpxx.h>

#if defined(__APPLE__)
#pragma clang diagnostic pop
#endif

#include <sstream>

template <class T>
class TOmath
{
	public:
		static inline T floor( const T &a );
		static inline T ceil( const T &a );
		static inline T hugeposint();
		static inline T hugenegint();
		static inline bool isInt( const T &a );
		static inline std::string toShortString( const T &a );
};


template<>
inline mpq_class TOmath<mpq_class>::floor( const mpq_class &a ){
	mpz_class b;
	mpz_fdiv_q( b.get_mpz_t(), a.get_num_mpz_t(), a.get_den_mpz_t() );
	return b;
}

template <class T>
inline T TOmath<T>::floor( const T &a ){
	throw std::runtime_error( "floor unimplemented for this data type." );
	T b;
	return b;
}

template<>
inline mpq_class TOmath<mpq_class>::ceil( const mpq_class &a ){
	mpz_class b;
	mpz_cdiv_q( b.get_mpz_t(), a.get_num_mpz_t(), a.get_den_mpz_t() );
	return b;
}

template <class T>
inline T TOmath<T>::ceil( const T &a ){
	throw std::runtime_error( "ceil unimplemented for this data type." );
	T b;
	return b;
}

template <class T>
inline T TOmath<T>::hugeposint(){
	throw std::runtime_error( "hugeposint unimplemented for this data type." );
	T b;
	return b;
}

template<>
inline mpq_class TOmath<mpq_class>::hugeposint(){
	std::stringstream str;
	str << std::numeric_limits<int64_t>::max();
	return mpz_class( str.str() );
}

template <class T>
inline T TOmath<T>::hugenegint(){
	return - TOmath<T>::hugeposint();
}

template<>
inline mpq_class TOmath<mpq_class>::hugenegint(){
	std::stringstream str;
	str << std::numeric_limits<int64_t>::min();
	return mpz_class( str.str() );
}

template <class T>
inline bool TOmath<T>::isInt( const T &a ){
	return TOmath<T>::floor( a ) == a;
}

template<>
inline bool TOmath<mpq_class>::isInt( const mpq_class &a ){
	return a.get_den() == 1;
}

template <class T>
inline std::string TOmath<T>::toShortString( const T &a ){
	return "(output unimplemented)";
}

template<>
inline std::string TOmath<mpq_class>::toShortString( const mpq_class &a ){
	std::stringstream str;
	str << a.get_d();
	return str.str();
}


#endif
