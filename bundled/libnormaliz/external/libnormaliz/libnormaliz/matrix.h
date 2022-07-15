/*
 * Normaliz
 * Copyright (C) 2007-2019  Winfried Bruns, Bogdan Ichim, Christof Soeger
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * As an exception, when this program is distributed through (i) the App Store
 * by Apple Inc.; (ii) the Mac App Store by Apple Inc.; or (iii) Google Play
 * by Google Inc., then that store may impose any digital rights management,
 * device limits and/or redistribution restrictions that are required by its
 * terms of service.
 */

//---------------------------------------------------------------------------
#ifndef LIBNORMALIZ_MATRIX_HPP
#define LIBNORMALIZ_MATRIX_HPP
//---------------------------------------------------------------------------

#include <vector>
#include <list>
#include <iostream>
#include <string>
#include <cmath>

#include <libnormaliz/general.h>
#include <libnormaliz/integer.h>
// #include <libnormaliz/convert.h>
#include <libnormaliz/vector_operations.h>
#include "libnormaliz/dynamic_bitset.h"
// #include <libnormaliz/sublattice_representation.h>

//---------------------------------------------------------------------------

namespace libnormaliz {
using std::list;
using std::string;
using std::vector;

template <typename Integer>
class Sublattice_Representation;

template <typename Integer>
class Matrix {
    template <typename>
    friend class Matrix;
    // template<typename> friend class Lineare_Transformation;
    template <typename>
    friend class Sublattice_Representation;

    // public:

    size_t nr;
    size_t nc;
    vector<vector<Integer> > elem;

    //---------------------------------------------------------------------------
    //              Private routines, used in the public routines
    //---------------------------------------------------------------------------

    //---------------------------------------------------------------------------
    //              Row and column reduction
    //---------------------------------------------------------------------------
    // return value false undicates failure because of overflow
    // for all the routines below

    // reduction via integer division and elemntary transformations
    bool reduce_row(size_t corner);           // reduction by the corner-th row
    bool reduce_row(size_t row, size_t col);  // corner at position (row,col)

    // replaces two columns by linear combinations of them
    bool linear_comb_columns(
        const size_t& col, const size_t& j, const Integer& u, const Integer& w, const Integer& v, const Integer& z);

    // column reduction with gcd method
    bool gcd_reduce_column(size_t corner, Matrix<Integer>& Right);

    //---------------------------------------------------------------------------
    //                      Work horses
    //---------------------------------------------------------------------------

    // takes |product of the diagonal elem|
    Integer compute_vol(bool& success);

    // Solve system with coefficients and right hand side in one matrix, using elementary transformations
    // solution replaces right hand side
    bool solve_destructive_inner(bool ZZinvertible, Integer& denom);

    // asembles the matrix of the system (left side the submatrix of mother given by key
    // right side from column vectors pointed to by RS
    // both in a single matrix
    void solve_system_submatrix_outer(const Matrix<Integer>& mother,
                                      const vector<key_t>& key,
                                      const vector<vector<Integer>*>& RS,
                                      Integer& denom,
                                      bool ZZ_invertible,
                                      bool transpose,
                                      size_t red_col,
                                      size_t sign_col,
                                      bool compute_denom = true,
                                      bool make_sol_prime = false);

    // size_t row_echelon_inner_elem(bool& success); // does the work and checks for overflows
    // size_t row_echelon_inner_bareiss(bool& success, Integer& det);
    // NOTE: Bareiss cannot be used if z-invertible transformations are needed

    size_t row_echelon(bool& success);                // transforms this into row echelon form and returns rank
    size_t row_echelon(bool& success, Integer& det);  // computes also |det|
    size_t row_echelon(bool& success, bool do_compute_vol, Integer& det);  // chooses elem (or bareiss in former time)

    // reduces the rows a matrix in row echelon form upwards, from left to right
    bool reduce_rows_upwards();
    size_t row_echelon_reduce(bool& success);  // combines row_echelon and reduce_rows_upwards

    // computes rank and index simultaneously, returns rank
    Integer full_rank_index(bool& success);

    vector<key_t> max_rank_submatrix_lex_inner(bool& success, vector<key_t> perm = vector<key_t>(0)) const;

    // A version of invert that circumvents protection and leaves it to the calling routine
    Matrix invert_unprotected(Integer& denom, bool& sucess) const;

