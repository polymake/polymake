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

#include "polymake/polytope/lrs_interface.h"
#include "polymake/hash_set"
#include "polymake/ListMatrix.h"

#define GMP
extern "C" {
  #include <lrslib.h>
}
#undef positive
#undef negative
#undef zero
#undef one
#undef gcd
#undef copy
#undef sign
#undef GMP

namespace polymake { namespace polytope { namespace lrs_interface {

class lrs_mp_vector_wrapper {
public:
   explicit lrs_mp_vector_wrapper(int n)
      : d(n-1), ptr(lrs_alloc_mp_vector(d))
   {
      if (!ptr) throw std::bad_alloc();
   }

   ~lrs_mp_vector_wrapper() { lrs_clear_mp_vector(ptr,d); }

   operator lrs_mp_vector() { return ptr; }

   typedef mpz_ptr iterator;
   typedef mpz_srcptr const_iterator;
   typedef MP_INT& reference;
   typedef const MP_INT& const_reference;

   iterator begin() { return *ptr; }
   iterator end() { return begin()+d+1; }
   const_iterator begin() const { return *ptr; }
   const_iterator end() const { return begin()+d+1; }

   int size() const { return d+1; }
   bool empty() const { return d<0; }

   reference front() { return *(ptr[0]); }
   reference back() { return *(ptr[d]); }
   reference operator[] (int i) { return *(ptr[i]); }

   const_reference front() const { return *(ptr[0]); }
   const_reference back() const { return *(ptr[d]); }
   const_reference operator[] (int i) const { return *(ptr[i]); }

   operator Vector<Rational>() const
   {
      Vector<Rational> V(d+1);
      int first=0;
      // leading zeroes
      while (first<=d && !mpz_sgn(ptr[first])) ++first;
      // the rest is divided thru the first non-zero
      V[first]=1;
      for (int i=first+1; i<=d; ++i)
         V[i].set(ptr[i],ptr[first]);
      return V;
   }
private:
   int d;
   lrs_mp_vector ptr;
};

class TempIntegerVector {
public:
   explicit TempIntegerVector(int n_arg)
      : n(n_arg), ptr(new mpz_t[n]) {}

   ~TempIntegerVector() { delete[] ptr; }

   operator lrs_mp_vector() { return ptr; }

   typedef pm::GMP::TempInteger value_type;
   typedef value_type* iterator;
   typedef value_type& reference;
   typedef reference const_reference;

   iterator begin() { return reinterpret_cast<iterator>(ptr); }
   iterator end() { return reinterpret_cast<iterator>(ptr+n); }

   int size() const { return n; }
   bool empty() const { return n==0; }
private:
   int n;
   lrs_mp_vector ptr;
};

class TempRationalVector : public GenericVector<TempRationalVector, pm::GMP::TempRational> {
public:
   TempRationalVector(const lrs_mp_vector_wrapper& v, bool oriented=true)
      : n(v.size()), ptr(new mpq_t[n])
   {
      lrs_mp_vector_wrapper::const_iterator first=v.begin(), src_end=v.end();
      mpq_ptr dst=ptr[0];
      int s=0;

      // leading zeroes
      for (; first != src_end && !(s=mpz_sgn(first));  ++first, ++dst) {
         mpz_init_set_si(mpq_numref(dst),0);
         mpz_init_set_ui(mpq_denref(dst),1);
      }

      // the rest is divided thru the first non-zero (or its abs if oriented)
      lrs_mp_vector_wrapper::const_iterator src=first;
      mpz_init_set_si(mpq_numref(dst),oriented ? s : 1);
      mpz_init_set_ui(mpq_denref(dst),1);

      mpz_t abs_first;
      if (oriented && s<0) first=Integer::_tmp_negate(abs_first,first);

      for (++src, ++dst;  src != src_end;  ++src, ++dst) {
         mpz_init_set(mpq_numref(dst),src);
         mpz_init_set(mpq_denref(dst),first);
         mpq_canonicalize(dst);
      }
   }

   ~TempRationalVector()
   {
      if (ptr) {
         for (int i=0; i<n; ++i)
            mpq_clear(ptr[i]);
         delete[] ptr;
      }
   }

