/*
 * Normaliz
 * Copyright (C) 2007-2013  Winfried Bruns, Bogdan Ichim, Christof Soeger
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
 */

//---------------------------------------------------------------------------
#ifndef MATRIX_HPP
#define MATRIX_HPP
//---------------------------------------------------------------------------


#include <vector>
#include <list>
#include <iostream>
#include <string>

#include "libnormaliz.h"

//---------------------------------------------------------------------------

namespace libnormaliz {
using std::list;
using std::vector;
using std::string;

template<typename Integer> class Matrix {
    size_t nr;
    size_t nc;
    vector< vector<Integer> > elements;

//---------------------------------------------------------------------------
//              Private routines, used in the public routines
//---------------------------------------------------------------------------

    void max_rank_submatrix_lex(vector<key_t>& v, const size_t& rank) const;
    //v will be a vector with entries the indices of the first rows in lexicographic
    //order of this forming a submatrix of maximal rank.
    //v shoud be a vector of size 0 by call!!!
    
    // Does the computation for the solution of linear systems
    void solve_destructive_Sol_inner(Matrix<Integer>& Right_side, vector< Integer >& diagonal, 
                    Integer& denom, Matrix<Integer>& Solution);     
  
//---------------------------------------------------------------------------
public:
//---------------------------------------------------------------------------
//                      Construction and destruction
//---------------------------------------------------------------------------

    Matrix();
    Matrix(size_t dim);                           //constructor of identity matrix
    Matrix(size_t row, size_t col);                 //main constructor, all entries 0
    Matrix(size_t row, size_t col, Integer value); //constructor, all entries set to value
    Matrix(const vector< vector<Integer> >& elem); //constuctor, elements=elem
    Matrix(const list< vector<Integer> >& elems);

//---------------------------------------------------------------------------
//                             Data access
//---------------------------------------------------------------------------

    void write(std::istream& in = std::cin);                // to be modified, just for tests
    void write(size_t row, const vector<Integer>& data); //write a row
    void write(size_t row, const vector<int>& data); //write a row
    void write_column(size_t col, const vector<Integer>& data); //write a column
    void write(size_t row, size_t col, Integer data);  // write data at (row,col)
    void print(const string& name, const string& suffix) const;         //  writes matrix into name.suffix
    void print(std::ostream& out) const;          // writes matrix to the stream
    void pretty_print(std::ostream& out, bool with_row_nr=false) const;  // writes matrix in a nice format to the stream
    void read() const;                 // to be modified, just for tests
    vector<Integer> read(size_t row) const;                   // read a row
    Integer read (size_t row, size_t col) const;         // read data at (row,col)
    size_t nr_of_rows() const;                       // returns nr
    size_t nr_of_columns() const;                   // returns nc
    /* generates a pseudo random matrix for tests, entries form 0 to mod-1 */
    void random(int mod=3);

    /* returns a submatrix with rows corresponding to indices given by
     * the entries of rows, Numbering from 0 to n-1 ! */
    Matrix submatrix(const vector<key_t>& rows) const;
    Matrix submatrix(const vector<int>& rows) const;
    Matrix submatrix(const vector<bool>& rows) const;

    vector<Integer> diagonale() const;     //returns the diagonale of this
                                  //this should be a quadratic matrix
    size_t maximal_decimal_length() const;    //return the maximal number of decimals
                                      //needed to write an entry

    void append(const Matrix& M); // appends the rows of M to this
    void append(const vector<Integer>& v); // append the row v to this
    void cut_columns(size_t c); // remove columns, only the first c columns will survive

    inline const Integer& get_elem(size_t row, size_t col) const {
        return elements[row][col];
    }
    inline const vector< vector<Integer> >& get_elements() const {
        return elements;
    }
    inline vector<Integer> const& operator[] (size_t row) const {
        return elements[row];
    }
    inline vector<Integer>& operator[] (size_t row) { 
        return elements[row];
    }



//---------------------------------------------------------------------------
//                  Basic matrices operations
//---------------------------------------------------------------------------

    Matrix add(const Matrix& A) const;                       // returns this+A
    Matrix multiplication(const Matrix& A) const;          // returns this*A
    Matrix multiplication(const Matrix& A, long m) const;// returns this*A (mod m)
    Matrix<Integer> multiplication_cut(const Matrix<Integer>& A, const size_t& c) const; // returns 
    // this*(first c columns of A)
    bool equal(const Matrix& A) const;             // returns this==A
    bool equal(const Matrix& A, long m) const;     // returns this==A (mod m)
    Matrix transpose() const;                     // returns the transpose of this

//---------------------------------------------------------------------------
//                          Scalar operations
//---------------------------------------------------------------------------

    void scalar_multiplication(const Integer& scalar);  //this=this*scalar
    void scalar_division(const Integer& scalar);
    //this=this div scalar, all the elements of this must be divisible with the scalar
    void reduction_modulo(const Integer& modulo);     //this=this mod scalar
    Integer matrix_gcd() const; //returns the gcd of all elements
    vector<Integer> make_prime();         //each row of this is reduced by its gcd
    //return a vector containing the gcd of the rows

    Matrix multiply_rows(const vector<Integer>& m) const;  //returns matrix were row i is multiplied by m[i]

//---------------------------------------------------------------------------
//                          Vector operations
//---------------------------------------------------------------------------

   vector<Integer> MxV(const vector<Integer>& v) const;//returns this*V
   vector<Integer> VxM(const vector<Integer>& v) const;//returns V*this

//---------------------------------------------------------------------------
//                      Rows and columns exchange
//---------------------------------------------------------------------------

    void exchange_rows(const size_t& row1, const size_t& row2);      //row1 is exchanged with row2
    void exchange_columns(const size_t& col1, const size_t& col2); // col1 is exchanged with col2

//---------------------------------------------------------------------------
//              Rows and columns reduction  in  respect to
//          the right-lower submatrix of this described by an int corner
//---------------------------------------------------------------------------

    void reduce_row(size_t corner);      //reduction by the corner-th row
    void reduce_row(size_t corner, Matrix& Left);//row reduction, Left used
    //for saving or copying the linear transformations
    void reduce_column(size_t corner);  //reduction by the corner-th column
    void reduce_column(size_t corner, Matrix& Right, Matrix& Right_Inv);
    //column reduction,  Right used for saving or copying the linear
    //transformations, Right_Inv used for saving the inverse linear transformations

//---------------------------------------------------------------------------
//                      Pivots for rows/columns operations
//---------------------------------------------------------------------------

    vector<long> pivot(size_t corner); //Find the position of an element x with
    //0<abs(x)<=abs(y) for all y!=0 in the right-lower submatrix of this
    //described by an int corner
    long pivot_column(size_t col);  //Find the position of an element x with
    //0<abs(x)<=abs(y) for all y!=0 in the lower half of the column of this
    //described by an int col

//---------------------------------------------------------------------------
//                          Matrices operations
//           --- this are more complicated algorithms ---
//---------------------------------------------------------------------------

    size_t diagonalize(); //computes rank and diagonalizes this, destructive

    size_t rank() const; //returns rank, nondestructive
    
    Integer vol_destructive();

    size_t rank_destructive(); //returns rank, destructive

    vector<key_t> max_rank_submatrix() const; //returns a vector with entries the
    //indices of the rows of this forming a submatrix of maximal rank

    vector<key_t>  max_rank_submatrix_lex() const; //returns a vector with entries
    //the indices of the first rows in lexicographic order of this forming
    //a submatrix of maximal rank.

    vector<key_t>  max_rank_submatrix_lex(const size_t& rank) const;
    //returns a vector with entries the indices of the first rows in lexicographic
    //order of this forming a submatrix of maximal rank, assuming that
    //the rank of this is known.
  
    // In the following routines denom is the absolute value of the determinant of the
    // left side matrix ( =this).

    Matrix solve(Matrix Right_side, Integer& denom) const;// solves the system
    //this*Solution=denom*Right_side. this should be a quadratic matrix with nonzero determinant.

    Matrix solve(Matrix Right_side, vector< Integer >& diagonal, Integer& denom) const;// solves the system
    //this*Solution=denom*Right_side. this should be a quadratic /matrix with nonzero determinant.
    //The diagonal of this after transformation into an upper triangular matrix
    //is saved in diagonal

    // Right_side and this get destroyed!
    Matrix solve_destructive(Matrix& Right_side, vector< Integer >& diagonal, Integer& denom);

    // Returns the solution of the system in Solution (for efficiency)
    void solve_destructive_Sol(Matrix<Integer>& Right_side, vector< Integer >& diagonal, 
                    Integer& denom, Matrix<Integer>& Solution);
                   
    Matrix invert(vector< Integer >& diagonal, Integer& denom) const;// solves the system
    //this*Solution=denom*I. this should be a quadratic matrix with nonzero determinant. 
    //The diagonal of this after transformation into an upper triangular matrix
    //is saved in diagonal

    vector<Integer> find_linear_form () const;
    // Tries to find a linear form which gives the same value an all rows of this
    // this should be a m x n matrix (m>=n) of maxinal rank
    // returns an empty vector if there does not exist such a linear form
  
    vector<Integer> find_linear_form_low_dim () const;
    //same as find_linear_form but also works with not maximal rank
    //uses a linear transformation to get a full rank matrix

    vector<Integer> solve(vector<Integer> v) const;
    // like find_linear_form, but for right side v

};
//class end *****************************************************************

} // namespace

//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
