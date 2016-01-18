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

#include <fstream>
#include <set>
#include <algorithm>
#include <math.h>

#include "libnormaliz/matrix.h"
#include "libnormaliz/vector_operations.h"
#include "libnormaliz/normaliz_exception.h"
#include "libnormaliz/sublattice_representation.h"

//---------------------------------------------------------------------------

namespace libnormaliz {
using namespace std;

//---------------------------------------------------------------------------
//Public
//---------------------------------------------------------------------------

template<typename Integer>
Matrix<Integer>::Matrix(){
    nr=0;
    nc=0;
}

//---------------------------------------------------------------------------

template<typename Integer>
Matrix<Integer>::Matrix(size_t dim){
    nr=dim;
    nc=dim;
    elem = vector< vector<Integer> >(dim, vector<Integer>(dim));
    for (size_t i = 0; i < dim; i++) {
        elem[i][i]=1;
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
Matrix<Integer>::Matrix(size_t row, size_t col){
    nr=row;
    nc=col;
    elem = vector< vector<Integer> >(row, vector<Integer>(col));
}

//---------------------------------------------------------------------------

template<typename Integer>
Matrix<Integer>::Matrix(size_t row, size_t col, Integer value){
    nr=row;
    nc=col;
    elem = vector< vector<Integer> > (row, vector<Integer>(col,value));
}

//---------------------------------------------------------------------------

template<typename Integer>
Matrix<Integer>::Matrix(const vector< vector<Integer> >& new_elem){
    nr=new_elem.size();
    if (nr>0) {
        nc=new_elem[0].size();
        elem=new_elem;
        //check if all rows have the same length
        for (size_t i=1; i<nr; i++) {
            if (elem[i].size() != nc) {
                errorOutput() << "Inconsistent lengths of rows in matrix!" << endl;
                throw BadInputException();
            }
        }
    } else {
        nc=0;
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
Matrix<Integer>::Matrix(const list< vector<Integer> >& new_elem){
    nr = new_elem.size();
    elem = vector< vector<Integer> > (nr);
    nc = 0;
    size_t i=0;
    typename list< vector<Integer> >::const_iterator it=new_elem.begin();
    for(; it!=new_elem.end(); ++it, ++i) {
        if(i == 0) {
            nc = (*it).size();
        } else {
            if ((*it).size() != nc) {
                errorOutput() << "Inconsistent lengths of rows in matrix!" << endl;
                throw BadInputException();
            }
        }
        elem[i]=(*it);
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::write(istream& in){
    size_t i,j;
    for(i=0; i<nr; i++){
        for(j=0; j<nc; j++) {
            in >> elem[i][j];
        }
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::write(size_t row, const vector<Integer>& data){
    assert(row >= 0);
    assert(row < nr); 
    assert(nc == data.size());
    
    elem[row]=data;
}

//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::write(size_t row, const vector<int>& data){
    assert(row >= 0);
    assert(row < nr); 
    assert(nc == data.size());

    for (size_t i = 0; i < nc; i++) {
        elem[row][i]=data[i];
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::write_column(size_t col, const vector<Integer>& data){
    assert(col >= 0);
    assert(col < nc); 
    assert(nr == data.size());

    for (size_t i = 0; i < nr; i++) {
        elem[i][col]=data[i];
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::write(size_t row, size_t col, Integer data){
    assert(row >= 0);
    assert(row < nr); 
    assert(col >= 0);
    assert(col < nc); 

    elem[row][col]=data;
}

//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::print(const string& name,const string& suffix) const{
    string file_name = name+"."+suffix;
    const char* file = file_name.c_str();
    ofstream out(file);
    print(out);
    out.close();
}

//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::print_append(const string& name,const string& suffix) const{
    string file_name = name+"."+suffix;
    const char* file = file_name.c_str();
    ofstream out(file,ios_base::app);
    print(out);
    out.close();
}

//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::print(ostream& out) const{
    size_t i,j;
    out<<nr<<endl<<nc<<endl;
    for (i = 0; i < nr; i++) {
        for (j = 0; j < nc; j++) {
            out<<elem[i][j]<<" ";
        }
        out<<endl;
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::pretty_print(ostream& out, bool with_row_nr) const{    
    size_t i,j,k;
    vector<size_t> max_length = maximal_decimal_length_columnwise();
    size_t max_index_length = decimal_length(nr);
    for (i = 0; i < nr; i++) {
        if (with_row_nr) {
            for (k= 0; k <= max_index_length - decimal_length(i); k++) {
                out<<" ";
            }
            out << i << ": ";
        }
        for (j = 0; j < nc; j++) {
            for (k= 0; k <= max_length[j] - decimal_length(elem[i][j]); k++) {
                out<<" ";
            }
            out<<elem[i][j];
        }
        out<<endl;
    }
}
//---------------------------------------------------------------------------


template<typename Integer>
void Matrix<Integer>::read() const{      //to overload for files
    size_t i,j;
    for(i=0; i<nr; i++){
        cout << "\n" ;
        for(j=0; j<nc; j++) {
            cout << elem[i][j] << " ";
        }
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
vector<Integer> Matrix<Integer>::read(size_t row) const{
    assert(row >= 0);
    assert(row < nr);

    return elem[row];
}

//---------------------------------------------------------------------------

template<typename Integer>
Integer Matrix<Integer>::read (size_t row, size_t col) const{
    assert(row >= 0);
    assert(row < nr); 
    assert(col >= 0);
    assert(col < nc); 

    return elem[row][col];
}

//---------------------------------------------------------------------------

template<typename Integer>
size_t Matrix<Integer>::nr_of_rows () const{
    return nr;
}

//---------------------------------------------------------------------------

template<typename Integer>
size_t Matrix<Integer>::nr_of_columns () const{
    return nc;
}

//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::random (int mod) {
    size_t i,j;
    int k;
    for (i = 0; i < nr; i++) {
        for (j = 0; j < nc; j++) {
            k = rand();
            elem[i][j]=k % mod;
        }
    }
}
//---------------------------------------------------------------------------
  
template<typename Integer>
void Matrix<Integer>::set_zero() {
    size_t i,j;
    for (i = 0; i < nr; i++) {
        for (j = 0; j < nc; j++) {
            elem[i][j] = 0;
        }
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::select_submatrix(const Matrix<Integer>& mother, const vector<key_t>& rows){

    assert(nr>=rows.size());
    assert(nc>=mother.nc);
    
    size_t size=rows.size(), j;
    for (size_t i=0; i < size; i++) {
        j=rows[i];
        for(size_t k=0;k<mother.nc;++k)
            elem[i][k]=mother[j][k];
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::select_submatrix_trans(const Matrix<Integer>& mother, const vector<key_t>& rows){

    assert(nc>=rows.size());
    assert(nr>=mother.nc);
    
    size_t size=rows.size(), j;
    for (size_t i=0; i < size; i++) {
        j=rows[i];
        for(size_t k=0;k<mother.nc;++k)
            elem[k][i]=mother[j][k];
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
Matrix<Integer> Matrix<Integer>::submatrix(const vector<key_t>& rows) const{
    size_t size=rows.size(), j;
    Matrix<Integer> M(size, nc);
    for (size_t i=0; i < size; i++) {
        j=rows[i];
        assert(j >= 0);
        assert(j < nr);
        M.elem[i]=elem[j];
    }
    return M;
}

//---------------------------------------------------------------------------

template<typename Integer>
Matrix<Integer> Matrix<Integer>::submatrix(const vector<int>& rows) const{
    size_t size=rows.size(), j;
    Matrix<Integer> M(size, nc);
    for (size_t i=0; i < size; i++) {
        j=rows[i];
        assert(j >= 0);
        assert(j < nr);
        M.elem[i]=elem[j];
    }
    return M;
}

//---------------------------------------------------------------------------

template<typename Integer>
Matrix<Integer> Matrix<Integer>::submatrix(const vector<bool>& rows) const{
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
}

//---------------------------------------------------------------------------

template<typename Integer>
Matrix<Integer>& Matrix<Integer>::remove_zero_rows() {
    size_t from = 0, to = 0; // maintain to <= from
    while (from < nr && v_is_zero(elem[from])) from++; //skip zero rows
    while (from < nr) {  // go over matrix
        // now from is a non-zero row
        if (to != from) elem[to].swap(elem[from]);
        ++to; ++from;
        while (from < nr && v_is_zero(elem[from])) from++; //skip zero rows
    }
    nr = to;
    elem.resize(nr);
    return *this;
}

//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::swap(Matrix<Integer>& x) {
    size_t tmp = nr; nr = x.nr; x.nr = tmp;
    tmp = nc; nc = x.nc; x.nc = tmp;
    elem.swap(x.elem);
}

//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::resize(size_t nr_rows, size_t nr_cols) {
    nc = nr_cols; //for adding new rows with the right length
    resize(nr_rows);
    resize_columns(nr_cols);
}

template<typename Integer>
void Matrix<Integer>::resize(size_t nr_rows) {
    if (nr_rows > elem.size()) {
        elem.resize(nr_rows, vector<Integer>(nc));
    }
    nr = nr_rows;
}

template<typename Integer>
void Matrix<Integer>::resize_columns(size_t nr_cols) {
    for (size_t i=0; i<nr; i++) {
        elem[i].resize(nr_cols);
    }
    nc = nr_cols;
}

//---------------------------------------------------------------------------

template<typename Integer>
vector<Integer> Matrix<Integer>::diagonal() const{
    assert(nr == nc); 
    vector<Integer> diag(nr);
    for(size_t i=0; i<nr;i++){
        diag[i]=elem[i][i];
    }
    return diag;
}

//---------------------------------------------------------------------------

template<typename Integer>
size_t Matrix<Integer>::maximal_decimal_length() const{
    size_t i,j,maxim=0;
    for (i = 0; i <nr; i++) {
        for (j = 0; j <nc; j++) {
            maxim=max(maxim,decimal_length(elem[i][j]));
        }
    }
    return maxim;
}

//---------------------------------------------------------------------------

template<typename Integer>
vector<size_t> Matrix<Integer>::maximal_decimal_length_columnwise() const{
    size_t i,j=0;
    vector<size_t> maxim(nc,0);
    for (i = 0; i <nr; i++) {
        for (j = 0; j <nc; j++) {
            maxim[j]=max(maxim[j],decimal_length(elem[i][j]));
        }
    }
    return maxim;
}

//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::append(const Matrix<Integer>& M) {
    assert (nc == M.nc);
    elem.reserve(nr+M.nr);
    for (size_t i=0; i<M.nr; i++) {
        elem.push_back(M.elem[i]);
    }
    nr += M.nr;
}

//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::append(const vector<vector<Integer> >& M) {
    if(M.size()==0)
        return;
    assert (nc == M[0].size());
    elem.reserve(nr+M.size());
    for (size_t i=0; i<M.size(); i++) {
        elem.push_back(M[i]);
    }
    nr += M.size();
}

//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::append(const vector<Integer>& V) {
    assert (nc == V.size());
    elem.push_back(V);
    nr++;
}

//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::append_column(const vector<Integer>& v) {
    assert (nr == v.size());
    for (size_t i=0; i<nr; i++) {
        elem[i].resize(nc+1);
        elem[i][nc] = v[i];
    }
    nc++;
}

//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::remove_row(const vector<Integer>& row) {
    size_t tmp_nr = nr;
    for (size_t i = 1; i <= tmp_nr; ++i) {
        if (elem[tmp_nr-i] == row) {
            elem.erase(elem.begin()+(tmp_nr-i));
            nr--;
        }
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::remove_duplicate_and_zero_rows() {
    bool remove_some = false;
    vector<bool> key(nr, true);

    set<vector<Integer> > SortedRows;
    SortedRows.insert( vector<Integer>(nc,0) );
    typename set<vector<Integer> >::iterator found;
    for (size_t i = 0; i<nr; i++) {
        found = SortedRows.find(elem[i]);
        if (found != SortedRows.end()) {
            key[i] = false;
            remove_some = true;
        }
        else
            SortedRows.insert(found,elem[i]);
    }

    if (remove_some) {
        *this = submatrix(key);
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
Matrix<Integer> Matrix<Integer>::add(const Matrix<Integer>& A) const{
    assert (nr == A.nr);
    assert (nc == A.nc);
    
    Matrix<Integer> B(nr,nc);
    size_t i,j;
    for(i=0; i<nr;i++){
        for(j=0; j<nc; j++){
            B.elem[i][j]=elem[i][j]+A.elem[i][j];
        }
    }
    return B;
}

//---------------------------------------------------------------------------

template<typename Integer>
Matrix<Integer> Matrix<Integer>::multiplication(const Matrix<Integer>& A) const{
    assert (nc == A.nr);

    Matrix<Integer> B(nr,A.nc,0);  //initialized with 0
    size_t i,j,k;
    for(i=0; i<B.nr;i++){
        for(j=0; j<B.nc; j++){
            for(k=0; k<nc; k++){
                B.elem[i][j]=B.elem[i][j]+elem[i][k]*A.elem[k][j];
            }
        }
    }
    return B;
}

//---------------------------------------------------------------------------

template<typename Integer>
Matrix<Integer> Matrix<Integer>::multiplication_cut(const Matrix<Integer>& A, const size_t& c) const{
    assert (nc == A.nr);
    assert(c<= A.nc);

    Matrix<Integer> B(nr,c,0);  //initialized with 0
    size_t i,j,k;
    for(i=0; i<B.nr;i++){
        for(j=0; j<c; j++){
            for(k=0; k<nc; k++){
                B.elem[i][j]=B.elem[i][j]+elem[i][k]*A.elem[k][j];
            }
        }
    }
    return B;
}


//---------------------------------------------------------------------------

template<typename Integer>
Matrix<Integer> Matrix<Integer>::multiplication(const Matrix<Integer>& A, long m) const{
    assert (nc == A.nr);

    Matrix<Integer> B(nr,A.nc,0);  //initialized with 0
    size_t i,j,k;
    for(i=0; i<B.nr;i++){
        for(j=0; j<B.nc; j++){
                for(k=0; k<nc; k++){
                B.elem[i][j]=(B.elem[i][j]+elem[i][k]*A.elem[k][j])%m;
                if (B.elem[i][j]<0) {
                    B.elem[i][j]=B.elem[i][j]+m;
                }
            }
        }
    }
    return B;
}

//---------------------------------------------------------------------------

template<typename Integer>
bool Matrix<Integer>::equal(const Matrix<Integer>& A) const{
    if ((nr!=A.nr)||(nc!=A.nc)){  return false; }
    size_t i,j;
    for (i=0; i < nr; i++) {
        for (j = 0; j < nc; j++) {
            if (elem[i][j]!=A.elem[i][j]) {
                return false;
            }
        }
    }
    return true;
}

//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------

template<typename Integer>
Matrix<Integer> Matrix<Integer>::transpose()const{
    Matrix<Integer> B(nc,nr);
    size_t i,j;
    for(i=0; i<nr;i++){
        for(j=0; j<nc; j++){
            B.elem[j][i]=elem[i][j];
        }
    }
    return B;
}

//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::scalar_multiplication(const Integer& scalar){
    size_t i,j;
    for(i=0; i<nr;i++){
        for(j=0; j<nc; j++){
            elem[i][j] *= scalar;
        }
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::scalar_division(const Integer& scalar){
    size_t i,j;
    assert(scalar != 0);
    for(i=0; i<nr;i++){
        for(j=0; j<nc; j++){
            assert (elem[i][j]%scalar == 0);
            elem[i][j] /= scalar;
        }
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::reduction_modulo(const Integer& modulo){
    size_t i,j;
    for(i=0; i<nr;i++){
        for(j=0; j<nc; j++){
            elem[i][j] %= modulo;
            if (elem[i][j] < 0) {
                elem[i][j] += modulo;
            }
        }
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
Integer Matrix<Integer>::matrix_gcd() const{
    Integer g=0,h;
    for (size_t i = 0; i <nr; i++) {
        h = v_gcd(elem[i]);
        g = libnormaliz::gcd<Integer>(g, h);
        if (g==1) return g;
    }
    return g;
}

//---------------------------------------------------------------------------

template<typename Integer>
vector<Integer> Matrix<Integer>::make_prime() {
    vector<Integer> g(nr);
    for (size_t i = 0; i <nr; i++) {
        g[i] = v_make_prime(elem[i]);
    }
    return g;
}

//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::make_cols_prime(size_t from_col, size_t to_col) {

    for (size_t k = from_col; k <= to_col; k++) {
        Integer g=0;
        for (size_t i = 0; i < nr; i++){
            g = libnormaliz::gcd(g,elem[i][k]);
            if (g==1) {
                break;
            }
        }
        for (size_t i = 0; i < nr; i++)
            elem[i][k]/=g;
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
Matrix<Integer> Matrix<Integer>::multiply_rows(const vector<Integer>& m) const{  //row i is multiplied by m[i]
  Matrix M = Matrix(nr,nc);
  size_t i,j;
  for (i = 0; i<nr; i++) {
     for (j = 0; j<nc; j++) {
        M.elem[i][j] = elem[i][j]*m[i];
     }
  }
  return M;
}

//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::MxV(vector<Integer>& result, const vector<Integer>& v) const{
    assert (nc == v.size());
    result.resize(nr);
    for(size_t i=0; i<nr;i++){
        result[i]=v_scalar_product(elem[i],v);
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
vector<Integer> Matrix<Integer>::MxV(const vector<Integer>& v) const{
    vector<Integer> w(nr);
    MxV(w, v);
    return w;
}

//---------------------------------------------------------------------------

template<typename Integer>
vector<Integer> Matrix<Integer>::VxM(const vector<Integer>& v) const{
    assert (nr == v.size());
    vector<Integer> w(nc,0);
    size_t i,j;
    for (i=0; i<nc; i++){
        for (j=0; j<nr; j++){
            w[i] += v[j]*elem[j][i];
        }
        if(!check_range(w[i]))
            break;
    }
    if(i==nc)  
        return w;
    Matrix<mpz_class> mpz_this(nr,nc);
    mat_to_mpz(*this,mpz_this);
    vector<mpz_class> mpz_v(nr);
    convert(mpz_v, v);
    vector<mpz_class> mpz_w=mpz_this.VxM(mpz_v);
    convert(w,mpz_w);
    return w;
}

//---------------------------------------------------------------------------

template<typename Integer>
vector<Integer> Matrix<Integer>::VxM_div(const vector<Integer>& v, const Integer& divisor, bool& success) const{
    assert (nr == v.size());
    vector<Integer> w(nc,0);
    success=true;
    size_t i,j;
    for (i=0; i<nc; i++){
        for (j=0; j<nr; j++){
            w[i] += v[j]*elem[j][i];
        }
        if(!check_range(w[i])){
            success=false;
            break;
        }
    }

    if(success)      
        v_scalar_division(w,divisor);  
        
    return w;
}

//---------------------------------------------------------------------------

template<typename Integer>
bool Matrix<Integer>::is_diagonal() const{

    for(size_t i=0;i<nr;++i)
        for(size_t j=0;j<nc;++j)
            if(i!=j && elem[i][j]!=0)
                return false;
    return true;
}

//---------------------------------------------------------------------------

template<typename Integer>
vector<long> Matrix<Integer>::pivot(size_t corner){
    assert(corner < nc);
    assert(corner < nr);
    size_t i,j;
    Integer help=0;
    vector<long> v(2,-1);

    for (i = corner; i < nr; i++) {
        for (j = corner; j < nc; j++) {
            if (elem[i][j]!=0) {
                if ((help==0)||(Iabs(elem[i][j])<help)) {
                    help=Iabs(elem[i][j]);
                    v[0]=i;
                    v[1]=j;
                    if (help == 1) return v;
                }
            }
        }
    }
    
    return v;
}

//---------------------------------------------------------------------------

template<typename Integer>
long Matrix<Integer>::pivot_column(size_t row,size_t col){
    assert(col < nc);
    assert(row < nr);
    size_t i;
    long j=-1;
    Integer help=0;

    for (i = row; i < nr; i++) {
        if (elem[i][col]!=0) {
            if ((help==0)||(Iabs(elem[i][col])<help)) {
                help=Iabs(elem[i][col]);
                j=i;
                if (help == 1) return j;
            }
        }
    }

    return j;
}

//---------------------------------------------------------------------------

template<typename Integer>
long Matrix<Integer>::pivot_column(size_t col){
    return pivot_column(col,col);
}

//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::exchange_rows(const size_t& row1, const size_t& row2){
    if (row1 == row2) return;
    assert(row1 < nr);
    assert(row2 < nr);
    elem[row1].swap(elem[row2]);
}

//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::exchange_columns(const size_t& col1, const size_t& col2){
    if (col1 == col2) return;
    assert(col1 < nc);
    assert(col2 < nc);
    for(size_t i=0; i<nr;i++){
        std::swap(elem[i][col1], elem[i][col2]);
    }
}

//---------------------------------------------------------------------------
 
template<typename Integer>
bool Matrix<Integer>::reduce_row (size_t row, size_t col) {
    assert(col < nc);
    assert(row < nr);
    size_t i,j;
    Integer help;
    for (i =row+1; i < nr; i++) {
        if (elem[i][col]!=0) {
            help=elem[i][col] / elem[row][col];
            for (j = col; j < nc; j++) {
                elem[i][j] -= help*elem[row][j];
                if (!check_range(elem[i][j]) ) {
                    return false;
                }
            }
            // v_el_trans<Integer>(elem[row],elem[i],-help,col);
        }
    }
    return true;
}

//---------------------------------------------------------------------------

template<typename Integer>
bool  Matrix<Integer>::reduce_row (size_t corner) {
    return reduce_row(corner,corner);
}

//---------------------------------------------------------------------------
 
template<typename Integer>
bool Matrix<Integer>::reduce_rows_upwards () {
// assumes that "this" is in row echelon form
// and reduces eevery column in which the rank jumps 
// by its lowest element
    
    if(nr==0)
        return true;

    for(size_t row=0;row<nr;++row){
        size_t col;
        for(col=0;col<nc;++col)
            if(elem[row][col]!=0)
                break;
        if(col==nc)
            continue;
        if(elem[row][col]<0)
            v_scalar_multiplication<Integer>(elem[row],-1);
        
        for(long i=row-1;i>=0;--i){
            Integer quot, rem;
            
            minimal_remainder(elem[i][col],elem[row][col],quot,rem);
            elem[i][col]=rem;
            for(size_t j=col+1;j<nc;++j){
                elem[i][j]-=quot* elem[row][j];
                if ( !check_range(elem[i][j]) ) {
                    return false;
                }
            }                                           
        }
    }
    return true;
}

//---------------------------------------------------------------------------
 
template<typename Integer>
bool Matrix<Integer>::linear_comb_columns(const size_t& col,const size_t& j,
            const Integer& u,const Integer& w,const Integer& v,const Integer& z){
                       
    for(size_t i=0;i<nr;++i){
        Integer rescue=elem[i][col];
        elem[i][col]=u*elem[i][col]+v*elem[i][j];
        elem[i][j]=w*rescue+z*elem[i][j];
        if ( (!check_range(elem[i][col])  || !check_range(elem[i][j]) )) {
            return false;
        }        
    }
    return true;
}

//---------------------------------------------------------------------------

template<typename Integer>
bool Matrix<Integer>::gcd_reduce_column (size_t corner, Matrix<Integer>& Right){
    assert(corner < nc);
    assert(corner < nr);
    Integer d,u,w,z,v;
    for(size_t j=corner+1;j<nc;++j){
       d=ext_gcd(elem[corner][corner],elem[corner][j],u,v);
       w=-elem[corner][j]/d;
       z=elem[corner][corner]/d;
       // Now we multiply the submatrix formed by columns "corner" and "j" 
       // and rows corner,...,nr from the right by the 2x2 matrix
       // | u w |
       // | v z |              
       if(!linear_comb_columns(corner,j,u,w,v,z))
           return false; 
       if(!Right.linear_comb_columns(corner,j,u,w,v,z))
           return false;  
    }   
    return true;
}


//---------------------------------------------------------------------------

template<typename Integer>
bool Matrix<Integer>::column_trigonalize(size_t rk, Matrix<Integer>& Right) { 
    assert(Right.nr == nc);
    assert(Right.nc == nc);
    vector<long> piv(2,0);       
    for(size_t j=0;j<rk;++j){
            piv=pivot(j);
            assert(piv[0]>=0); // protect against wrong rank
            exchange_rows (j,piv[0]);
            exchange_columns (j,piv[1]);
            Right.exchange_columns(j,piv[1]);
            if(!gcd_reduce_column(j, Right))
                return false;
    }
    return true;
}

//---------------------------------------------------------------------------

template<typename Integer>
Integer Matrix<Integer>::compute_vol(bool& success){
        
    assert(nr<=nc);
    
    Integer det=1;
    for(size_t i=0;i<nr;++i){
        det*=elem[i][i]; 
        if(!check_range(det)){
            success=false;
            return 0;
        }
    }
            
    det=Iabs(det);
    success=true;
    return det;
}

//---------------------------------------------------------------------------

template<typename Integer>
size_t Matrix<Integer>::row_echelon_inner_elem(bool& success){

    size_t pc=0;
    long piv=0, rk=0;
    success=true;

    if(nr==0)
        return 0;
    
    for (rk = 0; rk < (long) nr; rk++){
        for(;pc<nc;pc++){
            piv=pivot_column(rk,pc);
            if(piv>=0)
                break;
        }
        if(pc==nc)
            break;
        do{
            exchange_rows (rk,piv);
            if(!reduce_row(rk,pc)){
                success=false;
                return rk;
            }
            piv=pivot_column(rk,pc);
        }while (piv>rk);
    }
                
    return rk;
}

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
            piv=pivot_column(rk,pc);
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

template<typename Integer>
size_t Matrix<Integer>::row_echelon_reduce(bool& success){

    size_t rk=row_echelon_inner_elem(success);
    if(success)
        success=reduce_rows_upwards();
    return rk;
}

//---------------------------------------------------------------------------

template<typename Integer>
Integer Matrix<Integer>::full_rank_index(bool& success){

    size_t rk=row_echelon_inner_elem(success);
    Integer index=1;
    if(success){
        for(size_t i=0;i<rk;++i){
            index*=elem[i][i];
            if(!check_range(index)){
                success=false;
                index=0;
                return index;
            }
        }
    }
    assert(rk==nc); // must have full rank
    index=Iabs(index);
    return index;
}
//---------------------------------------------------------------------------

template<typename Integer>
Matrix<Integer> Matrix<Integer>::row_column_trigonalize(size_t& rk, bool& success) {

    Matrix<Integer> Right(nc);
    rk=row_echelon_reduce(success);
    if(success)
        success=column_trigonalize(rk,Right); 
    return Right; 
} 

//---------------------------------------------------------------------------

template<typename Integer>
size_t Matrix<Integer>::row_echelon(bool& success, bool do_compute_vol, Integer& det){
    
/*    if(using_GMP<Integer>()){
        return row_echelon_inner_bareiss(success,det);;
    }
    else{ */
        size_t rk=row_echelon_inner_elem(success);
        if(do_compute_vol)
            det=compute_vol(success);
        return rk;
//    }
}

//---------------------------------------------------------------------------

template<typename Integer>
size_t Matrix<Integer>::row_echelon(bool& success){
    
    Integer dummy;
    return row_echelon(success,false,dummy);
}

//---------------------------------------------------------------------------

template<typename Integer>
size_t Matrix<Integer>::row_echelon(bool& success, Integer& det){
    
    return row_echelon(success,true,det);
}



//---------------------------------------------------------------------------

template<typename Integer>
size_t Matrix<Integer>::rank_submatrix(const Matrix<Integer>& mother, const vector<key_t>& key){

    assert(nc>=mother.nc);
    if(nr<key.size()){
        elem.resize(key.size(),vector<Integer>(nc,0));
        nr=key.size();    
    }
    size_t save_nr=nr;
    size_t save_nc=nc;
    nr=key.size();
    nc=mother.nc;

    select_submatrix(mother,key);

    bool success;
    size_t rk=row_echelon(success);
    
    if(!success){        
        Matrix<mpz_class> mpz_this(nr,nc);
        mpz_submatrix(mpz_this,mother,key);
        rk=mpz_this.row_echelon(success);
    }
    
    nr=save_nr;
    nc=save_nc;
    return rk;                               
}

//---------------------------------------------------------------------------

template<typename Integer>
size_t Matrix<Integer>::rank_submatrix(const vector<key_t>& key) const{

    Matrix<Integer> work(key.size(),nc);
    return work.rank_submatrix(*this,key);              
}

//---------------------------------------------------------------------------

template<typename Integer>
size_t Matrix<Integer>::rank() const{
    vector<key_t> key(nr);
    for(size_t i=0;i<nr;++i)
        key[i]=i;
    return rank_submatrix(key);
}

//---------------------------------------------------------------------------

template<typename Integer>
Integer Matrix<Integer>::vol_submatrix(const Matrix<Integer>& mother, const vector<key_t>& key){

    assert(nc>=mother.nc);
    if(nr<key.size()){
        elem.resize(key.size(),vector<Integer>(nc,0));
        nr=key.size();    
    }
    size_t save_nr=nr;
    size_t save_nc=nc;
    nr=key.size();
    nc=mother.nc;
    
    select_submatrix(mother,key);

    bool success;
    Integer det;
    row_echelon(success,det);
    
    if(!success){        
        Matrix<mpz_class> mpz_this(nr,nc);
        mpz_submatrix(mpz_this,mother,key);
        mpz_class mpz_det;
        mpz_this.row_echelon(success,mpz_det);
        convert(det, mpz_det);
    }
    
    nr=save_nr;
    nc=save_nc;
    return det;                               
}
//---------------------------------------------------------------------------

template<typename Integer>
Integer Matrix<Integer>::vol_submatrix(const vector<key_t>& key) const{

    Matrix<Integer> work(key.size(),nc);
    return work.vol_submatrix(*this,key);              
}

//---------------------------------------------------------------------------

template<typename Integer>
Integer Matrix<Integer>::vol() const{
    vector<key_t> key(nr);
    for(size_t i=0;i<nr;++i)
        key[i]=i;
    return vol_submatrix(key);
}

//---------------------------------------------------------------------------

template<typename Integer>
vector<key_t>  Matrix<Integer>::max_rank_submatrix_lex_inner(bool& success) const{

    success=true;
    size_t max_rank=min(nr,nc);
    Matrix<Integer> Test(max_rank,nc);
    Test.nr=0;
    vector<key_t> col;
    col.reserve(max_rank);
    vector<key_t> key;
    key.reserve(max_rank);
    size_t rk=0;
    
    vector<vector<bool> > col_done(max_rank,vector<bool>(nc,false));
    
    vector<Integer> Test_vec(nc);
     
    for(size_t i=0;i<nr;++i){    
        Test_vec=elem[i];            
        for(size_t k=0;k<rk;++k){
            if(Test_vec[col[k]]==0)
                continue;
            Integer a=Test[k][col[k]];
            Integer b=Test_vec[col[k]];
            for(size_t j=0;j<nc;++j)
                if(!col_done[k][j]){
                Test_vec[j]=a*Test_vec[j]-b*Test[k][j];
                if (!check_range(Test_vec[j]) ) {
                    success=false;
                    return key;
                }
            }
        }
        
        size_t j=0;
        for(;j<nc;++j)
            if(Test_vec[j]!=0)
                break;
        if(j==nc)     // Test_vec=0
            continue;
            
        col.push_back(j);
        key.push_back(i);
        
        if(rk>0){
            col_done[rk]=col_done[rk-1];
            col_done[rk][col[rk-1]]=true;
        }

        Test.nr++;
        rk++;
        v_make_prime(Test_vec);
        Test[rk-1]=Test_vec;
            
        if(rk==max_rank)
            break;   
    }    
    return key;                
}

//---------------------------------------------------------------------------

template<typename Integer>
vector<key_t>  Matrix<Integer>::max_rank_submatrix_lex() const{
    bool success;
    vector<key_t> key=max_rank_submatrix_lex_inner(success);
    if(!success){
        Matrix<mpz_class> mpz_this(nr,nc);
        mat_to_mpz(*this,mpz_this);
        key=mpz_this.max_rank_submatrix_lex_inner(success);    
    }
    return key;
}

//---------------------------------------------------------------------------

template<typename Integer>
bool Matrix<Integer>::solve_destructive_inner(bool ZZinvertible,Integer& denom) {

    assert(nc>=nr);
    size_t dim=nr;
    bool success;
    
    size_t rk;
    
    if(ZZinvertible){
        rk=row_echelon_inner_elem(success); 
        if(!success)
            return false;        
        assert(rk==nr);
        denom=compute_vol(success);
    }
    else{
        rk=row_echelon(success,denom);
        if(!success)
            return false;    
    }

    if (denom==0) { 
        if(using_GMP<Integer>()){
            errorOutput() << "Cannot solve system (denom=0)!" << endl;
            throw ArithmeticException();
        }
        else
            return false;            
    }

    Integer S;
    size_t i;
    long j;
    size_t k;
    for (i = nr; i < nc; i++) {
        for (j = dim-1; j >= 0; j--) {
            S=denom*elem[j][i];
            for (k = j+1; k < dim; k++) {
                S-=elem[j][k]*elem[k][i];
            }
            if(!check_range(S))
                return false;
            elem[j][i]=S/elem[j][j];
        }
    }
    return true;
}

//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::customize_solution(size_t dim, Integer& denom, size_t red_col, 
                     size_t sign_col, bool make_sol_prime) {
                         
    assert(!(make_sol_prime && (sign_col>0 || red_col>0)));

    for(size_t j=0;j<red_col;++j){  // reduce first red_col columns of solution mod denom
        for(size_t k=0;k<dim;++k){
          elem[k][dim+j]%=denom;
          if(elem[k][dim+j]<0)
              elem[k][dim+j]+=Iabs(denom);
       }
    }
        
    for(size_t j=0;j<sign_col;++j)   // replace entries in the next sign_col columns by their signs
      for(size_t k=0;k<dim;++k){
        if(elem[k][dim+red_col+j]>0){
            elem[k][dim+red_col+j]=1;
            continue;
        } 
        if(elem[k][dim+red_col+j]<0){
            elem[k][dim+red_col+j]=-1;
            continue;
        }       
      }
      
    if(make_sol_prime) // make columns of solution coprime if wanted
        make_cols_prime(dim,nc-1);
}

//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::solve_system_submatrix_outer(const Matrix<Integer>& mother, const vector<key_t>& key, const vector<vector<Integer>* >& RS,
        Integer& denom, bool ZZ_invertible, bool transpose, size_t red_col, size_t sign_col, 
        bool compute_denom, bool make_sol_prime) {
     
    size_t dim=mother.nc;
    assert(key.size()==dim);
    assert(nr==dim);
    assert(dim+RS.size()<=nc);
    size_t save_nc=nc;
    nc=dim+RS.size();
    
    if(transpose)
       select_submatrix_trans(mother,key);           
    else
       select_submatrix(mother,key);
               
    for(size_t i=0;i<dim;++i)
       for(size_t k=0;k<RS.size();++k)
           elem[i][k+dim]= (*RS[k])[i];
    
    if(solve_destructive_inner(ZZ_invertible,denom)){
        customize_solution(dim, denom,red_col,sign_col,make_sol_prime);        
    }
    else{          
       #pragma omp atomic
       GMP_mat++;
    
       Matrix<mpz_class> mpz_this(nr,nc);
       mpz_class mpz_denom;
       if(transpose)
           mpz_submatrix_trans(mpz_this,mother,key);
       else            
           mpz_submatrix(mpz_this,mother,key);
           
       for(size_t i=0;i<dim;++i)
           for(size_t k=0;k<RS.size();++k)
               convert(mpz_this[i][k+dim], (*RS[k])[i]);
       mpz_this.solve_destructive_inner(ZZ_invertible,mpz_denom);
       mpz_this.customize_solution(dim, mpz_denom,red_col,sign_col,make_sol_prime);           
          
       for(size_t i=0;i<dim;++i)  // replace left side by 0, except diagonal if ZZ_invetible
          for(size_t j=0;j<dim;++j){
            if(i!=j || !ZZ_invertible)
                mpz_this[i][j]=0;              
          }
              
       mat_to_Int(mpz_this,*this);
       if(compute_denom)
           convert(denom, mpz_denom);                
    }    
    nc=save_nc;         
}

//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::solve_system_submatrix(const Matrix<Integer>& mother, const vector<key_t>& key, const vector<vector<Integer>* >& RS,
         vector< Integer >& diagonal, Integer& denom, size_t red_col, size_t sign_col) {

    solve_system_submatrix_outer(mother,key,RS,denom,true,false,red_col,sign_col);
    assert(diagonal.size()==nr);
    for(size_t i=0;i<nr;++i)
        diagonal[i]=elem[i][i];
                 
}


//---------------------------------------------------------------------------
// the same without diagonal
template<typename Integer>
void Matrix<Integer>::solve_system_submatrix(const Matrix<Integer>& mother, const vector<key_t>& key, const vector<vector<Integer>* >& RS,
         Integer& denom, size_t red_col, size_t sign_col, bool compute_denom, bool make_sol_prime) {

    solve_system_submatrix_outer(mother,key,RS,denom,false,false,red_col,sign_col, 
                compute_denom, make_sol_prime);
}

//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::solve_system_submatrix_trans(const Matrix<Integer>& mother, const vector<key_t>& key, const vector<vector<Integer>* >& RS,
         Integer& denom, size_t red_col, size_t sign_col) {
         
    solve_system_submatrix_outer(mother,key,RS,denom,false,true,red_col,sign_col);
}

//---------------------------------------------------------------------------

template<typename Integer>
Matrix<Integer> Matrix<Integer>::extract_solution() const {
    assert(nc>=nr);
    Matrix<Integer> Solution(nr,nc-nr); 
    for(size_t i=0;i<nr;++i){
        for(size_t j=0;j<Solution.nc;++j)
            Solution[i][j]=elem[i][j+nr];    
    }
    return Solution;  
}

//---------------------------------------------------------------------------

template<typename Integer>
vector<vector<Integer>* > Matrix<Integer>::row_pointers(){

    vector<vector<Integer>* > pointers(nr);
    for(size_t i=0;i<nr;++i)
        pointers[i]=&(elem[i]);
    return pointers;
}

//---------------------------------------------------------------------------

template<typename Integer>
vector<vector<Integer>* > Matrix<Integer>::submatrix_pointers(const vector<key_t>& key){

    vector<vector<Integer>* > pointers(key.size());
    for(size_t i=0;i<key.size();++i)
        pointers[i]=&(elem[key[i]]);
    return pointers;
}
//---------------------------------------------------------------------------

template<typename Integer>
Matrix<Integer> Matrix<Integer>::solve(const Matrix<Integer>& Right_side,vector<Integer>& diagonal,Integer& denom) const {

    Matrix<Integer> M(nr,nc+Right_side.nc);
    vector<key_t> key=identity_key(nr);
    Matrix<Integer> RS_trans=Right_side.transpose();
    vector<vector<Integer>* > RS=RS_trans.row_pointers();
    M.solve_system_submatrix(*this,key,RS,diagonal,denom,0,0);
    return M.extract_solution(); 
}

//---------------------------------------------------------------------------

template<typename Integer>
Matrix<Integer> Matrix<Integer>::solve(const Matrix<Integer>& Right_side, Integer& denom) const {

    Matrix<Integer> M(nr,nc+Right_side.nc);
    vector<key_t> key=identity_key(nr);
    Matrix<Integer> RS_trans=Right_side.transpose();
    vector<vector<Integer>* > RS=RS_trans.row_pointers();
    M.solve_system_submatrix(*this,key,RS,denom,0,0);
    return M.extract_solution(); 
}

//---------------------------------------------------------------------------

template<typename Integer>
Matrix<Integer> Matrix<Integer>::invert(Integer& denom) const{
    assert(nr == nc);
    Matrix<Integer> Right_side(nr);

    return solve(Right_side,denom);
}

//---------------------------------------------------------------------------

template<typename Integer>
Matrix<Integer> Matrix<Integer>::bundle_matrices(const Matrix<Integer>& Right_side) const {

    assert(nr == nc);
    assert(nc == Right_side.nr);
    Matrix<Integer> M(nr,nc+Right_side.nc);
    for(size_t i=0;i<nr;++i){
        for(size_t j=0;j<nc;++j)
            M[i][j]=elem[i][j];
        for(size_t j=nc;j<M.nc;++j)
            M[i][j]=Right_side[i][j-nc];
    }
    return M;
}
//---------------------------------------------------------------------------

template<typename Integer>
Matrix<Integer> Matrix<Integer>::invert_unprotected(Integer& denom, bool& success) const{
    assert(nr == nc);
    Matrix<Integer> Right_side(nr);
    Matrix<Integer> M=bundle_matrices(Right_side);
    success=M.solve_destructive_inner(false,denom);
    return M.extract_solution();;
}

//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::invert_submatrix(const vector<key_t>& key, Integer& denom, Matrix<Integer>& Inv, bool compute_denom, bool make_sol_prime) const{
    assert(key.size() == nc);
    Matrix<Integer> unit_mat(key.size());
    Matrix<Integer> M(key.size(),2*key.size());        
    vector<vector<Integer>* > RS_pointers=unit_mat.row_pointers();
    M.solve_system_submatrix(*this,key,RS_pointers,denom,0,0, compute_denom, make_sol_prime);
    Inv=M.extract_solution();;
}

//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::simplex_data(const vector<key_t>& key, Matrix<Integer>& Supp, Integer& vol, bool compute_vol) const{
    assert(key.size() == nc);
    invert_submatrix(key,vol,Supp,compute_vol,true);
    Supp=Supp.transpose();
    // Supp.make_prime(); now done internally
}
//---------------------------------------------------------------------------

template<typename Integer>
vector<Integer> Matrix<Integer>::solve_rectangular(const vector<Integer>& v, Integer& denom) const {
    if (nc == 0 || nr == 0) { //return zero-vector as solution
        return vector<Integer>(nc,0);
    }
    size_t i;
    vector<key_t>  rows=max_rank_submatrix_lex();
    Matrix<Integer> Left_Side=submatrix(rows);
    assert(nc == Left_Side.nr); //otherwise input hadn't full rank //TODO 
    Matrix<Integer> Right_Side(v.size(),1);
    Right_Side.write_column(0,v);
    Right_Side = Right_Side.submatrix(rows);
    Matrix<Integer> Solution=Left_Side.solve(Right_Side, denom);
    vector<Integer> Linear_Form(nc);
    for (i = 0; i <nc; i++) {
        Linear_Form[i] = Solution[i][0];  // the solution vector is called Linear_Form
    }
    vector<Integer> test = MxV(Linear_Form); // we have solved the system by taking a square submatrix
                        // now we must test whether the solution satisfies the full system
    for (i = 0; i <nr; i++) {
        if (test[i] != denom * v[i]){
            return vector<Integer>();
        }
    }
    Integer total_gcd = libnormaliz::gcd(denom,v_gcd(Linear_Form)); // extract the gcd of denom and solution
    denom/=total_gcd;
    v_scalar_division(Linear_Form,total_gcd);
    return Linear_Form;
}
//---------------------------------------------------------------------------

template<typename Integer>
vector<Integer> Matrix<Integer>::solve_ZZ(const vector<Integer>& v) const {

    Integer denom;
    vector<Integer> result=solve_rectangular(v,denom);
    if(denom!=1)
        result.clear();
    return result;
}
//---------------------------------------------------------------------------

template<typename Integer>
vector<Integer> Matrix<Integer>::find_linear_form() const {

    Integer denom;
    vector<Integer> result=solve_rectangular(vector<Integer>(nr,1),denom);
    v_make_prime(result);
    return result;
}

//---------------------------------------------------------------------------

template<typename Integer>
vector<Integer> Matrix<Integer>::find_linear_form_low_dim () const{
    size_t rank=(*this).rank();
    if (rank == 0) { //return zero-vector as linear form
        return vector<Integer>(nc,0);
    }
    if (rank == nc) { // basis change not necessary
        return (*this).find_linear_form();
    }

    Sublattice_Representation<Integer> Basis_Change(*this,true);
    vector<Integer> Linear_Form=Basis_Change.to_sublattice(*this).find_linear_form();
    if(Linear_Form.size()!=0)
        Linear_Form=Basis_Change.from_sublattice_dual(Linear_Form);

    return Linear_Form;
}

//---------------------------------------------------------------------------

template<typename Integer>
size_t Matrix<Integer>::row_echelon_reduce(){

    size_t rk;
    Matrix<Integer> Copy(*this);
    bool success;
    rk=row_echelon_reduce(success);
    if(success){
        Shrink_nr_rows(rk);
        return rk;
    }
    Matrix<mpz_class> mpz_Copy(nr,nc);
    mat_to_mpz(Copy,mpz_Copy);
    rk=mpz_Copy.row_echelon_reduce(success);
    mat_to_Int(mpz_Copy,*this);
    Shrink_nr_rows(rk); 
    return rk;
}
//---------------------------------------------------------------------------

template<typename Integer>
Integer Matrix<Integer>::full_rank_index() const{
    
    Matrix<Integer> Copy(*this);
    Integer index;
    bool success;
    index=Copy.full_rank_index(success);
    if(success)
        return index;
    Matrix<mpz_class> mpz_Copy(nr,nc);
    mat_to_mpz(*this,mpz_Copy);
    mpz_class mpz_index=mpz_Copy.full_rank_index(success);
    convert(index, mpz_index);
    return index;
}

//---------------------------------------------------------------------------

template<typename Integer>
size_t Matrix<Integer>::row_echelon(){
    
    Matrix<Integer> Copy(*this);
    bool success;
    size_t rk;
    rk=row_echelon(success);
    if(success){
        Shrink_nr_rows(rk);
        return rk;
    }
    Matrix<mpz_class> mpz_Copy(nr,nc);
    mat_to_mpz(Copy,mpz_Copy);
    rk=mpz_Copy.row_echelon_reduce(success); // reduce to make entries small
    mat_to_Int(mpz_Copy,*this);
    Shrink_nr_rows(rk);  
    return rk;
}

//---------------------------------------------------------------------------

template<typename Integer>
Matrix<Integer> Matrix<Integer>::kernel () const{
// computes a ZZ-basis of the solutions of (*this)x=0
// the basis is formed by the rOWS of the returned matrix

    size_t dim=nc;
    if(nr==0)
        return(Matrix<Integer>(dim));

    Matrix<Integer> Copy(*this);
    size_t rank;
    bool success;
    Matrix<Integer> Transf=Copy.row_column_trigonalize(rank,success);
    if(!success){
        Matrix<mpz_class> mpz_Copy(nr,nc);
        mat_to_mpz(*this,mpz_Copy);
        Matrix<mpz_class> mpz_Transf=mpz_Copy.row_column_trigonalize(rank,success);
        mat_to_Int(mpz_Transf,Transf);    
    }
    
    Matrix<Integer> ker_basis(dim-rank,dim);
    Matrix<Integer> Help =Transf.transpose();
    for (size_t i = rank; i < dim; i++) 
            ker_basis[i-rank]=Help[i];
    ker_basis.row_echelon_reduce();
    return(ker_basis);
}

//---------------------------------------------------------------------------
// Converts "this" into (column almost) Hermite normal form, returns column transformation matrix
template<typename Integer>
Matrix<Integer> Matrix<Integer>::AlmostHermite(size_t& rk){

    Matrix<Integer> Copy=*this;
    Matrix<Integer> Transf;
    bool success;
    Transf=row_column_trigonalize(rk,success);
    if(success)
        return Transf;
    
    Matrix<mpz_class> mpz_this(nr,nc);
    mat_to_mpz(Copy,mpz_this);
    Matrix<mpz_class> mpz_Transf=mpz_this.row_column_trigonalize(rk,success);
    mat_to_Int(mpz_this,*this);
    mat_to_Int(mpz_Transf,Transf);
    return Transf;
}

//---------------------------------------------------------------------------

template<typename Integer>
bool Matrix<Integer>::SmithNormalForm_inner(size_t& rk, Matrix<Integer>& Right){

    bool success=true;
    
    // first we diagonalize

    while(true){
        rk=row_echelon_reduce(success);
        if(!success)
            return false;
        if(rk==0)
            break;
        
        if(is_diagonal())
            break;
        
        success=column_trigonalize(rk,Right);
        if(!success)
            return false;
        
        if(is_diagonal())
            break;                                
    }
    
    // now we change the diagonal so that we have successive divisibilty
    
    if(rk<=1)
        return true;
           
    while(true){
        size_t i=0;
        for(;i<rk-1;++i)
            if(elem[i+1][i+1]%elem[i][i]!=0)
                break;
        if(i==rk-1)
            break;
        
        Integer u,v,w,z, d=ext_gcd(elem[i][i],elem[i+1][i+1],u,v);
        elem[i][i+1]=elem[i+1][i+1];
        w=-elem[i+1][i+1]/d;
        z=elem[i][i]/d;
        // Now we multiply the submatrix formed by columns "corner" and "j" 
        // and rows corner,...,nr from the right by the 2x2 matrix
        // | u w |
        // | v z |              
       if(!linear_comb_columns(i,i+1,u,w,v,z))
           return false; 
       if(!Right.linear_comb_columns(i,i+1,u,w,v,z))
           return false;
       elem[i+1][i]=0;        
     }
     
    return true;
}

// Converts "this" into Smith normal form, returns column transformation matrix
template<typename Integer>
Matrix<Integer> Matrix<Integer>::SmithNormalForm(size_t& rk){

    size_t dim=nc;
    Matrix<Integer> Transf(dim);
    if(dim==0)
        return Transf;
        
    Matrix<Integer> Copy=*this;
    bool success=SmithNormalForm_inner(rk,Transf);
    if(success)
        return Transf;
    
    Matrix<mpz_class> mpz_this(nr,dim);
    mat_to_mpz(Copy,mpz_this);
    Matrix<mpz_class> mpz_Transf(dim);
    mpz_this.SmithNormalForm_inner(rk,mpz_Transf);
    mat_to_Int(mpz_this,*this);
    mat_to_Int(mpz_Transf,Transf);
    return Transf;
}

//---------------------------------------------------------------------------
// Classless conversion routines
//---------------------------------------------------------------------------

template<typename ToType, typename FromType>
void convert(Matrix<ToType>& to_mat, const Matrix<FromType>& from_mat){
    size_t nrows = from_mat.nr_of_rows();
    size_t ncols = from_mat.nr_of_columns();
    to_mat.resize(nrows, ncols);
    for(size_t i=0; i<nrows; ++i)
        for(size_t j=0; j<ncols; ++j)
            convert(to_mat[i][j], from_mat[i][j]);
}

//---------------------------------------------------------------------------


template<typename Integer>
void mat_to_mpz(const Matrix<Integer>& mat, Matrix<mpz_class>& mpz_mat){
    //convert(mpz_mat, mat);
    // we allow the matrices to have different sizes
    size_t nrows = min(mat.nr_of_rows(),   mpz_mat.nr_of_rows());
    size_t ncols = min(mat.nr_of_columns(),mpz_mat.nr_of_columns());
    for(size_t i=0; i<nrows; ++i)
        for(size_t j=0; j<ncols; ++j)
            convert(mpz_mat[i][j], mat[i][j]);
	#pragma omp atomic
	GMP_mat++;
}

//---------------------------------------------------------------------------

template<typename Integer>
void mat_to_Int(const Matrix<mpz_class>& mpz_mat, Matrix<Integer>& mat){
    //convert(mat, mpz_mat);
    // we allow the matrices to have different sizes
    size_t nrows = min(mpz_mat.nr_of_rows(),   mat.nr_of_rows());
    size_t ncols = min(mpz_mat.nr_of_columns(),mat.nr_of_columns());
    for(size_t i=0; i<nrows; ++i)
        for(size_t j=0; j<ncols; ++j)
            convert(mat[i][j], mpz_mat[i][j]);
}

//---------------------------------------------------------------------------

template<typename Integer>
void mpz_submatrix(Matrix<mpz_class>& sub, const Matrix<Integer>& mother, const vector<key_t>& selection){

    assert(sub.nr_of_columns()>=mother.nr_of_columns());
    assert(sub.nr_of_rows()>=selection.size());
    for(size_t i=0;i<selection.size();++i)
        for(size_t j=0;j<mother.nr_of_columns();++j)
            convert(sub[i][j], mother[selection[i]][j]);
}

//---------------------------------------------------------------------------

template<typename Integer>
void mpz_submatrix_trans(Matrix<mpz_class>& sub, const Matrix<Integer>& mother, const vector<key_t>& selection){

    assert(sub.nr_of_columns()>=selection.size());
    assert(sub.nr_of_rows()>=mother.nr_of_columns());
    for(size_t i=0;i<selection.size();++i)
        for(size_t j=0;j<mother.nr_of_columns();++j)
            convert(sub[j][i], mother[selection[i]][j]);
}

//---------------------------------------------------------------------------

/* sorts rows of a matrix by a degree function and returns the permuation
* does not change matrix (yet)
 */
template<typename Integer>
vector<key_t> Matrix<Integer>::perm_sort_by_degree(const vector<key_t>& key, const vector<Integer>& grading, bool computed) const{

	list<vector<Integer>> rowList;
	vector<Integer> v;

	v.resize(nc+2);
	unsigned long i,j;
	
	for (i=0;i<key.size();i++){
		if (computed){
		v[0]=v_scalar_product((*this).elem[key[i]],grading);
		} else{
			v[0]=0;
			for (j=0;j<nc;j++) v[0]+=Iabs((*this).elem[key[i]][j]);
		}
		for (j=0;j<nc;j++){
			v[j+1] = (*this).elem[key[i]][j];
		}
		v[nc+1] = key[i]; // position of row
		rowList.push_back(v);
	}
	rowList.sort();
	vector<key_t> perm;
	perm.resize(key.size());
	i=0;
	for (typename list< vector<Integer> >::const_iterator it = rowList.begin();it!=rowList.end();++it){
		perm[i]=convertTo<long>((*it)[nc+1]);
		i++;
	}
	return perm;
}

//---------------------------------------------------------------------------


template<typename Integer>
bool weight_lex(const order_helper<Integer>& a, const order_helper<Integer>& b){
    
        if(a.weight < b.weight)
            return true;
        if(a.weight==b.weight)
            if(*(a.v)< *(b.v))
                return true;
        return false;
}

//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::order_rows_by_perm(const vector<key_t>& perm){
    order_by_perm(elem,perm);    
}

template<typename Integer>
Matrix<Integer>& Matrix<Integer>::sort_by_weights(const Matrix<Integer>& Weights, vector<bool> absolute){
    if(nr<=1)
        return *this;
    vector<key_t> perm=perm_by_weights(Weights,absolute);
    order_by_perm(elem,perm);
    return *this;   
}

template<typename Integer>
Matrix<Integer>& Matrix<Integer>::sort_lex(){
    if(nr<=1)
        return *this;
    vector<key_t> perm=perm_by_weights(Matrix<Integer>(0,nc),vector<bool>(0));
    order_by_perm(elem,perm);
    return *this;    
}

template<typename Integer>
vector<key_t> Matrix<Integer>::perm_by_weights(const Matrix<Integer>& Weights, vector<bool> absolute){
// the smallest entry is the row with index perm[0], then perm[1] etc.
    
    assert(Weights.nc==nc);
    assert(absolute.size()==Weights.nr);

    list<order_helper<Integer> > order;
    order_helper<Integer> entry;
    entry.weight.resize(Weights.nr);
    
    for(key_t i=0;i<nr; ++i){
        for(size_t j=0;j<Weights.nr;++j){
            if(absolute[j])
                entry.weight[j]=v_scalar_product(Weights[j],v_abs_value(elem[i]));
            else
                entry.weight[j]=v_scalar_product(Weights[j],elem[i]);                
        }
        entry.index=i;
        entry.v=&(elem[i]);
        order.push_back(entry);        
    }
    order.sort(weight_lex<Integer>);
    vector<key_t> perm(nr);
    typename list<order_helper<Integer> >::const_iterator ord=order.begin();
    for(key_t i=0;i<nr;++i, ++ord)
        perm[i]=ord->index; 
    
    return perm;
}

//---------------------------------------------------

template<typename Integer>
Matrix<Integer> Matrix<Integer>::solve_congruences(bool& zero_modulus) const{
 
    
    zero_modulus=false;
    size_t i,j;
    size_t nr_cong=nr, dim=nc-1;
    if(nr_cong==0)
        return Matrix<Integer>(dim); // give back unit matrix
    
    //add slack variables to convert congruences into equaitions
    Matrix<Integer> Cong_Slack(nr_cong, dim+nr_cong);
    for (i = 0; i < nr_cong; i++) {
        for (j = 0; j < dim; j++) {
            Cong_Slack[i][j]=elem[i][j];
        }
        Cong_Slack[i][dim+i]=elem[i][dim];
        if(elem[i][dim]==0){
            zero_modulus=true;
            return Matrix<Integer>(0,dim);
        }
    }
    
    //compute kernel
    
    Matrix<Integer> Help=Cong_Slack.kernel(); // gives the solutions to the the system with slack variables
    Matrix<Integer> Ker_Basis(dim,dim);   // must now project to first dim coordinates to get rid of them
    for(size_t i=0;i<dim;++i)
        for(size_t j=0;j<dim;++j)
            Ker_Basis[i][j]=Help[i][j];
    return Ker_Basis;
        
}

//---------------------------------------------------

template<typename Integer>
void Matrix<Integer>::saturate(){
    
    *this=kernel().kernel();    
}

//---------------------------------------------------

template<typename Integer>
vector<key_t> Matrix<Integer>::max_and_min(const vector<Integer>& L, const vector<Integer>& norm) const{

    vector<key_t> result(2,0);
    if(nr==0)
        return result;
    key_t maxind=0,minind=0;
    Integer maxval=v_scalar_product(L,elem[0]);
    Integer maxnorm=1,minnorm=1;
    if(norm.size()>0){
        maxnorm=v_scalar_product(norm,elem[0]);
        minnorm=maxnorm;              
    }
    Integer minval=maxval;
    for(key_t i=0;i<nr;++i){
        Integer val=v_scalar_product(L,elem[i]);
        if(norm.size()==0){
            if(val>maxval){
                maxind=i;
                maxval=val;            
            }
            if(val<minval){
                minind=i;
                minval=val;            
            }
        }
        else{
            Integer nm=v_scalar_product(norm,elem[i]);
            if(maxnorm*val>nm*maxval){
                maxind=i;
                maxval=val;            
            }
            if(minnorm*val<nm*minval){
                minind=i;
                minval=val;            
            }
        }            
    }
    result[0]=maxind;
    result[1]=minind;
    return result;
}

template<typename Integer>
size_t Matrix<Integer>::extreme_points_first(const vector<Integer> norm){
    
    if(nr==0)
        return 1;
    
    vector<long long> norm_copy;
    
    size_t nr_extr=0;
    Matrix<long long> HelpMat(nr,nc);
    try{
        convert(HelpMat,*this);
        convert(norm_copy,norm);
    }
    catch(ArithmeticException){
        return nr_extr;        
    }

    HelpMat.sort_lex();
    
    vector<bool> marked(nr,false);
    size_t no_success=0;
    // size_t nr_attempt=0;
    while(true){
        // nr_attempt++; cout << nr_attempt << endl;
        vector<long long> L=v_random<long long>(nc,10);
        vector<key_t> max_min_ind;
        max_min_ind=HelpMat.max_and_min(L,norm_copy);
            
        if(marked[max_min_ind[0]] && marked[max_min_ind[1]])
            no_success++;
        else
            no_success=0;
        if(no_success > 1000)
            break;
        marked[max_min_ind[0]]=true;
        marked[max_min_ind[1]]=true;
    }
    Matrix<long long> Extr(nr_extr,nc);  // the recognized extreme rays
    Matrix<long long> NonExtr(nr_extr,nc); // the other generators
    size_t j=0;
    vector<key_t> perm(nr);
    for(size_t i=0;i<nr;++i) {
        if(marked[i]){
            perm[j]=i;;
            j++;
        }
    }
    nr_extr=j;
    for(size_t i=0;i<nr;++i) {
        if(!marked[i]){
            perm[j]=i;;
            j++;
        }
    }
    order_rows_by_perm(perm);    
    // cout << nr_extr << "extreme points found"  << endl;
    return nr_extr;
    // exit(0);
}

template<typename Integer>
vector<Integer> Matrix<Integer>::find_inner_point(){
    vector<key_t> simplex=max_rank_submatrix_lex();
    vector<Integer> point(nc);
    for(size_t i=0;i<simplex.size();++i)
        point=v_add(point,elem[simplex[i]]);
   return point;    
}

template<typename Integer>
void Matrix<Integer>::Shrink_nr_rows(size_t new_nr_rows){

    if(new_nr_rows>=nr)
        return;
    nr=new_nr_rows;
    elem.resize(nr);
}

}  // namespace