    bool SmithNormalForm_inner(size_t& rk, Matrix<Integer>& Right);

    vector<Integer> optimal_subdivision_point_inner() const;

    //---------------------------------------------------------------------------
    //                      Pivots for rows/columns operations
    //---------------------------------------------------------------------------

    vector<long> pivot(size_t corner);  // Find the position of an element x with
    // 0<abs(x)<=abs(y) for all y!=0 in the right-lower submatrix of this
    // described by an int corner

    long pivot_in_column(size_t col);  // Find the position of an element x with
    // 0<abs(x)<=abs(y) for all y!=0 in the lower half of the column of this
    // described by an int col

    long pivot_in_column(size_t row, size_t col);  // in column col starting from row

    //---------------------------------------------------------------------------
    //                     Helpers for linear systems
    //---------------------------------------------------------------------------

    Matrix bundle_matrices(const Matrix<Integer>& Right_side) const;
    Matrix extract_solution() const;
    vector<vector<Integer>*> row_pointers();
    void customize_solution(size_t dim, Integer& denom, size_t red_col, size_t sign_col, bool make_sol_prime);

   public:
    typedef Integer elem_type;

    size_t row_echelon_inner_bareiss(bool& success, Integer& det);

    vector<vector<Integer>*> submatrix_pointers(const vector<key_t>& key);

    //---------------------------------------------------------------------------
    //                      Rows and columns exchange
    //---------------------------------------------------------------------------

    void exchange_rows(const size_t& row1, const size_t& row2);     // row1 is exchanged with row2
    void exchange_columns(const size_t& col1, const size_t& col2);  // col1 is exchanged with col2

    //---------------------------------------------------------------------------

    //---------------------------------------------------------------------------
    //                      Construction and destruction
    //---------------------------------------------------------------------------

    Matrix();
    Matrix(size_t dim);                             // constructor of unit matrix
    Matrix(size_t row, size_t col);                 // main constructor, all entries 0
    Matrix(size_t row, size_t col, Integer value);  // constructor, all entries set to value
    Matrix(const vector<vector<Integer> >& elem);   // constuctor, elem=elem
    Matrix(const list<vector<Integer> >& elems);
    Matrix(const vector<Integer>& row);

    //---------------------------------------------------------------------------
    //                             Data access
    //---------------------------------------------------------------------------

    void write_column(size_t col, const vector<Integer>& data);         // write a column
    void print(const string& name, const string& suffix) const;         //  writes matrix into name.suffix
    void print_append(const string& name, const string& suffix) const;  // the same, but appends matrix
    void print(std::ostream& out, bool with_format = true) const;       // writes matrix to the stream
    void debug_print(char mark = '*') const;
    void pretty_print(std::ostream& out, bool with_row_nr = false, bool count_from_one = false)
        const;                     // writes matrix in a nice format to the stream                   // read a row
    size_t nr_of_rows() const;     // returns nr
    size_t nr_of_columns() const;  // returns nc
    void set_nr_of_columns(size_t c);
    /* generates a pseudo random matrix for tests, entries form 0 to mod-1 */
    void random(int mod = 3);

    void set_zero();  // sets all entries to 0

    /* returns a submatrix with rows corresponding to indices given by
     * the entries of rows, Numbering from 0 to n-1 ! */
    Matrix submatrix(const vector<key_t>& rows) const;
    Matrix submatrix(const vector<int>& rows) const;
    Matrix submatrix(const vector<bool>& rows) const;
    // Matrix submatrix(const dynamic_bitset& rows) const;

    Matrix select_columns(const vector<bool>& cols) const;
    Matrix selected_columns_first(const vector<bool>& cols) const;

    void swap(Matrix<Integer>& x);

    // returns the permutation created by sorting the rows with a grading function
    // or by 1-norm if computed is false
    vector<key_t> perm_sort_by_degree(const vector<key_t>& key, const vector<Integer>& grading, bool computed) const;
    // according to the matrix of weights (taking absolute values first)
    vector<key_t> perm_by_weights(const Matrix<Integer>& Weights, vector<bool> absolute);
    // according to the number of zeoes -- descending --
    vector<key_t> perm_by_nr_zeroes();
    // according to lex order
    vector<key_t> perm_by_lex();

