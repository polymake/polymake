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

#include <fcntl.h>

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

class lrs_mp_vector_output {
public:
   explicit lrs_mp_vector_output(int n)
      : d(n-1)
      , ptr(lrs_alloc_mp_vector(d))
   {
      if (!ptr) throw std::bad_alloc();
   }

   ~lrs_mp_vector_output() { lrs_clear_mp_vector(ptr, d); }

   operator lrs_mp_vector() { return ptr; }

   const mpz_t& front() const { return ptr[0]; }

   class iterator {
   public:
      typedef std::input_iterator_tag iterator_category;
      typedef Rational value_type;
      typedef Rational reference;
      typedef void pointer;
      typedef ptrdiff_t difference_type;

      iterator(lrs_mp_vector_output& vec, bool or_arg)
         : leading(vec.ptr)
         , cur(leading)
         , last(leading+vec.d)
         , oriented(or_arg)
      {}

      Rational operator* ()
      {
         if (cur==leading) {
            // looking for the leading non-zero
            if (int sgn=mpz_sgn(*leading)) {
               if (!oriented)
                  // all following elements will be divided through the leading non-zero
                  sgn=1;
               else if (sgn<0)
                  // all following elements will be divided through abs(leading non-zero)
                  mpz_neg(*leading, *leading);
               ++cur;
               return Rational(sgn);
            } else {
               ++leading;
               return Rational(std::move(*cur++));
            }
         } else if (cur < last) {
            return Rational(std::move(*cur++), *leading);
         } else {
            return Rational(std::move(*cur++), std::move(*leading));
         }
      }

      iterator& operator++ () { return *this; }

   private:
      mpz_t* leading;
      mpz_t* cur;
      mpz_t* const last;
      const bool oriented;
   };

   Vector<Rational> make_Vector(bool oriented, bool repair=true)
   {
      Vector<Rational> result(d+1, iterator(*this, oriented));
      if (repair) {
         mpz_t* last=ptr+d;
         if ((**last)._mp_alloc) --last;
         for (mpz_t* cur=ptr;  cur<=last;  ++cur)
            mpz_init(*cur);
      }
      return result;
   }

private:
   int d;
   lrs_mp_vector ptr;
};

// lrs stores numerators and denominators in separate integer vectors
class lrs_mp_vector_input {
public:
   explicit lrs_mp_vector_input(int n_arg)
      : n(n_arg)
      , nums(new mpz_t[n])
      , dens(new mpz_t[n]) {}

   ~lrs_mp_vector_input()
   {
      delete[] nums;
      delete[] dens;
   }

   lrs_mp_vector get_nums() const { return nums; }
   lrs_mp_vector get_dens() const { return dens; }

   template <typename Iterator>
   void consume(Iterator&& src)
   {
      for (int i=0; i<n; ++i, ++src) {
         *nums[i] = *mpq_numref(src->get_rep());
         *dens[i] = *mpq_denref(src->get_rep());
      }
   }

private:
   int n;
   lrs_mp_vector nums;
   lrs_mp_vector dens;
};

class lrs_mp_matrix_output {
public:
   lrs_mp_matrix_output(lrs_mp_matrix A, int m_arg, int n_arg)
      : ptr(A)
      , m(m_arg)
      , n(n_arg) {}

   ~lrs_mp_matrix_output()
   {
      if (ptr) lrs_clear_mp_matrix(ptr, m, n);
   }

   class iterator {
   public:
      typedef std::input_iterator_tag iterator_category;
      typedef Rational value_type;
      typedef Rational reference;
      typedef void pointer;
      typedef ptrdiff_t difference_type;

      explicit iterator(lrs_mp_matrix_output& mat)
         : vec(mat.ptr)
         , i(0)
         , n(mat.n)
      {}

      Rational operator* ()
      {
         return Rational(std::move((*vec)[i]));
      }

      iterator& operator++ ()
      {
         if (++i==n) {
            i=0;
            ++vec;
         }
         return *this;
      }

   private:
      lrs_mp_vector* vec;
      int i;
      const int n;
   };

   Matrix<Rational> make_Matrix()
   {
      return Matrix<Rational>(m, n, iterator(*this));
   }

private:
   lrs_mp_matrix ptr;
   int m, n;
};

struct solver::dictionary {
   lrs_dat *Q;
   lrs_dic_struct *P;
   lrs_mp_matrix Lin;
   FILE* out_ptr = nullptr;
   int stdout_copy = -1;

   // stream cleanup and restore stdout
   void cleanup_ofp() {
      if (out_ptr != nullptr && out_ptr != stderr) {
         fflush(out_ptr);
         fclose(out_ptr);
      }
#if PM_LRS_SUPPRESS_OUTPUT == 2
      if (stdout_copy != -1) {
         if (stdout != nullptr)
            fflush(stdout);
         dup2(stdout_copy,1);
         close(stdout_copy);
      }
#endif
   }

