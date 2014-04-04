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

#include <fstream>
#include <algorithm>

#include "matrix.h"
#include "vector_operations.h"
#include "lineare_transformation.h"
#include "normaliz_exception.h"

//---------------------------------------------------------------------------

namespace libnormaliz {
using namespace std;

//---------------------------------------------------------------------------
//Private
//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::max_rank_submatrix_lex(vector<key_t>& v, const size_t& rank) const{
    size_t level=v.size();
    if (level==rank) {
        return;
    }
    if (level==0) {
        v.push_back(0);
    }
    else{
        v.push_back(v[level-1]);
    }
    for (; v[level] < nr; v[level]++) {
        Matrix<Integer> S=submatrix(v);
        if (S.rank()==S.nr_of_rows()) {
            max_rank_submatrix_lex(v,rank);
            return;
        }
    }
}

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
    assert(dim>=0);
    nr=dim;
    nc=dim;
    elements = vector< vector<Integer> >(dim, vector<Integer>(dim));
    for (size_t i = 0; i < dim; i++) {
        elements[i][i]=1;
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
Matrix<Integer>::Matrix(size_t row, size_t col){
    assert(row>=0);
    assert(col>=0);
    nr=row;
    nc=col;
    elements = vector< vector<Integer> >(row, vector<Integer>(col));
}

//---------------------------------------------------------------------------

template<typename Integer>
Matrix<Integer>::Matrix(size_t row, size_t col, Integer value){
    assert(row>=0);
    assert(col>=0);
    nr=row;
    nc=col;
    elements = vector< vector<Integer> > (row, vector<Integer>(col,value));
}

//---------------------------------------------------------------------------

template<typename Integer>
Matrix<Integer>::Matrix(const vector< vector<Integer> >& elem){
    nr=elem.size();
    if (nr>0) {
        nc=elem[0].size();
        elements=elem;
        //check if all rows have the same length
        for (size_t i=1; i<nr; i++) {
            if (elements[i].size() != nc) {
                throw BadInputException();
            }
        }
    } else {
        nc=0;
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
Matrix<Integer>::Matrix(const list< vector<Integer> >& elem){
    nr = elem.size();
    elements = vector< vector<Integer> > (nr);
    nc = 0;
    size_t i=0;
    typename list< vector<Integer> >::const_iterator it=elem.begin();
    for(; it!=elem.end(); ++it, ++i) {
        if(i == 0) {
            nc = (*it).size();
        } else {
            if ((*it).size() != nc) {
                throw BadInputException();
            }
        }
        elements[i]=(*it);
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::write(istream& in){
    size_t i,j;
    for(i=0; i<nr; i++){
        for(j=0; j<nc; j++) {
            in >> elements[i][j];
        }
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::write(size_t row, const vector<Integer>& data){
    assert(row >= 0);
    assert(row < nr); 
    assert(nc == data.size());
    
    elements[row]=data;
}

//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::write(size_t row, const vector<int>& data){
    assert(row >= 0);
    assert(row < nr); 
    assert(nc == data.size());

    for (size_t i = 0; i < nc; i++) {
        elements[row][i]=data[i];
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::write_column(size_t col, const vector<Integer>& data){
    assert(col >= 0);
    assert(col < nc); 
    assert(nr == data.size());

    for (size_t i = 0; i < nr; i++) {
        elements[i][col]=data[i];
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::write(size_t row, size_t col, Integer data){
    assert(row >= 0);
    assert(row < nr); 
    assert(col >= 0);
    assert(col < nc); 

    elements[row][col]=data;
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
void Matrix<Integer>::print(ostream& out) const{
    size_t i,j;
    out<<nr<<endl<<nc<<endl;
    for (i = 0; i < nr; i++) {
        for (j = 0; j < nc; j++) {
            out<<elements[i][j]<<" ";
        }
        out<<endl;
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::pretty_print(ostream& out, bool with_row_nr) const{
    size_t i,j,k;
    size_t max_length = maximal_decimal_length();
    size_t max_index_length = decimal_length(nr);
    for (i = 0; i < nr; i++) {
        if (with_row_nr) {
            for (k= 0; k <= max_index_length - decimal_length(i); k++) {
                out<<" ";
            }
            out << i << ": ";
        }
        for (j = 0; j < nc; j++) {
            for (k= 0; k <= max_length - decimal_length(elements[i][j]); k++) {
                out<<" ";
            }
            out<<elements[i][j];
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
            cout << elements[i][j] << " ";
        }
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
vector<Integer> Matrix<Integer>::read(size_t row) const{
    assert(row >= 0);
    assert(row < nr);

    return elements[row];
}

//---------------------------------------------------------------------------

template<typename Integer>
Integer Matrix<Integer>::read (size_t row, size_t col) const{
    assert(row >= 0);
    assert(row < nr); 
    assert(col >= 0);
    assert(col < nc); 

    return elements[row][col];
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
            elements[i][j]=k % mod;
        }
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
        M.elements[i]=elements[j];
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
        M.elements[i]=elements[j];
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
            M.elements[j++] = elements[i];
        }
    }
    return M;
}

//---------------------------------------------------------------------------

template<typename Integer>
vector<Integer> Matrix<Integer>::diagonale() const{
    assert(nr == nc); 
    vector<Integer> diag(nr);
    for(size_t i=0; i<nr;i++){
        diag[i]=elements[i][i];
    }
    return diag;
}

//---------------------------------------------------------------------------

template<typename Integer>
size_t Matrix<Integer>::maximal_decimal_length() const{
    size_t i,j,maxim=0;
    for (i = 0; i <nr; i++) {
        for (j = 0; j <nc; j++) {
            maxim=max(maxim,decimal_length(elements[i][j]));
        }
    }
    return maxim;
}

//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::append(const Matrix<Integer>& M) {
    assert (nc == M.nc);
    elements.reserve(nr+M.nr);
    for (size_t i=0; i<M.nr; i++) {
        elements.push_back(M.elements[i]);
    }
    nr += M.nr;
}

//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::append(const vector<Integer>& V) {
    assert (nc == V.size());
    elements.push_back(V);
    nr++;
}

//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::cut_columns(size_t c) {
    assert (c >= 0);
    assert (c <= nc);
    for (size_t i=0; i<nr; i++) {
        elements[i].resize(c);
    }
    nc = c;
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
            B.elements[i][j]=elements[i][j]+A.elements[i][j];
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
                B.elements[i][j]=B.elements[i][j]+elements[i][k]*A.elements[k][j];
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
                B.elements[i][j]=B.elements[i][j]+elements[i][k]*A.elements[k][j];
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
                B.elements[i][j]=(B.elements[i][j]+elements[i][k]*A.elements[k][j])%m;
                if (B.elements[i][j]<0) {
                    B.elements[i][j]=B.elements[i][j]+m;
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
            if (elements[i][j]!=A.elements[i][j]) {
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
            if (((elements[i][j]-A.elements[i][j]) % m)!=0) {
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
            B.elements[j][i]=elements[i][j];
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
            elements[i][j] *= scalar;
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
            assert (elements[i][j]%scalar == 0);
            elements[i][j] /= scalar;
        }
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::reduction_modulo(const Integer& modulo){
    size_t i,j;
    for(i=0; i<nr;i++){
        for(j=0; j<nc; j++){
            elements[i][j] %= modulo;
            if (elements[i][j] < 0) {
                elements[i][j] += modulo;
            }
        }
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
Integer Matrix<Integer>::matrix_gcd() const{
    Integer g=0,h;
    for (size_t i = 0; i <nr; i++) {
        h = v_gcd(elements[i]);
        g = gcd<Integer>(g, h);
        if (g==1) return g;
    }
    return g;
}

//---------------------------------------------------------------------------

template<typename Integer>
vector<Integer> Matrix<Integer>::make_prime() {
    vector<Integer> g(nr);
    for (size_t i = 0; i <nr; i++) {
        g[i] = v_make_prime(elements[i]);
    }
    return g;
}

//---------------------------------------------------------------------------

template<typename Integer>
Matrix<Integer> Matrix<Integer>::multiply_rows(const vector<Integer>& m) const{  //row i is multiplied by m[i]
  Matrix M = Matrix(nr,nc);
  size_t i,j;
  for (i = 0; i<nr; i++) {
     for (j = 0; j<nc; j++) {
        M.elements[i][j] = elements[i][j]*m[i];
     }
  }
  return M;
}

//---------------------------------------------------------------------------

template<typename Integer>
vector<Integer> Matrix<Integer>::MxV(const vector<Integer>& v) const{
    assert (nc == v.size());
    vector<Integer> w(nr);
    for(size_t i=0; i<nr;i++){
        w[i]=v_scalar_product(elements[i],v);
    }
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
            w[i] += v[j]*elements[j][i];
        }
    }
    return w;
}

//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::exchange_rows(const size_t& row1, const size_t& row2){
    if (row1 == row2) return;
    assert(row1 >= 0);
    assert(row1 < nr);
    assert(row2 >= 0);
    assert(row2 < nr);
    elements[row1].swap(elements[row2]);
}

//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::exchange_columns(const size_t& col1, const size_t& col2){
    if (col1 == col2) return;
    assert(col1 >= 0);
    assert(col1 < nc);
    assert(col2 >= 0);
    assert(col2 < nc);
    for(size_t i=0; i<nr;i++){
        std::swap(elements[i][col1], elements[i][col2]);
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::reduce_row (size_t corner) {
    assert(corner >= 0);
    assert(corner < nc);
    assert(corner < nr);
    size_t i,j;
    Integer help;
    for (i = corner+1; i < nr; i++) {
        if (elements[i][corner]!=0) {
            help=elements[i][corner] / elements[corner][corner];
            for (j = corner; j < nc; j++) {
                elements[i][j] -= help*elements[corner][j];
            }
        }
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::reduce_row (size_t corner, Matrix<Integer>& Left) {
    assert(corner >= 0);
    assert(corner < nc);
    assert(corner < nr);
    assert(Left.nr == nr);
    size_t i,j;
    Integer help1, help2=elements[corner][corner];
    for ( i = corner+1; i < nr; i++) {
        help1=elements[i][corner] / help2;
        if (help1!=0) {
            for (j = corner; j < nc; j++) {
                elements[i][j] -= help1*elements[corner][j];
            }
            for (j = 0; j < Left.nc; j++) {
                Left.elements[i][j] -= help1*Left.elements[corner][j];
            }
        }
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::reduce_column (size_t corner) {
    assert(corner >= 0);
    assert(corner < nc);
    assert(corner < nr);
    size_t i,j;
    Integer help1, help2=elements[corner][corner];
    for ( j = corner+1; j < nc; j++) {
        help1=elements[corner][j] / help2;
        if (help1!=0) {
            for (i = corner; i < nr; i++) {
                elements[i][j] -= help1*elements[i][corner];
            }
        }
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::reduce_column (size_t corner, Matrix<Integer>& Right, Matrix<Integer>& Right_Inv) {
    assert(corner >= 0);
    assert(corner < nc);
    assert(corner < nr);
    assert(Right.nr == nc);
    assert(Right.nc == nc);
    assert(Right_Inv.nr == nc);
    assert(Right_Inv.nc ==nc);
    size_t i,j;
    Integer help1, help2=elements[corner][corner];
    for ( j = corner+1; j < nc; j++) {
        help1=elements[corner][j] / help2;
        if (help1!=0) {
            for (i = corner; i < nr; i++) {
                elements[i][j] -= help1*elements[i][corner];
            }
            for (i = 0; i < nc; i++) {
                Right.elements[i][j] -= help1*Right.elements[i][corner];
                Right_Inv.elements[corner][i] += help1*Right_Inv.elements[j][i];
            }
        }
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
vector<long> Matrix<Integer>::pivot(size_t corner){
    assert(corner >= 0);
    assert(corner < nc);
    assert(corner < nr);
    size_t i,j;
    Integer help=0;
    vector<long> v(2,-1);

    for (i = corner; i < nr; i++) {
        for (j = corner; j < nc; j++) {
            if (elements[i][j]!=0) {
                if ((help==0)||(Iabs(elements[i][j])<help)) {
                    help=Iabs(elements[i][j]);
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
long Matrix<Integer>::pivot_column(size_t col){
    assert(col >= 0);
    assert(col < nc);
    assert(col < nr);
    size_t i,j=-1;
    Integer help=0;

    for (i = col; i < nr; i++) {
        if (elements[i][col]!=0) {
            if ((help==0)||(Iabs(elements[i][col])<help)) {
                help=Iabs(elements[i][col]);
                j=i;
                if (help == 1) return j;
            }
        }
    }

    return j;
}

//---------------------------------------------------------------------------

template<typename Integer>
size_t Matrix<Integer>::diagonalize(){
    long rk;
    long rk_max=min(nr,nc);
    vector<long> piv(2,-1);
    for (rk = 0; rk < rk_max; rk++) {
        piv=pivot(rk);
        if (piv[0]>=0) {
            do {
                exchange_rows (rk,piv[0]);
                exchange_columns (rk,piv[1]);
                reduce_row (rk);
                reduce_column (rk);
                piv=pivot(rk);
            } while ((piv[0]>rk)||(piv[1]>rk));
        }
        else
            break;
    }
    return rk;
}

//---------------------------------------------------------------------------

template<typename Integer>
size_t Matrix<Integer>::rank() const{
    Matrix<Integer> N(*this);
    return N.rank_destructive();
}

//---------------------------------------------------------------------------

template<typename Integer>
size_t Matrix<Integer>::rank_destructive(){
    size_t rk,i,j,Min_Row, rk_max=min(nr,nc);
    bool empty;
    Integer Test, Min;
    for (rk = 0; rk < rk_max; rk++) {
        for (i = rk; i < nr; i++) {   //TODO exchange i,j loops?
            for (j = rk; j < nc; j++)
                if (elements[i][j]!=0)
                    break;
            if (j < nc)
                break;
        }
        if (i >= nr)
            break;   //no element != 0 left
        if (rk!=i)
            exchange_rows (rk,i);
        if (rk!=j)
            exchange_columns (rk,j);
        do {
            Min=Iabs(elements[rk][rk]);
            Min_Row=rk;
            empty=true;
            for (i = rk+1; i < nr; i++) {
                Test=Iabs(elements[i][rk]);
                empty = empty && (Test==0);
                if (Test!=0 && (Test<Min)) {
                    Min=Test;
                    Min_Row=i;
                }
            }
            if (Min_Row!=rk) {
                exchange_rows (rk,Min_Row);    
            }
            reduce_row (rk);
        } while (!empty);
    }

    if(!test_arithmetic_overflow)
            return rk;
            
    Integer det=elements[0][0];
    for(i=1;i<rk;i++){
        det*=elements[i][i];
    }
        
    Integer test_det=elements[0][0]%overflow_test_modulus;
    for(i=1;i<rk;i++){
        test_det=(test_det*elements[i][i]%overflow_test_modulus)%overflow_test_modulus;
    }
    if(test_det!=det%overflow_test_modulus){
        errorOutput()<<"Arithmetic failure in computing rank. Most likely overflow.\n";
        throw ArithmeticException();
    } 
    
    return rk;         
}

//---------------------------------------------------------------------------

template<typename Integer>
Integer Matrix<Integer>::vol_destructive(){
    size_t rk,i,j,Min_Row, rk_max=nr; // we assume nr==nc
    bool empty;

    Integer Test, Min;
    for (rk = 0; rk < rk_max; rk++) {
        for (i = rk; i < nr; i++) {
            for (j = rk; j < nc; j++)
                if (elements[i][j]!=0)
                    break;
            if (j<nc)
                break;
        }
        if (i>=nr)
            break;
        if (rk!=i)
            exchange_rows (rk,i);
        if (rk!=j)
            exchange_columns (rk,j);
        do {
            Min=Iabs(elements[rk][rk]);
            Min_Row=rk;
            empty=true;
            for (i = rk+1; i < nr; i++) {
                Test=Iabs(elements[i][rk]);
                empty=empty && (Test==0);
                if (Test!=0&& (Test<Min)) {
                    Min=Test;
                    Min_Row=i;
                }
            }
            if (Min_Row!=rk) {
                exchange_rows (rk,Min_Row);    
            }
            reduce_row (rk);
        } while (!empty);
    }
    if(rk<nr)
        return 0;
    
    
    Integer det=elements[0][0];
    for(i=1;i<nr;i++){
        det*=elements[i][i];
    }

    if(!!test_arithmetic_overflow)
        return Iabs(det);
        
    Integer test_det=elements[0][0]%overflow_test_modulus;
    for(i=1;i<nr;i++){
        test_det=(test_det*elements[i][i]%overflow_test_modulus)%overflow_test_modulus;
    }
    if(test_det!=det%overflow_test_modulus){
        errorOutput()<<"Arithmetic failure in computing determinant. Most likely overflow.\n";
        throw ArithmeticException();
    } 
    
    return Iabs(det);                   
        
}

//---------------------------------------------------------------------------

template<typename Integer>
vector<key_t> Matrix<Integer>::max_rank_submatrix() const{
    //may be optimized in two ways
    //first only a triangular matrix is realy needed, no full diagonalization is necesary
    //second the matrix Rows_Exchanges may be computed by Lineare_transformation::transformation
    size_t i,j,k;
    long rk, rk_max=min(nr,nc);
    vector<long> piv(2);
    Matrix<Integer> M(*this);
    Matrix<Integer> Rows_Exchanges(nr);
    for (rk = 0; rk < rk_max; rk++) {
        piv=M.pivot(rk);
        if (piv[0]>=0) {
            do {
                M.exchange_rows (rk,piv[0]);
                Rows_Exchanges.exchange_columns(rk,piv[0]);
                M.exchange_columns (rk,piv[1]);
                M.reduce_row (rk);
                M.reduce_column (rk);  //optimization posible here
                piv=M.pivot(rk);
            } while ((piv[0]>rk)||(piv[1]>rk));
        }
        else
            break;
    }
    M=Rows_Exchanges.multiplication(M);
    vector<key_t> simplex(rk);
    k=0;
    for (i = 0; i < nr; i++) {
        for (j = 0; j < nc; j++) {
            if (M.elements[i][j]!=0) {
                simplex[k]=i;
                k++;
                //TODO break
            }
        }
    }
    return simplex;
}

//---------------------------------------------------------------------------

template<typename Integer>
vector<key_t>  Matrix<Integer>::max_rank_submatrix_lex() const{
    size_t rk=rank();
    vector<key_t> v(0);
    max_rank_submatrix_lex(v,rk);
    return v;
}

//---------------------------------------------------------------------------

template<typename Integer>
vector<key_t>  Matrix<Integer>::max_rank_submatrix_lex(const size_t& rank) const {
    vector<key_t> v(0);
    max_rank_submatrix_lex(v,rank);
    return v;
}

//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::solve_destructive_Sol_inner(Matrix<Integer>& Right_side, vector< Integer >& diagonal, Integer& denom, Matrix<Integer>& Solution) {
    size_t dim=Right_side.nr;
    size_t nr_sys=Right_side.nc;
    // cout << endl << "Sol.nc " << Solution.nc << " Sol.nr " << Solution.nr << " " << nr_sys << endl;
    assert(nr == nc);
    assert(nc == dim);
    assert(dim == diagonal.size());
    assert(Solution.nc>=nr_sys);
    assert(Solution.nr==dim);
    

    Integer S;
    size_t i;
    long rk, piv;

    for (rk = 0; rk < (long)dim; rk++) {
        piv=(*this).pivot_column(rk);
        if (piv>=0) {
            do {
                (*this).exchange_rows (rk,piv);
                Right_side.exchange_rows (rk,piv);
                (*this).reduce_row(rk, Right_side);
                piv=(*this).pivot_column(rk);
            } while (piv>rk);
        }
    }
    denom = 1;
    for (i = 0; i < dim; i++) {
        denom *= (*this).elements[i][i];
        diagonal[i] = (*this).elements[i][i];
    }

    if (denom==0) { 
        throw BadInputException(); //TODO welche Exception?
    }

    denom=Iabs(denom);
    long j;
    size_t k;
    for (i = 0; i < nr_sys; i++) {
        for (j = dim-1; j >= 0; j--) {
            S=denom*Right_side.elements[j][i];
            for (k = j+1; k < dim; k++) {
                S-=(*this).elements[j][k]*Solution.elements[k][i];
            }
            Solution.elements[j][i]=S/(*this).elements[j][j];
        }
    }
}


    
//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::solve_destructive_Sol(Matrix<Integer>& Right_side, vector< Integer >& diagonal, 
                    Integer& denom, Matrix<Integer>& Solution) {
                    
    if(!test_arithmetic_overflow){
        solve_destructive_Sol_inner(Right_side,diagonal,denom,Solution);
        return;
    }
    
    // now with test_arithmetic_overflow
    Matrix LS_Copy=*this;
    Matrix RS_x_denom=Right_side;
    solve_destructive_Sol_inner(Right_side,diagonal,denom,Solution);
    RS_x_denom.scalar_multiplication(denom);
    // cout << endl;
    // cout << RS_x_denom.nr << " " << RS_x_denom.nc << endl;  
    // RS_x_denom.pretty_print(cout);
    // cout << endl;

    Matrix RS_test=LS_Copy.multiplication_cut(Solution,RS_x_denom.nc);
    // cout << RS_test.nr << " " << RS_test.nc << endl; 
    // RS_test.pretty_print(cout);
    if (!RS_x_denom.equal(RS_test)) {
        errorOutput()<<"Arithmetic failure in solving a linear system. Most likely overflow.\n";
        throw ArithmeticException();
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
Matrix<Integer> Matrix<Integer>::solve_destructive(Matrix<Integer>& Right_side, vector< Integer >& diagonal, Integer& denom) {

    Matrix<Integer> Solution(Right_side.nr,Right_side.nc);  
    solve_destructive_Sol(Right_side,diagonal,denom,Solution);
    return Solution;
}

//---------------------------------------------------------------------------

template<typename Integer>
Matrix<Integer> Matrix<Integer>::solve(Matrix<Integer> Right_side, Integer& denom) const {
    Matrix<Integer> Left_side(*this);
    vector<Integer> dummy_diag(nr);
    return Left_side.solve_destructive(Right_side, dummy_diag, denom);
}

//---------------------------------------------------------------------------

template<typename Integer>
Matrix<Integer> Matrix<Integer>::solve(Matrix<Integer> Right_side, vector< Integer >& diagonal, Integer& denom) const {
    Matrix<Integer> Left_side(*this);
    return Left_side.solve_destructive(Right_side, diagonal, denom);
}    

//---------------------------------------------------------------------------

template<typename Integer>
Matrix<Integer> Matrix<Integer>::invert(vector< Integer >& diagonal, Integer& denom) const{
    assert(nr == nc);
    assert(nr == diagonal.size());
    Matrix<Integer> Left_side(*this);
    Matrix<Integer> Right_side(nr);

    return Left_side.solve_destructive(Right_side,diagonal,denom);
}

//---------------------------------------------------------------------------

template<typename Integer>
vector<Integer> Matrix<Integer>::solve(vector<Integer> v) const {
    if (nc == 0 || nr == 0) { //return zero-vector as solution
        return vector<Integer>(nc,0);
    }
    size_t i;
    Integer denom;
    vector<key_t>  rows=max_rank_submatrix_lex();
    Matrix<Integer> Left_Side=submatrix(rows);
    assert(nc == Left_Side.nr); //otherwise input hadn't full rank //TODO 
    Matrix<Integer> Right_Side(v.size(),1);
    Right_Side.write_column(0,v);
    Right_Side = Right_Side.submatrix(rows);
    Matrix<Integer> Solution=Left_Side.solve(Right_Side, denom);
    vector<Integer> Linear_Form(nc);
    for (i = 0; i <nc; i++) {
        Linear_Form[i] = Solution.read(i,0);
    }
    v_make_prime(Linear_Form);
    vector<Integer> test = MxV(Linear_Form);
    denom = test[0]/v[0];
    //cout << denom << " v= " << v;
    //cout << denom << " t= " << test; 
    for (i = 0; i <nr; i++) {
        if (test[i] != denom * v[i]){
            return vector<Integer>();
        }
    }
    return Linear_Form;
}

template<typename Integer>
vector<Integer> Matrix<Integer>::find_linear_form() const {
    return solve(vector<Integer>(nr,1));
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

    // prepare basis change
    vector <key_t> key = max_rank_submatrix_lex(rank);
    Matrix<Integer> Full_Rank_Matrix = submatrix(key);  // has maximal number of linear independent lines
    Lineare_Transformation<Integer> Basis_Change = Transformation(Full_Rank_Matrix);
    rank=Basis_Change.get_rank();
    Matrix<Integer> V=Basis_Change.get_right();
    Matrix<Integer> Change_To_Full_Emb(nc,rank);
    size_t i,j;
    for (i = 0; i <nc; i++) {
        for (j = 0; j < rank; j++) {
            Change_To_Full_Emb.write(i,j,V.read(i,j));
        }
    }
    
    //apply basis change
    Matrix<Integer> Full_Cone_Generators = Full_Rank_Matrix.multiplication(Change_To_Full_Emb);
    //compute linear form
    vector<Integer> Linear_Form = Full_Cone_Generators.find_linear_form();
    if (Linear_Form.size()==nc) {
        //lift linear form back
        Change_To_Full_Emb = Change_To_Full_Emb.transpose();  // preparing the matrix for transformation on the dual space
        vector<Integer> v;
        Linear_Form = Change_To_Full_Emb.VxM(Linear_Form);
        v_make_prime(Linear_Form);
    }
    return Linear_Form;
}

//---------------------------------------------------------------------------

template<typename Integer>
Matrix<Integer> solve(const Matrix<Integer>& Left_side, const Matrix<Integer>& Right_side,Integer& denom){
    return Left_side.solve(Right_side,denom);
}

//---------------------------------------------------------------------------

template<typename Integer>
Matrix<Integer> invert(const Matrix<Integer>& Left_side, vector< Integer >& diagonal, Integer& denom){
    return Left_side.invert(diagonal,denom);
}

//---------------------------------------------------------------------------


}  // namespace