   TempRationalVector(const TempRationalVector& v)
      : n(v.n), ptr(v.ptr) { v.ptr=0; }

   typedef pm::GMP::TempRational value_type;
   typedef value_type& reference;
   typedef value_type* iterator;
   typedef reference const_reference;
   typedef iterator const_iterator;
   typedef std::reverse_iterator<iterator> reverse_iterator;
   typedef reverse_iterator const_reverse_iterator;

   iterator begin() const
   {
      if (n>0) n=-n;
      return reinterpret_cast<iterator>(ptr);
   }
   iterator end() const
   {
      return reinterpret_cast<iterator>(ptr+size());
   }
   reverse_iterator rbegin() const
   {
      return reverse_iterator(end());
   }
   reverse_iterator rend() const
   {
      return reverse_iterator(begin());
   }

   mpq_srcptr const_begin() const { return ptr[0]; }
   mpq_srcptr const_end() const { return ptr[size()]; }

   int size() const { return std::abs(n); }
   bool empty() const { return n==0; }

   bool operator== (const TempRationalVector& v) const
   {
      if (size() != v.size()) return false;
      for (mpq_srcptr src1=const_begin(), end1=const_end(), src2=v.const_begin(); 
           src1 != end1;  ++src1, ++src2) {
         if (mpq_cmp(src1,src2)) return false;
      }
      return true;
   }
   bool operator!= (const TempRationalVector& v) const { return !operator==(v); }
private:
   mutable int n;
   mutable mpq_t *ptr;
};

class TempIntegerMatrix {
public:
   TempIntegerMatrix(lrs_mp_matrix A, int m_arg, int n_arg)
      : ptr(A), m(m_arg), n(n_arg) {}

   ~TempIntegerMatrix()
   {
      if (ptr) {
         // Caution: lrs_alloc_mp_matrix allocates an extra row and an extra column!
         lrs_mp_vector *row_end=ptr+m;
         for (lrs_mp_vector *row=ptr; row<row_end; ++row)
            free(*row);
         lrs_clear_mp_vector(*row_end, n);
         free(ptr);
      }
   }

   class it {
   public:
      typedef std::forward_iterator_tag iterator_category;
      typedef pm::GMP::TempInteger value_type;
      typedef value_type* pointer;
      typedef value_type& reference;
      typedef ptrdiff_t difference_type;
      typedef it iterator;
      typedef it const_iterator;

      it() {}
      it(lrs_mp_vector *row_arg, int n_arg)
         : row(row_arg), i(0), n(n_arg) {}

      reference operator* () const { return reinterpret_cast<reference>(*(*row)[i]); }
      pointer operator-> () const { return &(operator*()); }

      iterator& operator++ ()
      {
         if (++i==n) {
            mpz_clear((*row)[i]);
            i=0; ++row;
         }
         return *this;
      }
      const iterator operator++ (int) { iterator copy=*this; operator++(); return copy; }

      bool operator== (const iterator& it) const
      {
         return row==it.row && i==it.i;
      }
      bool operator!= (const iterator& it) const { return !operator==(it); }
   private:
      lrs_mp_vector *row;
      int i, n;
   };

   typedef it iterator;
   iterator begin() { return iterator(ptr,n); }
   iterator end() { return iterator(ptr+m,n); }
private:
   lrs_mp_matrix ptr;
   int m, n;
};

struct solver::dictionary {
   lrs_dat *Q;
   lrs_dic_struct *P;
   lrs_mp_matrix Lin;

   ~dictionary()
   {
      if (Lin) lrs_clear_mp_matrix(Lin, Q->nredundcol, Q->n);
      lrs_free_dic(P,Q);
      lrs_free_dat(Q);
   }

   // parameter ge: primal case: true for vertex, falso for linearity
   //                 dual case: true for inequality, false for equation
   void set_matrix(const Matrix<Rational>& A, int start_row=0, bool ge=true)
   {
      const int n=A.cols();
      TempIntegerVector num(n), den(n);  // lrs stores denominator and numerator in separate integer vectors
      const Rational *x=concat_rows(A).begin();

      // lrs enumerates rows starting with 1
      for (int r=start_row+1, r_end=r+A.rows(); r != r_end; ++r) {
         for (mpz_ptr num_x=num.begin(), den_x=den.begin(), num_end=num_x+n;  num_x != num_end;  ++x, ++num_x, ++den_x) {
            *num_x = *mpq_numref(x->get_rep());
            *den_x = *mpq_denref(x->get_rep());
         }
         lrs_set_row_mp(P,Q,r,num,den,ge);
      }
   }

