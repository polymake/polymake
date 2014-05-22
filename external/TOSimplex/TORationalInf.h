/* Copyright (c) 2011-2013
   Thomas Opfer (Technische Universitaet Darmstadt, Germany)

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version: http://www.gnu.org/licenses/gpl.txt.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
--------------------------------------------------------------------------------
   $Id$
*/

#ifndef TORATIONALINF_H
#define TORATIONALINF_H

namespace TOSimplex {

template <class T>
class TORationalInf
{
	public:
		TORationalInf( bool isInf = false ){
			this->isInf = isInf;
		}
		TORationalInf( T value ){
			this->isInf = false;
			this->value = value;
		}
		T value;
		bool isInf;
};

}

#endif
