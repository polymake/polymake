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
// This class implements the elementary divisor algoritm.
// An object Lineare_transformation consists of four matrices.
// Given a matrix M we want to obtain matrices U and V such that
// UMV=the diagonal form of M.
// In this class U is called Left, M Center and V Right.
// The inverse of V is also computed as Right_Inv.
// A Lineare_transormation should be initialized with a matrix M by the main
// constructor. After using the procedure transformation the result should be
// interpreted as follows:
// Left=U; Center=the diagonal form of M; Right=V;
// Right_Inv=the inverse of V; rk=rank of M.
// We recommend using the non-member function Transformation, which also test
// for possible arithmetic overflows
//---------------------------------------------------------------------------

#ifndef LINEARE_TRANSFORMATION_H
#define LINEARE_TRANSFORMATION_H

#include <string>

#include "libnormaliz.h"
#include "matrix.h"

//---------------------------------------------------------------------------

namespace libnormaliz {

template<typename Integer>
class Lineare_Transformation {
  long rk;
  string status;
  Integer index;
  Matrix<Integer> Center;
  Matrix<Integer> Right;
  Matrix<Integer> Right_Inv;
//---------------------------------------------------------------------------
public:
//---------------------------------------------------------------------------
//                      Construction and destruction
//---------------------------------------------------------------------------

  Lineare_Transformation();
  Lineare_Transformation(const Matrix<Integer>& M);      //main constructor

//---------------------------------------------------------------------------
//                         Data acces
//---------------------------------------------------------------------------

  void read() const;                   // to be modified, just for tests
  long get_rank() const;              //returns rank if status is
                               // "initialized, after transformation"
  string get_status()const;       //returns status, may be:
                             // "non initialized"
                            //  "initialized, before transformation"
                           //  "initialized, after transformation"
  Integer get_index() const;
  Matrix<Integer> get_center() const;          //read center matrix
  Matrix<Integer> get_right() const;          //read right matrix
  Matrix<Integer> get_right_inv() const;     //read the inverse of the right matrix
  void set_rank(const size_t rank);
  void set_center(const Matrix<Integer>& M);          //write center matrix
  void set_right(const Matrix<Integer>& M);          //write right matrix
  void set_right_inv(const Matrix<Integer>& M);     //write the inverse of the right matrix

//---------------------------------------------------------------------------
//                    Rows and columns exchange
//---------------------------------------------------------------------------

  void exchange_rows(size_t row1, size_t row2);     //similar to Matrix<Integer>::exchange_rows
  void exchange_columns(size_t col1, size_t col2); //similar to Matrix<Integer>::exchange_columns

//---------------------------------------------------------------------------
//                  Rows and columns reduction
//---------------------------------------------------------------------------

  void reduce_row(size_t corner);      //similar to Matrix<Integer>::reduce_row
  void reduce_column(size_t corner);  //similar to Matrix<Integer>::reduce_column

//---------------------------------------------------------------------------
//                   Algorithms
//---------------------------------------------------------------------------

  void transformation(); //makes the main computation
                        //no tests for errors
//---------------------------------------------------------------------------
//Tests
//---------------------------------------------------------------------------


  /* test the main computation for arithmetic overflow
   * uses multiplication mod m
   */
  bool test_transformation(const Matrix<Integer>& M, const size_t& m) const;


};
//class end *****************************************************************
//---------------------------------------------------------------------------

//makes the main computation, test for errors
template<typename Integer>
Lineare_Transformation<Integer> Transformation(const Matrix<Integer>& M);

} /* end namespace libnormaliz */

//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