   void set_obj_vector(const Vector<Rational>& V, bool maximize)
   {
      const int n=V.size();
      if (n!=Q->n) throw std::runtime_error("lrs_interface - inequalities/objective dimension mismatch");
      TempIntegerVector num(n), den(n);
      const Rational *x=V.begin();

      for (mpz_ptr num_x=num.begin(), den_x=den.begin(), num_end=num_x+n;
           num_x != num_end;  ++x, ++num_x, ++den_x) {
         *num_x = *mpq_numref(x->get_rep());
         *den_x = *mpq_denref(x->get_rep());
      }
      lrs_set_obj_mp(P,Q,num,den,maximize);
      Q->lponly=1;
   }


   dictionary(const Matrix<Rational>& Inequalities, const Matrix<Rational>& Equations,const bool dual)
   {
      // initialize static lrs data
      Lin=0;
      lrs_mp_init(0,NULL,NULL);
      char name[] = "polymake";
      Q=lrs_alloc_dat(name);
      if (!Q) throw std::bad_alloc();
      Q->m=Inequalities.rows()+Equations.rows();
      Q->n=Inequalities.cols();
      if (!Q->n) Q->n=Equations.cols();
      Q->hull=dual?0:1;

      // initialize dynamic lrs data
      P=lrs_alloc_dic(Q);
      if (!P) {
         lrs_free_dat(Q);
         throw std::bad_alloc();
      }
      // store inequalities/points and equations/lineality in Q 
      if (Inequalities.rows()) set_matrix(Inequalities);
      if (Equations.rows())    set_matrix(Equations,Inequalities.rows(),false);
   }


   // the following functions "run" lrs
   enum _filter_nothing { filter_nothing };
   enum _filter_bounded { filter_bounded };
   enum _filter_rays { filter_rays };
   enum _filter_facets { filter_facets };

   Matrix<Rational> get_solution_matrix(_filter_nothing)
   {
      ListMatrix<TempRationalVector> facets;

      lrs_mp_vector_wrapper output(Q->n);
      do {
         for (int col=0; col <= P->d; ++col)
            if (lrs_getsolution(P, Q, output, col))
               facets /= TempRationalVector(output);
      } while (lrs_getnextbasis (&P, Q, 0));

      return Matrix<Rational>(facets.rows(), facets.cols(), entire(rows(facets)));
   }

   long count_solutions(_filter_nothing)
   {
      long facets=0;

      lrs_mp_vector_wrapper output(Q->n);
      do {
         for (int col=0; col <= P->d; ++col)
            if (lrs_getsolution(P, Q, output, col))
               ++facets;
      } while (lrs_getnextbasis (&P, Q, 0));

      return facets;
   }

   Matrix<Rational> get_solution_matrix(_filter_rays)
   {
      // each vertex is computed only once, but rays can appear multiple times. 
      ListMatrix<TempRationalVector> vertices;
      hash_set<TempRationalVector> rays;

      lrs_mp_vector_wrapper output(Q->n);
      do {
         for (int col=0; col <= P->d; ++col)
            if (lrs_getsolution(P, Q, output, col)) {
               if (!mpz_sgn(output.begin())) {   // a ray starts with 0
                  rays.insert(TempRationalVector(output));
               } else {
                  vertices /= TempRationalVector(output,false);
               }
            }
      } while (lrs_getnextbasis (&P, Q, 0));

      return Matrix<Rational>(rays.size()+vertices.rows(), vertices.cols(), entire(concatenate(rays,rows(vertices))));
   }

   Matrix<Rational> get_linearities()
   {
      const int m=Q->nredundcol, n=Q->n;
      if (!m) return Matrix<Rational>();
      lrs_mp_matrix lin=Lin; Lin=0;
      return Matrix<Rational>(m, n, TempIntegerMatrix(lin,m,n).begin());
   }

