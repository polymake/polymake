/* Copyright (c) 2020
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


template <class T>
inline T TOmath<T>::floor( const T &a ){
	throw std::runtime_error( "floor unimplemented for this data type." );
	T b;
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

template <class T>
inline T TOmath<T>::hugenegint(){
	return - TOmath<T>::hugeposint();
}

template <class T>
inline bool TOmath<T>::isInt( const T &a ){
	return TOmath<T>::floor( a ) == a;
}

template <class T>
inline std::string TOmath<T>::toShortString( const T &a ){
	return "(output unimplemented)";
}


#endif
