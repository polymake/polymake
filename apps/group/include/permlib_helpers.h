/* Copyright (c) 1997-2020
   Ewgenij Gawrilow, Michael Joswig, and the polymake team
   Technische Universität Berlin, Germany
   https://polymake.org

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

#ifndef POLYMAKE_GROUP_DATAHELPERS_H_
#define POLYMAKE_GROUP_DATAHELPERS_H_

namespace polymake { namespace group {

// 1-dim array
template <typename ArrayType>
Array<Int> array2PolymakeArray(ArrayType array, Int dim)
{
   Array<Int> pmArray(dim);
   for (Int j = 0; j < dim; ++j) {
      pmArray[j] = static_cast<Int>(array[j]);
   }
   return pmArray;
}

// 2-dim array
template <typename ArrayType>
Array<Array<Int>> arrays2PolymakeArray(ArrayType array, Int dim1, Int dim2)
{
   Array<Array<Int>> pmArray(dim1);
   for (Int i = 0; i < dim1; ++i) {
      pmArray[i] = array2PolymakeArray(array[i], dim2);
   }
   return pmArray;
}


template <typename IntType>
IntType* polymakeArray2Array(const Array<Int>& array)
{
   IntType* ret = new IntType[array.size()];
   for (Int i = 0; i < array.size(); ++i)
      ret[i] = static_cast<IntType>(array[i]);
   return ret;
}

template <typename IntType>
IntType** polymakeArray2Arrays(const Array<Array<Int>>& array)
{
   IntType** ret = new IntType*[array.size()];
   for (Int i = 0; i < array.size(); ++i)
      ret[i] = polymakeArray2Array<IntType>(array[i]);
   return ret;
}

}} // end NS

#endif