   std::pair<long,long> count_solutions(_filter_rays)
   {
      std::pair<long,long> vertices(0,0);
      hash_set<TempRationalVector> rays;

      lrs_mp_vector_wrapper output(Q->n);
      do {
         for (int col=0; col <= P->d; ++col)
            if (lrs_getsolution(P, Q, output, col)) {
               if (mpz_sgn(output.begin()))
                  ++vertices.second;
               else
                  rays.insert(TempRationalVector(output));
            }

      } while (lrs_getnextbasis (&P, Q, 0));

      vertices.first=vertices.second+rays.size();
      return vertices;
   }

   long count_solutions(_filter_bounded)
   {
      long vertices=0;

      lrs_mp_vector_wrapper output(Q->n);
      do {
         for (int col=0; col <= P->d; ++col)
            if (lrs_getsolution(P, Q, output, col)) {
               if (mpz_sgn(output.begin()))
                  ++vertices;
            }
      } while (lrs_getnextbasis (&P, Q, 0));

      return vertices;
   }

   Matrix<Rational> get_solution_matrix(_filter_facets)
   {
      hash_set<TempRationalVector> facets(Q->m * Q->n);

      lrs_mp_vector_wrapper output(Q->n);
      do {
         for (int col=0; col <= P->d; ++col)
            if (lrs_getsolution(P, Q, output, col)) 
               facets.insert(TempRationalVector(output));
      } while (lrs_getnextbasis (&P, Q, 0));

      return Matrix<Rational>(facets.size(), Q->n, entire(facets));
   }
};

solver::matrix_pair
solver::enumerate_facets(const Matrix<Rational>& Points, const Matrix<Rational>& Lineality, const bool isCone, const bool primal) 
{
   dictionary D(Points, Lineality, false);
   // we have a polytope if and only if all first coordinates are !=0
   // FIXME find better name, vertex enumeration is unique for cones and polytopes
   D.Q->polytope= isCone || attach_selector(Points.col(0), operations::is_zero()).empty();

   if (!lrs_getfirstbasis(&D.P, D.Q, &D.Lin, 1) && !D.Q->nredundcol) throw infeasible();

   const Matrix<Rational> AH=isCone ? D.get_linearities().minor(~scalar2set(0),All) : D.get_linearities(), // always lrs returns the functional [1,0,0,0,...]
      F=D.Q->polytope
      ? D.get_solution_matrix(dictionary::filter_nothing)  // lrs computes facets only once if input is a polytope
      : D.get_solution_matrix(dictionary::filter_facets);  // FIXME can facets appear several times for unbounded polyhedra?
   return matrix_pair(F,AH);
}

// FIXME check: why are unbounded polyhedra not allowed?
long
solver::count_facets(const Matrix<Rational>& Points, const Matrix<Rational>& Lineality, const bool isCone )
{
   dictionary D(Points,Lineality,false);

   // CHECK: the restriction to bounded polyhedra has been added prior to the implementation
   // of convex hull rules for unbounded polyhedra, and apparently the reason is not known anymore
   // there is no documentation of this function in the lrs manual, so if we want to remove this, then someone has to check the code
   if ( !isCone && !attach_selector(Points.col(0), operations::is_zero()).empty())
      throw std::runtime_error("count_facets is not applicable to unbounded polyhedra");

   if (!lrs_getfirstbasis(&D.P, D.Q, &D.Lin, 1)) throw infeasible();

   return D.Q->nredundcol+1==D.Q->n
      ? 0       // lrs does not treat the special case of a single point correctly
      : D.count_solutions(dictionary::filter_nothing);
}

solver::matrix_pair
solver::enumerate_vertices(const Matrix<Rational>& Inequalities, const Matrix<Rational>& Equations, const bool isCone, const bool primal ) 
{
   dictionary D(Inequalities, Equations,true);

   if (!lrs_getfirstbasis(&D.P, D.Q, &D.Lin, 1)) throw infeasible();

   //      && !D.Q->nredundcol

   const Matrix<Rational> Lineality=D.get_linearities();
   Matrix<Rational>       Vertices= Lineality.rows()+1==Lineality.cols()
      ? Matrix<Rational>()      // lrs does not treat the special case of P=R^n correctly -- FIXME true?
      : D.get_solution_matrix(dictionary::filter_rays); 

   // lrs returns the origin as a vertex for cones
   // we have to remove this in our interpretation
   // FIXME better solution: don't add the origin in get_solution_matrix
   if ( isCone ) { 
      int i = 0;
      int n = Vertices.rows();
      while ( i<n && Vertices(i,0) == 0 ) ++i;
      if ( i < n ) 
         Vertices=Vertices.minor(~scalar2set(i),All); 
   }

   return matrix_pair(Vertices,Lineality);
}

solver::vertex_count
solver::count_vertices(const Matrix<Rational>& Inequalities, const Matrix<Rational>& Equations,
                       bool only_bounded)
{
   dictionary D(Inequalities, Equations,true);

   if (!lrs_getfirstbasis(&D.P, D.Q, &D.Lin, 1)) throw infeasible();

   solver::vertex_count count;
   count.lin = D.Q->nredundcol;
   if ( only_bounded ) {
      count.verts.first = 0;
      count.verts.second = D.count_solutions(dictionary::filter_bounded);
   } else
      count.verts = D.count_solutions(dictionary::filter_rays);

   return count;
}

//primal or dual
std::pair< Bitset, Matrix<Rational> >
solver::find_irredundant_representation(const Matrix<Rational>& Points, const Matrix<Rational>& Lineality, const bool dual)
{
   dictionary D(Points, Lineality, dual);

   if (!lrs_getfirstbasis(&D.P, D.Q, &D.Lin, 1)) throw infeasible();
   const Matrix<Rational> AH=D.get_linearities();
     
   Bitset V(Points.rows());
   for (int index=D.Q->lastdv+1, end=D.P->m_A+D.P->d; index<=end; ++index) 
      if ( !checkindex(D.P,D.Q,index) )
         V += D.Q->inequality[index - D.Q->lastdv]-1; 
     
   return std::pair< Bitset, Matrix<Rational> >(V,AH);
}

Vector<Rational>
solver::find_a_vertex(const Matrix<Rational>& Inequalities, const Matrix<Rational>& Equations)
{
   dictionary D(Inequalities, Equations,true);

   if (!lrs_getfirstbasis(&D.P, D.Q, &D.Lin, 1))
      throw infeasible();
   if (D.Q->nredundcol)
      throw not_pointed(D.Q->nredundcol);

   lrs_mp_vector_wrapper output(D.Q->n);
   for (int col=0; col <= D.P->d; ++col)
      if (lrs_getsolution(D.P,D.Q,output,col)) break;

   return output;
}

bool solver::check_feasibility(const Matrix<Rational>& Inequalities, const Matrix<Rational>& Equations)
{
   dictionary D(Inequalities, Equations,true);
   return lrs_getfirstbasis(&D.P, D.Q, &D.Lin, 1);
}

bool solver::check_feasibility(const Matrix<Rational>& Inequalities, const Matrix<Rational>& Equations, Vector<Rational>& ValidPoint)
{
   dictionary D(Inequalities, Equations,true);

   if (lrs_getfirstbasis(&D.P, D.Q, &D.Lin, 1)) {
      lrs_mp_vector_wrapper output(D.Q->n);
      for (int col=0; col <= D.P->d; ++col)
         if (lrs_getsolution(D.P,D.Q,output,col)) break;
      ValidPoint=output;
      return true;
   }
   return false;
}

solver::lp_solution
solver::solve_lp(const Matrix<Rational>& Inequalities, const Matrix<Rational>& Equations,
                 const Vector<Rational>& Objective, bool maximize, int* linearity_dim_p)
{
   dictionary D(Inequalities, Equations,true);
   D.set_obj_vector(Objective, maximize);

   if (!lrs_getfirstbasis(&D.P, D.Q, &D.Lin, 1)) throw infeasible();
   if (linearity_dim_p) *linearity_dim_p = D.Q->nredundcol;
   if (D.Q->unbounded) throw unbounded();

   lrs_mp_vector_wrapper output(D.Q->n);
   for (int col=0; col <= D.P->d; ++col)
      if (lrs_getsolution(D.P,D.Q,output,col)) break;

   return lp_solution(Rational(D.P->objnum, D.P->objden), output);
}

} } }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
