/* Copyright (c) 1997-2018
   Ewgenij Gawrilow, Michael Joswig (Technische Universitaet Berlin, Germany)
   http://www.polymake.org

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
template<typename ArrayType>
Array<int> array2PolymakeArray(ArrayType array, int dim) {
   Array<int> pmArray(dim);
   for (int j = 0; j < dim; ++j) {
      pmArray[j] = static_cast<int>(array[j]);
   }
   return pmArray;
}

// 2-dim array
template<typename ArrayType>
Array<Array<int> > arrays2PolymakeArray(ArrayType array, int dim1, int dim2) {
   Array< Array<int> > pmArray(dim1);
   for (int i = 0; i < dim1; ++i) {
      pmArray[i] = array2PolymakeArray(array[i], dim2);
   }
   return pmArray;
}


template<typename IntType>
IntType* polymakeArray2Array(const Array<int>& array) {
   IntType* ret = new IntType[array.size()];
   for (int i = 0; i < array.size(); ++i)
      ret[i] = static_cast<IntType>(array[i]);
   return ret;
}

template<typename IntType>
IntType** polymakeArray2Arrays(const Array<Array<int> >& array) {
   IntType** ret = new IntType*[array.size()];
   for (int i = 0; i < array.size(); ++i)
      ret[i] = polymakeArray2Array<IntType>(array[i]);
   return ret;
}

}} // end NS

#endif
