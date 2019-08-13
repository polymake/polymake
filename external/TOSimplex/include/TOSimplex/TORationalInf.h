/* Copyright (c) 2011-2019
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

#ifndef TORATIONALINF_H
#define TORATIONALINF_H

namespace TOSimplex {

template <class T>
class TORationalInf
{
	public:
		TORationalInf( bool isInf_ = false ):
			isInf( isInf_ ){}

		TORationalInf( const T& value_ ):
			value( value_ ),
			isInf( false ){}

	T value;
	bool isInf;
};

}

#endif