    void select_submatrix(const Matrix<Integer>& mother, const vector<key_t>& rows);
    void select_submatrix_trans(const Matrix<Integer>& mother, const vector<key_t>& rows);

    Matrix& remove_zero_rows();  // remove zero rows, modifies this

    // resizes the matrix to the given number of rows/columns
    void resize(size_t nr_rows);
    void resize(size_t nr_rows, size_t nr_cols);
    void resize_columns(size_t nr_cols);
    void Shrink_nr_rows(size_t new_nr_rows);

    vector<Integer> diagonal() const;       // returns the diagonale of this
                                            // this should be a quadratic matrix
    size_t maximal_decimal_length() const;  // return the maximal number of decimals
                                            // needed to write an entry

    vector<size_t> maximal_decimal_length_columnwise() const;  // the same per column

    void append(const Matrix& M);                    // appends the rows of M to this
    void append(const vector<vector<Integer> >& M);  // the same, but for another type of matrix
    void append(const vector<Integer>& v);           // append the row v to this
    void append_column(const vector<Integer>& v);    // append the column v to this
    void insert_column(const size_t pos, const vector<Integer>& v);
    void insert_column(const size_t pos, const Integer& val);
    void remove_row(const vector<Integer>& row);  // removes all appearances of this row, not very efficient!
    void remove_row(const size_t index);
    vector<size_t> remove_duplicate_and_zero_rows();
    void remove_duplicate(const Matrix& M);

    inline const Integer& get_elem(size_t row, size_t col) const {
        return elem[row][col];
    }
    inline const vector<vector<Integer> >& get_elements() const {
        assert(nr == elem.size());
        return elem;
    }
    inline vector<vector<Integer> >& access_elements() {
        assert(nr == elem.size());
        return elem;
    }
    inline vector<Integer> const& operator[](size_t row) const {
        return elem[row];
    }
    inline vector<Integer>& operator[](size_t row) {
        return elem[row];
    }
    void set_nc(size_t col) {
        nc = col;
    }
    void set_nr(size_t rows) {
        nc = rows;
    }

    //  convert the remaining matrix to nmz_float
    Matrix<nmz_float> nmz_float_without_first_column() const;

#ifdef ENFNORMALIZ
    void make_first_element_1_in_rows();
#endif
    void standardize_basis();
    bool standardize_rows(const vector<Integer>& Norm);
    bool standardize_rows();

    //---------------------------------------------------------------------------
    //                  Basic matrices operations
    //---------------------------------------------------------------------------

    Matrix add(const Matrix& A) const;                            // returns this+A
    Matrix multiplication(const Matrix& A) const;                 // returns this*A
    void multiplication(Matrix& B, const Matrix& A) const;        // the same, but result in B
    Matrix multiplication_trans(const Matrix& A) const;           // returns this*A.transpose()
    void multiplication_trans(Matrix& B, const Matrix& A) const;  // the same, but result in B
    Matrix multiplication(const Matrix& A, long m) const;         // returns this*A (mod m)
    bool equal(const Matrix& A) const;                            // returns this==A
    // bool equal(const Matrix& A, long m) const;     // returns this==A (mod m)
    Matrix transpose() const;  // returns the transpose of this
    void transpose_in_place();

    bool zero_product_with_transpose_of(const Matrix& B);

    bool is_diagonal() const;

    //---------------------------------------------------------------------------
    //                          Scalar operations
    //---------------------------------------------------------------------------

    void scalar_multiplication(const Integer& scalar);  // this=this*scalar
    void scalar_division(const Integer& scalar);
    // this=this div scalar, all the elem of this must be divisible with the scalar
    void reduction_modulo(const Integer& modulo);  // this=this mod scalar
    Integer matrix_gcd() const;                    // returns the gcd of all elem
    vector<Integer> make_prime();                  // each row of this is reduced by its gcd,
                                                   // vector of gcds returned
    void make_cols_prime(size_t from_col, size_t to_col);
    // the columns of this in the specified range are reduced by their gcd
    void simplify_rows(const vector<Integer>& Norm);  // applies v_standardize to the rows

    Matrix multiply_rows(const vector<Integer>& m) const;  // returns matrix were row i is multiplied by m[i]

    //---------------------------------------------------------------------------
    //                          Vector operations
    //---------------------------------------------------------------------------