   ~dictionary()
   {
      if (Lin) lrs_clear_mp_matrix(Lin, Q->nredundcol, Q->n);
      lrs_free_dic(P,Q);
      lrs_free_dat(Q);
      cleanup_ofp();
   }

   // parameter ge: primal case: true for vertex, falso for linearity
   //                 dual case: true for inequality, false for equation
   void set_matrix(const Matrix<Rational>& A, int start_row=0, bool ge=true)
   {
      lrs_mp_vector_input vec(A.cols());
      auto x=concat_rows(A).begin();

      // lrs enumerates rows starting with 1
      for (int r=start_row+1, r_end=r+A.rows(); r != r_end; ++r) {
         vec.consume(x);
         lrs_set_row_mp(P, Q, r, vec.get_nums(), vec.get_dens(), ge);
      }
   }

   void set_obj_vector(const Vector<Rational>& V, bool maximize)
   {
      const int n=V.size();
      if (n != Q->n)
         throw std::runtime_error("lrs_interface - inequalities/objective dimension mismatch");
      lrs_mp_vector_input vec(n);
      vec.consume(V.begin());
      lrs_set_obj_mp(P, Q, vec.get_nums(), vec.get_dens(), maximize);
      Q->lponly=1;
   }


   dictionary(const Matrix<Rational>& Inequalities, const Matrix<Rational>& Equations,const bool dual)
   {
      // this avoids a segfault in some lrs versions
      if (dual && Inequalities.cols() == 0 && Equations.cols() == 0)
         throw std::runtime_error("lrs_interface - cannot handle ambient dimension 0 in dual mode");
      // initialize static lrs data
      Lin=0;
      // lrs needs a non-null out_ptr if it was built without LRS_QUIET
      // we redirect this to /dev/null unless debugging output is desired
      // the bundled version is built with LRS_QUIET
      int verbose_lrs = perl::get_custom("$polytope::verbose_lrs");
      if (verbose_lrs > 0)
         out_ptr = stderr;

#ifdef PM_LRS_SUPPRESS_OUTPUT
      int output_fd = open("/dev/null", O_WRONLY);
      if (out_ptr == nullptr)
         out_ptr = fdopen(output_fd, "w");
#if PM_LRS_SUPPRESS_OUTPUT == 2
      stdout_copy = dup(1);
      dup2(output_fd,1);
#endif
#endif

      lrs_mp_init(0, nullptr, out_ptr);
      char name[] = "polymake";
      Q=lrs_alloc_dat(name);
      if (!Q) {
         cleanup_ofp();
         throw std::bad_alloc();
      }
      if (verbose_lrs > 0)
         Q->debug=1;
      Q->m=Inequalities.rows()+Equations.rows();
      Q->n=Inequalities.cols();
      if (!Q->n) Q->n=Equations.cols();
      Q->hull=dual?0:1;

      // initialize dynamic lrs data
      P=lrs_alloc_dic(Q);
      if (!P) {
         lrs_free_dat(Q);
         cleanup_ofp();
         throw std::bad_alloc();
      }
      // store inequalities/points and equations/lineality in Q
      if (Inequalities.rows()) set_matrix(Inequalities);
      if (Equations.rows())    set_matrix(Equations, Inequalities.rows(), false);
   }


   // the following functions "run" lrs
   enum _filter_nothing { filter_nothing };
   enum _filter_bounded { filter_bounded };
   enum _filter_rays { filter_rays };
   enum _filter_facets { filter_facets };

   Matrix<Rational> get_solution_matrix(_filter_nothing)
   {
      ListMatrix<Vector<Rational>> facets(0,Q->n);

      lrs_mp_vector_output output(Q->n);
      do {
         for (int col=0; col <= P->d; ++col)
            if (lrs_getsolution(P, Q, output, col))
               facets /= output.make_Vector(true);
      } while (lrs_getnextbasis (&P, Q, 0));

      return Matrix<Rational>(facets.rows(), facets.cols(), operations::move(), entire(rows(facets)));
   }

   long count_solutions(_filter_nothing)
   {
      long facets=0;

      lrs_mp_vector_output output(Q->n);
      do {
         for (int col=0; col <= P->d; ++col)
            if (lrs_getsolution(P, Q, output, col))
               ++facets;
      } while (lrs_getnextbasis (&P, Q, 0));

      return facets;
   }

