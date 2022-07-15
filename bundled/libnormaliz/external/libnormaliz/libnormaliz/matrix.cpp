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

#include <fstream>
#include <set>
#include <algorithm>
#include <cmath>
#include <iomanip>

#include "libnormaliz/matrix.h"
#include "libnormaliz/cone.h"
#include "libnormaliz/sublattice_representation.h"
#include "libnormaliz/project_and_lift.h"
#include "libnormaliz/dynamic_bitset.h"

#ifdef NMZ_FLINT
#include "flint/flint.h"
#include "flint/fmpz.h"
#include "flint/fmpz_mat.h"
#endif

//---------------------------------------------------------------------------

namespace libnormaliz {
using namespace std;

//---------------------------------------------------------------------------
// Public
//---------------------------------------------------------------------------

// the templated version is only usable where numbers of larger absolute
// value have longer decomal representations
// slight efficiency advantage compared to specialized version below
template <typename Integer>
vector<size_t> Matrix<Integer>::maximal_decimal_length_columnwise() const {
    size_t i, j = 0;
    vector<size_t> maxim(nc, 0);
    vector<Integer> pos_max(nc, 0), neg_max(nc, 0);
    for (i = 0; i < nr; i++) {
        for (j = 0; j < nc; j++) {
            // maxim[j]=max(maxim[j],decimal_length(elem[i][j]));
            if (elem[i][j] < 0) {
                if (elem[i][j] < neg_max[j])
                    neg_max[j] = elem[i][j];
                continue;
            }
            if (elem[i][j] > pos_max[j])
                pos_max[j] = elem[i][j];
        }
    }
    for (size_t j = 0; j < nc; ++j)
        maxim[j] = max(decimal_length(neg_max[j]), decimal_length(pos_max[j]));
    return maxim;
}

//---------------------------------------------------------------------------

#ifdef ENFNORMALIZ
template <>
vector<size_t> Matrix<renf_elem_class>::maximal_decimal_length_columnwise() const {
    size_t i, j = 0;
    vector<size_t> maxim(nc, 0);
    for (i = 0; i < nr; i++) {
        for (j = 0; j < nc; j++) {
            maxim[j] = max(maxim[j], decimal_length(elem[i][j]));
        }
    }
    return maxim;
}
#endif

template <typename Integer>
Matrix<Integer>::Matrix() {
    nr = 0;
    nc = 0;
}

//---------------------------------------------------------------------------

template <typename Integer>
Matrix<Integer>::Matrix(size_t dim) {
    nr = dim;
    nc = dim;
    elem = vector<vector<Integer> >(dim, vector<Integer>(dim));
    for (size_t i = 0; i < dim; i++) {
        elem[i][i] = 1;
    }
}

//---------------------------------------------------------------------------

template <typename Integer>
Matrix<Integer>::Matrix(size_t row, size_t col) {
    nr = row;
    nc = col;
    elem = vector<vector<Integer> >(row, vector<Integer>(col));
}

//---------------------------------------------------------------------------

template <typename Integer>
Matrix<Integer>::Matrix(size_t row, size_t col, Integer value) {
    nr = row;
    nc = col;
    elem = vector<vector<Integer> >(row, vector<Integer>(col, value));
}

//---------------------------------------------------------------------------

template <typename Integer>
Matrix<Integer>::Matrix(const vector<vector<Integer> >& new_elem) {
    nr = new_elem.size();
    if (nr > 0) {
        nc = new_elem[0].size();
        elem = new_elem;
        // check if all rows have the same length
        for (size_t i = 1; i < nr; i++) {
            if (elem[i].size() != nc) {
                throw BadInputException("Inconsistent lengths of rows in matrix!");
            }
        }
    }
    else {
        nc = 0;
    }
}

//---------------------------------------------------------------------------

template <typename Integer>
Matrix<Integer>::Matrix(const list<vector<Integer> >& new_elem) {
    nr = new_elem.size();
    elem = vector<vector<Integer> >(nr);
    nc = 0;
    size_t i = 0;
    for (const auto& it : new_elem) {
        if (i == 0) {
            nc = it.size();
        }
        else {
            if (it.size() != nc) {
                throw BadInputException("Inconsistent lengths of rows in matrix!");
            }
        }
        elem[i++] = it;
    }
}

//---------------------------------------------------------------------------

template <typename Integer>
Matrix<Integer>::Matrix(const vector<Integer>& row) {
    nr = 1;
    nc = row.size();
    elem.push_back(row);
}

//---------------------------------------------------------------------------
/*
template<typename Integer>
void Matrix<Integer>::write(istream& in){
    size_t i,j;
    for(i=0; i<nr; i++){
        for(j=0; j<nc; j++) {
            in >> elem[i][j];
        }
    }
}
*/
//---------------------------------------------------------------------------

template <typename Integer>
void Matrix<Integer>::write_column(size_t col, const vector<Integer>& data) {
    assert(col >= 0);
    assert(col < nc);
    assert(nr == data.size());

    for (size_t i = 0; i < nr; i++) {
        elem[i][col] = data[i];
    }
}

//---------------------------------------------------------------------------

template <typename Integer>
void Matrix<Integer>::print(const string& name, const string& suffix) const {
    string file_name = name + "." + suffix;
    const char* file = file_name.c_str();
    ofstream out(file);
    print(out);
    out.close();
}

//---------------------------------------------------------------------------

/*
template <typename Integer>
void Matrix<Integer>::print_append(const string& name, const string& suffix) const {
    string file_name = name + "." + suffix;
    const char* file = file_name.c_str();
    ofstream out(file, ios_base::app);
    print(out);
    out.close();
}
*/

//---------------------------------------------------------------------------

template <typename Integer>
void Matrix<Integer>::print(ostream& out, bool with_format) const {
    size_t i, j;
    if (with_format)
        out << nr << endl << nc << endl;
    for (i = 0; i < nr; i++) {
        for (j = 0; j < nc; j++) {
            out << elem[i][j] << " ";
        }
        out << endl;
    }
}

template <typename Integer>
void Matrix<Integer>::debug_print(char mark) const {
    for(int i = 0; i < 19; ++i)
        cout << mark;
    cout << endl;
    pretty_print(cout);
    for(int i = 0; i < 19; ++i)
        cout << mark;
    cout << endl;
}

//---------------------------------------------------------------------------

template <typename Integer>
void Matrix<Integer>::pretty_print(ostream& out, bool with_row_nr, bool count_from_one) const {
    if (nr > 1000000 && !with_row_nr) {
        print(out, false);
        return;
    }
    size_t i, j;
    vector<size_t> max_length = maximal_decimal_length_columnwise();
    size_t max_index_length = decimal_length(nr);
    if (count_from_one)
        max_index_length = decimal_length(nr + 1);
    for (i = 0; i < nr; i++) {
        if (with_row_nr) {
            size_t j = i;
            if (count_from_one)
                j++;
            out << std::setw(max_index_length + 1) << std::setprecision(6) << j << ": ";
        }
        for (j = 0; j < nc; j++) {
            out << std::setw(max_length[j] + 1) << std::setprecision(6) << elem[i][j];
        }
        out << endl;
    }
}

#ifdef ENFNORMALIZ
template <>
void Matrix<renf_elem_class>::pretty_print(ostream& out, bool with_row_nr, bool count_from_one) const {
    if (nr > 1000000 && !with_row_nr) {
        print(out);
        return;
    }
    size_t i, j, k;
    vector<size_t> max_length = maximal_decimal_length_columnwise();
    size_t max_index_length = decimal_length(nr);
    if (count_from_one)
        max_index_length = decimal_length(nr + 1);
    for (i = 0; i < nr; i++) {
        if (with_row_nr) {
            size_t j = i;
            if (count_from_one)
                j++;
            for (k = 0; k <= max_index_length - decimal_length(j); k++) {
                out << " ";
            }
            out << j << ": ";
        }
        for (j = 0; j < nc; j++) {
            ostringstream to_print;
            to_print << elem[i][j];
            for (k = 0; k <= max_length[j] - to_print.str().size(); k++) {
                out << " ";
            }
            out << to_print.str();
        }
        out << endl;
    }
}
#endif

//---------------------------------------------------------------------------

template <>
void Matrix<nmz_float>::pretty_print(ostream& out, bool with_row_nr, bool count_from_one) const {
    for (size_t i = 0; i < nr; ++i) {
        if (with_row_nr) {
            size_t j = i;
            if (count_from_one)
                j++;
            out << std::setw(7) << j << ": ";
        }
        for (size_t j = 0; j < nc; ++j) {
            out << std::setw(10) << elem[i][j] << " ";
        }
        out << endl;
    }
}
//---------------------------------------------------------------------------

template <typename Integer>
size_t Matrix<Integer>::nr_of_rows() const {
    return nr;
}

//---------------------------------------------------------------------------

template <typename Integer>
size_t Matrix<Integer>::nr_of_columns() const {
    return nc;
}

//---------------------------------------------------

template <typename Integer>
void Matrix<Integer>::Shrink_nr_rows(size_t new_nr_rows) {
    if (new_nr_rows >= nr)
        return;
    nr = new_nr_rows;
    elem.resize(nr);
}

//---------------------------------------------------------------------------

template <typename Integer>
void Matrix<Integer>::set_nr_of_columns(size_t c) {
    nc = c;
}

//---------------------------------------------------------------------------

template <typename Integer>
bool Matrix<Integer>::check_projection(vector<key_t>& projection_key) {
    // coordinate proection_key[i] is mapped to coordinate i
    // we do not check whether the matrix has full rank

    /*cout << "----" << endl;
    pretty_print(cout);
    cout << "****" << endl;*/

    vector<key_t> tentative_key;
    for (size_t j = 0; j < nc; ++j) {
        size_t i = 0;
        for (; i < nr; ++i) {
            if (elem[i][j] != 0) {
                if (elem[i][j] == 1)
                    break;
                else {
                    return false;
                }
            }
        }
        if (i == nr) {  // column is zero
            return false;
        }
        tentative_key.push_back(i);
        i++;
        for (; i < nr; i++) {
            if (elem[i][j] != 0) {
                return false;
            }
        }
    }

    projection_key = tentative_key;
    // cout << "~~~~~~~~~ " << projection_key;
    return true;
}

//---------------------------------------------------------------------------

template <typename Integer>
Matrix<Integer> Matrix<Integer>::select_coordinates(const vector<key_t>& projection_key) const {
    Matrix<Integer> N(nr, projection_key.size());
    for (size_t i = 0; i < nr; ++i)
        N[i] = v_select_coordinates(elem[i], projection_key);
    return N;
}

//---------------------------------------------------------------------------

template <typename Integer>
Matrix<Integer> Matrix<Integer>::insert_coordinates(const vector<key_t>& projection_key, const size_t nr_cols) const {
    Matrix<Integer> N(nr, nr_cols);
    for (size_t i = 0; i < nr; ++i)
        N[i] = v_insert_coordinates(elem[i], projection_key, nr_cols);
    return N;
}

//---------------------------------------------------------------------------

/*
template <typename Integer>
void Matrix<Integer>::random(int mod) {
    size_t i, j;
    int k;
    for (i = 0; i < nr; i++) {
        for (j = 0; j < nc; j++) {
            k = rand();
            elem[i][j] = k % mod;
        }
    }
}
*/
//---------------------------------------------------------------------------

template <typename Integer>
void Matrix<Integer>::set_zero() {
    size_t i, j;
    for (i = 0; i < nr; i++) {
        for (j = 0; j < nc; j++) {
            elem[i][j] = 0;
        }
    }
}

//---------------------------------------------------------------------------

template <typename Integer>
void Matrix<Integer>::select_submatrix(const Matrix<Integer>& mother, const vector<key_t>& rows) {
    assert(nr >= rows.size());
    assert(nc >= mother.nc);

    size_t size = rows.size(), j;
    for (size_t i = 0; i < size; i++) {
        j = rows[i];
        for (size_t k = 0; k < mother.nc; ++k)
            elem[i][k] = mother[j][k];
    }
}

//---------------------------------------------------------------------------

template <typename Integer>
void Matrix<Integer>::select_submatrix_trans(const Matrix<Integer>& mother, const vector<key_t>& rows) {
    assert(nc >= rows.size());
    assert(nr >= mother.nc);

    size_t size = rows.size(), j;
    for (size_t i = 0; i < size; i++) {
        j = rows[i];
        for (size_t k = 0; k < mother.nc; ++k)
            elem[k][i] = mother[j][k];
    }
}

//---------------------------------------------------------------------------

template <typename Integer>
Matrix<Integer> Matrix<Integer>::submatrix(const vector<key_t>& rows) const {
    size_t size = rows.size(), j;
    Matrix<Integer> M(size, nc);
    for (size_t i = 0; i < size; i++) {
        j = rows[i];
        assert(j >= 0);
        assert(j < nr);
        M.elem[i] = elem[j];
    }
    return M;
}

//---------------------------------------------------------------------------

/*
template <typename Integer>
Matrix<Integer> Matrix<Integer>::submatrix(const vector<int>& rows) const {
    size_t size = rows.size(), j;
    Matrix<Integer> M(size, nc);
    for (size_t i = 0; i < size; i++) {
        j = rows[i];
        assert(j >= 0);
        assert(j < nr);
        M.elem[i] = elem[j];
    }
    return M;
}
*/

//---------------------------------------------------------------------------

template <typename Integer>
Matrix<Integer> Matrix<Integer>::submatrix(const vector<bool>& rows) const {
    assert(rows.size() == nr);
    size_t size = 0;
    for (const auto& row : rows) {
        if (row) {
            size++;
        }
    }
    Matrix<Integer> M(size, nc);
    size_t j = 0;
    for (size_t i = 0; i < nr; i++) {
        if (rows[i]) {
            M.elem[j++] = elem[i];
        }
    }
    return M;
}

/*//---------------------------------------------------------------------------

template<typename Integer>
Matrix<Integer> Matrix<Integer>::submatrix(const dynamic_bitset& rows) const{
    assert(rows.size() == nr);
    size_t size=0;
    for (size_t i = 0; i <rows.size(); i++) {
        if (rows[i]) {
            size++;
        }
    }
    Matrix<Integer> M(size, nc);
    size_t j = 0;
    for (size_t i = 0; i < nr; i++) {
        if (rows[i]) {
            M.elem[j++] = elem[i];
        }
    }
    return M;
}*/

//---------------------------------------------------------------------------

template <typename Integer>
Matrix<Integer> Matrix<Integer>::select_columns(const vector<bool>& cols) const {
    return transpose().submatrix(cols).transpose();
}

//---------------------------------------------------------------------------

template <typename Integer>
Matrix<Integer> Matrix<Integer>::selected_columns_first(const vector<bool>& cols) const {
    assert(cols.size() == nc);
    Matrix<Integer> M(nr, nc);
    for (size_t i = 0; i < nr; ++i) {
        size_t j = 0;
        for (size_t k = 0; k < nc; ++k)
            if (cols[k]) {
                M[i][j] = elem[i][k];
                j++;
            }
        for (size_t k = 0; k < nc; ++k)
            if (!cols[k]) {
                M[i][j] = elem[i][k];
                j++;
            }
    }
    return M;
}

//---------------------------------------------------------------------------

/*
template <typename Integer>
Matrix<Integer>& Matrix<Integer>::remove_zero_rows() {
    size_t from = 0, to = 0;  // maintain to <= from
    while (from < nr && v_is_zero(elem[from]))
        from++;          // skip zero rows
    while (from < nr) {  // go over matrix
        // now from is a non-zero row
        if (to != from)
            elem[to].swap(elem[from]);
        ++to;
        ++from;
        while (from < nr && v_is_zero(elem[from]))
            from++;  // skip zero rows
    }
    nr = to;
    elem.resize(nr);
    return *this;
}
*/

//---------------------------------------------------------------------------

template <typename Integer>
Matrix<nmz_float> Matrix<Integer>::nmz_float_without_first_column() const {
    Matrix<nmz_float> Ret(nr, nc - 1);
    for (size_t i = 0; i < nr; ++i)  // without first column
        for (size_t j = 1; j < nc; ++j)
            convert(Ret[i][j - 1], elem[i][j]);

    // We scale the inequalities for LLL so that right hand side has absolute value 1
    // If RHS is zero, we divide by absolute value of first non-zero element
    for (size_t i = 0; i < nr; ++i) {
        nmz_float denom = Iabs(convertTo<nmz_float>(elem[i][0]));
        if (denom == 0) {
            denom = 1;  // auxiliary choice if vector is 0 everywhere
            for (size_t j = 0; j < Ret.nc; ++j)
                if (Ret[i][j] != 0)
                    denom = Iabs(Ret[i][j]);
        }
        v_scalar_division(Ret[i], denom);
    }

    return Ret;
}

template <>
Matrix<nmz_float> Matrix<mpq_class>::nmz_float_without_first_column() const {
    assert(false);
    return Matrix<nmz_float>(0, 0);
}

#ifdef ENFNORMALIZ
template <>
Matrix<nmz_float> Matrix<renf_elem_class>::nmz_float_without_first_column() const {
    assert(false);
    return Matrix<nmz_float>(0, 0);
}
#endif

//---------------------------------------------------------------------------

template <typename Integer>
void Matrix<Integer>::swap(Matrix<Integer>& x) {
    size_t tmp = nr;
    nr = x.nr;
    x.nr = tmp;
    tmp = nc;
    nc = x.nc;
    x.nc = tmp;
    elem.swap(x.elem);
}

//---------------------------------------------------------------------------

template <typename Integer>
void Matrix<Integer>::resize(size_t nr_rows, size_t nr_cols) {
    nc = nr_cols;  // for adding new rows with the right length
    resize(nr_rows);
    resize_columns(nr_cols);
}

template <typename Integer>
void Matrix<Integer>::resize(size_t nr_rows) {
    if (nr_rows > elem.size()) {
        elem.resize(nr_rows, vector<Integer>(nc));
    }
    if (nr_rows < elem.size())
        elem.resize(nr_rows);
    nr = nr_rows;
}

template <typename Integer>
void Matrix<Integer>::resize_columns(size_t nr_cols) {
    for (size_t i = 0; i < nr; i++) {
        elem[i].resize(nr_cols);
    }
    nc = nr_cols;
}

//---------------------------------------------------------------------------

/*
template <typename Integer>
vector<Integer> Matrix<Integer>::diagonal() const {
    assert(nr == nc);
    vector<Integer> diag(nr);
    for (size_t i = 0; i < nr; i++) {
        diag[i] = elem[i][i];
    }
    return diag;
}
*/

//---------------------------------------------------------------------------

/*
template <typename Integer>
size_t Matrix<Integer>::maximal_decimal_length() const {
    size_t i, maxim = 0;
    vector<size_t> maxim_col;
    maxim_col = maximal_decimal_length_columnwise();
    for (i = 0; i < nr; i++)
        maxim = max(maxim, maxim_col[i]);
    return maxim;
}
*/

//---------------------------------------------------------------------------

template <typename Integer>
void Matrix<Integer>::append(const Matrix<Integer>& M) {
    assert(nc == M.nc);
    elem.resize(nr);
    /* for (size_t i=0; i<M.nr; i++) {
        elem.push_back(M.elem[i]);
    }*/
    elem.insert(elem.end(), M.elem.begin(), M.elem.end());
    nr += M.nr;
}

//---------------------------------------------------------------------------

template <typename Integer>
void Matrix<Integer>::append(const vector<vector<Integer> >& M) {
    if (M.size() == 0)
        return;
    assert(nc == M[0].size());
    elem.resize(nr);
    for (size_t i = 0; i < M.size(); i++) {
        elem.push_back(M[i]);
    }
    nr += M.size();
}

//---------------------------------------------------------------------------

template <typename Integer>
void Matrix<Integer>::append(const vector<Integer>& V) {
    assert(nc == V.size());
    elem.resize(nr);
    elem.push_back(V);
    nr++;
}

//---------------------------------------------------------------------------

template <typename Integer>
void Matrix<Integer>::append_column(const vector<Integer>& v) {
    assert(nr == v.size());
    for (size_t i = 0; i < nr; i++) {
        elem[i].resize(nc + 1);
        elem[i][nc] = v[i];
    }
    nc++;
}

//---------------------------------------------------------------------------

template <typename Integer>
void Matrix<Integer>::insert_column(const size_t pos, const vector<Integer>& v) {
    assert(nr == v.size());
    for (size_t i = 0; i < nr; i++) {
        elem[i].resize(nc + 1);
        for (long j = nc - 1; j >= (long)pos; --j)
            elem[i][j + 1] = elem[i][j];
        elem[i][pos] = v[i];
    }
    nc++;
}

//-----------------------------------------------------

template <typename Integer>
void Matrix<Integer>::insert_column(const size_t pos, const Integer& val) {
    for (size_t i = 0; i < nr; i++) {
        elem[i].resize(nc + 1);
        for (long j = nc - 1; j >= (long)pos; --j)
            elem[i][j + 1] = elem[i][j];
        elem[i][pos] = val;
    }
    nc++;
}

//---------------------------------------------------------------------------

template <typename Integer>
void Matrix<Integer>::remove_row(const vector<Integer>& row) {
    size_t tmp_nr = nr;
    for (size_t i = 1; i <= tmp_nr; ++i) {
        if (elem[tmp_nr - i] == row) {
            elem.erase(elem.begin() + (tmp_nr - i));
            nr--;
        }
    }
}

//---------------------------------------------------------------------------

template <typename Integer>
void Matrix<Integer>::remove_row(const size_t index) {
    assert(index < nr);
    nr--;
    elem.erase(elem.begin() + (index));
}

//---------------------------------------------------------------------------

template <typename Integer>
vector<size_t> Matrix<Integer>::remove_duplicate_and_zero_rows() {
    bool remove_some = false;
    vector<bool> key(nr, true);
    vector<size_t> original_row;

    set<vector<Integer> > SortedRows;
    SortedRows.insert(vector<Integer>(nc, 0));
    for (size_t i = 0; i < nr; i++) {
        auto found = SortedRows.find(elem[i]);
        if (found != SortedRows.end()) {
            key[i] = false;
            remove_some = true;
        }
        else {
            SortedRows.insert(found, elem[i]);
            original_row.push_back(i);
        }
    }

    if (remove_some) {
        *this = submatrix(key);
    }
    return original_row;
}

//---------------------------------------------------------------------------

/*
template <typename Integer>
void Matrix<Integer>::remove_duplicate(const Matrix<Integer>& M) {
    bool remove_some = false;
    vector<bool> key(nr, true);

    // TODO more efficient! sorted rows
    for (size_t i = 0; i < nr; i++) {
        for (size_t j = 0; j < M.nr_of_rows(); j++) {
            if (elem[i] == M[j]) {
                remove_some = true;
                key[i] = false;
                break;
            }
        }
    }

    if (remove_some) {
        *this = submatrix(key);
    }
}
*/

//---------------------------------------------------------------------------

/*
template <typename Integer>
Matrix<Integer> Matrix<Integer>::add(const Matrix<Integer>& A) const {
    assert(nr == A.nr);
    assert(nc == A.nc);

    Matrix<Integer> B(nr, nc);
    size_t i, j;
    for (i = 0; i < nr; i++) {
        for (j = 0; j < nc; j++) {
            B.elem[i][j] = elem[i][j] + A.elem[i][j];
        }
    }
    return B;
}
*/
//---------------------------------------------------------------------------

// B = (*this)*A.transpose()
template <typename Integer>
void Matrix<Integer>::multiplication_trans(Matrix<Integer>& B, const Matrix<Integer>& A) const {
    assert(nc == A.nc);
    assert(B.nr == nr);
    assert(B.nc == A.nr);

    bool skip_remaining = false;
    std::exception_ptr tmp_exception;

#pragma omp parallel for
    for (size_t i = 0; i < B.nr; i++) {
        if (skip_remaining)
            continue;
        try {
            INTERRUPT_COMPUTATION_BY_EXCEPTION

            for (size_t j = 0; j < B.nc; j++) {
                B[i][j] = v_scalar_product(elem[i], A[j]);
            }
        } catch (const std::exception&) {
            tmp_exception = std::current_exception();
            skip_remaining = true;
#pragma omp flush(skip_remaining)
        }
    }  // end for i

    if (!(tmp_exception == 0))
        std::rethrow_exception(tmp_exception);
}

//---------------------------------------------------------------------------

// B = (*this)*A
template <typename Integer>
void Matrix<Integer>::multiplication(Matrix<Integer>& B, const Matrix<Integer>& A) const {
    multiplication_trans(B, A.transpose());
}
//---------------------------------------------------------------------------

template <typename Integer>
Matrix<Integer> Matrix<Integer>::multiplication(const Matrix<Integer>& A) const {
    Matrix<Integer> B(nr, A.nc);
    multiplication(B, A);
    return B;
}

//---------------------------------------------------------------------------

template <typename Integer>
Matrix<Integer> Matrix<Integer>::multiplication_trans(const Matrix<Integer>& A) const {
    Matrix<Integer> B(nr, A.nr);
    multiplication_trans(B, A);
    return B;
}

//---------------------------------------------------------------------------

/*
template <typename Integer>
Matrix<Integer> Matrix<Integer>::multiplication(const Matrix<Integer>& A, long m) const {
    assert(nc == A.nr);

    Matrix<Integer> B(nr, A.nc, 0);  // initialized with 0
    size_t i, j, k;
    for (i = 0; i < B.nr; i++) {
        for (j = 0; j < B.nc; j++) {
            for (k = 0; k < nc; k++) {
                B.elem[i][j] = (B.elem[i][j] + elem[i][k] * A.elem[k][j]) % m;
                if (B.elem[i][j] < 0) {
                    B.elem[i][j] = B.elem[i][j] + m;
                }
            }
        }
    }
    return B;
}

template <>
Matrix<nmz_float> Matrix<nmz_float>::multiplication(const Matrix<nmz_float>& A, long m) const {
    assert(false);
    return A;
}

template <>
Matrix<mpq_class> Matrix<mpq_class>::multiplication(const Matrix<mpq_class>& A, long m) const {
    assert(false);
    return A;
}

#ifdef ENFNORMALIZ
template <>
Matrix<renf_elem_class> Matrix<renf_elem_class>::multiplication(const Matrix<renf_elem_class>& A, long m) const {
    assert(false);
    return A;
}
#endif
*/
//---------------------------------------------------------------------------

template <typename Integer>
bool Matrix<Integer>::equal(const Matrix<Integer>& A) const {
    if ((nr != A.nr) || (nc != A.nc)) {
        return false;
    }
    size_t i, j;
    for (i = 0; i < nr; i++) {
        for (j = 0; j < nc; j++) {
            if (elem[i][j] != A.elem[i][j]) {
                return false;
            }
        }
    }
    return true;
}

//---------------------------------------------------------------------------
/*
template<typename Integer>
bool Matrix<Integer>::equal(const Matrix<Integer>& A, long m) const{
    if ((nr!=A.nr)||(nc!=A.nc)){  return false; }
    size_t i,j;
    for (i=0; i < nr; i++) {
        for (j = 0; j < nc; j++) {
            if (((elem[i][j]-A.elem[i][j]) % m)!=0) {
                return false;
            }
        }
    }
    return true;
}
*/
//---------------------------------------------------------------------------

template <typename Integer>
Matrix<Integer> Matrix<Integer>::transpose() const {
    Matrix<Integer> B(nc, nr);
    size_t i, j;
    for (i = 0; i < nr; i++) {
        for (j = 0; j < nc; j++) {
            B.elem[j][i] = elem[i][j];
        }
    }
    return B;
}

//---------------------------------------------------------------------------

template <typename Integer>
void Matrix<Integer>::transpose_in_place() {
    assert(nr == nc);
    size_t i, j;
    Integer help;
    for (i = 0; i < nr; i++) {
        for (j = i + 1; j < nc; j++) {
            help = elem[i][j];
            elem[i][j] = elem[j][i];
            elem[j][i] = help;
        }
    }
}
//---------------------------------------------------------------------------

template <typename Integer>
void Matrix<Integer>::scalar_multiplication(const Integer& scalar) {
    size_t i, j;
    for (i = 0; i < nr; i++) {
        for (j = 0; j < nc; j++) {
            elem[i][j] *= scalar;
        }
    }
}

//---------------------------------------------------------------------------

template <typename Integer>
void Matrix<Integer>::scalar_division(const Integer& scalar) {
    size_t i, j;
    assert(scalar != 0);
    if (scalar == 1)
        return;
    for (i = 0; i < nr; i++) {
        for (j = 0; j < nc; j++) {
            assert(elem[i][j] % scalar == 0);
            elem[i][j] /= scalar;
        }
    }
}

template <>
void Matrix<nmz_float>::scalar_division(const nmz_float& scalar) {
    assert(false);
}
/* body
    size_t i, j;
    assert(scalar != 0);
    for (i = 0; i < nr; i++) {
        for (j = 0; j < nc; j++) {
            elem[i][j] /= scalar;
        }
    }
}
*/

template <>
void Matrix<mpq_class>::scalar_division(const mpq_class& scalar) {
    assert(false);
}
/*
    size_t i, j;
    assert(scalar != 0);
    for (i = 0; i < nr; i++) {
        for (j = 0; j < nc; j++) {
            elem[i][j] /= scalar;
        }
    }
}*/

#ifdef ENFNORMALIZ
template <>
void Matrix<renf_elem_class>::scalar_division(const renf_elem_class& scalar) {
    size_t i, j;
    assert(scalar != 0);
    if (scalar == 1)
        return;
    for (i = 0; i < nr; i++) {
        for (j = 0; j < nc; j++) {
            elem[i][j] /= scalar;
        }
    }
}
#endif
//---------------------------------------------------------------------------

/*
template <typename Integer>
void Matrix<Integer>::reduction_modulo(const Integer& modulo) {
    size_t i, j;
    for (i = 0; i < nr; i++) {
        for (j = 0; j < nc; j++) {
            elem[i][j] %= modulo;
            if (elem[i][j] < 0) {
                elem[i][j] += modulo;
            }
        }
    }
}

template <>
void Matrix<nmz_float>::reduction_modulo(const nmz_float& modulo) {
    assert(false);
}

template <>
void Matrix<mpq_class>::reduction_modulo(const mpq_class& modulo) {
    assert(false);
}

#ifdef ENFNORMALIZ
template <>
void Matrix<renf_elem_class>::reduction_modulo(const renf_elem_class& modulo) {
    assert(false);
}
#endif
*/

//---------------------------------------------------------------------------

template <typename Integer>
Integer Matrix<Integer>::matrix_gcd() const {
    Integer g = 0, h;
    for (size_t i = 0; i < nr; i++) {
        h = v_gcd(elem[i]);
        g = libnormaliz::gcd<Integer>(g, h);
        if (g == 1)
            return g;
    }
    return g;
}

template <>
mpq_class Matrix<mpq_class>::matrix_gcd() const {
    assert(false);
    return 1;
}

#ifdef ENFNORMALIZ
template <>
renf_elem_class Matrix<renf_elem_class>::matrix_gcd() const {
    assert(false);
    return 1;
}

#endif

//---------------------------------------------------------------------------

template <typename Integer>
vector<Integer> Matrix<Integer>::make_prime() {
    vector<Integer> g(nr);
    for (size_t i = 0; i < nr; i++) {
        g[i] = v_make_prime(elem[i]);
    }
    return g;
}

//---------------------------------------------------------------------------

template <typename Integer>
void Matrix<Integer>::make_cols_prime(size_t from_col, size_t to_col) {
    for (size_t k = from_col; k <= to_col; k++) {
        Integer g = 0;
        for (size_t i = 0; i < nr; i++) {
            g = libnormaliz::gcd(g, elem[i][k]);
            if (g == 1) {
                break;
            }
        }
        for (size_t i = 0; i < nr; i++)
            elem[i][k] /= g;
    }
}

template <>
void Matrix<mpq_class>::make_cols_prime(size_t from_col, size_t to_col) {
    assert(false);
}

#ifdef ENFNORMALIZ
template <>
void Matrix<renf_elem_class>::make_cols_prime(size_t from_col, size_t to_col) {
    assert(false);
}
#endif
//---------------------------------------------------------------------------

/*
template <typename Integer>
Matrix<Integer> Matrix<Integer>::multiply_rows(const vector<Integer>& m) const {  // row i is multiplied by m[i]
    Matrix M = Matrix(nr, nc);
    size_t i, j;
    for (i = 0; i < nr; i++) {
        for (j = 0; j < nc; j++) {
            M.elem[i][j] = elem[i][j] * m[i];
        }
    }
    return M;
}*/

template <typename Integer>
void Matrix<Integer>::standardize_basis() {
    row_echelon_reduce();
}

#ifdef ENFNORMALIZ
template <>
void Matrix<renf_elem_class>::make_first_element_1_in_rows() {
    for (size_t i = 0; i < nr; ++i) {
        for (size_t j = 0; j < nc; ++j) {
            if (elem[i][j] != 0) {
                renf_elem_class pivot = elem[i][j];
                v_scalar_division(elem[i], pivot);
                break;
            }
        }
    }
}

template <>
void Matrix<renf_elem_class>::standardize_basis() {
    row_echelon_reduce();
    make_first_element_1_in_rows();
}
#endif

template <typename Integer>
bool Matrix<Integer>::standardize_rows(const vector<Integer>& Norm) {
    assert(false);
    return {};
}

template <typename Integer>
bool Matrix<Integer>::standardize_rows() {
    assert(false);
    return {};
}

template <>
bool Matrix<nmz_float>::standardize_rows(const vector<nmz_float>& Norm) {
    nmz_float val;
    bool non_zero = true;
    for (size_t i = 0; i < nr; i++) {
        val = v_standardize(elem[i], Norm);
        if (val == 0)
            non_zero = false;
    }
    return non_zero;
}

template <>
bool Matrix<nmz_float>::standardize_rows() {
    vector<nmz_float> dummy(0);
    for (size_t i = 0; i < nr; i++) {
        v_standardize(elem[i], dummy);
    }
    return true;
}

//---------------------------------------------------------------------------

#ifdef ENFNORMALIZ
template <>
bool Matrix<renf_elem_class>::standardize_rows(const vector<renf_elem_class>& Norm) {
    renf_elem_class val;
    bool non_zero = true;
    for (size_t i = 0; i < nr; i++) {
        val = v_standardize(elem[i], Norm);
        if (val == 0)
            non_zero = false;
    }
    return non_zero;
}

template <>
bool Matrix<renf_elem_class>::standardize_rows() {
    vector<renf_elem_class> dummy(0);
    for (size_t i = 0; i < nr; i++) {
        v_standardize(elem[i], dummy);
    }
    return true;
}
#endif

//---------------------------------------------------------------------------

template <typename Integer>
void Matrix<Integer>::MxV(vector<Integer>& result, const vector<Integer>& v) const {
    assert(nc == v.size());
    result.resize(nr);
    for (size_t i = 0; i < nr; i++) {
        result[i] = v_scalar_product(elem[i], v);
    }
}

//---------------------------------------------------------------------------

template <typename Integer>
vector<Integer> Matrix<Integer>::MxV(const vector<Integer>& v) const {
    vector<Integer> w(nr);
    MxV(w, v);
    return w;
}

//---------------------------------------------------------------------------

template <typename Integer>
vector<Integer> Matrix<Integer>::VxM(const vector<Integer>& v) const {
    assert(nr == v.size());
    vector<Integer> w(nc, 0);
    size_t i, j;
    for (i = 0; i < nc; i++) {
        for (j = 0; j < nr; j++) {
            w[i] += v[j] * elem[j][i];
        }
        if (!check_range(w[i]))
            break;
    }
    if (i == nc)
        return w;
    Matrix<mpz_class> mpz_this(nr, nc);
    mat_to_mpz(*this, mpz_this);
    vector<mpz_class> mpz_v(nr);
    convert(mpz_v, v);
    vector<mpz_class> mpz_w = mpz_this.VxM(mpz_v);
    convert(w, mpz_w);
    return w;
}

template <>
vector<mpq_class> Matrix<mpq_class>::VxM(const vector<mpq_class>& v) const {
    assert(false);
    return {};
}
/* body
    assert(nr == v.size());
    vector<mpq_class> w(nc, 0);
    size_t i, j;
    for (i = 0; i < nc; i++) {
        for (j = 0; j < nr; j++) {
            w[i] += v[j] * elem[j][i];
        }
    }
    return w;
}
*/

//---------------------------------------------------------------------------

template <typename Integer>
vector<Integer> Matrix<Integer>::VxM_div(const vector<Integer>& v, const Integer& divisor, bool& success) const {
    assert(nr == v.size());
    vector<Integer> w(nc, 0);
    success = true;
    size_t i, j;
    for (i = 0; i < nc; i++) {
        for (j = 0; j < nr; j++) {
            w[i] += v[j] * elem[j][i];
        }
        if (!check_range(w[i])) {
            success = false;
            break;
        }
    }

    if (success)
        v_scalar_division(w, divisor);

    return w;
}

template <>
vector<nmz_float> Matrix<nmz_float>::VxM_div(const vector<nmz_float>& v, const nmz_float& divisor, bool& success) const {
    assert(false);
    return {};
}

//---------------------------------------------------------------------------

template <typename Integer>
bool Matrix<Integer>::check_congruences(const vector<Integer>& v) const {
    // if(nr==0)
    //   return true;

    assert(nc == v.size() + 1);

    for (size_t k = 0; k < nr; ++k) {
        if (v_scalar_product_vectors_unequal_lungth(v, elem[k]) % elem[k][nc - 1] != 0) {  // congruence not satisfied
            return false;
        }
    }
    return true;
}

template <>
bool Matrix<nmz_float>::check_congruences(const vector<nmz_float>& v) const {
    assert(false);
    return false;
}

template <>
bool Matrix<mpq_class>::check_congruences(const vector<mpq_class>& v) const {
    assert(false);
    return false;
}

#ifdef ENFNORMALIZ
template <>
bool Matrix<renf_elem_class>::check_congruences(const vector<renf_elem_class>& v) const {
    assert(false);
    return false;
}
#endif

//---------------------------------------------------------------------------

template <typename Integer>
bool Matrix<Integer>::is_diagonal() const {
    for (size_t i = 0; i < nr; ++i)
        for (size_t j = 0; j < nc; ++j)
            if (i != j && elem[i][j] != 0)
                return false;
    return true;
}

//---------------------------------------------------------------------------

template <typename Integer>
vector<long> Matrix<Integer>::pivot(size_t corner) {
    assert(corner < nc);
    assert(corner < nr);
    size_t i, j;
    Integer help = 0;
    vector<long> v(2, -1);

    for (i = corner; i < nr; i++) {
        for (j = corner; j < nc; j++) {
            if (elem[i][j] != 0) {
                if ((help == 0) || (Iabs(elem[i][j]) < help)) {
                    help = Iabs(elem[i][j]);
                    v[0] = i;
                    v[1] = j;
                    if (help == 1)
                        return v;
                }
            }
        }
    }

    return v;
}

//---------------------------------------------------------------------------

template <typename Integer>
long Matrix<Integer>::pivot_in_column(size_t row, size_t col) {
    assert(col < nc);
    assert(row < nr);
    size_t i;
    long j = -1;
    Integer help = 0;

    for (i = row; i < nr; i++) {
        if (elem[i][col] != 0) {
            if ((help == 0) || (Iabs(elem[i][col]) < help)) {
                help = Iabs(elem[i][col]);
                j = i;
                if (help == 1)
                    return j;
            }
        }
    }

    return j;
}

//---------------------------------------------------------------------------

template <typename Integer>
long Matrix<Integer>::pivot_in_column(size_t col) {
    return pivot_in_column(col, col);
}

//---------------------------------------------------------------------------

template <typename Integer>
void Matrix<Integer>::exchange_rows(const size_t& row1, const size_t& row2) {
    if (row1 == row2)
        return;
    assert(row1 < nr);
    assert(row2 < nr);
    elem[row1].swap(elem[row2]);
}

//---------------------------------------------------------------------------

template <typename Integer>
void Matrix<Integer>::exchange_columns(const size_t& col1, const size_t& col2) {
    if (col1 == col2)
        return;
    assert(col1 < nc);
    assert(col2 < nc);
    for (size_t i = 0; i < nr; i++) {
        std::swap(elem[i][col1], elem[i][col2]);
    }
}

//---------------------------------------------------------------------------

template <typename Integer>
bool Matrix<Integer>::reduce_row(size_t row, size_t col) {
    assert(col < nc);
    assert(row < nr);
    size_t i, j;
    Integer help;
    if (using_renf<Integer>()) {
        for (i = row + 1; i < nr; i++) {
            if (elem[i][col] != 0) {
                elem[i][col] /= elem[row][col];
                for (j = col + 1; j < nc; j++) {
                    if (elem[row][j] != 0) {
                        help = elem[i][col];
                        help *= elem[row][j];
                        elem[i][j] -= help;
                    }
                }
                elem[i][col] = 0;
            }
        }
    }
    else {
        Integer help1;
        for (i = row + 1; i < nr; i++) {
            if (elem[i][col] != 0) {
                help = elem[i][col];
                help /= elem[row][col];
                for (j = col; j < nc; j++) {
                    help1 = help;
                    help1 *= elem[row][j];
                    elem[i][j] -= help1;
                    if (!check_range(elem[i][j])) {
                        return false;
                    }
                }
                if (using_float<Integer>())
                    elem[i][col] = 0;
            }
        }
    }
    return true;
}

//---------------------------------------------------------------------------

template <typename Integer>
bool Matrix<Integer>::reduce_row(size_t corner) {
    return reduce_row(corner, corner);
}

//---------------------------------------------------------------------------

template <typename Integer>
bool Matrix<Integer>::reduce_rows_upwards() {
    // assumes that "this" is in row echelon form
    // and reduces eevery column in which the rank jumps
    // by its lowest element

    if (nr == 0)
        return true;

    for (size_t row = 0; row < nr; ++row) {
        size_t col;
        for (col = 0; col < nc; ++col)
            if (elem[row][col] != 0)
                break;
        if (col == nc)
            continue;
        if (elem[row][col] < 0)
            v_scalar_multiplication<Integer>(elem[row], -1);

        for (long i = row - 1; i >= 0; --i) {
            Integer quot, rem;

            minimal_remainder(elem[i][col], elem[row][col], quot, rem);
            elem[i][col] = rem;
            for (size_t j = col + 1; j < nc; ++j) {
                elem[i][j] -= quot * elem[row][j];
                if (!check_range(elem[i][j])) {
                    return false;
                }
            }
        }
    }
    return true;
}

template <>
bool Matrix<nmz_float>::reduce_rows_upwards() {
    assert(false);  // for the time being
    return true;
}

#ifdef ENFNORMALIZ
template <>
bool Matrix<renf_elem_class>::reduce_rows_upwards() {
    // assumes that "this" is in row echelon form
    // and reduces eevery column in which the rank jumps
    // by its lowest element
    //
    if (nr == 0)
        return true;

    for (size_t row = 0; row < nr; ++row) {
        size_t col;
        for (col = 0; col < nc; ++col)
            if (elem[row][col] != 0)
                break;
        if (col == nc)  // zero row
            continue;
        if (elem[row][col] < 0)
            v_scalar_multiplication<renf_elem_class>(elem[row], -1);  // make corner posizive

        for (long i = row - 1; i >= 0; --i) {
            renf_elem_class quot;
            // minimal_remainder(elem[i][col],elem[row][col],quot,rem);
            quot = elem[i][col] / elem[row][col];
            elem[i][col] = 0;  // rem
            for (size_t j = col + 1; j < nc; ++j) {
                elem[i][j] -= quot * elem[row][j];
            }
        }
    }

    return true;
}
#endif

//---------------------------------------------------------------------------

template <typename Integer>
bool Matrix<Integer>::linear_comb_columns(
    const size_t& col, const size_t& j, const Integer& u, const Integer& w, const Integer& v, const Integer& z) {
    for (size_t i = 0; i < nr; ++i) {
        Integer rescue = elem[i][col];
        elem[i][col] = u * elem[i][col] + v * elem[i][j];
        elem[i][j] = w * rescue + z * elem[i][j];
        if ((!check_range(elem[i][col]) || !check_range(elem[i][j]))) {
            return false;
        }
    }
    return true;
}

//---------------------------------------------------------------------------

template <typename Integer>
bool Matrix<Integer>::gcd_reduce_column(size_t corner, Matrix<Integer>& Right) {
    assert(corner < nc);
    assert(corner < nr);
    Integer d, u, w, z, v;
    for (size_t j = corner + 1; j < nc; ++j) {
        d = ext_gcd(elem[corner][corner], elem[corner][j], u, v);
        w = -elem[corner][j] / d;
        z = elem[corner][corner] / d;
        // Now we multiply the submatrix formed by columns "corner" and "j"
        // and rows corner,...,nr from the right by the 2x2 matrix
        // | u w |
        // | v z |
        if (!linear_comb_columns(corner, j, u, w, v, z))
            return false;
        if (!Right.linear_comb_columns(corner, j, u, w, v, z))
            return false;
    }
    return true;
}

#ifdef ENFNORMALIZ
template <>
bool Matrix<renf_elem_class>::gcd_reduce_column(size_t corner, Matrix<renf_elem_class>& Right) {
    assert(corner < nc);
    assert(corner < nr);
    renf_elem_class d, u, w, z, v;
    for (size_t j = corner + 1; j < nc; ++j) {
        d = elem[corner][corner], elem[corner];  // ext_gcd(elem[corner][corner],elem[corner][j],u,v);
        u = 1;
        v = 0;
        w = -elem[corner][j] / d;
        z = elem[corner][corner] / d;
        // Now we multiply the submatrix formed by columns "corner" and "j"
        // and rows corner,...,nr from the right by the 2x2 matrix
        // | u w |
        // | v z |
        if (!linear_comb_columns(corner, j, u, w, v, z))
            return false;
        if (!Right.linear_comb_columns(corner, j, u, w, v, z))
            return false;
    }
    return true;
}
#endif

template <>
bool Matrix<nmz_float>::gcd_reduce_column(size_t corner, Matrix<nmz_float>& Right) {
    assert(false);
    return true;
}

template <>
bool Matrix<mpq_class>::gcd_reduce_column(size_t corner, Matrix<mpq_class>& Right) {
    assert(false);
    return true;
}

//---------------------------------------------------------------------------

template <typename Integer>
bool Matrix<Integer>::column_trigonalize(size_t rk, Matrix<Integer>& Right) {
    assert(Right.nr == nc);
    assert(Right.nc == nc);
    vector<long> piv(2, 0);
    for (size_t j = 0; j < rk; ++j) {
        piv = pivot(j);
        assert(piv[0] >= 0);  // protect against wrong rank
        exchange_rows(j, piv[0]);
        exchange_columns(j, piv[1]);
        Right.exchange_columns(j, piv[1]);
        if (!gcd_reduce_column(j, Right))
            return false;
    }
    return true;
}

//---------------------------------------------------------------------------

template <typename Integer>
Integer Matrix<Integer>::compute_vol(bool& success) {
    assert(nr <= nc);

    Integer det = 1;
    for (size_t i = 0; i < nr; ++i) {
        det *= elem[i][i];
        if (!check_range(det)) {
            success = false;
            return 0;
        }
    }

    det = Iabs(det);
    success = true;
    return det;
}

//---------------------------------------------------------------------------

template <typename Integer>
size_t Matrix<Integer>::row_echelon_inner_elem(bool& success) {
    size_t pc = 0;
    long piv = 0, rk = 0;
    success = true;

    if (nr == 0)
        return 0;

    for (rk = 0; rk < (long)nr; rk++) {
        for (; pc < nc; pc++) {
            piv = pivot_in_column(rk, pc);
            if (piv >= 0)
                break;
        }
        if (pc == nc)
            break;
        do {
            exchange_rows(rk, piv);
            if (!reduce_row(rk, pc)) {
                success = false;
                return rk;
            }
            piv = pivot_in_column(rk, pc);
        } while (piv > rk);
    }

    return rk;
}

/*
template <typename Integer>
void Matrix<Integer>::make_first_element_1_in_rows() {
    assert(false);
}

template <>
long Matrix<mpq_class>::pivot_in_column(size_t row, size_t col) {
    size_t i;
    long j = -1;
    mpq_class help = 0;

    for (i = row; i < nr; i++) {
        if (elem[i][col] != 0) {
            j = i;
            break;
        }
    }

    return j;
}
*/
template <>
size_t Matrix<mpq_class>::row_echelon_inner_elem(bool& success) {
    assert(false);
    return 0;
}

/* body
    success = true;

    size_t pc = 0;
    long piv = 0, rk = 0;

    if (nr == 0)
        return 0;

    for (rk = 0; rk < (long)nr; rk++) {
        for (; pc < nc; pc++) {
            piv = pivot_in_column(rk, pc);
            if (piv >= 0)
                break;
        }
        if (pc == nc)
            break;

        exchange_rows(rk, piv);
        reduce_row(rk, pc);
    }

    return rk;
}


template <>
void Matrix<mpq_class>::make_first_element_1_in_rows() {
    for (size_t i = 0; i < nr; ++i) {
        for (size_t j = 0; j < nc; ++j) {
            if (elem[i][j] != 0) {
                mpq_class pivot = elem[i][j];
                v_scalar_division(elem[i], pivot);
                break;
            }
        }
    }
}
*/

template <>
size_t Matrix<mpq_class>::row_echelon() {
    size_t rk;
    bool dummy;
    rk = row_echelon_inner_elem(dummy);
    Shrink_nr_rows(rk);
    return rk;
}

//-----------------------------------------------------------
//
// variants for numberfield
//
//-----------------------------------------------------------

#ifdef ENFNORMALIZ
template <>
long Matrix<renf_elem_class>::pivot_in_column(size_t row, size_t col) {
    size_t i;
    long j = -1;

    for (i = row; i < nr; i++) {
        if (elem[i][col] != 0) {
            j = i;
            break;
        }
    }

    return j;
}

template <>
size_t Matrix<renf_elem_class>::row_echelon_inner_elem(bool& success) {
    success = true;

    size_t pc = 0;
    long piv = 0, rk = 0;

    if (nr == 0)
        return 0;

    for (rk = 0; rk < (long)nr; rk++) {
        for (; pc < nc; pc++) {
            piv = pivot_in_column(rk, pc);
            if (piv >= 0)
                break;
        }
        if (pc == nc)
            break;

        exchange_rows(rk, piv);
        reduce_row(rk, pc);
    }

    return rk;
}

template <>
size_t Matrix<renf_elem_class>::row_echelon() {
    size_t rk;
    bool dummy;
    rk = row_echelon_inner_elem(dummy);
    Shrink_nr_rows(rk);
    return rk;
}
#endif

//---------------------------------------------------------------------------

/*
template<typename Integer>
size_t Matrix<Integer>::row_echelon_inner_bareiss(bool& success, Integer& det){
// no overflow checks since this is supposed to be only used with GMP

    success=true;
    if(nr==0)
        return 0;
    assert(using_GMP<Integer>());

    size_t pc=0;
    long piv=0, rk=0;
    vector<bool> last_time_mult(nr,false),this_time_mult(nr,false);
    Integer last_div=1,this_div=1;
    size_t this_time_exp=0,last_time_exp=0;
    Integer det_factor=1;

    for (rk = 0; rk < (long) nr; rk++){

        for(;pc<nc;pc++){
            piv=pivot_in_column(rk,pc);
            if(piv>=0)
                break;
        }
        if(pc==nc)
            break;

        if(!last_time_mult[piv]){
            for(size_t k=rk;k<nr;++k)
                if(elem[k][pc]!=0 && last_time_mult[k]){
                    piv=k;
                    break;
                }
        }

        exchange_rows (rk,piv);
        v_bool_entry_swap(last_time_mult,rk,piv);

        if(!last_time_mult[rk])
            for(size_t i=0;i<nr;++i)
                last_time_mult[i]=false;

        Integer a=elem[rk][pc];
        this_div=Iabs(a);
        this_time_exp=0;

        for(size_t i=rk+1;i<nr;++i){
            if(elem[i][pc]==0){
                this_time_mult[i]=false;
                continue;
            }

            this_time_exp++;
            this_time_mult[i]=true;
            bool divide=last_time_mult[i] && (last_div!=1);
            if(divide)
                last_time_exp--;
            Integer b=elem[i][pc];
            elem[i][pc]=0;
            if(a==1){
                for(size_t j=pc+1;j<nc;++j){
                    elem[i][j]=elem[i][j]-b*elem[rk][j];
                    if(divide){
                        elem[i][j]/=last_div;
                    }
                }
            }
            else{
                if(a==-1){
                    for(size_t j=pc+1;j<nc;++j){
                        elem[i][j]=-elem[i][j]-b*elem[rk][j];
                        if(divide){
                            elem[i][j]/=last_div;
                        }
                    }
                }
                else{
                    for(size_t j=pc+1;j<nc;++j){
                        elem[i][j]=a*elem[i][j]-b*elem[rk][j];
                       if(divide){
                            elem[i][j]/=last_div;
                        }
                    }
                }
            }
        }

        for(size_t i=0;i<last_time_exp;++i)
            det_factor*=last_div;
        last_time_mult=this_time_mult;
        last_div=this_div;
        last_time_exp=this_time_exp;
    }

    det=0;
    if (nr <= nc && rk == (long) nr) { // must allow nonsquare matrices
        det=1;
        for(size_t i=0;i<nr;++i)
            det*=elem[i][i];
        det=Iabs<Integer>(det/det_factor);
    }

    return rk;
}
*/

//---------------------------------------------------------------------------

template <typename Integer>
size_t Matrix<Integer>::row_echelon_reduce(bool& success) {
    size_t rk = row_echelon_inner_elem(success);
    if (success)
        success = reduce_rows_upwards();
    return rk;
}

//---------------------------------------------------------------------------

template <typename Integer>
Integer Matrix<Integer>::full_rank_index(bool& success) {
    size_t rk = row_echelon_inner_elem(success);
    if (!success)
        return 0;
    Integer index = 1;
    if (success) {
        for (size_t i = 0; i < rk; ++i) {
            index *= elem[i][i];
            if (!check_range(index)) {
                success = false;
                index = 0;
                return index;
            }
        }
    }
    assert(rk == nc);  // must have full rank
    index = Iabs(index);
    return index;
}

#ifdef ENFNORMALIZ
template <>
renf_elem_class Matrix<renf_elem_class>::full_rank_index(bool& success) {
    assert(false);
    return 0;
}
/* body
    size_t rk = row_echelon_inner_elem(success);
    renf_elem_class index = 1;
    if (success) {
        for (size_t i = 0; i < rk; ++i) {
            index *= elem[i][i];
            if (!check_range(index)) {
                success = false;
                index = 0;
                return index;
            }
        }
    }
    assert(rk == nc);  // must have full rank
    index = Iabs(index);
    return index;
}
*/
#endif
//---------------------------------------------------------------------------

template <typename Integer>
Matrix<Integer> Matrix<Integer>::row_column_trigonalize(size_t& rk, bool& success) {
    Matrix<Integer> Right(nc);
    rk = row_echelon_reduce(success);
    if (success)
        success = column_trigonalize(rk, Right);
    return Right;
}

//---------------------------------------------------------------------------

template <typename Integer>
size_t Matrix<Integer>::row_echelon(bool& success, bool do_compute_vol, Integer& det) {
    /*    if(using_GMP<Integer>()){
            return row_echelon_inner_bareiss(success,det);;
        }
        else{ */
    size_t rk = row_echelon_inner_elem(success);
    if (do_compute_vol)
        det = compute_vol(success);
    return rk;
    //    }
}

//---------------------------------------------------------------------------

template <typename Integer>
size_t Matrix<Integer>::row_echelon(bool& success) {
    static Integer dummy;
    return row_echelon(success, false, dummy);
}

//---------------------------------------------------------------------------

template <typename Integer>
size_t Matrix<Integer>::row_echelon(bool& success, Integer& det) {
    return row_echelon(success, true, det);
}

//---------------------------------------------------------------------------

template <typename Integer>
size_t Matrix<Integer>::rank_submatrix(const Matrix<Integer>& mother, const vector<key_t>& key) {
    assert(nc >= mother.nc);
    if (nr < key.size()) {
        elem.resize(key.size(), vector<Integer>(nc, 0));
        nr = key.size();
    }
    size_t save_nr = nr;
    size_t save_nc = nc;
    nr = key.size();
    nc = mother.nc;

    select_submatrix(mother, key);

    bool success;
    size_t rk = row_echelon(success);

    if (!success) {
        Matrix<mpz_class> mpz_this(nr, nc);
        mpz_submatrix(mpz_this, mother, key);
        rk = mpz_this.row_echelon(success);
    }

    nr = save_nr;
    nc = save_nc;
    return rk;
}

template <>
size_t Matrix<mpq_class>::rank_submatrix(const Matrix<mpq_class>& mother, const vector<key_t>& key) {
    assert(false);
    return 0;
}
/* body
    assert(nc >= mother.nc);
    if (nr < key.size()) {
        elem.resize(key.size(), vector<mpq_class>(nc, 0));
        nr = key.size();
    }
    size_t save_nr = nr;
    size_t save_nc = nc;
    nr = key.size();
    nc = mother.nc;

    select_submatrix(mother, key);

    bool success;
    size_t rk = row_echelon(success);

    nr = save_nr;
    nc = save_nc;
    return rk;
}
*/

/*
void flint_mat_select(fmpz_mat_t fmat, const Matrix<mpz_class>& nmz_mat,const vector<key_t>& key ){

    for(size_t i=0;i<key.size();++i)
        for(size_t j=0;j<nmz_mat.nr_of_columns();++j)
            fmpz_set_mpz(fmpz_mat_entry(fmat, (slong) i, (slong) j),nmz_mat[key[i]][j].get_mpz_t());
}

void flint_mat(fmpz_mat_t fmat, const Matrix<mpz_class>& nmz_mat){

    for(size_t i=0;i<nmz_mat.nr_of_rows();++i)
        for(size_t j=0;j<nmz_mat.nr_of_columns();++j)
            fmpz_set_mpz(fmpz_mat_entry(fmat, (slong) i, (slong)j),nmz_mat[i][j].get_mpz_t());
}


void nmz_mat(Matrix<mpz_class>& nmz_mat, const fmpz_mat_t fmat){

    size_t r=fmpz_mat_nrows(fmat);
    size_t c=fmpz_mat_ncols(fmat);
    nmz_mat.resize(r,c);
    mpz_t t;
    mpz_init(t);
    for(size_t i=0;i<r;++i)
        for(size_t j=0;j<c;++j){
            fmpz_get_mpz(t,fmpz_mat_entry(fmat, (slong) i, (slong)j));
            nmz_mat[i][j]=mpz_class(t);
        }
    mpz_clear(t);
}

*/
/*
 * fmpz_get_mpz(t,f)
 * fmpz_set_mpz(f,t)
 */
//---------------------------------------------------------------------------

/*
template<>
size_t Matrix<mpz_class>::rank_submatrix(const Matrix<mpz_class>& mother, const vector<key_t>& key){

    assert(nc>=mother.nc);
    if(nr<key.size()){
        elem.resize(key.size(),vector<mpz_class>(nc,0));
        nr=key.size();
    }
    size_t save_nr=nr;
    size_t save_nc=nc;
    nr=key.size();
    nc=mother.nc;

    fmpz_mat_t fmat;
    fmpz_mat_init(fmat, (slong) nr, (slong) nc);
    flint_mat_select(fmat,mother,key);
    // flint_mat_select(fmat,*this);
    size_t rk= (size_t) fmpz_mat_rank(fmat);
    fmpz_mat_clear(fmat);


    nr=save_nr;
    nc=save_nc;
    return rk;
} */

//---------------------------------------------------------------------------

template <typename Integer>
size_t Matrix<Integer>::rank_submatrix(const vector<key_t>& key) const {
    Matrix<Integer> work(key.size(), nc);
    return work.rank_submatrix(*this, key);
}

//---------------------------------------------------------------------------

template <typename Integer>
size_t Matrix<Integer>::rank() const {
    vector<key_t> key(nr);
    for (size_t i = 0; i < nr; ++i)
        key[i] = i;
    return rank_submatrix(key);
}

//---------------------------------------------------------------------------

template <typename Integer>
Integer Matrix<Integer>::vol_submatrix(const Matrix<Integer>& mother, const vector<key_t>& key) {
    assert(nc >= mother.nc);
    if (nr < key.size()) {
        elem.resize(key.size(), vector<Integer>(nc, 0));
        nr = key.size();
    }
    size_t save_nr = nr;
    size_t save_nc = nc;
    nr = key.size();
    nc = mother.nc;

    select_submatrix(mother, key);

    bool success;
    Integer det;
    row_echelon(success, det);

    if (!success) {
        Matrix<mpz_class> mpz_this(nr, nc);
        mpz_submatrix(mpz_this, mother, key);
        mpz_class mpz_det;
        mpz_this.row_echelon(success, mpz_det);
        convert(det, mpz_det);
    }

    nr = save_nr;
    nc = save_nc;
    return det;
}

template <>
mpq_class Matrix<mpq_class>::vol_submatrix(const Matrix<mpq_class>& mother, const vector<key_t>& key) {
    assert(false);
    return {};
}
/* body
    assert(nc >= mother.nc);
    if (nr < key.size()) {
        elem.resize(key.size(), vector<mpq_class>(nc, 0));
        nr = key.size();
    }
    size_t save_nr = nr;
    size_t save_nc = nc;
    nr = key.size();
    nc = mother.nc;

    select_submatrix(mother, key);

    bool success;
    mpq_class det;
    row_echelon(success, det);

    nr = save_nr;
    nc = save_nc;
    return det;
}
*/

#ifdef ENFNORMALIZ
template <>
renf_elem_class Matrix<renf_elem_class>::vol_submatrix(const Matrix<renf_elem_class>& mother, const vector<key_t>& key) {
    assert(nc >= mother.nc);
    if (nr < key.size()) {
        elem.resize(key.size(), vector<renf_elem_class>(nc, 0));
        nr = key.size();
    }
    size_t save_nr = nr;
    size_t save_nc = nc;
    nr = key.size();
    nc = mother.nc;

    select_submatrix(mother, key);

    bool success;
    renf_elem_class det;
    row_echelon(success, det);

    nr = save_nr;
    nc = save_nc;
    return det;
}
#endif

//---------------------------------------------------------------------------

template <typename Integer>
Integer Matrix<Integer>::vol_submatrix(const vector<key_t>& key) const {
    Matrix<Integer> work(key.size(), nc);
    return work.vol_submatrix(*this, key);
}

//---------------------------------------------------------------------------

template <typename Integer>
Integer Matrix<Integer>::vol() const {
    vector<key_t> key(nr);
    for (size_t i = 0; i < nr; ++i)
        key[i] = i;
    return vol_submatrix(key);
}

//---------------------------------------------------------------------------

template <typename Integer>
vector<key_t> Matrix<Integer>::max_rank_submatrix_lex_inner(bool& success, vector<key_t> perm) const {
    success = true;
    size_t max_rank = min(nr, nc);
    Matrix<Integer> Test(max_rank, nc);
    Test.nr = 0;
    vector<key_t> col;
    col.reserve(max_rank);
    vector<key_t> key;
    key.reserve(max_rank);
    size_t rk = 0;

    bool perm_set = false;
    if (perm.size() > 0)
        perm_set = true;

    vector<vector<bool> > col_done(max_rank, vector<bool>(nc, false));

    vector<Integer> Test_vec(nc);

    for (size_t i = 0; i < nr; ++i) {
        if (perm_set)
            Test_vec = elem[perm[i]];
        else
            Test_vec = elem[i];
        for (size_t k = 0; k < rk; ++k) {
            if (Test_vec[col[k]] == 0)
                continue;
            Integer a = Test[k][col[k]];
            Integer b = Test_vec[col[k]];
            for (size_t j = 0; j < nc; ++j)
                if (!col_done[k][j]) {
                    Test_vec[j] = a * Test_vec[j] - b * Test[k][j];
                    if (!check_range(Test_vec[j])) {
                        success = false;
                        return key;
                    }
                }
        }

        size_t j = 0;
        for (; j < nc; ++j)
            if (Test_vec[j] != 0)
                break;
        if (j == nc)  // Test_vec=0
            continue;

        col.push_back(j);
        if (perm_set)
            key.push_back(perm[i]);
        else
            key.push_back(i);

        if (rk > 0) {
            col_done[rk] = col_done[rk - 1];
            col_done[rk][col[rk - 1]] = true;
        }

        Test.nr++;
        rk++;
        v_make_prime(Test_vec);
        Test[rk - 1] = Test_vec;

        if (rk == max_rank)
            break;
    }
    return key;
}

//---------------------------------------------------------------------------
// perm allows a reordering of the matrix
// vectors are inserted into the test according to the order given by perm
template <typename Integer>
vector<key_t> Matrix<Integer>::max_rank_submatrix_lex(vector<key_t> perm) const {
    bool success;
    vector<key_t> key = max_rank_submatrix_lex_inner(success);
    if (!success) {
        Matrix<mpz_class> mpz_this(nr, nc);
        mat_to_mpz(*this, mpz_this);
        key = mpz_this.max_rank_submatrix_lex_inner(success);
    }
    return key;
}

//---------------------------------------------------------------------------

template <typename Integer>
bool Matrix<Integer>::solve_destructive_inner(bool ZZinvertible, Integer& denom) {
    assert(nc >= nr);
    bool success = true;  // to make gcc happy

    size_t rk;

    if (ZZinvertible) {
        rk = row_echelon_inner_elem(success);
        if (!success)
            return false;
        assert(rk == nr);
        denom = compute_vol(success);
    }
    else {
        rk = row_echelon(success, denom);
        if (!success)
            return false;
    }

    if (denom == 0) {
        if (using_GMP<Integer>() || using_renf<Integer>()) {
            errorOutput() << "Cannot solve system (denom=0)!" << endl;
            throw ArithmeticException();
        }
        else
            return false;
    }

    if (!using_renf<Integer>()) {
        for (int i = nr - 1; i >= 0; --i) {
            for (size_t j = nr; j < nc; ++j) {
                elem[i][j] *= denom;
                if (!check_range(elem[i][j]))
                    return false;
            }
            for (int k = i + 1; k < (int)nr; ++k) {
                for (size_t j = nr; j < nc; ++j) {
                    elem[i][j] -= elem[i][k] * elem[k][j];
                    if (!check_range(elem[i][j]))
                        return false;
                }
            }
            for (size_t j = nr; j < nc; ++j)
                elem[i][j] /= elem[i][i];
        }
    }
    else {  // we can divide in this case, somewhat faster

        // make pivot elemnst 1 and multiply RHS by denom as in the case with
        // integer types for uniform behavior
        Integer fact, help;
        for (int i = nr - 1; i >= 0; --i) {
            fact = 1 / elem[i][i];
            Integer fact_times_denom = fact * denom;
            for (size_t j = i; j < nr; ++j)
                if (elem[i][j] != 0)
                    elem[i][j] *= fact;
            for (size_t j = nr; j < nc; ++j)
                if (elem[i][j] != 0)
                    elem[i][j] *= fact_times_denom;
        }
        for (int i = nr - 1; i >= 0; --i) {
            for (int k = i - 1; k >= 0; --k) {
                if (elem[k][i] != 0) {
                    fact = elem[k][i];
                    for (size_t j = i; j < nc; ++j) {
                        if (elem[i][j] != 0) {
                            help = elem[i][j];
                            help *= fact;
                            elem[k][j] -= help;
                        }
                    }
                }
            }
        }
    }

    return true;
}

//---------------------------------------------------------------------------

template <typename Integer>
void Matrix<Integer>::customize_solution(size_t dim, Integer& denom, size_t red_col, size_t sign_col, bool make_sol_prime) {
    assert(!(make_sol_prime && (sign_col > 0 || red_col > 0)));

    for (size_t j = 0; j < red_col; ++j) {  // reduce first red_col columns of solution mod denom
        for (size_t k = 0; k < dim; ++k) {
            elem[k][dim + j] %= denom;
            if (elem[k][dim + j] < 0)
                elem[k][dim + j] += Iabs(denom);
        }
    }

    for (size_t j = 0; j < sign_col; ++j)  // replace entries in the next sign_col columns by their signs
        for (size_t k = 0; k < dim; ++k) {
            if (elem[k][dim + red_col + j] > 0) {
                elem[k][dim + red_col + j] = 1;
                continue;
            }
            if (elem[k][dim + red_col + j] < 0) {
                elem[k][dim + red_col + j] = -1;
                continue;
            }
        }

    if (make_sol_prime)  // make columns of solution coprime if wanted
        make_cols_prime(dim, nc - 1);
}

#ifdef ENFNORMALIZ
template <>
void Matrix<renf_elem_class>::customize_solution(
    size_t dim, renf_elem_class& denom, size_t red_col, size_t sign_col, bool make_sol_prime) {
    return;
}
#endif

template <>
void Matrix<mpq_class>::customize_solution(size_t dim, mpq_class& denom, size_t red_col, size_t sign_col, bool make_sol_prime) {
    return;
}

//---------------------------------------------------------------------------

template <>
void Matrix<nmz_float>::customize_solution(size_t dim, nmz_float& denom, size_t red_col, size_t sign_col, bool make_sol_prime) {
    assert(false);
}

//---------------------------------------------------------------------------

template <typename Integer>
void Matrix<Integer>::solve_system_submatrix_outer(const Matrix<Integer>& mother,
                                                   const vector<key_t>& key,
                                                   const vector<vector<Integer>*>& RS,
                                                   Integer& denom,
                                                   bool ZZ_invertible,
                                                   bool transpose,
                                                   size_t red_col,
                                                   size_t sign_col,
                                                   bool compute_denom,
                                                   bool make_sol_prime) {
    size_t dim = mother.nc;
    assert(key.size() == dim);
    assert(nr == dim);
    assert(dim + RS.size() <= nc);
    size_t save_nc = nc;
    nc = dim + RS.size();

    if (transpose)
        select_submatrix_trans(mother, key);
    else
        select_submatrix(mother, key);

    for (size_t i = 0; i < dim; ++i)
        for (size_t k = 0; k < RS.size(); ++k)
            elem[i][k + dim] = (*RS[k])[i];

    if (solve_destructive_inner(ZZ_invertible, denom)) {
        customize_solution(dim, denom, red_col, sign_col, make_sol_prime);
    }
    else {
#pragma omp atomic
        GMP_mat++;

        Matrix<mpz_class> mpz_this(nr, nc);
        mpz_class mpz_denom;
        if (transpose)
            mpz_submatrix_trans(mpz_this, mother, key);
        else
            mpz_submatrix(mpz_this, mother, key);

        for (size_t i = 0; i < dim; ++i)
            for (size_t k = 0; k < RS.size(); ++k)
                convert(mpz_this[i][k + dim], (*RS[k])[i]);
        mpz_this.solve_destructive_inner(ZZ_invertible, mpz_denom);
        mpz_this.customize_solution(dim, mpz_denom, red_col, sign_col, make_sol_prime);

        for (size_t i = 0; i < dim; ++i)  // replace left side by 0, except diagonal if ZZ_invetible
            for (size_t j = 0; j < dim; ++j) {
                if (i != j || !ZZ_invertible)
                    mpz_this[i][j] = 0;
            }

        mat_to_Int(mpz_this, *this);
        if (compute_denom)
            convert(denom, mpz_denom);
    }
    nc = save_nc;
}

template <>
void Matrix<mpq_class>::solve_system_submatrix_outer(const Matrix<mpq_class>& mother,
                                                     const vector<key_t>& key,
                                                     const vector<vector<mpq_class>*>& RS,
                                                     mpq_class& denom,
                                                     bool ZZ_invertible,
                                                     bool transpose,
                                                     size_t red_col,
                                                     size_t sign_col,
                                                     bool compute_denom,
                                                     bool make_sol_prime) {
    assert(false);
}

#ifdef ENFNORMALIZ
template <>
void Matrix<renf_elem_class>::solve_system_submatrix_outer(const Matrix<renf_elem_class>& mother,
                                                           const vector<key_t>& key,
                                                           const vector<vector<renf_elem_class>*>& RS,
                                                           renf_elem_class& denom,
                                                           bool ZZ_invertible,
                                                           bool transpose,
                                                           size_t red_col,
                                                           size_t sign_col,
                                                           bool compute_denom,
                                                           bool make_sol_prime) {
    size_t dim = mother.nc;
    assert(key.size() == dim);
    assert(nr == dim);
    assert(dim + RS.size() <= nc);
    size_t save_nc = nc;
    nc = dim + RS.size();

    if (transpose)
        select_submatrix_trans(mother, key);
    else
        select_submatrix(mother, key);

    for (size_t i = 0; i < dim; ++i)
        for (size_t k = 0; k < RS.size(); ++k)
            elem[i][k + dim] = (*RS[k])[i];

    if (solve_destructive_inner(ZZ_invertible, denom)) {
        customize_solution(dim, denom, red_col, sign_col, make_sol_prime);
    }
    nc = save_nc;
}
#endif

//---------------------------------------------------------------------------

template <typename Integer>
void Matrix<Integer>::solve_system_submatrix(const Matrix<Integer>& mother,
                                             const vector<key_t>& key,
                                             const vector<vector<Integer>*>& RS,
                                             vector<Integer>& diagonal,
                                             Integer& denom,
                                             size_t red_col,
                                             size_t sign_col) {
    solve_system_submatrix_outer(mother, key, RS, denom, true, false, red_col, sign_col);
    assert(diagonal.size() == nr);
    for (size_t i = 0; i < nr; ++i)
        diagonal[i] = elem[i][i];
}

//---------------------------------------------------------------------------
// the same without diagonal
template <typename Integer>
void Matrix<Integer>::solve_system_submatrix(const Matrix<Integer>& mother,
                                             const vector<key_t>& key,
                                             const vector<vector<Integer>*>& RS,
                                             Integer& denom,
                                             size_t red_col,
                                             size_t sign_col,
                                             bool compute_denom,
                                             bool make_sol_prime) {
    solve_system_submatrix_outer(mother, key, RS, denom, false, false, red_col, sign_col, compute_denom, make_sol_prime);
}

//---------------------------------------------------------------------------

template <typename Integer>
void Matrix<Integer>::solve_system_submatrix_trans(const Matrix<Integer>& mother,
                                                   const vector<key_t>& key,
                                                   const vector<vector<Integer>*>& RS,
                                                   Integer& denom,
                                                   size_t red_col,
                                                   size_t sign_col) {
    solve_system_submatrix_outer(mother, key, RS, denom, false, true, red_col, sign_col);
}

//---------------------------------------------------------------------------

template <typename Integer>
Matrix<Integer> Matrix<Integer>::extract_solution() const {
    assert(nc >= nr);
    Matrix<Integer> Solution(nr, nc - nr);
    for (size_t i = 0; i < nr; ++i) {
        for (size_t j = 0; j < Solution.nc; ++j)
            Solution[i][j] = elem[i][j + nr];
    }
    return Solution;
}

//---------------------------------------------------------------------------

template <typename Integer>
vector<vector<Integer>*> Matrix<Integer>::row_pointers() {
    vector<vector<Integer>*> pointers(nr);
    for (size_t i = 0; i < nr; ++i)
        pointers[i] = &(elem[i]);
    return pointers;
}

//---------------------------------------------------------------------------

template <typename Integer>
vector<vector<Integer>*> Matrix<Integer>::submatrix_pointers(const vector<key_t>& key) {
    vector<vector<Integer>*> pointers(key.size());
    for (size_t i = 0; i < key.size(); ++i)
        pointers[i] = &(elem[key[i]]);
    return pointers;
}
//---------------------------------------------------------------------------

/* not used at present
template <typename Integer>
Matrix<Integer> Matrix<Integer>::solve(const Matrix<Integer>& Right_side, vector<Integer>& diagonal, Integer& denom) const {
    Matrix<Integer> M(nr, nc + Right_side.nc);
    vector<key_t> key = identity_key(nr);
    Matrix<Integer> RS_trans = Right_side.transpose();
    vector<vector<Integer>*> RS = RS_trans.row_pointers();
    M.solve_system_submatrix(*this, key, RS, diagonal, denom, 0, 0);
    return M.extract_solution();
}
*/

//---------------------------------------------------------------------------

template <typename Integer>
Matrix<Integer> Matrix<Integer>::solve(const Matrix<Integer>& Right_side, Integer& denom) const {
    Matrix<Integer> M(nr, nc + Right_side.nc);
    vector<key_t> key = identity_key(nr);
    Matrix<Integer> RS_trans = Right_side.transpose();
    vector<vector<Integer>*> RS = RS_trans.row_pointers();
    M.solve_system_submatrix(*this, key, RS, denom, 0, 0);
    return M.extract_solution();
}

//---------------------------------------------------------------------------

template <typename Integer>
Matrix<Integer> Matrix<Integer>::invert(Integer& denom) const {
    assert(nr == nc);
    Matrix<Integer> Right_side(nr);

    return solve(Right_side, denom);
}

//---------------------------------------------------------------------------

template <typename Integer>
Matrix<Integer> Matrix<Integer>::bundle_matrices(const Matrix<Integer>& Right_side) const {
    assert(nr == nc);
    assert(nc == Right_side.nr);
    Matrix<Integer> M(nr, nc + Right_side.nc);
    for (size_t i = 0; i < nr; ++i) {
        for (size_t j = 0; j < nc; ++j)
            M[i][j] = elem[i][j];
        for (size_t j = nc; j < M.nc; ++j)
            M[i][j] = Right_side[i][j - nc];
    }
    return M;
}
//---------------------------------------------------------------------------

template <typename Integer>
Matrix<Integer> Matrix<Integer>::invert_unprotected(Integer& denom, bool& success) const {
    assert(nr == nc);
    Matrix<Integer> Right_side(nr);
    Matrix<Integer> M = bundle_matrices(Right_side);
    success = M.solve_destructive_inner(false, denom);
    return M.extract_solution();
    ;
}

//---------------------------------------------------------------------------

template <typename Integer>
void Matrix<Integer>::invert_submatrix(
    const vector<key_t>& key, Integer& denom, Matrix<Integer>& Inv, bool compute_denom, bool make_sol_prime) const {
    assert(key.size() == nc);
    Matrix<Integer> unit_mat(key.size());
    Matrix<Integer> M(key.size(), 2 * key.size());
    vector<vector<Integer>*> RS_pointers = unit_mat.row_pointers();
    M.solve_system_submatrix(*this, key, RS_pointers, denom, 0, 0, compute_denom, make_sol_prime);
    Inv = M.extract_solution();
}

template <typename Integer>
void Matrix<Integer>::invert_submatrix(const vector<key_t>& key,
                                       Integer& denom,
                                       Matrix<Integer>& Inv,
                                       Matrix<Integer>& Work,
                                       Matrix<Integer>& UnitMat,
                                       bool compute_denom,
                                       bool make_sol_prime) const {
    assert(key.size() == nc);
    // cout << "WWWWWWWWWWW " << key.size() << " -- " << Work.nr << " " << Work.nc << endl;
    assert(Work.nr == key.size());
    assert(Work.nc == 2 * key.size());
    assert(UnitMat.nc == key.size());

    vector<vector<Integer>*> RS_pointers = UnitMat.row_pointers();
    Work.solve_system_submatrix(*this, key, RS_pointers, denom, 0, 0, compute_denom, make_sol_prime);
    Inv = Work.extract_solution();
}

//---------------------------------------------------------------------------

template <typename Integer>
void Matrix<Integer>::simplex_data(const vector<key_t>& key, Matrix<Integer>& Supp, Integer& vol, bool compute_vol) const {
    assert(key.size() == nc);
    invert_submatrix(key, vol, Supp, compute_vol, true);
    // Supp=Supp.transpose();
    Supp.transpose_in_place();
    // Supp.make_prime(); now done internally
}

template <typename Integer>
void Matrix<Integer>::simplex_data(const vector<key_t>& key,
                                   Matrix<Integer>& Supp,
                                   Integer& vol,
                                   Matrix<Integer>& Work,
                                   Matrix<Integer>& UnitMat,
                                   bool compute_vol) const {
    assert(key.size() == nc);
    // cout << "WWWWWWWWWWW " << key.size() << " -- " << Work.nr << " " << Work.nc << endl;
    invert_submatrix(key, vol, Supp, Work, UnitMat, compute_vol, true);
    // Supp=Supp.transpose();
    Supp.transpose_in_place();
    // Supp.make_prime(); now done internally
}
//---------------------------------------------------------------------------

template <typename Integer>
vector<Integer> Matrix<Integer>::solve_rectangular(const vector<Integer>& v, Integer& denom) const {
    if (nc == 0 || nr == 0) {  // return zero-vector as solution
        return vector<Integer>(nc, 0);
    }
    size_t i;
    vector<key_t> rows = max_rank_submatrix_lex();
    Matrix<Integer> Left_Side = submatrix(rows);
    assert(nc == Left_Side.nr);  // otherwise input hadn't full rank //TODO
    Matrix<Integer> Right_Side(v.size(), 1);
    Right_Side.write_column(0, v);
    Right_Side = Right_Side.submatrix(rows);
    Matrix<Integer> Solution = Left_Side.solve(Right_Side, denom);
    vector<Integer> Linear_Form(nc);
    for (i = 0; i < nc; i++) {
        Linear_Form[i] = Solution[i][0];  // the solution vector is called Linear_Form
    }
    vector<Integer> test = MxV(Linear_Form);  // we have solved the system by taking a square submatrix
                                              // now we must test whether the solution satisfies the full system
    for (i = 0; i < nr; i++) {
        if (test[i] != denom * v[i]) {
            return vector<Integer>();
        }
    }
    Integer total_gcd = libnormaliz::gcd(denom, v_gcd(Linear_Form));  // extract the gcd of denom and solution
    denom /= total_gcd;
    v_scalar_division(Linear_Form, total_gcd);
    return Linear_Form;
}

template <>
vector<mpq_class> Matrix<mpq_class>::solve_rectangular(const vector<mpq_class>& v, mpq_class& denom) const {
    assert(false);
    return {};
}

/* body
    if (nc == 0 || nr == 0) {  // return zero-vector as solution
        return vector<mpq_class>(nc, 0);
    }
    size_t i;
    vector<key_t> rows = max_rank_submatrix_lex();
    Matrix<mpq_class> Left_Side = submatrix(rows);
    assert(nc == Left_Side.nr);  // otherwise input hadn't full rank //TODO
    Matrix<mpq_class> Right_Side(v.size(), 1);
    Right_Side.write_column(0, v);
    Right_Side = Right_Side.submatrix(rows);
    Matrix<mpq_class> Solution = Left_Side.solve(Right_Side, denom);
    vector<mpq_class> Linear_Form(nc);
    for (i = 0; i < nc; i++) {
        Linear_Form[i] = Solution[i][0];  // the solution vector is called Linear_Form
    }
    vector<mpq_class> test = MxV(Linear_Form);  // we have solved the system by taking a square submatrix
                                                // now we must test whether the solution satisfies the full system
    for (i = 0; i < nr; i++) {
        if (test[i] != denom * v[i]) {
            return vector<mpq_class>();
        }
    }
    mpq_class total_gcd = 1;  // libnormaliz::gcd(denom,v_gcd(Linear_Form)); // extract the gcd of denom and solution
    denom /= total_gcd;
    v_scalar_division(Linear_Form, total_gcd);
    return Linear_Form;
}
*/

#ifdef ENFNORMALIZ
template <>
vector<renf_elem_class> Matrix<renf_elem_class>::solve_rectangular(const vector<renf_elem_class>& v,
                                                                   renf_elem_class& denom) const {
    if (nc == 0 || nr == 0) {  // return zero-vector as solution
        return vector<renf_elem_class>(nc, 0);
    }
    size_t i;
    vector<key_t> rows = max_rank_submatrix_lex();
    Matrix<renf_elem_class> Left_Side = submatrix(rows);
    assert(nc == Left_Side.nr);  // otherwise input hadn't full rank //TODO
    Matrix<renf_elem_class> Right_Side(v.size(), 1);
    Right_Side.write_column(0, v);
    Right_Side = Right_Side.submatrix(rows);
    Matrix<renf_elem_class> Solution = Left_Side.solve(Right_Side, denom);
    vector<renf_elem_class> Linear_Form(nc);
    for (i = 0; i < nc; i++) {
        Linear_Form[i] = Solution[i][0];  // the solution vector is called Linear_Form
    }
    vector<renf_elem_class> test = MxV(Linear_Form);  // we have solved the system by taking a square submatrix
                                                      // now we must test whether the solution satisfies the full system
    for (i = 0; i < nr; i++) {
        if (test[i] != denom * v[i]) {
            return vector<renf_elem_class>();
        }
    }
    renf_elem_class total_gcd = 1;  // libnormaliz::gcd(denom,v_gcd(Linear_Form)); // extract the gcd of denom and solution
    denom /= total_gcd;
    v_scalar_division(Linear_Form, total_gcd);
    return Linear_Form;
}
#endif

//---------------------------------------------------------------------------

template <typename Integer>
vector<Integer> Matrix<Integer>::solve_ZZ(const vector<Integer>& v) const {
    Integer denom;
    vector<Integer> result = solve_rectangular(v, denom);
    if (denom != 1)
        result.clear();
    return result;
}
//---------------------------------------------------------------------------

template <typename Integer>
vector<Integer> Matrix<Integer>::find_linear_form() const {
    Integer denom;
    vector<Integer> result = solve_rectangular(vector<Integer>(nr, 1), denom);
    v_make_prime(result);
    return result;
}

//---------------------------------------------------------------------------

/*
template <typename Integer>
vector<Integer> Matrix<Integer>::find_linear_form_low_dim() const {
    size_t rank = (*this).rank();
    if (rank == 0) {  // return zero-vector as linear form
        return vector<Integer>(nc, 0);
    }
    if (rank == nc) {  // basis change not necessary
        return (*this).find_linear_form();
    }

    Sublattice_Representation<Integer> Basis_Change(*this, true);
    vector<Integer> Linear_Form = Basis_Change.to_sublattice(*this).find_linear_form();
    if (Linear_Form.size() != 0)
        Linear_Form = Basis_Change.from_sublattice_dual(Linear_Form);

    return Linear_Form;
}

template <>
vector<mpq_class> Matrix<mpq_class>::find_linear_form_low_dim() const {
    assert(false);
    return vector<mpq_class>(0);
}

#ifdef ENFNORMALIZ
template <>
vector<renf_elem_class> Matrix<renf_elem_class>::find_linear_form_low_dim() const {
    assert(false);
    return vector<renf_elem_class>(0);
}
#endif

*/

//---------------------------------------------------------------------------

template <typename Integer>
size_t Matrix<Integer>::row_echelon_reduce() {
    size_t rk;
    Matrix<Integer> Copy(*this);
    bool success;
    rk = row_echelon_reduce(success);
    if (success) {
        Shrink_nr_rows(rk);
        return rk;
    }
    Matrix<mpz_class> mpz_Copy(nr, nc);
    mat_to_mpz(Copy, mpz_Copy);
    rk = mpz_Copy.row_echelon_reduce(success);
    mat_to_Int(mpz_Copy, *this);
    Shrink_nr_rows(rk);
    return rk;
}

template <>
size_t Matrix<mpq_class>::row_echelon_reduce() {
    assert(false);
    return 0;
}
/* body
    size_t rk;
    Matrix<mpq_class> Copy(*this);
    bool success;
    rk = row_echelon_reduce(success);
    if (success) {
        Shrink_nr_rows(rk);
        return rk;
    }
    return rk;
}
*/
//---------------------------------------------------------------------------

template <typename Integer>
Integer Matrix<Integer>::full_rank_index() const {
    Matrix<Integer> Copy(*this);
    Integer index;
    bool success;
    index = Copy.full_rank_index(success);
    if (success)
        return index;
    Matrix<mpz_class> mpz_Copy(nr, nc);
    mat_to_mpz(*this, mpz_Copy);
    mpz_class mpz_index = mpz_Copy.full_rank_index(success);
    convert(index, mpz_index);
    return index;
}

template <>
mpq_class Matrix<mpq_class>::full_rank_index() const {
    assert(false);
    return {};
}
/* body
    Matrix<mpq_class> Copy(*this);
    mpq_class index;
    bool success;
    index = Copy.full_rank_index(success);

    return index;
}
*/

#ifdef ENFNORMALIZ
template <>
renf_elem_class Matrix<renf_elem_class>::full_rank_index() const {
    assert(false);
    return {};
}
/* body
    Matrix<renf_elem_class> Copy(*this);
    renf_elem_class index;
    bool success;
    index = Copy.full_rank_index(success);

    return index;
}
*/
#endif

//---------------------------------------------------------------------------

template <typename Integer>
size_t Matrix<Integer>::row_echelon() {
    Matrix<Integer> Copy(*this);
    bool success;
    size_t rk;
    rk = row_echelon(success);
    if (success) {
        Shrink_nr_rows(rk);
        return rk;
    }
    Matrix<mpz_class> mpz_Copy(nr, nc);
    mat_to_mpz(Copy, mpz_Copy);
    rk = mpz_Copy.row_echelon_reduce(success);  // reduce to make entries small
    mat_to_Int(mpz_Copy, *this);
    Shrink_nr_rows(rk);
    return rk;
}

//-----------------------------------------------------------
//
// variants for floating point
//
//-----------------------------------------------------------

template <>
long Matrix<nmz_float>::pivot_in_column(size_t row, size_t col) {
    size_t i;
    long j = -1;
    nmz_float help = 0;

    for (i = row; i < nr; i++) {
        if (Iabs(elem[i][col]) > nmz_epsilon) {
            if ((help == 0) || (Iabs(elem[i][col]) > help)) {
                help = Iabs(elem[i][col]);
                j = i;
            }
        }
    }

    return j;
}

template <>
size_t Matrix<nmz_float>::row_echelon_inner_elem(bool& success) {
    success = true;

    size_t pc = 0;
    long piv = 0, rk = 0;

    if (nr == 0)
        return 0;

    for (rk = 0; rk < (long)nr; rk++) {
        for (; pc < nc; pc++) {
            piv = pivot_in_column(rk, pc);
            if (piv >= 0)
                break;
        }
        if (pc == nc)
            break;

        exchange_rows(rk, piv);
        reduce_row(rk, pc);
    }

    return rk;
}

template <>
size_t Matrix<nmz_float>::row_echelon() {
    assert(false);
    return 0;
}
/* body
    size_t rk;
    bool dummy;
    rk = row_echelon_inner_elem(dummy);
    Shrink_nr_rows(rk);
    return rk;
}
*/
//---------------------------------------------------------------------------

template <typename Integer>
Matrix<Integer> Matrix<Integer>::kernel(bool use_LLL) const {
    // computes a ZZ-basis of the solutions of (*this)x=0
    // the basis is formed by the rOWS of the returned matrix

    size_t dim = nc;
    if (nr == 0)
        return (Matrix<Integer>(dim));

    Matrix<Integer> Copy(*this);
    size_t rank;
    bool success;
    Matrix<Integer> Transf = Copy.row_column_trigonalize(rank, success);
    if (!success) {
        Matrix<mpz_class> mpz_Copy(nr, nc);
        mat_to_mpz(*this, mpz_Copy);
        Matrix<mpz_class> mpz_Transf = mpz_Copy.row_column_trigonalize(rank, success);
        mat_to_Int(mpz_Transf, Transf);
    }

    Matrix<Integer> ker_basis(dim - rank, dim);
    Matrix<Integer> Help = Transf.transpose();
    for (size_t i = rank; i < dim; i++)
        ker_basis[i - rank] = Help[i];

    if (use_LLL)
        return ker_basis.LLL();
    else {
        ker_basis.standardize_basis();
        return (ker_basis);
    }
}

template <>
Matrix<mpq_class> Matrix<mpq_class>::kernel(bool use_LLL) const {
    assert(false);
    return {};
}
/* body
    // computes a ZZ-basis of the solutions of (*this)x=0
    // the basis is formed by the rOWS of the returned matrix

    size_t dim = nc;
    if (nr == 0)
        return (Matrix<mpq_class>(dim));

    Matrix<mpq_class> Copy(*this);
    size_t rank;
    bool success;
    Matrix<mpq_class> Transf = Copy.row_column_trigonalize(rank, success);

    Matrix<mpq_class> ker_basis(dim - rank, dim);
    Matrix<mpq_class> Help = Transf.transpose();
    for (size_t i = rank; i < dim; i++)
        ker_basis[i - rank] = Help[i];

    ker_basis.standardize_basis();
    return (ker_basis);
}*/

//---------------------------------------------------------------------------
// Converts "this" into (column almost) Hermite normal form, returns column transformation matrix
template <typename Integer>
Matrix<Integer> Matrix<Integer>::AlmostHermite(size_t& rk) {
    Matrix<Integer> Copy = *this;
    Matrix<Integer> Transf;
    bool success;
    Transf = row_column_trigonalize(rk, success);
    if (success)
        return Transf;

    Matrix<mpz_class> mpz_this(nr, nc);
    mat_to_mpz(Copy, mpz_this);
    Matrix<mpz_class> mpz_Transf = mpz_this.row_column_trigonalize(rk, success);
    mat_to_Int(mpz_this, *this);
    mat_to_Int(mpz_Transf, Transf);
    return Transf;
}

template <>
Matrix<mpq_class> Matrix<mpq_class>::AlmostHermite(size_t& rk) {
    assert(false);
    return Matrix<mpq_class>(0, 0);
}

#ifdef ENFNORMALIZ
template <>
Matrix<renf_elem_class> Matrix<renf_elem_class>::AlmostHermite(size_t& rk) {
    assert(false);
    return Matrix<renf_elem_class>(0, 0);
}
#endif

//---------------------------------------------------------------------------

template <typename Integer>
bool Matrix<Integer>::SmithNormalForm_inner(size_t& rk, Matrix<Integer>& Right) {
    bool success = true;

    // first we diagonalize

    while (true) {
        rk = row_echelon_reduce(success);
        if (!success)
            return false;
        if (rk == 0)
            break;

        if (is_diagonal())
            break;

        success = column_trigonalize(rk, Right);
        if (!success)
            return false;

        if (is_diagonal())
            break;
    }

    // now we change the diagonal so that we have successive divisibilty

    if (rk <= 1)
        return true;

    while (true) {
        size_t i = 0;
        for (; i < rk - 1; ++i)
            if (elem[i + 1][i + 1] % elem[i][i] != 0)
                break;
        if (i == rk - 1)
            break;

        Integer u, v, w, z, d = ext_gcd(elem[i][i], elem[i + 1][i + 1], u, v);
        elem[i][i + 1] = elem[i + 1][i + 1];
        w = -elem[i + 1][i + 1] / d;
        z = elem[i][i] / d;
        // Now we multiply the submatrix formed by columns "corner" and "j"
        // and rows corner,...,nr from the right by the 2x2 matrix
        // | u w |
        // | v z |
        if (!linear_comb_columns(i, i + 1, u, w, v, z))
            return false;
        if (!Right.linear_comb_columns(i, i + 1, u, w, v, z))
            return false;
        elem[i + 1][i] = 0;
    }

    return true;
}

template <>
bool Matrix<nmz_float>::SmithNormalForm_inner(size_t& rk, Matrix<nmz_float>& Right) {
    assert(false);
    return {};
}

template <>
bool Matrix<mpq_class>::SmithNormalForm_inner(size_t& rk, Matrix<mpq_class>& Right) {
    assert(false);
    return {};
}

#ifdef ENFNORMALIZ
template <>
bool Matrix<renf_elem_class>::SmithNormalForm_inner(size_t& rk, Matrix<renf_elem_class>& Right) {
    assert(false);
    return {};
}
#endif

// Converts "this" into Smith normal form, returns column transformation matrix
template <typename Integer>
Matrix<Integer> Matrix<Integer>::SmithNormalForm(size_t& rk) {
    size_t dim = nc;
    Matrix<Integer> Transf(dim);
    if (dim == 0)
        return Transf;

    Matrix<Integer> Copy = *this;
    bool success = SmithNormalForm_inner(rk, Transf);
    if (success)
        return Transf;

    Matrix<mpz_class> mpz_this(nr, dim);
    mat_to_mpz(Copy, mpz_this);
    Matrix<mpz_class> mpz_Transf(dim);
    mpz_this.SmithNormalForm_inner(rk, mpz_Transf);
    mat_to_Int(mpz_this, *this);
    mat_to_Int(mpz_Transf, Transf);
    return Transf;
}

template <>
Matrix<nmz_float> Matrix<nmz_float>::SmithNormalForm(size_t& rk) {
    assert(false);
    return *this;
}

template <>
Matrix<mpq_class> Matrix<mpq_class>::SmithNormalForm(size_t& rk) {
    assert(false);
    return *this;
}

#ifdef ENFNORMALIZ
template <>
Matrix<renf_elem_class> Matrix<renf_elem_class>::SmithNormalForm(size_t& rk) {
    assert(false);
    return *this;
}
#endif

//---------------------------------------------------------------------------
// Classless conversion routines
//---------------------------------------------------------------------------

template <typename Integer>
void mat_to_mpz(const Matrix<Integer>& mat, Matrix<mpz_class>& mpz_mat) {
    // convert(mpz_mat, mat);
    // we allow the matrices to have different sizes
    size_t nrows = min(mat.nr_of_rows(), mpz_mat.nr_of_rows());
    size_t ncols = min(mat.nr_of_columns(), mpz_mat.nr_of_columns());
    for (size_t i = 0; i < nrows; ++i)
        for (size_t j = 0; j < ncols; ++j)
            convert(mpz_mat[i][j], mat[i][j]);
#pragma omp atomic
    GMP_mat++;
}

template <>
void mat_to_mpz(const Matrix<mpq_class>& mat, Matrix<mpz_class>& mpz_mat) {
    assert(false);
    // convert(mpz_mat, mat);
    // we allow the matrices to have different sizes
    /*   size_t nrows = min(mat.nr_of_rows(),   mpz_mat.nr_of_rows());
       size_t ncols = min(mat.nr_of_columns(),mpz_mat.nr_of_columns());
       for(size_t i=0; i<nrows; ++i)
           for(size_t j=0; j<ncols; ++j)
               convert(mpz_mat[i][j], mat[i][j]);
           #pragma omp atomic
       GMP_mat++;
       */
}

#ifdef ENFNORMALIZ
template <>
void mat_to_mpz(const Matrix<renf_elem_class>& mat, Matrix<mpz_class>& mpz_mat) {
    assert(false);
    // convert(mpz_mat, mat);
    // we allow the matrices to have different sizes
    /*   size_t nrows = min(mat.nr_of_rows(),   mpz_mat.nr_of_rows());
       size_t ncols = min(mat.nr_of_columns(),mpz_mat.nr_of_columns());
       for(size_t i=0; i<nrows; ++i)
           for(size_t j=0; j<ncols; ++j)
               convert(mpz_mat[i][j], mat[i][j]);
           #pragma omp atomic
       GMP_mat++;
       */
}
#endif

template void mat_to_mpz<long>(const Matrix<long>&, Matrix<mpz_class>&);
template void mat_to_mpz<long long>(const Matrix<long long>&, Matrix<mpz_class>&);
template void mat_to_mpz<mpz_class>(const Matrix<mpz_class>&, Matrix<mpz_class>&);

//---------------------------------------------------------------------------

template <typename Integer>
void mat_to_Int(const Matrix<mpz_class>& mpz_mat, Matrix<Integer>& mat) {
    // convert(mat, mpz_mat);
    // we allow the matrices to have different sizes
    size_t nrows = min(mpz_mat.nr_of_rows(), mat.nr_of_rows());
    size_t ncols = min(mpz_mat.nr_of_columns(), mat.nr_of_columns());
    for (size_t i = 0; i < nrows; ++i)
        for (size_t j = 0; j < ncols; ++j)
            convert(mat[i][j], mpz_mat[i][j]);
}

template void mat_to_Int<long>(const Matrix<mpz_class>&, Matrix<long>&);
template void mat_to_Int<long long>(const Matrix<mpz_class>&, Matrix<long long>&);
template void mat_to_Int<mpz_class>(const Matrix<mpz_class>&, Matrix<mpz_class>&);

//---------------------------------------------------------------------------

template <typename Integer>
void mpz_submatrix(Matrix<mpz_class>& sub, const Matrix<Integer>& mother, const vector<key_t>& selection) {
    assert(sub.nr_of_columns() >= mother.nr_of_columns());
    assert(sub.nr_of_rows() >= selection.size());
    for (size_t i = 0; i < selection.size(); ++i)
        for (size_t j = 0; j < mother.nr_of_columns(); ++j)
            convert(sub[i][j], mother[selection[i]][j]);
}

//---------------------------------------------------------------------------

template <typename Integer>
void mpz_submatrix_trans(Matrix<mpz_class>& sub, const Matrix<Integer>& mother, const vector<key_t>& selection) {
    assert(sub.nr_of_columns() >= selection.size());
    assert(sub.nr_of_rows() >= mother.nr_of_columns());
    for (size_t i = 0; i < selection.size(); ++i)
        for (size_t j = 0; j < mother.nr_of_columns(); ++j)
            convert(sub[j][i], mother[selection[i]][j]);
}

//---------------------------------------------------

template <typename Integer>
void Matrix<Integer>::saturate() {
    *this = kernel().kernel();
}

//---------------------------------------------------------------------------

/* sorts rows of a matrix by a degree function and returns the permuation
 * does not change matrix (yet)
 */

/*
template <typename Integer>
vector<key_t> Matrix<Integer>::perm_sort_by_degree(const vector<key_t>& key,
                                                   const vector<Integer>& grading,
                                                   bool computed) const {
    list<vector<Integer> > rowList;
    vector<Integer> v;

    v.resize(nc + 2);
    unsigned long i, j;

    for (i = 0; i < key.size(); i++) {
        if (computed) {
            v[0] = v_scalar_product((*this).elem[key[i]], grading);
        }
        else {
            v[0] = 0;
            for (j = 0; j < nc; j++)
                v[0] += Iabs((*this).elem[key[i]][j]);
        }
        for (j = 0; j < nc; j++) {
            v[j + 1] = (*this).elem[key[i]][j];
        }
        v[nc + 1] = key[i];  // position of row
        rowList.push_back(v);
    }
    rowList.sort();
    vector<key_t> perm;
    perm.resize(key.size());
    i = 0;
    for (const auto& it : rowList) {
        perm[i] = convertToLong(it[nc + 1]);
        i++;
    }
    return perm;
}

template <>
vector<key_t> Matrix<mpq_class>::perm_sort_by_degree(const vector<key_t>& key,
                                                     const vector<mpq_class>& grading,
                                                     bool computed) const {
    assert(false);
    return vector<key_t>(0);
}

#ifdef ENFNORMALIZ
template <>
vector<key_t> Matrix<renf_elem_class>::perm_sort_by_degree(const vector<key_t>& key,
                                                           const vector<renf_elem_class>& grading,
                                                           bool computed) const {
    assert(false);
    return vector<key_t>(0);
}
#endif
*/

//---------------------------------------------------------------------------
// sorting routines

template <typename Integer>
bool weight_lex(const order_helper<Integer>& a, const order_helper<Integer>& b) {
    if (a.weight < b.weight)
        return true;
    if (a.weight == b.weight)
        if (*(a.v) < *(b.v))
            return true;
    return false;
}

//---------------------------------------------------------------------------
// orders the rows of matrix:
// such that row perm[0] is the new 0th row, row perm[1] the new 1st row etc.

template <typename Integer>
void Matrix<Integer>::order_rows_by_perm(const vector<key_t>& perm) {
    order_by_perm(elem, perm);
}

// sorts the rows accoring to the weight matrix (taking the absolute values of selected rows first)
template <typename Integer>
Matrix<Integer>& Matrix<Integer>::sort_by_weights(const Matrix<Integer>& Weights, vector<bool> absolute) {
    if (nr <= 1)
        return *this;
    vector<key_t> perm = perm_by_weights(Weights, absolute);
    order_by_perm(elem, perm);
    return *this;
}

template <typename Integer>
Matrix<Integer>& Matrix<Integer>::sort_lex() {
    if (nr <= 1)
        return *this;
    vector<key_t> perm = perm_by_weights(Matrix<Integer>(0, nc), vector<bool>(0));
    order_by_perm(elem, perm);
    return *this;
}

/* not used at present
template <typename Integer>
// sortes rows by descending number or zeroes
Matrix<Integer>& Matrix<Integer>::sort_by_nr_of_zeroes() {
    if (nr <= 1)
        return *this;
    vector<key_t> perm = perm_by_nr_zeroes();
    order_by_perm(elem, perm);
    return *this;
}

template <typename Integer>
vector<key_t> Matrix<Integer>::perm_by_lex() {
    return perm_by_weights(Matrix<Integer>(0, nc), vector<bool>(0));
}

template <typename Integer>
vector<key_t> Matrix<Integer>::perm_by_nr_zeroes() {  //
    // the row with index perm[0] has the maximum number of zeoes, then perm[1] etc.

    vector<vector<key_t> > order(nr, vector<key_t>(2, 0));

    for (key_t i = 0; i < nr; ++i) {
        order[i][1] = i;
        for (size_t j = 0; j < nc; ++j) {
            if (elem[i][j] == 0)
                order[i][0]++;
        }
    }

    sort(order.rbegin(), order.rend());
    vector<key_t> perm(nr);
    for (size_t i = 0; i < nr; ++i)
        perm[i] = order[i][1];
    return perm;
}
*/

template <typename Integer>
vector<key_t> Matrix<Integer>::perm_by_weights(const Matrix<Integer>& Weights, vector<bool> absolute) {
    // the smallest entry is the row with index perm[0], then perm[1] etc.
    // Computes only perm, matrix unchanged

    assert(Weights.nc == nc);
    assert(absolute.size() == Weights.nr);

    list<order_helper<Integer> > order;
    order_helper<Integer> entry;
    entry.weight.resize(Weights.nr);

    for (key_t i = 0; i < nr; ++i) {
        for (size_t j = 0; j < Weights.nr; ++j) {
            if (absolute[j])
                entry.weight[j] = v_scalar_product(Weights[j], v_abs_value(elem[i]));
            else
                entry.weight[j] = v_scalar_product(Weights[j], elem[i]);
        }
        entry.index = i;
        entry.v = &(elem[i]);
        order.push_back(entry);
    }
    order.sort(weight_lex<Integer>);
    vector<key_t> perm(nr);
    typename list<order_helper<Integer> >::const_iterator ord = order.begin();
    for (key_t i = 0; i < nr; ++i, ++ord)
        perm[i] = ord->index;

    return perm;
}

//==========================================================

template <typename Integer>
Matrix<Integer> Matrix<Integer>::solve_congruences(bool& zero_modulus) const {
    zero_modulus = false;
    size_t i, j;
    size_t nr_cong = nr, dim = nc - 1;
    if (nr_cong == 0)
        return Matrix<Integer>(dim);  // give back unit matrix

    // add slack variables to convert congruences into equaitions
    Matrix<Integer> Cong_Slack(nr_cong, dim + nr_cong);
    for (i = 0; i < nr_cong; i++) {
        for (j = 0; j < dim; j++) {
            Cong_Slack[i][j] = elem[i][j];
        }
        Cong_Slack[i][dim + i] = elem[i][dim];
        if (elem[i][dim] == 0) {
            zero_modulus = true;
            return Matrix<Integer>(0, dim);
        }
    }

    // compute kernel

    Matrix<Integer> Help = Cong_Slack.kernel();  // gives the solutions to the the system with slack variables
    Matrix<Integer> Ker_Basis(dim, dim);         // must now project to first dim coordinates to get rid of them
    for (size_t i = 0; i < dim; ++i)
        for (size_t j = 0; j < dim; ++j)
            Ker_Basis[i][j] = Help[i][j];
    return Ker_Basis;
}

#ifdef ENFNORMALIZ
template <>
Matrix<renf_elem_class> Matrix<renf_elem_class>::solve_congruences(bool& zero_modulus) const {
    assert(false);
    return Matrix<renf_elem_class>(0, 0);
}
#endif

//---------------------------------------------------

template <typename Integer>
vector<key_t> Matrix<Integer>::max_and_min(const vector<Integer>& L, const vector<Integer>& norm) const {
    vector<key_t> result(2, 0);
    if (nr == 0)
        return result;
    key_t maxind = 0, minind = 0;
    Integer maxval = v_scalar_product(L, elem[0]);
    Integer minval = maxval;
    Integer maxnorm = 1, minnorm = 1;
    if (norm.size() > 0) {
        maxnorm = v_scalar_product(norm, elem[0]);
        minnorm = maxnorm;
    }
    for (key_t i = 0; i < nr; ++i) {
        Integer val = v_scalar_product(L, elem[i]);
        if (norm.size() == 0) {
            if (val > maxval) {
                maxind = i;
                maxval = val;
            }
            if (val < minval) {
                minind = i;
                minval = val;
            }
        }
        else {
            Integer nm = v_scalar_product(norm, elem[i]);
            if (maxnorm * val > nm * maxval) {
                maxind = i;
                maxval = val;
            }
            if (minnorm * val < nm * minval) {
                minind = i;
                minval = val;
            }
        }
    }
    result[0] = maxind;
    result[1] = minind;
    return result;
}

vector<key_t> max_and_min_values(const vector<nmz_float> Values) {
    vector<key_t> result(2, 0);
    if (Values.size() == 0)
        return result;
    key_t maxind = 0, minind = 0;
    nmz_float maxval = Values[0];
    nmz_float minval = maxval;
    for (key_t i = 0; i < Values.size(); ++i) {
    nmz_float val = Values[i];
        if (val > maxval) {
            maxind = i;
            maxval = val;
        }
        if (val < minval) {
            minind = i;
            minval = val;
        }
    }
    result[0] = maxind;
    result[1] = minind;
    return result;
}

template <typename Integer>
size_t Matrix<Integer>::extreme_points_first(bool verbose, vector<key_t>& perm) {
    assert(false);
    return 0;
}

template <>
size_t Matrix<nmz_float>::extreme_points_first(bool verbose, vector<key_t>& perm) {

    if (nr == 0)
        return 0;

    if (verbose)
        verboseOutput() << "Trying to find extreme points" << endl;

    size_t nr_extr = 0;

    vector<bool> marked(nr, false);
    size_t no_success = 0;
    // size_t nr_attempt=0;

    size_t counter_100 = 0;
    while (true) {
        INTERRUPT_COMPUTATION_BY_EXCEPTION

        // nr_attempt++; cout << nr_attempt << endl;

        vector<vector<key_t> > max_min_ind(10 * nc);
#pragma omp parallel
        {;
        vector<nmz_float> Values(nr,0);
        vector<nmz_float> L(nc,0), Lmod(nc,0);
#pragma omp for
        for (size_t j = 0; j <  max_min_ind.size(); ++j) {

            /* nmz_float displacement;
            size_t coord;

            while(true){
                Lmod = L;
                coord = rand() % nc;
                displacement = 2*((double) rand() / (RAND_MAX))-1;
                Lmod[coord] += displacement;
                size_t c = 0;
                for(; c < nc; ++c)
                    if(Lmod[c] != 0)
                        break;
                if(c < nc)
                    break;
            }

            for(size_t v = 0; v < nr; ++v)
                Values[v] += elem[v][coord]*displacement;*/

            while(true){
                bool is_zero = true;
                bool is_too_large = false;
                nmz_float norm = 0;
                for(size_t i = 0; i < nc; ++i){
                    L[i] = 2*((double) rand() / (RAND_MAX))-1;
                    if(L[i] != 0)
                        is_zero = false;
                    norm += L[i] * L[i];
                    if(norm > 1.0){
                        is_too_large = true;
                        break;
                    }

                }
                if(is_zero || is_too_large)
                    continue;

                break;
            }

            for(size_t v = 0; v < nr; ++v)
                Values[v] = v_scalar_product(L, elem[v]);
            max_min_ind[j] = max_and_min_values(Values);
        }
        } // parallel

        size_t new_hits = 0;

        for (size_t j = 0; j < max_min_ind.size(); ++j) {
            if (!marked[max_min_ind[j][0]])
                new_hits++;
            if (!marked[max_min_ind[j][1]])
                new_hits++;
            marked[max_min_ind[j][0]] = true;
            marked[max_min_ind[j][1]] = true;
        }

        counter_100 += new_hits;

        if (new_hits == 0)
            no_success++;
        else {
            no_success = 0;
            nr_extr += new_hits;
            if (verbose && counter_100 >= 100) {
                verboseOutput() << "Extreme points " << nr_extr << endl;
                counter_100 = 0;
            }
        }
        if (no_success > nc)
            break;
    }

    Matrix<long long> Extr(nr_extr, nc);     // the recognized extreme rays
    size_t j = 0;
    perm = vector<key_t> (nr);
    for (size_t i = 0; i < nr; ++i) {
        if (marked[i]) {
            perm[j] = i;
            j++;
        }
    }
    nr_extr = j;
    for (size_t i = 0; i < nr; ++i) {
        if (!marked[i]) {
            perm[j] = i;
            j++;
        }
    }
    // order_rows_by_perm(perm);
    // cout << nr_extr << "extreme points found"  << endl;
    return nr_extr;
}

//---------------------------------------------------

template <typename Integer>
vector<Integer> Matrix<Integer>::find_inner_point() {
    vector<key_t> simplex = max_rank_submatrix_lex();
    vector<Integer> point(nc);
    for (unsigned int& i : simplex)
        point = v_add(point, elem[i]);
    return point;
}

//---------------------------------------------------

template <typename Integer>
bool Matrix<Integer>::zero_product_with_transpose_of(const Matrix& B) {
    if (nr == 0 || B.nr == 0)
        return true;

    assert(nc == B.nc);
    for (size_t i = 0; i < nr; ++i)
        for (size_t j = 0; j < B.nr; ++j)
            if (v_scalar_product(elem[i], B[j]) != 0)
                return false;
    return true;
}

//---------------------------------------------------

//---------------------------------------------------------------------------
// version with full number of points
// and search for optimal point

template <typename Integer>
vector<Integer> Matrix<Integer>::optimal_subdivision_point() const {
    return optimal_subdivision_point_inner();
}

// In mpz_class we first try machine integer
template <>
vector<mpz_class> Matrix<mpz_class>::optimal_subdivision_point() const {
    try {
        Matrix<MachineInteger> GensMI;
        convert(GensMI, *this);
        vector<MachineInteger> PMI = GensMI.optimal_subdivision_point_inner();
        vector<mpz_class> P;
        convert(P, PMI);
        return P;
    } catch (const ArithmeticException& e) {
        return optimal_subdivision_point_inner();
    }
}

/*
 * Version with LL for every matrix --- seems to be the best choice
 */
// version with a single point, only top of the search polytope
// After 2 attempts without improvement, g raised to opt_value-1

template <typename Integer>
vector<Integer> Matrix<Integer>::optimal_subdivision_point_inner() const {
    // returns empty vector if simplex cannot be subdivided with smaller detsum

    // cout << "***************" << endl;

    assert(nr > 0);
    assert(nr == nc);

    Sublattice_Representation<Integer> NewCoord = LLL_coordinates<Integer, Integer>(*this);
    Matrix<Integer> Gred = NewCoord.to_sublattice(*this);

    vector<Integer> opt_point;

    vector<Integer> N = Gred.find_linear_form();
    assert(N.size() == nr);
    Integer G = v_scalar_product(N, Gred[0]);
    if (G <= 1)
        return opt_point;
    Matrix<Integer> Supp;
    Integer V;
    vector<key_t> dummy(nr);
    for (size_t i = 0; i < nr; ++i)
        dummy[i] = i;
    Gred.simplex_data(dummy, Supp, V, true);
    Integer MinusOne = -1;
    vector<Integer> MinusN(N);
    v_scalar_multiplication(MinusN, MinusOne);
    Supp.append(MinusN);
    Supp.resize_columns(nr + 1);
    Supp.exchange_columns(0, nc);  // grading to the front!

    Integer opt_value = G;
    Integer empty_value = 0;
    Integer g = G - 1;

    Integer den = 2;

    vector<Integer> Zero(nr + 1);  // the excluded vector
    Zero[0] = 1;

    // Incidence matrix for projectand lift
    vector<dynamic_bitset> Ind(nr + 1);
    for (size_t i = 0; i < nr + 1; ++i) {
        Ind[i].resize(nc + 1);
        for (size_t j = 0; j < nc + 1; ++j)
            Ind[i][j] = true;
        Ind[i][i] = false;
    }

    size_t nothing_found = 0;
    while (true) {
        vector<Integer> SubDiv;
        // cout << "Opt " << opt_value << " test " << g << " empty " << empty_value << " nothing "  << nothing_found << endl;
        Supp[nr][0] = g;  // the degree at which we cut the simplex1;
        ProjectAndLift<Integer, Integer> PL(Supp, Ind, nr + 1);
        PL.set_excluded_point(Zero);
        PL.set_verbose(false);
        PL.compute(false);  // only a single point
        PL.put_single_point_into(SubDiv);
        if (SubDiv.size() == 0) {  // no point found
            nothing_found++;
            if (g == opt_value - 1) {
                if (opt_point.size() == 0)
                    return opt_point;
                return NewCoord.from_sublattice(opt_point);  // optimal point found (or nothing found)
            }
            empty_value = g;
            if (nothing_found < 1)  // can't be true if "1" is not raised to a higher value
                g = empty_value + 1 + (den - 1) * (opt_value - empty_value - 2) / den;
            else
                g = opt_value - 1;
            den *= 2;  // not used in the present setting (see above)
        }
        else {  // point found
            nothing_found = 0;
            den = 2;  // back to start value
            opt_point = SubDiv;
            std::swap(opt_point[0], opt_point[nc]);
            opt_point.resize(nc);
            if (opt_value == empty_value + 1) {
                if (opt_point.size() == 0)
                    return opt_point;
                return NewCoord.from_sublattice(opt_point);
            }
            opt_value = v_scalar_product(opt_point, N);
            g = empty_value + 1 + (opt_value - empty_value - 2) / 2;
        }
    }
}

template <>
vector<mpq_class> Matrix<mpq_class>::optimal_subdivision_point_inner() const {
    assert(false);
    return {};
}

template <>
vector<nmz_float> Matrix<nmz_float>::optimal_subdivision_point_inner() const {
    assert(false);
    return {};
}

#ifdef ENFNORMALIZ
template <>
vector<renf_elem_class> Matrix<renf_elem_class>::optimal_subdivision_point_inner() const {
    assert(false);
    return {};
}
#endif

//---------------------------------------------------------------------------

// incremental Gram-Schmidt on rows r, from <= r < to (ATTENTION <)
// The orthogonal matrix is B
// Coefficients in M
template <typename Integer>
void Matrix<Integer>::GramSchmidt(Matrix<nmz_float>& B, Matrix<nmz_float>& M, int from, int to) {
    // from=0;
    // to= (int) nr_of_rows();
    assert(to <= (int)nr_of_rows());
    size_t dim = nr_of_columns();
    for (int i = from; i < to; ++i) {
        convert(B[i], elem[i]);
        // cout << B[i];
        for (int j = 0; j < i; ++j) {
            nmz_float sp = 0;
            for (size_t k = 0; k < dim; ++k) {
                nmz_float fact;
                convert(fact, elem[i][k]);
                sp += fact * B[j][k];
            }
            M[i][j] = sp / v_scalar_product(B[j], B[j]);
            // cout << "GS " << i << " " << j << " " << sp << " " << v_scalar_product(B[j],B[j]) << " " <<  M[i][j] << endl;
            for (size_t k = 0; k < dim; ++k)
                B[i][k] -= M[i][j] * B[j][k];
        }
    }
}

template <>
void Matrix<mpq_class>::GramSchmidt(Matrix<nmz_float>& B, Matrix<nmz_float>& M, int from, int to) {
    assert(false);

    /*
        // from=0;
        // to= (int) nr_of_rows();
        assert(to <= (int) nr_of_rows());
        size_t dim=nr_of_columns();
        for(int i=from;i<to;++i){
            convert(B[i],elem[i]);
            // cout << B[i];
            for(int j=0;j<i;++j){
                nmz_float sp=0;
                for(size_t k=0;k<dim;++k){
                    nmz_float fact;
                    convert(fact,elem[i][k]);
                    sp+=fact*B[j][k];
                }
                M[i][j]=sp/v_scalar_product(B[j],B[j]);
                // cout << "GS " << i << " " << j << " " << sp << " " << v_scalar_product(B[j],B[j]) << " " <<  M[i][j] << endl;
                for(size_t k=0;k<dim;++k)
                    B[i][k]-=M[i][j]*B[j][k];
            }
        }*/
}

#ifdef ENFNORMALIZ
template <>
void Matrix<renf_elem_class>::GramSchmidt(Matrix<nmz_float>& B, Matrix<nmz_float>& M, int from, int to) {
    assert(false);

    /*
        // from=0;
        // to= (int) nr_of_rows();
        assert(to <= (int) nr_of_rows());
        size_t dim=nr_of_columns();
        for(int i=from;i<to;++i){
            convert(B[i],elem[i]);
            // cout << B[i];
            for(int j=0;j<i;++j){
                nmz_float sp=0;
                for(size_t k=0;k<dim;++k){
                    nmz_float fact;
                    convert(fact,elem[i][k]);
                    sp+=fact*B[j][k];
                }
                M[i][j]=sp/v_scalar_product(B[j],B[j]);
                // cout << "GS " << i << " " << j << " " << sp << " " << v_scalar_product(B[j],B[j]) << " " <<  M[i][j] << endl;
                for(size_t k=0;k<dim;++k)
                    B[i][k]-=M[i][j]*B[j][k];
            }
        }*/
}
#endif

/*
template<typename Integer>
Matrix<Integer> Matrix<Integer>::LLL_red(Matrix<Integer>& T, Matrix<Integer>& Tinv) const{
// returns Lred =LLL_reduced(L) (sublattice generated by the rows!)
// Lred=T*this, Tinv=inverse(T)
// We follow Gerhard and von zur Gathen; also see Cohen, p.89 (5)

    T=Tinv=Matrix<Integer>(nr);

    Matrix<Integer> Lred=*this;
    size_t dim=nr_of_columns();
    int n=nr_of_rows();
    // pretty_print(cout);
    assert((int) rank()==n);
    if(n<=1)
        return Lred;

    Matrix<nmz_float> G(n,dim);
    Matrix<nmz_float> M(n,n);

    Lred.GramSchmidt(G,M,0,2);

    int i=1;
    while(true){

        for(int j=i-1;j>=0;--j){
            Integer fact;
            cout << "MMMMM " << i << " " << j << " " << M[i][j] << endl;
            cout << i << "---" << G[i];
            cout << j << "---" << G[j];
            convert(fact,round(M[i][j]));
            v_el_trans<Integer>(Lred[j],Lred[i],-fact,0);
            v_el_trans<Integer>(T[j],T[i],-fact,0);
            v_el_trans<Integer>(Tinv[i],Tinv[j],fact,0);
            Lred.GramSchmidt(G,M,i,i+1);
        }
        if(i==0){
            i=1;
            Lred.GramSchmidt(G,M,0,2);
            continue;
        }
        nmz_float t1=v_scalar_product(G[i-1],G[i-1]);
        nmz_float t2=v_scalar_product(G[i],G[i]);
        if(t1> 2*t2){
            std::swap(Lred[i],Lred[i-1]);
            std::swap(T[i],T[i-1]);
            std::swap(Tinv[i],Tinv[i-1]);
            Lred.GramSchmidt(G,M,i-1,i); // i-1,i+1);
            // cout << i-1 << "---" << G[i-1];
            i--;
        }
        else{
            i++;
            if(i>=n)
                break;
            Lred.GramSchmidt(G,M,i,i+1);
        }
    }

    Tinv=Tinv.transpose();

    return Lred;
}*/

/*
#ifdef ENFNORMALIZ
template<>
Matrix<renf_elem_class> Matrix<renf_elem_class>::LLL_red(Matrix<renf_elem_class>& T, Matrix<renf_elem_class>& Tinv) const{

    assert(false);
    return Matrix<renf_elem_class(0,0);

#endif
*/

template <typename Integer>
Matrix<Integer> Matrix<Integer>::LLL() const {
    Matrix<Integer> Dummy1, Dummy2;
    return LLL_red(*this, Dummy1, Dummy2);
}

template <>
Matrix<mpq_class> Matrix<mpq_class>::LLL() const {
    assert(false);
    return {};
}

template <typename Integer>
Matrix<Integer> Matrix<Integer>::LLL_transpose() const {
    return transpose().LLL().transpose();
}

template class Matrix<long>;
template class Matrix<long long>;
template class Matrix<mpz_class>;
template class Matrix<mpq_class>;
template class Matrix<nmz_float>;
#ifdef ENFNORMALIZ
template class Matrix<renf_elem_class>;
#endif

//---------------------------------------------------
// routines for binary matrices

// insert binary expansion of val at "planar" coordinates (i,j)
template <typename Integer>
void BinaryMatrix<Integer>::insert(long val, key_t i, key_t j) {
    assert(i < nr_rows);
    assert(j < nr_columns);

    vector<bool> bin_exp = binary_expansion(val);
    /*
    while (val != 0) {  // binary expansion of val
        Integer bin_digit = val % 2;
        if (bin_digit == 1)
            bin_exp.push_back(true);
        else
            bin_exp.push_back(false);
        val /= 2;
    }*/

    long add_layers = bin_exp.size() - get_nr_layers();
    if (add_layers > 0) {
        for (long k = 0; k < add_layers; ++k)
            Layers.push_back(vector<dynamic_bitset>(nr_rows, dynamic_bitset(nr_columns)));
    }
    else {
        for (size_t k = bin_exp.size(); k < get_nr_layers(); ++k)  // to be on the safe side
            Layers[k][i][j] = false;                               // in case this object was used before
    }

    for (size_t k = 0; k < bin_exp.size(); ++k) {
        Layers[k][i][j] = bin_exp[k];
    }
}

// put rows and columns into the order determined by row_order and col:order
template <typename Integer>
BinaryMatrix<Integer> BinaryMatrix<Integer>::reordered(const vector<key_t>& row_order, const vector<key_t>& col_order) const {
    assert(nr_rows == row_order.size());
    assert(nr_columns == col_order.size());
    size_t ll = get_nr_layers();
    BinaryMatrix<Integer> MatReordered(nr_rows, nr_columns, ll);
    for (size_t i = 0; i < nr_rows; ++i) {
        for (size_t j = 0; j < nr_columns; ++j) {
            for (size_t k = 0; k < ll; ++k) {
                MatReordered.Layers[k][i][j] = Layers[k][row_order[i]][col_order[j]];
            }
        }
    }
    MatReordered.values = values;
    MatReordered.mpz_values = mpz_values;
    return MatReordered;
}

// constructors
template <typename Integer>
BinaryMatrix<Integer>::BinaryMatrix() {
    nr_rows = 0;
    nr_columns = 0;
}

template <typename Integer>
BinaryMatrix<Integer>::BinaryMatrix(size_t m, size_t n) {
    nr_rows = m;
    nr_columns = n;
    // we need at least one layer -- in case only the value 0 is inserted
    Layers.push_back(vector<dynamic_bitset>(nr_rows, dynamic_bitset(nr_columns)));
}

template <typename Integer>
BinaryMatrix<Integer>::BinaryMatrix(size_t m, size_t n, size_t height) {
    nr_rows = m;
    nr_columns = n;
    for (size_t k = 0; k < height; ++k)
        Layers.push_back(vector<dynamic_bitset>(nr_rows, dynamic_bitset(nr_columns)));
}

// data access & equality

// test bit k in binary expansion at "planar" coordiantes (i,j)
template <typename Integer>
bool BinaryMatrix<Integer>::test(key_t i, key_t j, key_t k) const {
    assert(i < nr_rows);
    assert(j < nr_columns);
    assert(k < Layers.size());
    return Layers[k][i].test(j);
}

template <typename Integer>
size_t BinaryMatrix<Integer>::get_nr_layers() const {
    return Layers.size();
}

template <typename Integer>
size_t BinaryMatrix<Integer>::get_nr_rows() const {
    return nr_rows;
}

template <typename Integer>
size_t BinaryMatrix<Integer>::get_nr_columns() const {
    return nr_columns;
}

template <typename Integer>
bool BinaryMatrix<Integer>::equal(const BinaryMatrix& Comp) const {
    if (nr_rows != Comp.nr_rows || nr_columns != Comp.nr_columns || get_nr_layers() != Comp.get_nr_layers())
        return false;
    for (size_t i = 0; i < get_nr_layers(); ++i)
        if (Layers[i] != Comp.Layers[i])
            return false;
    return true;
}

template <typename Integer>
void BinaryMatrix<Integer>::set_values(const vector<Integer>& V) {
    values = V;
}

template <typename Integer>
void BinaryMatrix<Integer>::get_data_mpz(BinaryMatrix<mpz_class>& BM_mpz) {
    swap(Layers, BM_mpz.Layers);
    swap(mpz_values, BM_mpz.values);
    values.resize(0);
}

template <typename Integer>
const vector<vector<dynamic_bitset> >& BinaryMatrix<Integer>::get_layers() const {
    return Layers;
}

template <typename Integer>
const vector<Integer>& BinaryMatrix<Integer>::get_values() const {
    return values;
}

template <typename Integer>
const vector<mpz_class>& BinaryMatrix<Integer>::get_mpz_values() const {
    return mpz_values;
}

template <typename Integer>
long BinaryMatrix<Integer>::val_entry(size_t i, size_t j) const {
    assert(i < nr_rows);
    assert(j < nr_columns);

    long v = 0, p2 = 1;

    for (size_t k = 0; k < get_nr_layers(); ++k) {
        long n = 0;
        if (test(i, j, k))
            n = 1;
        v += p2 * n;
        p2 *= 2;
    }
    return v;
}

template <typename Integer>
Matrix<Integer> BinaryMatrix<Integer>::get_value_mat() const {
    Matrix<Integer> VM(nr_rows, nr_columns);
    for (size_t i = 0; i < nr_rows; ++i) {
        for (size_t j = 0; j < nr_columns; ++j) {
            cout << "EEEEEE " << val_entry(i, j) << endl;
            VM[i][j] = values[val_entry(i, j)];
        }
    }
    return VM;
}

template <typename Integer>
Matrix<mpz_class> BinaryMatrix<Integer>::get_mpz_value_mat() const {
    Matrix<mpz_class> VM(nr_rows, nr_columns);
    for (size_t i = 0; i < nr_rows; ++i) {
        for (size_t j = 0; j < nr_columns; ++j) {
            VM[i][j] = mpz_values[val_entry(i, j)];
        }
    }
    return VM;
}

template <typename Integer>
void BinaryMatrix<Integer>::pretty_print(std::ostream& out, bool with_row_nr) const {
    if (values.size() > 0) {
        Matrix<Integer> PM = get_value_mat();
        PM.pretty_print(out, with_row_nr);
    }
    else if (mpz_values.size() > 0) {
        Matrix<mpz_class> PM = get_mpz_value_mat();
        PM.pretty_print(out, with_row_nr);
    }
}

template class BinaryMatrix<long>;
template class BinaryMatrix<long long>;
template class BinaryMatrix<mpz_class>;
#ifdef ENFNORMALIZ
template class BinaryMatrix<renf_elem_class>;
#endif

//-----------------------------------------------------------------------

// determines the maximal subsets in a vector of subsets given by their indicator vectors
// result returned in is_max_subset
// if  is_max_subset has size 0, it is fully set in this routine
// otherwise it is supposed to be pre-information: the entry false means:
//   already known not to be not maximal (or irrelevant)
// if a set occurs more than once, only the last instance is recognized as maximal
template <typename IncidenceVector>
void maximal_subsets(const vector<IncidenceVector>& ind, IncidenceVector& is_max_subset) {
    if (ind.size() == 0)
        return;

    if (is_max_subset.size() == 0) {
        is_max_subset.resize(ind.size());
        for (size_t i = 0; i < is_max_subset.size(); ++i)
            is_max_subset[i] = true;
    }

    assert(is_max_subset.size() == ind.size());

    size_t nr_sets = ind.size();
    size_t card = ind[0].size();
    vector<key_t> elem(card);

    for (size_t i = 0; i < nr_sets; i++) {
        if (!is_max_subset[i])  // already known to be non-maximal
            continue;

        size_t k = 0;  // counts the number of elements in set with index i
        for (size_t j = 0; j < card; j++) {
            if (ind[i][j]) {
                elem[k] = j;
                k++;
            }
        }

        for (size_t j = 0; j < nr_sets; j++) {
            if (i == j || !is_max_subset[j])  // don't compare with itself or something known not to be maximal
                continue;
            size_t t;
            for (t = 0; t < k; t++) {
                if (!ind[j][elem[t]])
                    break;  // not a superset
            }
            if (t == k) {  // found a superset
                is_max_subset[i] = false;
                break;  // the loop over j
            }
        }
    }
}

template <>
void maximal_subsets(const vector<dynamic_bitset>& ind, dynamic_bitset& is_max_subset) {
    if (ind.size() == 0)
        return;

    if (is_max_subset.size() == 0) {
        is_max_subset.resize(ind.size());
        is_max_subset.set();
    }

    assert(is_max_subset.size() == ind.size());

    size_t nr_sets = ind.size();
    for (size_t i = 0; i < nr_sets; ++i) {
        if (!is_max_subset[i])
            continue;
        for (size_t j = 0; j < nr_sets; ++j) {
            if (i == j || !is_max_subset[j])  // don't compare to itself or something known not to be maximal
                continue;
            if (ind[i].is_subset_of(ind[j])) {
                is_max_subset[i] = false;
                break;
            }
        }
    }
}

template void maximal_subsets(const vector<vector<bool> >&, vector<bool>&);
// template void maximal_subsets(const vector<dynamic_bitset>&, dynamic_bitset&);

template <typename Integer>
void makeIncidenceMatrix(vector<dynamic_bitset>& IncidenceMatrix, const Matrix<Integer>& Gens, const Matrix<Integer>& LinForms) {
    IncidenceMatrix = vector<dynamic_bitset>(LinForms.nr_of_rows(), dynamic_bitset(Gens.nr_of_rows()));

    std::exception_ptr tmp_exception;
    bool skip_remaining = false;

#pragma omp parallel for
    for (size_t i = 0; i < LinForms.nr_of_rows(); ++i) {
        if (skip_remaining)
            continue;

        try {
            INTERRUPT_COMPUTATION_BY_EXCEPTION

            for (size_t j = 0; j < Gens.nr_of_rows(); ++j) {
                if (v_scalar_product(LinForms[i], Gens[j]) == 0)
                    IncidenceMatrix[i][j] = 1;
            }

        } catch (const std::exception&) {
            tmp_exception = std::current_exception();
            skip_remaining = true;
#pragma omp flush(skip_remaining)
        }
    }
    if (!(tmp_exception == 0))
        std::rethrow_exception(tmp_exception);
}

template void makeIncidenceMatrix(vector<dynamic_bitset>&, const Matrix<long>&, const Matrix<long>&);
template void makeIncidenceMatrix(vector<dynamic_bitset>&, const Matrix<long long>&, const Matrix<long long>&);
template void makeIncidenceMatrix(vector<dynamic_bitset>&, const Matrix<mpz_class>&, const Matrix<mpz_class>&);
#ifdef ENFNORMALIZ
template void makeIncidenceMatrix(vector<dynamic_bitset>&, const Matrix<renf_elem_class>&, const Matrix<renf_elem_class>&);
#endif

}  // namespace libnormaliz