    void MxV(vector<Integer>& result, const vector<Integer>& v) const;  // result = this*V
    vector<Integer> MxV(const vector<Integer>& v) const;                // returns this*V
    vector<Integer> VxM(const vector<Integer>& v) const;                // returns V*this
    vector<Integer> VxM_div(const vector<Integer>& v,
                            const Integer& divisor,
                            bool& success) const;  // additionally divides by divisor

    bool check_congruences(const vector<Integer>& v) const;  // *this represents congruences

    //---------------------------------------------------------------------------
    //                          Matrix operations
    //           --- these are more complicated algorithms ---
    //---------------------------------------------------------------------------

    // Normal forms

    size_t row_echelon_inner_elem(bool& success);  // does the work and checks for overflows

    // converts this to row echelon form over ZZ and returns rank, GMP protected, uses only elementary transformations over ZZ
    size_t row_echelon();

    // public version of row_echelon_reduce, GMP protected, uses only elementary transformations over ZZ
    size_t row_echelon_reduce();

    // transforms matrix into lower triangular form via column transformations
    // assumes that rk is the rank and that the matrix is zero after the first rk rows
    // Right = Right*(column transformation of this call)
    bool column_trigonalize(size_t rk, Matrix<Integer>& Right);

    // combines row_echelon_reduce and column_trigonalize
    // returns column transformation matrix
    Matrix<Integer> row_column_trigonalize(size_t& rk, bool& success);

    // rank and determinant

    size_t rank() const;                                    // returns rank
    Integer full_rank_index() const;                        // returns index of full rank sublattice
    size_t rank_submatrix(const vector<key_t>& key) const;  // returns rank of submarix defined by key

    // returns rank of submatrix of mother. "this" is used as work space
    size_t rank_submatrix(const Matrix<Integer>& mother, const vector<key_t>& key);

    // vol stands for |det|
    Integer vol() const;
    Integer vol_submatrix(const vector<key_t>& key) const;
    Integer vol_submatrix(const Matrix<Integer>& mother, const vector<key_t>& key);

    // find linearly indepenpendent submatrix of maximal rank

    vector<key_t> max_rank_submatrix_lex(vector<key_t> perm = vector<key_t>(0)) const;  // returns a vector with entries
    // the indices of the first rows in lexicographic order of this forming
    // a submatrix of maximal rank.

    // Solution of linear systems with square matrix

    // In the following routines, denom is the absolute value of the determinant of the
    // left side matrix.
    // If the diagonal is asked for, ZZ-invertible transformations are used.
    // Otherwise ther is no restriction on the used algorithm

    // The diagonal of left hand side after transformation into an upper triangular matrix
    // is saved in diagonal, denom is |determinant|.

    // System with "this" as left side
    Matrix solve(const Matrix& Right_side, Integer& denom) const;
    Matrix solve(const Matrix& Right_side, vector<Integer>& diagonal, Integer& denom) const;
    // solve the system this*Solution=denom*Right_side.

    // system is defined by submatrix of mother given by key (left side) and column vectors pointed to by RS (right side)
    // NOTE: this is used as the matrix for the work
    void solve_system_submatrix(const Matrix& mother,
                                const vector<key_t>& key,
                                const vector<vector<Integer>*>& RS,
                                vector<Integer>& diagonal,
                                Integer& denom,
                                size_t red_col,
                                size_t sign_col);
    void solve_system_submatrix(const Matrix& mother,
                                const vector<key_t>& key,
                                const vector<vector<Integer>*>& RS,
                                Integer& denom,
                                size_t red_col,
                                size_t sign_col,
                                bool compute_denom = true,
                                bool make_sol_prime = false);
    // the left side gets transposed
    void solve_system_submatrix_trans(const Matrix& mother,
                                      const vector<key_t>& key,
                                      const vector<vector<Integer>*>& RS,
                                      Integer& denom,
                                      size_t red_col,
                                      size_t sign_col);

    // For non-square matrices

    // The next two solve routines do not require the matrix to be square.
    // However, we want rank = number of columns, ensuring unique solvability

    vector<Integer> solve_rectangular(const vector<Integer>& v, Integer& denom) const;
    // computes solution vector for right side v, solution over the rationals
    // matrix needs not be quadratic, but must have rank = number of columns
    // with denominator denom.
    // gcd of denom and solution is extracted !!!!!