   Matrix<Rational> get_solution_matrix(_filter_rays, bool isCone)
   {
      // each vertex is computed only once, but rays can appear multiple times.
      ListMatrix<Vector<Rational>> vertices(0,Q->n);
      hash_set<Vector<Rational>> rays;

      lrs_mp_vector_output output(Q->n);
      do {
         for (int col=0; col <= P->d; ++col) {
            if (lrs_getsolution(P, Q, output, col)) {
               if (!mpz_sgn(output.front())) {   // a ray starts with 0
                  rays.insert(output.make_Vector(true));
               } else if (!isCone) {
                  // lrs returns the origin as a vertex for cones
                  // we have to remove this in our interpretation
                  vertices /= output.make_Vector(false);
               }
            }
         }
      } while (lrs_getnextbasis (&P, Q, 0));

      if (isCone)
         return Matrix<Rational>(rays.size(), Q->n, operations::move(), entire(rays));
      else
         return Matrix<Rational>(rays.size()+vertices.rows(), Q->n, operations::move(), entire(rays), entire(rows(vertices)));
   }

   Matrix<Rational> get_linearities()
   {
      const int m=Q->nredundcol, n=Q->n;
      lrs_mp_matrix_output output(Lin, m, n);
      Lin=nullptr;
      return output.make_Matrix();
   }

   std::pair<long, long> count_solutions(_filter_rays)
   {
      std::pair<long, long> vertices(0, 0);
      hash_set<Vector<Rational>> rays;

      lrs_mp_vector_output output(Q->n);
      do {
         for (int col=0; col <= P->d; ++col)
            if (lrs_getsolution(P, Q, output, col)) {
               if (mpz_sgn(output.front()))
                  ++vertices.second;
               else
                  rays.insert(output.make_Vector(true));
            }

      } while (lrs_getnextbasis (&P, Q, 0));

      vertices.first=vertices.second+rays.size();
      return vertices;
   }

   long count_solutions(_filter_bounded)
   {
      long vertices=0;

      lrs_mp_vector_output output(Q->n);
      do {
         for (int col=0; col <= P->d; ++col)
            if (lrs_getsolution(P, Q, output, col)) {
               if (mpz_sgn(output.front()))
                  ++vertices;
            }
      } while (lrs_getnextbasis (&P, Q, 0));

      return vertices;
   }

   Matrix<Rational> get_solution_matrix(_filter_facets)
   {
      hash_set<Vector<Rational>> facets(Q->m * Q->n);

      lrs_mp_vector_output output(Q->n);
      do {
         for (int col=0; col <= P->d; ++col)
            if (lrs_getsolution(P, Q, output, col))
               facets.insert(output.make_Vector(true));
      } while (lrs_getnextbasis (&P, Q, 0));

      return Matrix<Rational>(facets.size(), Q->n, operations::move(), entire(facets));
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

   const Matrix<Rational> AH=isCone ? D.get_linearities().minor(~scalar2set(0),All) : D.get_linearities(); // always lrs returns the functional [1,0,0,0,...]
   const Matrix<Rational> F=D.Q->polytope
                            ? D.get_solution_matrix(dictionary::filter_nothing)  // lrs computes facets only once if input is a polytope
                            : D.get_solution_matrix(dictionary::filter_facets);  // FIXME can facets appear several times for unbounded polyhedra?
   return matrix_pair(F, AH);
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
solver::enumerate_vertices(const Matrix<Rational>& Inequalities, const Matrix<Rational>& Equations, const bool isCone, const bool primal)
{
   dictionary D(Inequalities, Equations,true);

   if (!lrs_getfirstbasis(&D.P, D.Q, &D.Lin, 1)) throw infeasible();

   //      && !D.Q->nredundcol

   const Matrix<Rational> Lineality=D.get_linearities();
   Matrix<Rational>       Vertices= D.get_solution_matrix(dictionary::filter_rays, isCone);

   return matrix_pair(Vertices, Lineality);
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

   lrs_mp_vector_output output(D.Q->n);
   for (int col=0; col <= D.P->d; ++col)
      if (lrs_getsolution(D.P, D.Q, output, col)) break;

   return output.make_Vector(false, false);
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
      lrs_mp_vector_output output(D.Q->n);
      for (int col=0; col <= D.P->d; ++col)
         if (lrs_getsolution(D.P, D.Q, output, col)) break;
      ValidPoint=output.make_Vector(false, false);
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

   // sometimes there is lineality that fits the objective function but unbounded is not set
   // hence we check all lineality rows that lrs computed manually
   if (*linearity_dim_p) {
      Matrix<Rational> lin = D.get_linearities();
      for (auto r = entire(rows(lin)); !r.at_end(); ++r)
         if (Objective * (*r) != 0)
            throw unbounded();
   }

   lrs_mp_vector_output output(D.Q->n);
   for (int col=0; col <= D.P->d; ++col)
      if (lrs_getsolution(D.P, D.Q, output, col)) break;

   return lp_solution(Rational(std::move(D.P->objnum), std::move(D.P->objden)),
                      output.make_Vector(false, false));
}

} } }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
