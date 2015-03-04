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
#include <algorithm>

#include "matrix.h"
#include "vector_operations.h"
#include "lineare_transformation.h"
#include "normaliz_exception.h"
#include "sublattice_representation.h"

//---------------------------------------------------------------------------

namespace libnormaliz {
using namespace std;

//---------------------------------------------------------------------------
//Private
//---------------------------------------------------------------------------

template<typename Integer>
vector<key_t>  Matrix<Integer>::max_rank_submatrix_lex() const{

    vector<key_t> v;
    size_t max_rank=min(nr,nc);
    size_t rk;
    Matrix<Integer> Test(max_rank,nc);
    Matrix<Integer> TestCopy(max_rank,nc);
    Test.nr=1;
   
    for(size_t i=0;i<nr;++i){
   
        Test[Test.nr-1]=elements[i];
        TestCopy=Test;
        rk=TestCopy.row_echelon(); //false fehlt!
        if(rk==Test.nr){
            v.push_back(i);
            Test.nr++;
        }
        if(rk==max_rank)
            return v;
    }
       
    return v;
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
                errorOutput() << "Inconsistent lengths of rows in matrix!" << endl;
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
Matrix<Integer>& Matrix<Integer>::remove_zero_rows() {
    size_t from = 0, to = 0; // maintain to <= from
    while (from < nr && v_is_zero(elements[from])) from++; //skip zero rows
    while (from < nr) {  // go over matrix
        // now from is a non-zero row
        if (to != from) swap(elements[to],elements[from]);
        ++to; ++from;
        while (from < nr && v_is_zero(elements[from])) from++; //skip zero rows
    }
    nr = to;
    elements.resize(nr);
    return *this;
}

//---------------------------------------------------------------------------

template<typename Integer>
vector<Integer> Matrix<Integer>::diagonal() const{
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
void Matrix<Integer>::append(const vector<vector<Integer> >& M) {
    if(M.size()==0)
        return;
    assert (nc == M[0].size());
    elements.reserve(nr+M.size());
    for (size_t i=0; i<M.size(); i++) {
        elements.push_back(M[i]);
    }
    nr += M.size();
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
    reduce_row(corner,corner);
}

//---------------------------------------------------------------------------
 
template<typename Integer>
void Matrix<Integer>::reduce_row (size_t row, size_t col) {
    assert(col >= 0);
    assert(col < nc);
    assert(row < nr);
    assert(row >= 0);
    size_t i,j;
    Integer help;
    const Integer max_half = test_arithmetic_overflow ? int_max_value_half<Integer>() : 0;
    for (i =row+1; i < nr; i++) {
        if (elements[i][col]!=0) {
            help=elements[i][col] / elements[row][col];
            for (j = col; j < nc; j++) {
                elements[i][j] -= help*elements[row][j];
                if (test_arithmetic_overflow && Iabs(elements[i][j]) >= max_half) {
                    errorOutput()<<"Arithmetic failure in reduce_row. Most likely overflow.\n";
                    throw ArithmeticException();
                }
            }
        }
    }
}

//---------------------------------------------------------------------------
/* 
template<typename Integer>
void Matrix<Integer>::reduce_row (size_t row, size_t col) {
    assert(col >= 0);
    assert(col < nc);
    assert(row < nr);
    assert(row >= 0);
    size_t i,j;
    Integer div, quot, rem;
    for (i =row+1; i < nr; i++) {
        if (elements[i][col]!=0) {
            div=elements[row][col];
            quot=elements[i][col] / div;
            rem=elements[i][col]-quot*div;
            if(2*Iabs(rem)>Iabs(elements[row][col])){
                if((rem<0 && div>0) || (rem >0 && div <0)){                
                    rem+=elements[row][col];
                    quot--;
                }
                else{
                    rem-=elements[row][col];
                    quot++;                
                }
            }
            elements[i][col]=rem;            
            for (j = col+1; j < nc; j++) {
                elements[i][j] -= quot*elements[row][j];
            }
        }
    }
}
*/

//---------------------------------------------------------------------------

template<typename Integer>
void Matrix<Integer>::reduce_row (size_t corner, Matrix<Integer>& Left) {
    assert(corner >= 0);
    assert(corner < nc);
    assert(corner < nr);
    assert(Left.nr == nr);
    size_t i,j;
    Integer help1, help2=elements[corner][corner];
    const Integer max_half = test_arithmetic_overflow ? int_max_value_half<Integer>() : 0;
    for ( i = corner+1; i < nr; i++) {
        help1=elements[i][corner] / help2;
        if (help1!=0) {
            for (j = corner; j < nc; j++) {
                elements[i][j] -= help1*elements[corner][j];
                if (test_arithmetic_overflow && Iabs(elements[i][j]) >= max_half) {
                    errorOutput()<<"Arithmetic failure in reduce_row. Most likely overflow.\n";
                    throw ArithmeticException();
                }
            }
            for (j = 0; j < Left.nc; j++) {
                Left.elements[i][j] -= help1*Left.elements[corner][j];
                if (test_arithmetic_overflow && Iabs(Left.elements[i][j]) >= max_half) {
                    errorOutput()<<"Arithmetic failure in reduce_row. Most likely overflow.\n";
                    throw ArithmeticException();
                }
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
    return pivot_column(col,col);
}

//---------------------------------------------------------------------------

template<typename Integer>
long Matrix<Integer>::pivot_column(size_t row,size_t col){
    assert(col >= 0);
    assert(col < nc);
    assert(row < nr);
    assert(row >= 0);
    size_t i;
    long j=-1;
    Integer help=0;

    for (i = row; i < nr; i++) {
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
size_t Matrix<Integer>::rank() const{
    Matrix<Integer> N(*this);
    return N.rank_destructive();
}

//---------------------------------------------------------------------------

template<typename Integer>
size_t Matrix<Integer>::row_echelon(){

    size_t pc=0;
    long piv=0, rk=0;
    
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
            /* print(cout);
                cout << "++++++++++++++++++++++++" << endl; */
            reduce_row(rk,pc);
                /* print(cout);
                 cout << "**********************" << endl; */
            piv=pivot_column(rk,pc);
        }while (piv>rk);
    }
    
    return rk;
}

//---------------------------------------------------------------------------

template<typename Integer>
size_t Matrix<Integer>::rank_destructive(){
    
    
    if(!test_arithmetic_overflow)
        return row_echelon();
        
    size_t rk=row_echelon();
    Integer det=1, test_det=1;
    for(size_t i=0;i<rk;++i){
        size_t j=i;
        for(;j<nc;j++)
            if(elements[i][j]!=0)
                break;
        det*=elements[i][j];
        test_det=(test_det *(elements[i][j]%overflow_test_modulus))%overflow_test_modulus;
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

    //cout << "--------vol_destructive start------------" << endl;
    //pretty_print(cout);

    row_echelon();
    //cout << "-------- row_echolon done ------------" << endl;
    //pretty_print(cout);

	Integer det = 1;
    for (size_t i=0; i<nr; i++){
        det*=elements[i][i];
        #ifdef _WIN32 //for 32 and 64 bit windows to workaround a bug in intel compiler
            if (nr != nc) cout << flush;
        #endif
    }
    
    //cout << "==========vol_destructive end=========" << endl;
    
    if(!test_arithmetic_overflow)
        return Iabs(det);
        
    Integer test_det = 1;
    for (size_t i=0; i<nr; i++){
        test_det=(test_det*elements[i][i]%overflow_test_modulus)%overflow_test_modulus;
    }
    if(test_det!=det%overflow_test_modulus){
        errorOutput()<<"Arithmetic failure in computing determinant. Most likely overflow.\n";
        throw ArithmeticException();
    }
    
    return Iabs(det);
}

//---------------------------------------------------------------------------

/*template<typename Integer>
vector<key_t>  Matrix<Integer>::max_rank_submatrix_lex() const{

    vector<key_t> v;
    size_t max_rank=min(nr,nc);
    size_t rk;
    Matrix<Integer> Test(max_rank,nc);
    Test.nr=1;
    
    for(size_t i=0;i<nr;++i){
    
        Test[Test.nr-1]=elements[i];
        rk=Test.row_echelon();
        if(rk==Test.nr){
            v.push_back(i);
            Test.nr++;
        }
        if(rk==max_rank)
            return v;
    }
        
    return v;
}*/


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
        errorOutput() << "Cannot solve system (denom=0)!" << endl;
        throw ArithmeticException(); //TODO welche Exception?
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
Matrix<Integer> Matrix<Integer>::solve(const Matrix<Integer>& Right_side, Integer& denom) const {
    // Matrix<Integer> Left_side(*this);
    vector<Integer> dummy_diag(nr);
    return solve(Right_side, dummy_diag, denom);
}

//---------------------------------------------------------------------------

template<typename Integer>
Matrix<Integer> Matrix<Integer>::solve(const Matrix<Integer>& Right_side, vector< Integer >& diagonal, Integer& denom) const {
    Matrix<Integer> Left_side(*this);
    Matrix<Integer> Copy_Right_Side=Right_side;
    return Left_side.solve_destructive(Copy_Right_Side, diagonal, denom);
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
vector<Integer> Matrix<Integer>::solve(const vector<Integer>& v, Integer& denom) const {
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
    // cout << denom << " v= " << v;
    // denom/=v_make_prime(Linear_Form);
    vector<Integer> test = MxV(Linear_Form); // we have solved the system by taking a square submatrix
    // cout << denom << " v= " << v;         // now we must test whether the solution satisfies the full system
    // cout << denom << " t= " << test; 
    for (i = 0; i <nr; i++) {
        if (test[i] != denom * v[i]){
            return vector<Integer>();
        }
    }
    Integer total_gcd =gcd(denom,v_gcd(Linear_Form)); // extract the gcd of denom and solution
    denom/=total_gcd;
    v_scalar_division(Linear_Form,total_gcd);
    return Linear_Form;
}

template<typename Integer>
vector<Integer> Matrix<Integer>::solve(const vector<Integer>& v) const {

    Integer denom;
    vector<Integer> result=solve(v,denom);
    if(denom!=1)
        result.clear();
    return result;
}

template<typename Integer>
vector<Integer> Matrix<Integer>::find_linear_form() const {

    Integer denom;
    vector<Integer> result=solve(vector<Integer>(nr,1),denom);
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
Matrix<Integer> Matrix<Integer>::kernel () const{
// computes a ZZ-basis of the solutions of (*this)x=0
// the basis is formed by the ROWS of the returned matrix

    size_t dim=nc;
    if(nr==0)
        return(Matrix<Integer>(dim));
    Lineare_Transformation<Integer> NewLT = Transformation(*this);
    size_t rank = NewLT.get_rank();
    Matrix<Integer> ker_basis(dim-rank,dim);
    Matrix<Integer> Help = NewLT.get_right().transpose();
    for (size_t i = rank; i < dim; i++) 
            ker_basis[i-rank]=Help[i];
    return(ker_basis);

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
