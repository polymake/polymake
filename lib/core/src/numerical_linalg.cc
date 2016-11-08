/* Copyright (c) 1997-2015
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
#include <cmath>
#include "polymake/Matrix.h"
#include "polymake/linalg.h"
#include "polymake/Vector.h"
#include "polymake/Set.h"
#include "polymake/GenericMatrix.h"
#include "polymake/pair.h"
#include "polymake/internal/operations.h"


#include <algorithm>
#include <tuple>

namespace pm {

namespace {
   const double epsilon=1e-14; // machine epislon

   struct matrixTriple {Matrix<double> diag; Matrix<double> left; Matrix<double> right;};
   struct matrixPair {Matrix<double> left; Matrix<double> right;};

   int non_zero_sign(const double v)
   {
      int s=sign(v);
      if (s==0) {
         s=1;
      }
      return s;
   }

  matrixTriple  bidiag(Matrix<double> M)
  {
     const int colsM=M.cols()-1;  
     const int rowsM=M.rows()-1;
     Matrix<double> U=unit_matrix<double>(rowsM+1);
     const int dimU=U.rows()-1;
     Matrix<double> V=unit_matrix<double>(colsM+1);
     const int dimV=V.rows()-1;
     for (int i=0; i<=std::min(rowsM,colsM); i++) // check "<"
        {
    
           const Vector<double>v=M.col(i).slice(i);
           const Matrix<double>Ui=householder_trafo(v);
           const Matrix<double>M1=non_zero_sign(v[0])*Ui*M.minor(range(i,rowsM),range(i,colsM));
           M.minor(range(i,rowsM),range(i,colsM))=M1;
           const Matrix<double>U1=non_zero_sign(v[0])*Ui*U.minor(range(i,dimU),range(0,dimU)); 
           U.minor(range(i,dimU),range(0,dimU))=U1;
    
           if (i<=colsM-2){
              const Vector<double>v=M.row(i).slice(i+1);
              const Matrix<double>Vi=householder_trafo(v);
              const Matrix<double>M1=non_zero_sign(v[0])*M.minor(range(i,rowsM),range(i+1,colsM))*Vi;
              M.minor(range(i,rowsM),range(i+1,colsM))=M1;
              const Matrix<double>V1=non_zero_sign(v[0])*V.minor(range(0,dimV),range(i+1,dimV))*Vi;
              V.minor(range(0,dimV),range(i+1,dimV))=V1;
           }
        } 
     //V=T(V);
     matrixTriple biDuv; // fixme: constructor
     biDuv.diag=M;
     biDuv.left=U;
     biDuv.right=V;
     return biDuv;
  }


   Matrix<double> givens_rot(Vector<double> v)
   {
      Matrix<double> U(2,2);
      double abs_val=sqrt(v[0]*v[0]+v[1]*v[1]);     
      if (abs(v[0])<epsilon){
            U[0][0]=0;
            U[0][1]=1;
            U[1][0]=1;
            U[1][1]=0; 
         }
      else{
      const double c=v[0]/(sign(v[0])*abs_val);
      const double s=-v[1]/(sign(v[0])*abs_val);
      U[0][0]=c;
      U[0][1]=s;
      U[1][0]=-s;
      U[1][1]=c;      
      }     
      return U;
   }

}

   double eigenValuesOfT(double dm, double dn, double fm, double fmMinus1 )
   {
      double lambda;
      double T00;
      double T11;
      double T10;
      double eigen_val1;
      double eigen_val2;
            
      T00= dm*dm+fmMinus1*fmMinus1;
      T11=dn*dn+fm*fm;
      T10=dm*fm;
      eigen_val1=(T00+T11+sqrt((T00-T11)*(T00-T11)+4*T10*T10))/2;
      eigen_val2=(T00+T11-sqrt((T00-T11)*(T00-T11)+4*T10*T10))/2;
      lambda=std::min(abs(eigen_val1-T11),abs(eigen_val2-T11));                    
      return lambda;
   }
   
   SingularValueDecomposition singular_value_decomposition(Matrix<double> M)
   { 
      const int colsM=M.cols();  
      const int rowsM=M.rows();
       double max_entry=0;
      for (int i=0; i<rowsM-1; i++){ 
         for (int j=0; j<colsM-1; j++){
            if(abs(M[i][j])>max_entry){
               max_entry=abs(M[i][j]);
            }          
         }
      }
      if (colsM>rowsM)
         {
            Matrix<double> M1=T(M);
            M=M1;          
         }
      matrixTriple biDuv=bidiag(M);
      const int colsDiag=biDuv.diag.cols();  
      const int rowsDiag=biDuv.diag.rows();
      const int dimU=biDuv.left.cols();  
      const int dimV=biDuv.right.rows();
        
      Vector<double> u(2);
      int fuse=1;
      while (fuse>0) {
         fuse=0;
         for (int i=0; i<colsDiag; i++) {
              if (abs(biDuv.diag[i][i])<epsilon){
               if (i==colsDiag-1) {
                  for (int j=colsDiag-2; j>=0; j=j-1)   {
                     Set<int> setj(j);
                     setj+=colsDiag-1;
                     u[0]=biDuv.diag[j][j];
                     u[1]=biDuv.diag[j][colsDiag-1];
                     Matrix<double> Rij= givens_rot(u);
                     Matrix<double> B1=biDuv.diag.minor(sequence(0,rowsDiag),setj)*Rij;
                     biDuv.diag.minor(sequence(0,rowsDiag),setj)=B1;
                     Matrix<double> V1=biDuv.right.minor(sequence(0,dimV),setj)*Rij; 
                     biDuv.right.minor(sequence(0,dimV),setj)=V1;
                  }
               }
               else{                      
                  for (int j=i+1; j<=colsDiag-1; j++)   {
                     Set<int> setj(i);
                     setj+=j;
                     u[0]=biDuv.diag[j][j];
                     u[1]=biDuv.diag[i][j];
                     Matrix<double> Rij= givens_rot(u);
                     const Matrix<double> B2=Rij*biDuv.diag.minor(setj,sequence(0,colsDiag)); 
                     biDuv.diag.minor(setj,sequence(0,colsDiag))=B2;
                     const Matrix<double> U1=Rij*biDuv.left.minor(setj,sequence(0,dimU)); 
                     biDuv.left.minor(setj,sequence(0,dimU))=U1;
                  }  
               }
            }
         }
         double lambda;
         double dm =biDuv.diag[rowsM-2][colsM-2];
         double fm=biDuv.diag[rowsM-2][colsM-1];
         double dn=biDuv.diag[rowsM-1][colsM-1];
         double fmMinus1=biDuv.diag[rowsM-3][colsM-2];
         lambda=eigenValuesOfT(dm,dn,fm,fmMinus1);
         
         for (int i=0; i<=colsDiag-2; i++) {
            Set<int> setij(i);
            setij += i+1;
            u[0]=biDuv.diag[i][i]*biDuv.diag[i][i]-lambda;
            u[1]=biDuv.diag[i][i]*biDuv.diag[i][i+1];

            Matrix<double> Rij= givens_rot(u);
            const Matrix<double> B1=biDuv.diag.minor(sequence(0,rowsDiag),setij)*Rij; 
            biDuv.diag.minor(sequence(0,rowsDiag),setij)=B1;

            const Matrix<double> V1=biDuv.right.minor(sequence(0,dimV),setij)*Rij; 
            biDuv.right.minor(sequence(0,dimV),setij)=V1;
            u[0]=biDuv.diag[i][i];
            u[1]=biDuv.diag[i+1][i];
 
            Rij= givens_rot(u);
            Matrix<double> B2=T(Rij)*biDuv.diag.minor(setij,sequence(0,colsDiag)); 
             biDuv.diag.minor(setij,sequence(0,colsDiag))=B2;
            
            Matrix<double> U1=T(Rij)*biDuv.left.minor(setij,sequence(0,dimU)); 
            biDuv.left.minor(setij,sequence(0,dimU))=U1;
         }
         for (int i=0; i<=colsDiag-2; i++) {
            double left=abs(biDuv.diag[i][i+1]);
            double right=epsilon*colsM*rowsM*max_entry;
            if (left<=right)
               {
                  biDuv.diag[i][i+1]=0;
               }
            if (left>right){
               fuse=1;
               break;
            }
         }
      } 
      SparseMatrix<double> Diag(rowsDiag,colsDiag);
      for (int i=0; i<=colsDiag-1; i++)
         {
            Diag[i][i]=biDuv.diag[i][i];
         }
       
      SingularValueDecomposition Out;
       if (colsM>rowsM){
          Out.sigma=T(Diag);
          Out.left_companion=T(biDuv.right);
          Out.right_companion=biDuv.left;
         }
       else{
          Out.sigma=Diag;
          Out.left_companion=T(biDuv.left);
          Out.right_companion=biDuv.right;
       }
       return Out;
   }
   
   // Wikipedia: ...
   Matrix<double> householder_trafo(const Vector<double>& v)
   {
      Matrix<double> Mhouse;
      const int dimv=v.dim();
      double max_entry=0;
      for (int i=0; i<dimv; i++){ 
         if(abs(v[i])>max_entry){
            max_entry=abs(v[i]);
         }          
      }
      const double zero=epsilon*dimv*max_entry;
      Vector<double> u=v+non_zero_sign(v[0])*sqrt(v*v)*unit_vector<double>(dimv,0);

      if (u*u<=zero*zero) {
         Mhouse=unit_matrix<double>(dimv);
      } else {
         u/=sqrt(u*u);
         Mhouse=2*vector2col(u)*vector2row(u)-unit_matrix<double>(dimv);
      }
      return Mhouse;
   }
   
   // Golub ...
   // input matrix M needs to be copied
   std::pair< Matrix<double>,Matrix<double> > qr_decomp(Matrix<double> M)
   {
      //exeption
      const int colsM=M.cols()-1; 
      const int rowsM=M.rows()-1;
      Matrix<double> Q=unit_matrix<double>(rowsM+1);
      const int dimQ=Q.cols()-1;
      for (int i=0; i<=colsM; i++) 
      {
         Vector<double>v=M.col(i).slice(i);
         Matrix<double>Qi=householder_trafo(v);
         Matrix<double>M1=Qi*M.minor(range(i,rowsM),range(i,colsM));
         M.minor(range(i,rowsM),range(i,colsM))=M1;
         Matrix<double>Q1=Qi*Q.minor(range(i,dimQ),range(0,dimQ)); 
         Q.minor(range(i,dimQ),range(0,dimQ))=Q1;
      } 
      return std::pair<Matrix<double>,Matrix<double> >(T(Q),M);
   }

   
   Matrix<double> moore_penrose_inverse(const Matrix<double>& M)
   {
      SingularValueDecomposition biDuv=singular_value_decomposition(M);
      const int colsDiag=biDuv.sigma.cols();  
      const int rowsDiag=biDuv.sigma.rows();
      double max_entry_Diag=0;
      for (int i=0; i<std::min(colsDiag,rowsDiag); i++){ 
         if(abs(biDuv.sigma[i][i])>max_entry_Diag){
            max_entry_Diag=abs(biDuv.sigma[i][i]);
         }          
      }
      double zero=epsilon*std::max(colsDiag,rowsDiag)*max_entry_Diag;
      for (int i=0; i<std::min(colsDiag,rowsDiag); i++){ 
         if(abs(biDuv.sigma[i][i])>zero){
            biDuv.sigma[i][i]=1/biDuv.sigma[i][i];
         }          
      }
      Matrix<double> pseudo_inv=biDuv.right_companion*T(biDuv.sigma)*T(biDuv.left_companion);
      return pseudo_inv;
   }

   Vector<double> lin_solve(Matrix<double> A, Vector<double> b)
   {
      Vector<double> x= moore_penrose_inverse(A)*b;
      return x;
   }
    
   Vector<double> eigenvalues(Matrix<double> M)
   // computes a vector tof eigenvalues only for  square positive-semidefinte matrix
   { 
      SingularValueDecomposition Mdecomp=singular_value_decomposition(M);
      return Mdecomp.sigma.diagonal();
   }
 
}

// end namespace pm

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