    vector<Integer> solve_ZZ(const vector<Integer>& v) const;
    // computes solution vector for right side v
    // insists on integrality of the solution

    // homogenous linear systems

    Matrix<Integer> kernel(bool use_LLL = true) const;
    // computes a ZZ-basis of the solutions of (*this)x=0
    // the basis is formed by the ROWS of the returned matrix

    // inverse matrix

    // this*Solution=denom*I. "this" should be a quadratic matrix with nonzero determinant.
    Matrix invert(Integer& denom) const;

    void invert_submatrix(const vector<key_t>& key,
                          Integer& denom,
                          Matrix<Integer>& Inv,
                          bool compute_denom = true,
                          bool make_sol_prime = false) const;

    // the sdame with prefabricated work matrices
    void invert_submatrix(const vector<key_t>& key,
                          Integer& denom,
                          Matrix<Integer>& Inv,
                          Matrix<Integer>& Work,
                          Matrix<Integer>& unitMat,
                          bool compute_denom = true,
                          bool make_sol_prime = false) const;

    // find linear form that is constant on the rows

    vector<Integer> find_linear_form() const;
    // Tries to find a linear form which gives the same value an all rows of this
    // this should be a m x n matrix (m>=n) of maxinal rank
    // returns an empty vector if there does not exist such a linear form

    vector<Integer> find_linear_form_low_dim() const;
    // same as find_linear_form but also works with not maximal rank
    // uses a linear transformation to get a full rank matrix

    // normal forms

    Matrix AlmostHermite(size_t& rk);
    // Converts "this" into lower trigonal column Hermite normal form, returns column
    // transformation matrix
    // Almost: elements left of diagonal are not reduced mod diagonal

    // Computes Smith normal form and returns column transformation matrix
    Matrix SmithNormalForm(size_t& rk);

    // for simplicial subcones

    // computes support hyperplanes and volume, second version with prefabricated work matrices
    void simplex_data(const vector<key_t>& key, Matrix<Integer>& Supp, Integer& vol, bool compute_vol) const;
    void simplex_data(const vector<key_t>& key,
                      Matrix<Integer>& Supp,
                      Integer& vol,
                      Matrix<Integer>& Work,
                      Matrix<Integer>& unitMat,
                      bool compute_vol) const;

    // finds subdivision points
    vector<Integer> optimal_subdivision_point() const;

    // Sorting of rows

    void order_rows_by_perm(const vector<key_t>& perm);

    Matrix& sort_by_weights(const Matrix<Integer>& Weights, vector<bool> absolute);
    Matrix& sort_lex();
    Matrix<Integer>& sort_by_nr_of_zeroes();

    // solve homogeneous congruences

    Matrix<Integer> solve_congruences(bool& zero_modulus) const;

    // saturate sublattice generated by rows

    void saturate();

    // find the indices of the rows in which the linear form L takes its max and min values

    vector<key_t> max_and_min(const vector<Integer>& L, const vector<Integer>& norm) const;

    // try to sort the rows in such a way that the extreme points of the polytope spanned by the rows come first

    size_t extreme_points_first(bool verbose, vector<key_t>& perm);

    // find an inner point in the cone spanned by the rows of the matrix

    vector<Integer> find_inner_point();

    //  LLL (for functions that return transformation matrices see below)

    Matrix<Integer> LLL() const;

    Matrix<Integer> LLL_transpose() const;

    void GramSchmidt(Matrix<nmz_float>& B, Matrix<nmz_float>& M, int from, int to);

    // check maztrix for defining a projection and using it

    bool check_projection(vector<key_t>& projection_key);
    Matrix select_coordinates(const vector<key_t>& projection_key) const;                        // applies the projection
    Matrix insert_coordinates(const vector<key_t>& projection_key, const size_t nr_cols) const;  // defines the "inverse"
};
// class end *****************************************************************

//---------------------------------------------------------------------------
//                  Matrices of binary expansions
//---------------------------------------------------------------------------

/*
 * Binary matrices contain matrices of nonnegative integers.
 * Each entry is stored "vertically" as the binary expansion of an
 * index (relaive to values) i. The k-th binary digit of i (counting k from 0)
 * is in layer k.
 *
 * The "true" value represented by i is values[i] (or mpz_values[i], see below).
 *
 * The goal is to store large matrices of relatively
 * small numbers with as little space as possible.
 *
 * Moreover this structure needs as a brifge to nauty.
 *
 * It can happen that mpz_class values must be taken into account,
 * even if Integer = long or long long. (See nmz_nauty.cpp,
 * makeMMFromGensOnly). Therefore we have a field mpz_values in
 * addition to values. Transfer to *this via get_data_mpz.
 */

template <typename Integer>
class BinaryMatrix {
    template <typename>
    friend class BinaryMatrix;

    vector<vector<dynamic_bitset> > Layers;
    size_t nr_rows, nr_columns;
    // mpz_class offset;  // to be added to "entries" to get true value

    vector<Integer> values;
    vector<mpz_class> mpz_values;

   public:
    void insert(long val, key_t i, key_t j);

    size_t get_nr_rows() const;
    size_t get_nr_columns() const;
    size_t get_nr_layers() const;

    bool test(key_t i, key_t j, key_t k) const;
    long val_entry(size_t i, size_t j) const;
    Matrix<Integer> get_value_mat() const;
    Matrix<mpz_class> get_mpz_value_mat() const;

    const vector<vector<dynamic_bitset> >& get_layers() const;
    const vector<Integer>& get_values() const;
    const vector<mpz_class>& get_mpz_values() const;

    BinaryMatrix();
    BinaryMatrix(size_t m, size_t n);
    BinaryMatrix(size_t m, size_t n, size_t height);
    BinaryMatrix reordered(const vector<key_t>& row_order, const vector<key_t>& col_order) const;
    bool equal(const BinaryMatrix& Comp) const;

    void get_data_mpz(BinaryMatrix<mpz_class>& BM_mpz);
    void set_values(const vector<Integer>& V);

    void pretty_print(std::ostream& out, bool with_row_nr = false) const;
};

template <typename Integer>
bool BM_compare(const BinaryMatrix<Integer>& A, const BinaryMatrix<Integer>& B) {
    if (A.get_nr_rows() < B.get_nr_rows())
        return true;
    if (A.get_nr_rows() > B.get_nr_rows())
        return false;
    if (A.get_nr_columns() < B.get_nr_columns())
        return true;
    if (A.get_nr_columns() > B.get_nr_columns())
        return false;
    if (A.get_values() < B.get_values())
        return true;
    if (A.get_values() > B.get_values())
        return false;
    if (A.get_mpz_values() < B.get_mpz_values())
        return true;
    if (A.get_mpz_values() > B.get_mpz_values())
        return false;
    if (A.get_layers() < B.get_layers())
        return true;
    return false;
}
// class end *****************************************************************
//                  LLL with returned transformation matrices
//---------------------------------------------------------------------------

template <typename Integer>  // to break circular dependence
void v_el_trans(const vector<Integer>& av, vector<Integer>& bv, const Integer& F, const size_t start);

template <typename Integer, typename Number>  // ditto
Matrix<Number> LLL_red(const Matrix<Number>& U, Matrix<Integer>& T, Matrix<Integer>& Tinv) {
    // returns Lred =LLL_reduced(U) (sublattice generated by the rows!)
    // Lred=T*M, Tinv=inverse(T)
    // Original version with c = 0.9

    T = Tinv = Matrix<Integer>(U.nr_of_rows());

    Matrix<Number> Lred = U;
    size_t dim = U.nr_of_columns();
    int n = U.nr_of_rows();
    // pretty_print(cout);
    assert((int)U.rank() == n);
    if (n <= 1)
        return Lred;

    Matrix<nmz_float> G(n, dim);
    Matrix<nmz_float> M(n, n);

    Lred.GramSchmidt(G, M, 0, 2);

    int i = 1;
    while (true) {
        for (int j = i - 1; j >= 0; --j) {
            Integer fact;
            /* cout << "MMMMM " << i << " " << j << " " << M[i][j] << endl;
            cout << i << "---" << G[i];
            cout << j << "---" << G[j];*/
            if (std::isnan(M[i][j])) {
                T = Tinv = Matrix<Integer>(U.nr_of_rows());
                return U;
            }
            convert(fact, round(M[i][j]));
            // cout << fact << " " << M[i][j] << endl;
            if (fact != 0) {
                v_el_trans<Number>(Lred[j], Lred[i], -convertTo<Number>(fact), 0);
                v_el_trans<Integer>(T[j], T[i], -fact, 0);
                v_el_trans<Integer>(Tinv[i], Tinv[j], fact, 0);
                Lred.GramSchmidt(G, M, i, i + 1);
            }
        }
        if (i == 0) {
            i = 1;
            Lred.GramSchmidt(G, M, 0, 2);
            continue;
        }
        nmz_float t1 = v_scalar_product(G[i - 1], G[i - 1]);
        nmz_float t2 = v_scalar_product(G[i], G[i]);
        nmz_float fact = 0.9 - M[i][i - 1] * M[i][i - 1];
        if (t2 < fact * t1) {
            std::swap(Lred[i], Lred[i - 1]);
            std::swap(T[i], T[i - 1]);
            std::swap(Tinv[i], Tinv[i - 1]);
            Lred.GramSchmidt(G, M, i - 1, i);  // i-1,i+1);
            // cout << i-1 << "---" << G[i-1];
            i--;
        }
        else {
            i++;
            if (i >= n)
                break;
            Lred.GramSchmidt(G, M, i, i + 1);
        }
    }

    Tinv = Tinv.transpose();

    return Lred;
}

template <typename Integer, typename Number>
Matrix<Number> LLL_red_transpose(const Matrix<Number>& U, Matrix<Integer>& T, Matrix<Integer>& Tinv) {
    // column version -- needed for coordinate transformations in ambient lattice
    // returns Lred=this*T, Tinv=inverse(T)

    Matrix<Number> this_trans = U.transpose();
    Matrix<Number> red_trans;
    Matrix<Integer> T_trans, Tinv_trans;
    red_trans = LLL_red(this_trans, T_trans, Tinv_trans);
    T = T_trans.transpose();
    Tinv = Tinv_trans.transpose();
    return red_trans.transpose();
}

//---------------------------------------------------------------------------
//                  Utilities
//---------------------------------------------------------------------------

template <typename Integer>
class order_helper {
   public:
    vector<Integer> weight;
    key_t index;
    vector<Integer>* v;
};

template <typename T>
vector<vector<T> > to_matrix(const vector<T>& v) {
    vector<vector<T> > mat(1);
    mat[0] = v;
    return mat;
}

// For
// Matrix<Integer> readMatrix(const string project);
// see inout.h

//---------------------------------------------------------------------------
//                  Conversion between integer types
//---------------------------------------------------------------------------

template <typename ToType, typename FromType>
void convert(Matrix<ToType>& to_mat, const Matrix<FromType>& from_mat);

template <typename Integer>
void mat_to_mpz(const Matrix<Integer>& mat, Matrix<mpz_class>& mpz_mat);

template <typename Integer>
void mat_to_Int(const Matrix<mpz_class>& mpz_mat, Matrix<Integer>& mat);

template <typename Integer>
void mpz_submatrix(Matrix<mpz_class>& sub, const Matrix<Integer>& mother, const vector<key_t>& selection);

template <typename Integer>
void mpz_submatrix_trans(Matrix<mpz_class>& sub, const Matrix<Integer>& mother, const vector<key_t>& selection);

template <typename ToType, typename FromType>
void convert(Matrix<ToType>& to_mat, const Matrix<FromType>& from_mat) {
    size_t nrows = from_mat.nr_of_rows();
    size_t ncols = from_mat.nr_of_columns();
    to_mat.resize(nrows, ncols);
    for (size_t i = 0; i < nrows; ++i)
        for (size_t j = 0; j < ncols; ++j)
            convert(to_mat[i][j], from_mat[i][j]);
}

//---------------------------------------------------------------------------
//                  Matrix relateed functions
//---------------------------------------------------------------------------
// determines the maximal subsets in a vector of subsets given by their indicator vectors
// result returned in is_max_subset -- must be initialized outside
// only set to false in this routine
// if a set occurs more than once, only the last instance is recognized as maximal
template <typename IncidenceVector>
void maximal_subsets(const vector<IncidenceVector>& ind, IncidenceVector& is_max_subset);

// computes the incidence of LinForms withz Gens ijn the following sense:
// Incidence[i][j] = 1 <==> scalar product(LinForms[i], Gnes[j]) == 0
template <typename Integer>
void makeIncidenceMatrix(vector<dynamic_bitset>& Incidence, const Matrix<Integer>& Gens, const Matrix<Integer>& LinForms);

}  // namespace libnormaliz

//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
